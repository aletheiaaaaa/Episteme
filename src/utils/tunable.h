#pragma once

#include <array>
#include <cmath>
#include <string>
#include <functional>

#ifndef ENABLE_TUNING
    #define ENABLE_TUNING 0
#endif

namespace episteme::tunable {

void init_lmr_table();

#if ENABLE_TUNING
    struct Tunable {
        std::string name;
        int32_t value;
        int32_t min;
        int32_t max;
        double step;
        std::function<void()> setter;
    };

    Tunable& add_tunable(const std::string& name, int32_t default_value, int32_t min, int32_t max, double step, std::function<void()> setter);

    #define TUNABLE(name, default_value, min, max, step, setter) \
        inline Tunable& _tunable_##name = add_tunable(#name, default_value, min, max, step, setter); \
        [[nodiscard]] inline int32_t name() { return _tunable_##name.value; }

    #define TUNABLE_FLOAT(name, default_value, min, max, step, setter) \
        inline Tunable& _tunable_##name = add_tunable(#name, static_cast<int32_t>(default_value * 1024), static_cast<int32_t>(min * 1024), static_cast<int32_t>(max * 1024), step * 1024, setter); \
        [[nodiscard]] inline double name() { return static_cast<double>(_tunable_##name.value) / 1024.0; }
#else
    #define TUNABLE(name, default_value, min, max, step, setter) \
        [[nodiscard]] constexpr int32_t name() { return default_value; }

    #define TUNABLE_FLOAT(name, default_value, min, max, step, setter) \
        [[nodiscard]] constexpr double name() { return default_value; }
#endif

    TUNABLE_FLOAT(lmr_table_base, 0.5, 0.25, 4.0, 0.25, init_lmr_table);
    TUNABLE_FLOAT(lmr_table_div, 3.0, 1.0, 8.0, 0.5, init_lmr_table);

    TUNABLE(SEE_pawn_val, 100, 0, 512, 16, nullptr);
    TUNABLE(SEE_knight_val, 300, 0, 1024, 32, nullptr);
    TUNABLE(SEE_bishop_val, 300, 0, 1024, 32, nullptr);
    TUNABLE(SEE_rook_val, 500, 0, 2048, 64, nullptr);
    TUNABLE(SEE_queen_val, 900, 0, 4096, 128, nullptr);

    TUNABLE(hist_bonus_mult, 300, 0, 1024, 16, nullptr);
    TUNABLE(hist_bonus_max, 2500, 256, 8192, 128, nullptr);

    TUNABLE(quiet_hist_mult, 128, 0, 512, 8, nullptr);
    TUNABLE(cont_hist_mult, 128, 0, 512, 8, nullptr);
    TUNABLE(pawn_hist_mult, 128, 0, 512, 8, nullptr);
    TUNABLE(capt_hist_mult, 128, 0, 512, 8, nullptr);

    TUNABLE(pawn_corrhist_mult, 250, 0, 1024, 64, nullptr);
    TUNABLE(minor_corrhist_mult, 220, 0, 1024, 64, nullptr);
    // TUNABLE(major_corrhist_mult, 220, 0, 1024, 64, nullptr);
    TUNABLE(nonpawn_stm_corrhist_mult, 240, 0, 1024, 64, nullptr);
    TUNABLE(nonpawn_ntm_corrhist_mult, 240, 0, 1024, 64, nullptr);

    TUNABLE(rfp_mult, 100, 16, 512, 8, nullptr);
    TUNABLE(rfp_base, 25, -256, 512, 16, nullptr);

    TUNABLE(fp_base, 25, -256, 512, 16, nullptr);
    TUNABLE(fp_mult, 250, 64, 2048, 16, nullptr);

    TUNABLE(quiet_see_base, -30, -256, 64, 8, nullptr);
    TUNABLE(quiet_see_mult, -60, -256, 16, 4, nullptr);
    TUNABLE(noisy_see_base, -15, -256, 64, 4, nullptr);
    TUNABLE(noisy_see_mult, -30, -256, 16, 2, nullptr);

    TUNABLE(hist_prune_quiet_base, -2600, -8192, 1024, 128, nullptr);
    TUNABLE(hist_prune_quiet_mult, 600, 0, 4096, 64, nullptr);
    TUNABLE(hist_prune_noisy_base, -2600, -8192, 1024, 128, nullptr);
    TUNABLE(hist_prune_noisy_mult, 600, 0, 4096, 64, nullptr);

    TUNABLE(double_ext_margin, 50, 0, 512, 8, nullptr);

    TUNABLE(lmr_base_mult, 128, 64, 512, 8, nullptr);
    TUNABLE(lmr_improving_mult, 128, 64, 512, 8, nullptr);
    TUNABLE(lmr_is_PV_mult, 128, 64, 512, 8, nullptr);
    TUNABLE(lmr_tt_PV_mult, 128, 64, 512, 8, nullptr);
    TUNABLE(lmr_cut_node_mult, 256, 64, 1024, 16, nullptr);
    TUNABLE(lmr_hist_mult, 128, 64, 512, 8, nullptr);
    TUNABLE(lmr_corrplexity_mult, 64, 32, 128, 4, nullptr);
}
