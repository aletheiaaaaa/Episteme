#pragma once

#include <array>
#include <cmath>
#include <string>
#include <functional>
#include <exception>
#include <iostream>
#include <vector>

#ifndef ENABLE_TUNING
    #define ENABLE_TUNING 0
#endif

namespace episteme::tunable {

extern std::array<std::array<int16_t, 64>, 64> lmr_table_noisy;
extern std::array<std::array<int16_t, 64>, 64> lmr_table_quiet;
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
    
    std::vector<Tunable>& tunables();
    Tunable& add_tunable(const std::string& name, int32_t default_value, int32_t min, int32_t max, double step, std::function<void()> setter);

    #define TUNABLE(name, default_value, min, max, step, setter) \
        inline Tunable& _tunable_##name = add_tunable(#name, default_value, min, max, step, setter); \
        [[nodiscard]] inline int32_t name() { return _tunable_##name.value; }
#else
    #define TUNABLE(name, default_value, min, max, step, setter) \
        [[nodiscard]] constexpr int32_t name() { return default_value; }
#endif

    TUNABLE(lmr_table_quiet_base, 80, -128, 512, 8, init_lmr_table);
    TUNABLE(lmr_table_quiet_div, 220, 64, 1024, 16, init_lmr_table);

    TUNABLE(lmr_table_noisy_base, -10, -128, 512, 8, init_lmr_table);
    TUNABLE(lmr_table_noisy_div, 250, 64, 1024, 16, init_lmr_table);

    TUNABLE(SEE_pawn_val, 100, 0, 512, 16, nullptr);
    TUNABLE(SEE_knight_val, 300, 0, 1024, 32, nullptr);
    TUNABLE(SEE_bishop_val, 300, 0, 1024, 32, nullptr);
    TUNABLE(SEE_rook_val, 500, 0, 2048, 64, nullptr);
    TUNABLE(SEE_queen_val, 900, 0, 4096, 128, nullptr);

    TUNABLE(hist_bonus_mult, 300, 0, 1024, 16, nullptr);
    TUNABLE(hist_bonus_max, 2500, 256, 8192, 128, nullptr);

    TUNABLE(mvv_lva_mult, 128, 0, 512, 8, nullptr);
    TUNABLE(quiet_hist_mult, 128, 0, 512, 8, nullptr);
    TUNABLE(cont_hist_mult, 128, 0, 512, 8, nullptr);
    TUNABLE(pawn_hist_mult, 128, 0, 512, 8, nullptr);
    TUNABLE(capt_hist_mult, 128, 0, 512, 8, nullptr);

    TUNABLE(pawn_corrhist_mult, 200, 0, 1024, 64, nullptr);
    TUNABLE(minor_corrhist_mult, 180, 0, 1024, 64, nullptr);
    // TUNABLE(major_corrhist_mult, 220, 0, 1024, 64, nullptr);
    TUNABLE(nonpawn_stm_corrhist_mult, 190, 0, 1024, 64, nullptr);
    TUNABLE(nonpawn_ntm_corrhist_mult, 190, 0, 1024, 64, nullptr);
    TUNABLE(cont_corrhist_mult, 180, 0, 1024, 64, nullptr);

    TUNABLE(rfp_mult, 100, 16, 512, 8, nullptr);
    TUNABLE(rfp_base, 25, -256, 512, 16, nullptr);

    TUNABLE(hindsight_ext_thresh, -20, -100, 100, 4, nullptr);
    TUNABLE(hindsight_red_thresh, 50, -100, 100, 4, nullptr);

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

    TUNABLE(lmr_improving_mult, 128, 0, 1024, 8, nullptr);
    TUNABLE(lmr_is_PV_mult, 128, 0, 1024, 8, nullptr);
    TUNABLE(lmr_tt_PV_mult, 128, 0, 1024, 8, nullptr);
    TUNABLE(lmr_cut_node_mult, 256, 0, 1024, 16, nullptr);
    TUNABLE(lmr_hist_mult, 128, 0, 1024, 8, nullptr);
    TUNABLE(lmr_corrplexity_mult, 64, 0, 1024, 8, nullptr);
    TUNABLE(lmr_corrplexity_thresh, 250, 0, 1024, 16, nullptr);
}
