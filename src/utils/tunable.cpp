#include "tunable.h"

namespace episteme::tunable {
    std::array<std::array<int16_t, 64>, 64> lmr_table_noisy{};
    std::array<std::array<int16_t, 64>, 64> lmr_table_quiet{};
    
    void init_lmr_table() {
        const double noisy_base = lmr_table_noisy_base() / 100.0;
        const double noisy_div = lmr_table_noisy_div() / 100.0;

        const double quiet_base = lmr_table_quiet_base() / 100.0;
        const double quiet_div = lmr_table_quiet_div() / 100.0;

        for (int i = 1; i < 64; i++) {
            for (int j = 1; j < 64; j++) {
                lmr_table_noisy[i][j] = 128 * (noisy_base + std::log(i) * std::log(j) / noisy_div);
                lmr_table_quiet[i][j] = 128 * (quiet_base + std::log(i) * std::log(j) / quiet_div);
            }
        }
    }
#if ENABLE_TUNING
    std::vector<Tunable>& tunables() {
        static std::vector<Tunable> tunables = []() {
            std::vector<Tunable> v;
            v.reserve(128);
            return v;
        }();
        return tunables;
    }

    Tunable& add_tunable(const std::string& name, int32_t default_value, int32_t min, int32_t max, double step, std::function<void()> setter) {
        auto& vec = tunables();
        vec.emplace_back(Tunable{name, default_value, min, max, step, setter});
        return vec.back();
    }
#endif
}