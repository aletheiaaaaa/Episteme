#pragma once

#include "../chess/move.h"

#include <cstdint>
#include <vector>
#include <random>
#include <bit>

namespace episteme::tt {
    enum class NodeType : uint8_t {
        PVNode, AllNode, CutNode, None
    };

    struct Entry {
        uint64_t hash = 0; 
        Move move = {};
        bool tt_PV = false;
        int32_t score = 0;
        uint8_t depth = 0;
        NodeType node_type = NodeType::None;
    };

    class Table {
        public:
            Table(uint32_t size);

            inline void resize(uint32_t size) {
                ttable.clear();
                const size_t entries = (size * 1024 * 1024) / sizeof(Entry);
                ttable.resize(entries);
            }

            inline void reset() {
                std::fill(ttable.begin(), ttable.end(), Entry());
            }

            [[nodiscard]] inline uint64_t table_index(uint64_t hash) {
                return static_cast<uint64_t>((static_cast<unsigned __int128>(hash) * static_cast<unsigned __int128>(ttable.size())) >> 64);
            }

            [[nodiscard]] inline Entry probe(uint64_t hash) {
                uint64_t index = table_index(hash);
                Entry entry;

                if (ttable[index].hash == hash) entry = ttable[index];

                return entry;
            }

            inline void add(Entry tt_entry) {
                uint64_t index = table_index(tt_entry.hash);
                ttable[index] = tt_entry;
            }

            [[nodiscard]] inline int32_t hashfull() const {
                size_t filled = 0;
                size_t sample_size = std::min(size_t(1000), ttable.size());

                for (size_t i = 0; i < sample_size; ++i) {
                    if (ttable[i].hash != 0) filled++;
                }

                return (filled * 1000) / sample_size;
            }
        private:
            std::vector<Entry> ttable;
    };
}