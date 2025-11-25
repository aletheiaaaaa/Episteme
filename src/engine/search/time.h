#include <cstdint>
#include <chrono>
#include <optional>

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

            inline bool time_approaching() const {
                return (soft_limit != -1) && duration_cast<milliseconds>(steady_clock::now() - start_time).count() >= soft_limit;
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

            inline bool limits_approaching(uint64_t nodes) const {
                return time_approaching() || nodes_approaching(nodes);
            }

            inline bool limits_exceeded(uint64_t nodes, int16_t depth) const {
                return time_exceeded() || nodes_exceeded(nodes);
            }

            inline void start() {
                start_time = steady_clock::now();
                if (config.move_time) {
                    hard_limit = config.move_time;
                } else if (config.time_left) {
                    hard_limit = std::max(1, config.time_left / 20 + config.increment / 2);
                    soft_limit = std::max(1, hard_limit * 5 / 8);
                }
            }

        private:
            Config config{};

            steady_clock::time_point start_time;

            int32_t hard_limit = -1;
            int32_t soft_limit = -1;
    };
}