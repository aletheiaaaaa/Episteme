#pragma once

#include "../chess/movegen.h"
#include "../evaluation/evaluate.h"
#include "../../utils/datagen.h"
#include "ttable.h"
#include "history.h"
#include "stack.h"

#include <cstdint>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <atomic>
#include <optional>
#include <memory>

namespace episteme::search {
    using namespace std::chrono;

    // BEGIN MOVEPICKING //

    struct ScoredMove {
        Move move = {};
        int32_t score = 0;
    };

    struct ScoredList {
        inline void add(const ScoredMove& move) {
            list[count] = move;
            count++;
        }

        inline void clear() {
            count = 0;
        }

        inline void swap(int src_idx, int dst_idx) {
            std::iter_swap(list.begin() + src_idx, list.begin() + dst_idx);
        }

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

    struct Parameters {
        std::array<int32_t, 2> time = {};
        std::array<int32_t, 2> inc = {};

        int16_t depth = MAX_SEARCH_PLY;
        uint64_t nodes = 0;
        uint64_t soft_nodes = 0;
        int32_t num_games = 0;
    };

    struct SearchLimits {
        std::optional<steady_clock::time_point> end;
        std::optional<uint64_t> max_nodes;
    
        bool time_exceeded() const {
            return end && steady_clock::now() >= *end;
        }
    
        bool node_exceeded(uint64_t current_nodes) const {
            return max_nodes && current_nodes >= *max_nodes;
        }
    };    

    struct Line {
        size_t length = 0;
        std::array<Move, MAX_SEARCH_PLY + 1> moves = {};
        
        inline void clear() {
            length = 0;
        }

        inline void append(Move move) {
            moves[length++] = move;
        }

        inline void append(const Line& line) {
            std::copy(&line.moves[0], &line.moves[line.length], &moves[length]);
            length += line.length;
        }

        inline void update_line(Move move, const Line& line) {
            clear();
            append(move);
            append(line);
        }
    };

    struct Report {
        int16_t depth;
        int64_t time;
        uint64_t nodes;
        int64_t nps;
        int32_t score;
        Line line;
    };

    class Worker {
        public:
            Worker(tt::Table& ttable) : ttable(ttable), nodes(0), should_stop(false) {};

            inline void reset_accum() {
                accumulator = {};
                accum_history.clear();
                accum_history.shrink_to_fit();
            }

            inline void reset_history() {
                history.reset();
            }

            inline void reset_nodes() {
                nodes = 0;
            }

            inline void reset_stop() {
                should_stop = false;
            }

            [[nodiscard]] inline bool stopped() {
                return should_stop;
            }

            [[nodiscard]] inline uint64_t node_count() {
                return nodes;
            }

            ScoredMove score_move(const Position& position, const Move& move, const tt::Entry& tt_entry, std::optional<int32_t> ply);

            template<typename F>
            ScoredList generate_scored_targets(const Position& position, F generator, const tt::Entry& tt_entry, std::optional<int32_t> ply = std::nullopt);

            inline ScoredList generate_scored_moves(const Position& position, const tt::Entry& tt_entry, int32_t ply) {
                return generate_scored_targets(position, generate_all_moves, tt_entry, ply);
            }
        
            inline ScoredList generate_scored_captures(const Position& position, const tt::Entry& tt_entry) {
                return generate_scored_targets(position, generate_all_captures, tt_entry);
            }

            int32_t correct_static_eval(int32_t eval, Position& position);

            template<bool PV_node>
            int32_t search(Position& position, Line& PV, int16_t depth, int16_t ply, int32_t alpha, int32_t beta, bool cut_node, SearchLimits limits = {});

            template<bool PV_node>
            int32_t quiesce(Position& position, Line& PV, int16_t ply, int32_t alpha, int32_t beta, SearchLimits limits);

            Report run(int32_t last_score, const Parameters& params, Position& position, const SearchLimits& limits, bool is_absolute);
            int32_t eval(Position& position);
            void bench(int depth);

        private:
            nn::Accumulator accumulator;
            std::vector<nn::Accumulator> accum_history;

            Position position;

            tt::Table& ttable;
            hist::Table history;
            stack::Stack stack;

            std::atomic<uint64_t> nodes;
            std::atomic<bool> should_stop;
    };

    struct Config {
        Parameters params = {};
        uint32_t hash_size = 32;
        uint16_t num_threads = 1;
        Position position;
    };

    class Engine {
        public:
            Engine(const search::Config& cfg) : ttable(cfg.hash_size), params(cfg.params) {
                workers.reserve(cfg.num_threads);
                for (uint16_t i = 0; i < cfg.num_threads; ++i) {
                    workers.emplace_back(std::make_unique<Worker>(ttable));
                }
            }

            inline void set_hash(search::Config& cfg) {
                ttable.resize(cfg.hash_size);
            }

            inline void update_params(search::Parameters& new_params) {
                params = new_params;
            }

            inline void reset_nodes() {
                for (auto& worker : workers) {
                    worker->reset_nodes();
                }
            }

            inline void reset_go() {
                for (auto& worker : workers) {
                    worker->reset_stop();
                }
            }

            inline void reset_game() {
                ttable.reset();
                for (auto& worker : workers) {
                    worker->reset_history();
                    worker->reset_accum();
                    worker->reset_stop();
                }
            }

            void run(Position& position);
            ScoredMove datagen_search(Position& position);
            void eval(Position& position);
            void bench(int depth);

        private:
            tt::Table ttable;
            Parameters params;

            std::vector<std::unique_ptr<Worker>> workers;
    };
}
