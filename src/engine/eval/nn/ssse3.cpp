#include "common.h"
#include <iostream>

namespace episteme::eval::nn {
#if !(defined(USE_AVX512) && defined(USE_VNNI)) && !(defined(USE_AVX2)) && defined(USE_SSSE3)
    L0Output NNUE::l0_pairwise(const Accumulator& accum, Color stm) const {
        L0Output out = {};

        const auto& accum_stm = (!color_idx(stm)) ? (accum.white) : (accum.black);
        const auto& accum_ntm = (!color_idx(stm)) ? (accum.black) : (accum.white);

        for (int i = 0; i < 2; i++) {
            auto accum = (i == 0) ? accum_stm : accum_ntm;
            bool is_ntm = (i == 1);

            for (int j = 0; j < L1_WIDTH / 2; j += 16) {
                __m128i acc00 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&accum[j + 0 + 0 * (L1_WIDTH / 2)]));
                __m128i acc01 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&accum[j + 0 + 1 * (L1_WIDTH / 2)]));
                __m128i acc10 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&accum[j + 8 + 0 * (L1_WIDTH / 2)]));
                __m128i acc11 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&accum[j + 8 + 1 * (L1_WIDTH / 2)]));

                acc00 = _mm_min_epi16(_mm_max_epi16(acc00, _mm_setzero_si128()), _mm_set1_epi16(QA));
                acc10 = _mm_min_epi16(_mm_max_epi16(acc10, _mm_setzero_si128()), _mm_set1_epi16(QA));
                acc01 = _mm_min_epi16(acc01, _mm_set1_epi16(QA));
                acc11 = _mm_min_epi16(acc11, _mm_set1_epi16(QA));

                __m128i pair0 = _mm_mulhi_epi16(_mm_slli_epi16(acc00, 16 - SHIFT), acc01);
                __m128i pair1 = _mm_mulhi_epi16(_mm_slli_epi16(acc10, 16 - SHIFT), acc11);

                __m128i pair = _mm_packus_epi16(pair0, pair1);

                _mm_storeu_si128(reinterpret_cast<__m128i*>(&out[j + is_ntm * (L1_WIDTH / 2)]), pair);
            }
        }

        return out;
    }

    L1Output NNUE::l1_forward(const L0Output& in) const {
        L1Output out = {};

        for (int i = 0; i < L2_WIDTH; i += BLOCK_HEIGHT) {
            __m128i acc0 = _mm_setzero_si128();
            __m128i acc1 = _mm_setzero_si128();

            for (int j = 0; j < L1_WIDTH; j += 8) {
                __m128i x0 = _mm_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 0]));
                __m128i x1 = _mm_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 4]));

                __m128i w0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&l1_weights[i / BLOCK_HEIGHT][j / 4 + 0]));
                __m128i w1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&l1_weights[i / BLOCK_HEIGHT][j / 4 + 1]));

                acc0 = _mm_add_epi32(acc0, _mm_madd_epi16(_mm_maddubs_epi16(x0, w0), _mm_set1_epi16(1)));
                acc1 = _mm_add_epi32(acc1, _mm_madd_epi16(_mm_maddubs_epi16(x1, w1), _mm_set1_epi16(1)));
            }

            __m128i acc = _mm_add_epi32(acc0, acc1);
            __m128 b = _mm_loadu_ps(&l1_biases[i]);

            __m128 pre = _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(acc), _mm_set1_ps(float(1 << SHIFT) / float(QA * QA * QB))), b);
            pre = _mm_min_ps(_mm_max_ps(pre, _mm_setzero_ps()), _mm_set1_ps(1.0f));
            pre = _mm_mul_ps(pre, pre);

            _mm_storeu_ps(&out[i], pre);
        }

        return out;
    }

    L2Output NNUE::l2_forward(const L1Output& in) const {
        L2Output out = {};

        for (int i = 0; i < L3_WIDTH; i += 2) {
            __m128 acc0 = _mm_setzero_ps();
            __m128 acc1 = _mm_setzero_ps();

            for (int j = 0; j < L2_WIDTH; j += 4) {
                __m128 x = _mm_loadu_ps(&in[j]);

                __m128 w0 = _mm_loadu_ps(&l2_weights[i + 0][j]);
                __m128 w1 = _mm_loadu_ps(&l2_weights[i + 1][j]);

                acc0 = _mm_add_ps(_mm_mul_ps(x, w0), acc0);
                acc1 = _mm_add_ps(_mm_mul_ps(x, w1), acc1);
            }

            auto _mm_reduce_add_ps = [] (const __m128& vec) {
                __m128 out = _mm_hadd_ps(vec, vec);
                out = _mm_hadd_ps(out, out);

                return _mm_cvtss_f32(out);
            };

            float out0 = _mm_reduce_add_ps(acc0) + l2_biases[i + 0];
            float out1 = _mm_reduce_add_ps(acc1) + l2_biases[i + 1];

            out0 = std::min(std::max(out0, 0.0f), 1.0f);
            out1 = std::min(std::max(out1, 0.0f), 1.0f);

            out[i + 0] = out0 * out0;
            out[i + 1] = out1 * out1;
        }

        return out;
    }

    L3Output NNUE::l3_forward(const L2Output& in) const {
        float out = 0.0f;

        auto _mm_reduce_add_ps = [] (const __m128& vec) {
            __m128 out = _mm_hadd_ps(vec, vec);
            out = _mm_hadd_ps(out, out);

            return _mm_cvtss_f32(out);
        };

        for (int i = 0; i < L3_WIDTH; i += 4) {
            __m128 x = _mm_loadu_ps(&in[i]);
            __m128 w = _mm_loadu_ps(&l3_weights[i]);
            out += _mm_reduce_add_ps(_mm_mul_ps(x, w));
        }

        out += l3_bias;
        return static_cast<int32_t>(out * EVAL_SCALE);

    }
#endif
}