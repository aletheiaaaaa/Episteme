#pragma once

#include <cstdint>
#include <vector>

#include "../chess/move.hpp"

namespace episteme::tt {
enum class NodeType : uint8_t { PVNode, AllNode, CutNode, None };

struct Entry {
  uint64_t hash = 0;
  Move move = {};
  int32_t score = 0;
  uint8_t depth = 0;
  NodeType node_type = NodeType::None;
  bool tt_PV = false;
};

struct Packed {
  uint16_t move_data = 0;
  int32_t score = 0;
  uint8_t depth = 0;
  uint8_t misc = 0;

  Packed() = default;
  Packed(const Entry& entry)
    : move_data(entry.move.data()),
      score(entry.score),
      depth(entry.depth),
      misc(static_cast<uint8_t>(entry.node_type) | (static_cast<uint8_t>(entry.tt_PV) << 2)) {}
};

class Table {
  public:
  Table(uint32_t size) {
    const size_t num_entries = (size * 1024 * 1024) / sizeof(Packed);
    ttable.resize(num_entries);
    hashes.resize(num_entries);
  }

  void resize(uint32_t size) {
    ttable.clear();
    hashes.clear();
    const size_t num_entries = (size * 1024 * 1024) / sizeof(Packed);
    ttable.resize(num_entries);
    hashes.resize(num_entries);
  }

  void reset() {
    std::fill(ttable.begin(), ttable.end(), Packed{});
    std::fill(hashes.begin(), hashes.end(), 0);
  }

  uint64_t table_index(uint64_t hash) {
    return static_cast<uint64_t>(
      (static_cast<unsigned __int128>(hash) * static_cast<unsigned __int128>(ttable.size())) >> 64
    );
  }

  Entry probe(uint64_t hash) {
    uint64_t index = table_index(hash);
    Entry entry;

    if (hashes[index] == hash) entry = get_entry(index);

    return entry;
  }

  void add(Entry tt_entry) {
    uint64_t index = table_index(tt_entry.hash);
    ttable[index] = tt_entry;
    hashes[index] = tt_entry.hash;
  }

  int32_t hashfull() const {
    size_t filled = 0;
    size_t sample_size = std::min(size_t(1000), ttable.size());

    for (size_t i = 0; i < sample_size; ++i) {
      if (hashes[i] != 0) filled++;
    }

    return (filled * 1000) / sample_size;
  }

  Entry get_entry(int idx) const {
    return Entry{
      .hash = hashes[idx],
      .move = Move(ttable[idx].move_data),
      .score = ttable[idx].score,
      .depth = ttable[idx].depth,
      .node_type = NodeType(ttable[idx].misc & 0b11),
      .tt_PV = static_cast<bool>((ttable[idx].misc >> 2) & 0b1)
    };
  }

  private:
  std::vector<Packed> ttable;
  std::vector<uint64_t> hashes;
};
}  // namespace episteme::tt
