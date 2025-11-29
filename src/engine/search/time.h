#pragma once

#include "../chess/move.h"

#include <cstdint>
#include <chrono>
#include <array>

namespace episteme::time {
    using namespace std::chrono;

    struct Config {
        uint64_t nodes = 0;
        uint64_t soft_nodes = 0;

        int32_t move_time = 0;
        int32_t time_left = 0;
        int32_t increment = 0;
        bool infinite = false;
    };

    class Limiter {
        public:
            inline void set_config(const Config& config) {
                this->config = config;
            }

            inline bool time_exceeded() const {
                return (hard_limit != -1) && duration_cast<milliseconds>(steady_clock::now() - start_time).count() >= hard_limit;
            }

            inline bool nodes_approaching(uint64_t nodes) const {
                return config.soft_nodes && nodes >= config.soft_nodes;
            }

            inline bool nodes_exceeded(uint64_t nodes) const {
                return config.nodes && nodes >= config.nodes;
            }

            inline void update_node_count(Move move, uint64_t count) {
                node_counts[move.data() & 0x0FFF] += count;
            }

            bool time_approaching(Move move, uint64_t nodes);
            void start();

        private:
            Config config{};

            steady_clock::time_point start_time;

            int32_t hard_limit = -1;
            int32_t soft_limit = -1;

            std::array<uint64_t, 4096> node_counts{};
            Move prev_best{};
            int32_t move_stability = 0;
    };
}