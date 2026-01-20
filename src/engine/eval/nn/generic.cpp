#include "common.h"

#include <algorithm>

namespace episteme::eval::nn {
#if !(defined(USE_AVX512) && defined(USE_VNNI)) && !defined(USE_AVX2) && !defined(USE_SSSE3)
    int32_t NNUE::l1_forward(const Accumulator& accum, Color stm) const {
        const auto& accum_stm = (!color_idx(stm)) ? (accum.white) : (accum.black);
        const auto& accum_ntm = (!color_idx(stm)) ? (accum.black) : (accum.white);

        int32_t out = 0;

        for (int i = 0; i < L1_WIDTH; i++) {
            int16_t stm_ = std::clamp(accum_stm[i], int16_t(0), int16_t(QA));
            int16_t ntm_ = std::clamp(accum_ntm[i], int16_t(0), int16_t(QA));

            out += (stm_ * l1_weights[i + 0 * L1_WIDTH]) * stm_;
            out += (ntm_ * l1_weights[i + 1 * L1_WIDTH]) * ntm_;
        }

        out /= QA;
        out += l1_bias;
        out *= EVAL_SCALE;
        out /= (QA * QB);

        return out;
    }
#endif
}