#include "common.h"

namespace episteme::eval::nn {
#if defined(USE_AVX512) && defined(USE_VNNI)
    L0Output NNUE::l0_pairwise(const Accumulator& accum, Color stm) const {
        L0Output out = {};

        const auto& accum_stm = (!color_idx(stm)) ? (accum.white) : (accum.black);
        const auto& accum_ntm = (!color_idx(stm)) ? (accum.black) : (accum.white);

        for (int i = 0; i < 2; i++) {
            auto accum = (i == 0) ? accum_stm : accum_ntm;
            bool is_ntm = (i == 1);

            for (int j = 0; j < L1_WIDTH / 2; j += 256) {
                auto process_chunk = [&](int offset) {
                    __m512i acc00 = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(&accum[j + offset + 0 + 0 * (L1_WIDTH / 2)]));
                    __m512i acc01 = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(&accum[j + offset + 0 + 1 * (L1_WIDTH / 2)]));
                    __m512i acc10 = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(&accum[j + offset + 32 + 0 * (L1_WIDTH / 2)]));
                    __m512i acc11 = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(&accum[j + offset + 32 + 1 * (L1_WIDTH / 2)]));

                    acc00 = _mm512_min_epi16(_mm512_max_epi16(acc00, _mm512_setzero_si512()), _mm512_set1_epi16(QA));
                    acc10 = _mm512_min_epi16(_mm512_max_epi16(acc10, _mm512_setzero_si512()), _mm512_set1_epi16(QA));
                    acc01 = _mm512_min_epi16(acc01, _mm512_set1_epi16(QA));
                    acc11 = _mm512_min_epi16(acc11, _mm512_set1_epi16(QA));

                    __m512i pair0 = _mm512_mulhi_epi16(_mm512_slli_epi16(acc00, 16 - SHIFT), acc01);
                    __m512i pair1 = _mm512_mulhi_epi16(_mm512_slli_epi16(acc10, 16 - SHIFT), acc11);

                    _mm512_storeu_si512(reinterpret_cast<__m512i*>(&out[j + offset + is_ntm * (L1_WIDTH / 2)]), _mm512_packus_epi16(pair0, pair1));
                };

                process_chunk(0);
                process_chunk(64);
                process_chunk(128);
                process_chunk(192);
            }
        }

        return out;
    }

    L1Output NNUE::l1_forward(const L0Output& in) const {
        L1Output out = {};

        for (int i = 0; i < L2_WIDTH; i += BLOCK_HEIGHT) {
            __m512i acc0 = _mm512_setzero_si512();
            __m512i acc1 = _mm512_setzero_si512();
            __m512i acc2 = _mm512_setzero_si512();
            __m512i acc3 = _mm512_setzero_si512();

            for (int j = 0; j < L1_WIDTH; j += 16) {
                __m512i x0 = _mm512_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 0]));
                __m512i x1 = _mm512_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 4]));
                __m512i x2 = _mm512_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 8]));
                __m512i x3 = _mm512_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 12]));

                __m512i w0 = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(&l1_weights[i / BLOCK_HEIGHT][j / 4 + 0]));
                __m512i w1 = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(&l1_weights[i / BLOCK_HEIGHT][j / 4 + 1]));
                __m512i w2 = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(&l1_weights[i / BLOCK_HEIGHT][j / 4 + 2]));
                __m512i w3 = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(&l1_weights[i / BLOCK_HEIGHT][j / 4 + 3]));

                acc0 = _mm512_dpbusd_epi32(acc0, x0, w0);
                acc1 = _mm512_dpbusd_epi32(acc1, x1, w1);
                acc2 = _mm512_dpbusd_epi32(acc2, x2, w2);
                acc3 = _mm512_dpbusd_epi32(acc3, x3, w3);
            }

            __m512i acc = _mm512_add_epi32(_mm512_add_epi32(_mm512_add_epi32(acc0, acc1), acc2), acc3);
            __m512 b = _mm512_loadu_ps(&l1_biases[i]);

            __m512 pre = _mm512_fmadd_ps(_mm512_cvtepi32_ps(acc), _mm512_set1_ps(float(1 << SHIFT) / float(QA * QA * QB)), b);
            pre = _mm512_min_ps(_mm512_max_ps(pre, _mm512_setzero_ps()), _mm512_set1_ps(1.0f));
            pre = _mm512_mul_ps(pre, pre);

            _mm512_storeu_ps(&out[i], pre);
        }

        return out;
    }

    L2Output NNUE::l2_forward(const L1Output& in) const {
        L2Output out = {};

        for (int i = 0; i < L3_WIDTH; i += 4) {
            __m512 acc0 = _mm512_setzero_ps();
            __m512 acc1 = _mm512_setzero_ps();
            __m512 acc2 = _mm512_setzero_ps();
            __m512 acc3 = _mm512_setzero_ps();

            for (int j = 0; j < L2_WIDTH; j += 16) {
                __m512 x = _mm512_loadu_ps(&in[j]);

                __m512 w0 = _mm512_loadu_ps(&l2_weights[i + 0][j + 0]);
                __m512 w1 = _mm512_loadu_ps(&l2_weights[i + 1][j + 0]);
                __m512 w2 = _mm512_loadu_ps(&l2_weights[i + 2][j + 0]);
                __m512 w3 = _mm512_loadu_ps(&l2_weights[i + 3][j + 0]);

                acc0 = _mm512_fmadd_ps(x, w0, acc0);
                acc1 = _mm512_fmadd_ps(x, w1, acc1);
                acc2 = _mm512_fmadd_ps(x, w2, acc2);
                acc3 = _mm512_fmadd_ps(x, w3, acc3);
            }

            out[i + 0] = _mm512_reduce_add_ps(acc0) + l2_biases[i + 0];
            out[i + 1] = _mm512_reduce_add_ps(acc1) + l2_biases[i + 1];
            out[i + 2] = _mm512_reduce_add_ps(acc2) + l2_biases[i + 2];
            out[i + 3] = _mm512_reduce_add_ps(acc3) + l2_biases[i + 3];
        }

        return out;
    }

    L3Output NNUE::l3_forward(const L2Output& in) const {
        float out = 0.0f;

        for (int i = 0; i < L3_WIDTH; i += 16) {
            __m512 x = _mm512_loadu_ps(&in[i]);
            __m512 w = _mm512_loadu_ps(&l3_weights[i]);
            out += _mm512_reduce_add_ps(_mm512_mul_ps(x, w));
        }

        out += l3_bias;
        return static_cast<int32_t>(out * EVAL_SCALE);
    }
#endif
}