#include <cstdint>
#include <chrono>

namespace episteme::time {
    using namespace std::chrono;

    struct Config {
        int32_t nodes = -1;
        int32_t soft_nodes = -1;

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

            inline void start() {
                start_time = steady_clock::now();
            }

            inline bool time_approaching() const {
                return (soft_limit != -1) && duration_cast<milliseconds>(steady_clock::now() - start_time).count() >= soft_limit;
            }

            inline bool time_exceeded() const {
                return (hard_limit != -1) && duration_cast<milliseconds>(steady_clock::now() - start_time).count() >= hard_limit;
            }

            inline bool nodes_approaching(int32_t nodes) const {
                return (config.soft_nodes != -1) && nodes >= config.soft_nodes;
            }

            inline bool nodes_exceeded(int32_t nodes) const {
                return config.nodes != -1 && nodes >= config.nodes;
            }

            inline bool limits_approaching(int32_t nodes) const {
                return time_approaching() || nodes_approaching(nodes);
            }

            inline bool limits_exceeded(int32_t nodes, int16_t depth) const {
                return time_exceeded() || nodes_exceeded(nodes);
            }

            inline void calculate_limits() {
                if (config.move_time) {
                    hard_limit = config.move_time;
                } else if (config.time_left) {
                    hard_limit = config.time_left / 20 + config.increment / 2;
                    soft_limit = hard_limit * 3 / 5;
                }
            }

        private:
            Config config{};

            steady_clock::time_point start_time;

            int32_t hard_limit = -1;
            int32_t soft_limit = -1;
    };
}