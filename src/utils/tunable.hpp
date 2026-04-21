#pragma once

#include <array>
#include <cmath>
#include <exception>
#include <functional>
#include <print>
#include <string>
#include <vector>

#ifndef ENABLE_TUNING
#define ENABLE_TUNING 0
#endif

#define TUNABLE_INT(name, default_value, min_value, max_value, step_value, setter) \
  inline Tunable<int> name{#name, default_value, min_value, max_value, step_value, setter};

namespace episteme::tunable {
static std::array<std::array<std::string, 2>, 2> type_names = {
  {{"string", "spin"}, {"float", "int"}}
};

extern std::array<std::array<int16_t, 64>, 64> lmr_table_noisy;
extern std::array<std::array<int16_t, 64>, 64> lmr_table_quiet;
void init_lmr_table();

template <typename F, bool Enabled = ENABLE_TUNING>
struct Tunable {
  std::string name;
  F value;
  F min;
  F max;
  F step;
  std::function<void()> callable;
  static inline std::vector<Tunable<F>*> registry;

  Tunable(
    std::string name,
    F default_value,
    F min_value,
    F max_value,
    F step_value,
    std::function<void()> func
  )
    : value(default_value) {
    if (default_value < min_value || default_value > max_value) {
      std::println(stderr, "out of range for tunable {}", name);
    }
    if constexpr (Enabled) {
      this->name = std::move(name);
      min = min_value;
      max = max_value;
      step = step_value;
      callable = func;
      registry.push_back(this);
      if (callable) callable();
    }
  }

  void set(F new_value) {
    if constexpr (Enabled) {
      if (new_value < min || new_value > max) {
        std::println(stderr, "Value out of range for tunable {}", name);
        return;
      }
      value = new_value;
      if (callable) callable();
    }
  }

  void print(bool use_type = false) const {
    if constexpr (Enabled) {
      constexpr bool is_int = std::is_same_v<F, int>;
      std::println("option name {} type {} default {} min {} max {}",
        name, type_names[use_type][is_int], value, min, max);
    }
  }

  constexpr operator F() const { return value; }
};

TUNABLE_INT(lmr_table_quiet_base, 80, -128, 512, 8, init_lmr_table)
TUNABLE_INT(lmr_table_quiet_div, 220, 64, 1024, 16, init_lmr_table)

TUNABLE_INT(lmr_table_noisy_base, -10, -128, 512, 8, init_lmr_table)
TUNABLE_INT(lmr_table_noisy_div, 250, 64, 1024, 16, init_lmr_table)

TUNABLE_INT(SEE_pawn_val, 100, 0, 512, 16, nullptr)
TUNABLE_INT(SEE_knight_val, 300, 0, 1024, 32, nullptr)
TUNABLE_INT(SEE_bishop_val, 300, 0, 1024, 32, nullptr)
TUNABLE_INT(SEE_rook_val, 500, 0, 2048, 64, nullptr)
TUNABLE_INT(SEE_queen_val, 900, 0, 4096, 128, nullptr)

TUNABLE_INT(hist_bonus_mult, 300, 0, 1024, 16, nullptr)
TUNABLE_INT(hist_bonus_max, 2500, 256, 8192, 128, nullptr)

TUNABLE_INT(mvv_lva_mult, 128, 0, 512, 8, nullptr)
TUNABLE_INT(quiet_hist_mult, 128, 0, 512, 8, nullptr)
TUNABLE_INT(cont_hist_mult, 128, 0, 512, 8, nullptr)
TUNABLE_INT(pawn_hist_mult, 128, 0, 512, 8, nullptr)
TUNABLE_INT(capt_hist_mult, 128, 0, 512, 8, nullptr)
TUNABLE_INT(qs_capt_hist_mult, 96, 0, 512, 8, nullptr)

TUNABLE_INT(pawn_corrhist_mult, 200, 0, 1024, 64, nullptr)
TUNABLE_INT(minor_corrhist_mult, 180, 0, 1024, 64, nullptr)
// TUNABLE_INT(major_corrhist_mult, 220, 0, 1024, 64, nullptr)
TUNABLE_INT(nonpawn_stm_corrhist_mult, 190, 0, 1024, 64, nullptr)
TUNABLE_INT(nonpawn_ntm_corrhist_mult, 190, 0, 1024, 64, nullptr)
TUNABLE_INT(cont_corrhist_mult, 180, 0, 1024, 64, nullptr)

TUNABLE_INT(rfp_mult, 100, 16, 512, 8, nullptr)
TUNABLE_INT(rfp_base, 25, -256, 512, 16, nullptr)

TUNABLE_INT(hindsight_ext_thresh, -20, -100, 100, 4, nullptr)
TUNABLE_INT(hindsight_red_thresh, 50, -100, 100, 4, nullptr)

TUNABLE_INT(fp_base, 25, -256, 512, 16, nullptr)
TUNABLE_INT(fp_mult, 250, 64, 2048, 16, nullptr)

TUNABLE_INT(quiet_see_base, -30, -256, 64, 8, nullptr)
TUNABLE_INT(quiet_see_mult, -60, -256, 16, 4, nullptr)
TUNABLE_INT(noisy_see_base, -15, -256, 64, 4, nullptr)
TUNABLE_INT(noisy_see_mult, -30, -256, 16, 2, nullptr)

TUNABLE_INT(hist_prune_base, -2600, -8192, 1024, 128, nullptr)
TUNABLE_INT(hist_prune_mult, 600, 0, 4096, 64, nullptr)

TUNABLE_INT(double_ext_margin, 20, 0, 512, 8, nullptr)

TUNABLE_INT(lmr_improving_mult, 128, 0, 1024, 8, nullptr)
TUNABLE_INT(lmr_is_PV_mult, 128, 0, 1024, 8, nullptr)
TUNABLE_INT(lmr_tt_PV_mult, 128, 0, 1024, 8, nullptr)
TUNABLE_INT(lmr_cut_node_mult, 256, 0, 1024, 16, nullptr)
TUNABLE_INT(lmr_hist_mult, 128, 0, 1024, 8, nullptr)
TUNABLE_INT(lmr_corrplexity_mult, 64, 0, 1024, 8, nullptr)
TUNABLE_INT(lmr_corrplexity_thresh, 250, 0, 1024, 16, nullptr)
}  // namespace episteme::tunable
