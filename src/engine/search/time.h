#include <cstdint>
#include <chrono>
#include <iostream>

namespace episteme::time {
    using namespace std::chrono;

    struct Config {
        int32_t nodes = 0;
        int32_t soft_nodes = 0;

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

            inline void reset() {
                config = {};
                start_time = steady_clock::time_point{};
                hard_end = steady_clock::time_point::min();
                soft_end = steady_clock::time_point::min();
            }

            inline bool time_approaching() const {
                return (soft_end != steady_clock::time_point::min()) && steady_clock::now() >= soft_end;
            }

            inline bool time_exceeded() const {
                return (hard_end != steady_clock::time_point::min()) && steady_clock::now() > hard_end;
            }

            inline bool nodes_approaching(int32_t nodes) const {
                return (config.soft_nodes) && nodes >= config.soft_nodes;
            }

            inline bool nodes_exceeded(int32_t nodes) const {
                return (config.nodes) && nodes >= config.nodes;
            }

            inline bool limits_approaching(int32_t nodes) const {
                return time_approaching() || nodes_approaching(nodes);
            }

            inline bool limits_exceeded(int32_t nodes) const {
                return time_exceeded() || nodes_exceeded(nodes);
            }

            inline void start() {
                start_time = steady_clock::now();
                if (config.move_time) {
                    hard_end = start_time + milliseconds(config.move_time);
                } else if (config.time_left) {
                    hard_end = start_time + milliseconds(config.time_left / 20 + config.increment / 2);
                    soft_end = start_time + milliseconds((config.time_left / 20 + config.increment / 2) * 3 / 5);
                }
            }

        private:
            Config config{};

            steady_clock::time_point start_time;

            steady_clock::time_point hard_end = steady_clock::time_point::min();
            steady_clock::time_point soft_end = steady_clock::time_point::min();
    };
}