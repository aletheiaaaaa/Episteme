#include "tunable.h"

namespace episteme::tunable {
std::array<std::array<int16_t, 64>, 64> lmr_table_noisy{};
std::array<std::array<int16_t, 64>, 64> lmr_table_quiet{};

void init_lmr_table() {
  const double noisy_base = lmr_table_noisy_base / 100.0;
  const double noisy_div = lmr_table_noisy_div / 100.0;

  const double quiet_base = lmr_table_quiet_base / 100.0;
  const double quiet_div = lmr_table_quiet_div / 100.0;

  for (int i = 1; i < 64; i++) {
    for (int j = 1; j < 64; j++) {
      lmr_table_noisy[i][j] =
          128 * (noisy_base + std::log(i) * std::log(j) / noisy_div);
      lmr_table_quiet[i][j] =
          128 * (quiet_base + std::log(i) * std::log(j) / quiet_div);
    }
  }
}
} // namespace episteme::tunable