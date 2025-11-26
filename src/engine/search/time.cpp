#include "time.h"

namespace episteme::time {
    using namespace std::chrono;

    bool Limiter::time_approaching(Move move, uint64_t nodes) {
        float nodes_prop = static_cast<float>(node_counts[move.data() & 0x0FFF]) / static_cast<float>(nodes);
        float node_scale = 2.5f - nodes_prop * 1.5f;

        if (move != prev_best) {
            prev_best = move;
            move_stability = 1;
        } else {
            move_stability++;
        }

        float move_scale = 1.25f - (static_cast<float>(move_stability) / 20.0f);

        return (soft_limit != -1) && duration_cast<milliseconds>(steady_clock::now() - start_time).count() >= static_cast<int32_t>(soft_limit * node_scale * move_scale);
    }

    void Limiter::start() {
        start_time = steady_clock::now();
        node_counts.fill(0);
        prev_best = Move();

        if (config.move_time) {
            hard_limit = config.move_time;
        } else if (config.time_left) {
            hard_limit = std::max(1, config.time_left / 20 + config.increment / 2);
            soft_limit = std::max(1, hard_limit * 5 / 8);
        }
    }
}