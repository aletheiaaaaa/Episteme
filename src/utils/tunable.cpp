#include "tunable.h"

namespace episteme::tunable {
    std::array<std::array<int16_t, 64>, 64> lmr_table{};
    void init_lmr_table() {
        for (int i = 1; i < 64; i++) {
            for (int j = 1; j < 64; j++) {
                lmr_table[i][j] = lmr_table_base() + std::log(i) * std::log(j) / lmr_table_div();
            }
        }
    }
}