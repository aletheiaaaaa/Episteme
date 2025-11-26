#include "time.h"

namespace episteme::time {
    using namespace std::chrono;

    bool Limiter::time_approaching(Move move, uint64_t nodes) const {
        float prop = static_cast<float>(node_counts[move.data() & 0x0FFF]) / static_cast<float>(nodes);
        float scale = 2.5f - prop * 1.5f;

        return (soft_limit != -1) && duration_cast<milliseconds>(steady_clock::now() - start_time).count() >= static_cast<int32_t>(soft_limit * scale);
    }

    void Limiter::start() {
        start_time = steady_clock::now();
        node_counts.fill(0);

        if (config.move_time) {
            hard_limit = config.move_time;
        } else if (config.time_left) {
            hard_limit = std::max(1, config.time_left / 20 + config.increment / 2);
            soft_limit = std::max(1, hard_limit * 5 / 8);
        }
    }
}