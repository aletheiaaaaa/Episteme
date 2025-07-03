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
#include <optional>

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

    struct Parameters {
        std::array<int32_t, 2> time = {};
        std::array<int32_t, 2> inc = {};

        int16_t depth = MAX_SEARCH_PLY;
        uint64_t nodes = 0;
        uint64_t soft_nodes = 0;
        int32_t num_games = 0;

        Position position;
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

    struct Config {
        Parameters params = {};
        uint32_t hash_size = 32;
        uint16_t num_threads = 1;
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

    struct ThreadReport {
        int16_t depth;
        int64_t time;
        uint64_t nodes;
        int64_t nps;
        int32_t score;
        Line line;
    };

    class Thread {
        public:
            Thread(tt::Table& ttable) : ttable(ttable), should_stop(false) {};

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

            template<bool PV_node>
            int32_t search(Position& position, Line& PV, int16_t depth, int16_t ply, int32_t alpha, int32_t beta, SearchLimits limits = {});

            int32_t quiesce(Position& position, Line& PV, int16_t ply, int32_t alpha, int32_t beta, SearchLimits limits);

            ThreadReport run(int32_t last_score, const Parameters& params, const SearchLimits& limits, bool is_absolute);
            int32_t eval(const Parameters& params);
            void bench(int depth);

        private:
            nn::Accumulator accumulator;
            std::vector<nn::Accumulator> accum_history;

            tt::Table& ttable;
            hist::Table history;
            stack::Stack stack;

            uint64_t nodes;

            bool should_stop;
    };

    class Instance {
        public:
            Instance(Config& cfg) : ttable(cfg.hash_size), params(cfg.params), thread(ttable) {};

            inline void set_hash(search::Config& cfg) {
                ttable.resize(cfg.hash_size);
            }

            inline void update_params(search::Parameters& new_params) {
                params = new_params;
            }

            inline void reset_go() {
                thread.reset_history();
                thread.reset_stop();
            }

            inline void reset_game() {
                ttable.reset();
                thread.reset_history();
                thread.reset_stop();
            }

            void run();
            ScoredMove datagen();
            void eval(const Parameters& params);
            void bench(int depth);

        private:
            tt::Table ttable;
            Parameters params;

            Thread thread;
    };
}
