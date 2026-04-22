#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>

#include "../chess/movegen.hpp"
#include "../eval/nn/common.hpp"
#include "history.hpp"
#include "stack.hpp"
#include "time.hpp"
#include "ttable.hpp"

namespace episteme::search {
using namespace std::chrono;

// BEGIN MOVEPICKING //

struct ScoredMove {
  Move move = {};
  int32_t score = 0;
};

struct ScoredList {
  void add(const ScoredMove& move) {
    list[count] = move;
    count++;
  }

  void clear() { count = 0; }

  void swap(int src_idx, int dst_idx) {
    std::iter_swap(list.begin() + src_idx, list.begin() + dst_idx);
  }

  ScoredMove operator[](size_t idx) const { return list[idx]; }

  std::array<ScoredMove, 256> list;
  size_t count = 0;
};

void pick_move(ScoredList& scored_list, int start);

// BEGIN SEARCH //

constexpr int32_t INF = 1048576;
constexpr int32_t MATE = 1048575;

constexpr int32_t DELTA = 20;
constexpr int32_t MAX_SEARCH_PLY = 256;

extern std::array<std::array<int16_t, 64>, 64> lmr_table;
void init_lmr_table();

struct Line {
  size_t length = 0;
  std::array<Move, MAX_SEARCH_PLY + 1> moves = {};

  void clear() { length = 0; }

  void append(Move move) { moves[length++] = move; }

  void append(const Line& line) {
    std::copy(&line.moves[0], &line.moves[line.length], &moves[length]);
    length += line.length;
  }

  void update_line(Move move, const Line& line) {
    clear();
    append(move);
    append(line);
  }
};

struct Parameters {
  int32_t move_time = 0;

  std::array<int32_t, 2> time = {};
  std::array<int32_t, 2> inc = {};

  int16_t depth = MAX_SEARCH_PLY;
  uint64_t nodes = 0;
  uint64_t soft_nodes = 0;
  int32_t num_games = 0;
};

struct Report {
  int16_t depth;
  int16_t seldepth;
  int64_t time;
  uint64_t nodes;
  int32_t score;
  int32_t hashfull;
  Line line;
};

struct Config {
  Parameters params = {};
  uint32_t hash_size = 32;
  uint16_t num_threads = 1;
  Position position;
};

class Worker {
  public:
  Worker(tt::Table& ttable, time::Limiter& limiter);
  ~Worker();

  void dispatch(Position& pos, Parameters& p);

  void reset_accum() {
    accumulator = {};
    accum_history.clear();
    accum_history.shrink_to_fit();
  }

  void reset_history() { history.reset(); }

  void reset_nodes() { nodes = 0; }

  void reset_seldepth() { seldepth = 0; }

  void reset_stop() { should_stop = false; }

  void reset_stack() { stack.reset(); }

  bool stopped() { return should_stop; }

  uint64_t node_count() { return nodes; }

  ScoredMove score_move(
    const Position& position,
    const Move& move,
    const tt::Entry& tt_entry,
    std::optional<int32_t> ply
  );

  template <typename F>
  ScoredList generate_scored_targets(
    const Position& position,
    F generator,
    const tt::Entry& tt_entry,
    std::optional<int32_t> ply = std::nullopt
  );

  ScoredList generate_scored_moves(
    const Position& position, const tt::Entry& tt_entry, int32_t ply
  ) {
    return generate_scored_targets(position, generate_all_moves, tt_entry, ply);
  }

  ScoredList generate_scored_captures(const Position& position, const tt::Entry& tt_entry) {
    return generate_scored_targets(position, generate_all_captures, tt_entry);
  }

  int32_t eval_correction(int16_t ply, Position& position);

  template <bool PV_node>
  int32_t search(
    Position& position,
    Line& PV,
    int16_t depth,
    int16_t ply,
    int32_t alpha,
    int32_t beta,
    bool cut_node
  );

  template <bool PV_node>
  int32_t quiesce(Position& position, Line& PV, int16_t ply, int32_t alpha, int32_t beta);

  Report run(int32_t last_score, const Parameters& params, Position& position, bool is_absolute);
  int32_t eval(Position& position);
  void bench(int depth);

  private:
  std::thread thread;
  std::mutex mutex;
  std::condition_variable cond;
  bool assigned = false;
  bool quit = false;

  eval::nn::Accumulator accumulator;
  std::vector<eval::nn::Accumulator> accum_history;

  Position position;
  Parameters params;
  int32_t last_score = 0;

  tt::Table& ttable;
  time::Limiter& limiter;
  hist::Table history;
  stack::Stack stack;

  int16_t seldepth;

  Line exploring;

  std::atomic<uint64_t> nodes;
  std::atomic<bool> should_stop;
};
}  // namespace episteme::search
