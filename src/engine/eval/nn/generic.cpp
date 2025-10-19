#include "common.h"

namespace episteme::eval::nn {
#if !(defined(USE_AVX512) && defined(USE_VNNI)) && !(defined(USE_AVX2)) && !defined(USE_SSSE3) 
    L0Output NNUE::l0_pairwise(const Accumulator& accum, Color stm) const {
        L0Output out = {};

        const auto& accum_stm = (!color_idx(stm)) ? (accum.white) : (accum.black);
        const auto& accum_ntm = (!color_idx(stm)) ? (accum.black) : (accum.white);

        for (int i = 0; i < 2; i++) {
            auto accum = (i == 0) ? accum_stm : accum_ntm;
            bool is_ntm = (i == 1);

            for (int j = 0; j < L1_WIDTH / 2; j++) {
                int16_t a = std::clamp<int16_t>(accum[j + 0 * L1_WIDTH / 2], 0, QA);
                int16_t b = std::clamp<int16_t>(accum[j + 1 * L1_WIDTH / 2], 0, QA);
                out[j + is_ntm * L1_WIDTH / 2] = static_cast<uint8_t>((a * b) >> SHIFT);
            }
        }

        return out;
    }

    L1Output NNUE::l1_forward(const L0Output& in) const {
        L1Output out = {};

        for (int i = 0; i < L2_WIDTH; i++) {
            int32_t acc = 0;

            for (int j = 0; j < L1_WIDTH; j++) {
                acc += static_cast<int32_t>(in[j] * l1_weights[i][j]);
            }

            float pre = static_cast<float>(acc) * (float(1 << SHIFT) / float(QA * QA * QB)) + l1_biases[i];
            pre = std::clamp<float>(pre, 0.0f, 1.0f);
            out[i] = pre * pre;
        }

        return out;
    }

    L2Output NNUE::l2_forward(const L1Output& in) const {
        L2Output out = {};

        for (int i = 0; i < L3_WIDTH; i++) {
            float acc = 0.0f;

            for (int j = 0; j < L2_WIDTH; j++) {
                acc += in[j] * l2_weights[i][j];
            }

            float pre = acc + l2_biases[i];
            pre = std::clamp<float>(pre, 0.0f, 1.0f);
            out[i] = pre * pre;
        }

        return out;
    }

    L3Output NNUE::l3_forward(const L2Output& in) const {
        float out = 0.0f;

        for (int i = 0; i < L3_WIDTH; i++) {
            out += in[i] * l3_weights[i];
        }

        out += l3_bias;
        return static_cast<int32_t>(out * EVAL_SCALE);
    }
#endif
}