#include "common.h"

namespace episteme::eval::nn {
#if !(defined(USE_AVX512) && defined(USE_VNNI)) && defined(USE_AVX2)
    L0Output NNUE::l0_pairwise(const Accumulator& accum, Color stm) const {
        L0Output out = {};

        const auto& accum_stm = (!color_idx(stm)) ? (accum.white) : (accum.black);
        const auto& accum_ntm = (!color_idx(stm)) ? (accum.black) : (accum.white);

        for (int i = 0; i < 2; i++) {
            auto accum = (i == 0) ? accum_stm : accum_ntm;
            bool is_ntm = (i == 1);

            for (int j = 0; j < L1_WIDTH / 2; j += 64) {
                auto process_chunk = [&](int offset) {
                    __m256i acc00 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&accum[j + offset + 0 + 0 * (L1_WIDTH / 2)]));
                    __m256i acc01 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&accum[j + offset + 0 + 1 * (L1_WIDTH / 2)]));
                    __m256i acc10 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&accum[j + offset + 16 + 0 * (L1_WIDTH / 2)]));
                    __m256i acc11 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&accum[j + offset + 16 + 1 * (L1_WIDTH / 2)]));

                    acc00 = _mm256_min_epi16(_mm256_max_epi16(acc00, _mm256_setzero_si256()), _mm256_set1_epi16(QA));
                    acc10 = _mm256_min_epi16(_mm256_max_epi16(acc10, _mm256_setzero_si256()), _mm256_set1_epi16(QA));
                    acc01 = _mm256_min_epi16(acc01, _mm256_set1_epi16(QA));
                    acc11 = _mm256_min_epi16(acc11, _mm256_set1_epi16(QA));

                    __m256i pair0 = _mm256_mulhi_epi16(_mm256_slli_epi16(acc00, 16 - SHIFT), acc01);
                    __m256i pair1 = _mm256_mulhi_epi16(_mm256_slli_epi16(acc10, 16 - SHIFT), acc11);

                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out[j + offset + is_ntm * (L1_WIDTH / 2)]), _mm256_packus_epi16(pair0, pair1));
                };

                process_chunk(0);
                process_chunk(32);
            }
        }

        return out;
    }

    L1Output NNUE::l1_forward(const L0Output& in) const {
        L1Output out = {};

        for (int i = 0; i < L2_WIDTH; i += L1_BLOCK_HEIGHT) {
            __m256i acc0 = _mm256_setzero_si256();
            __m256i acc1 = _mm256_setzero_si256();
            __m256i acc2 = _mm256_setzero_si256();
            __m256i acc3 = _mm256_setzero_si256();

            for (int j = 0; j < L1_WIDTH; j += 16) {
                __m256i x0 = _mm256_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 0]));
                __m256i x1 = _mm256_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 4]));
                __m256i x2 = _mm256_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 8]));
                __m256i x3 = _mm256_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 12]));

                __m256i w0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&l1_weights[i / L1_BLOCK_HEIGHT][j / 4 + 0]));
                __m256i w1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&l1_weights[i / L1_BLOCK_HEIGHT][j / 4 + 1]));
                __m256i w2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&l1_weights[i / L1_BLOCK_HEIGHT][j / 4 + 2]));
                __m256i w3 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&l1_weights[i / L1_BLOCK_HEIGHT][j / 4 + 3]));

                acc0 = _mm256_add_epi32(acc0, _mm256_madd_epi16(_mm256_maddubs_epi16(x0, w0), _mm256_set1_epi16(1)));
                acc1 = _mm256_add_epi32(acc1, _mm256_madd_epi16(_mm256_maddubs_epi16(x1, w1), _mm256_set1_epi16(1)));
                acc2 = _mm256_add_epi32(acc2, _mm256_madd_epi16(_mm256_maddubs_epi16(x2, w2), _mm256_set1_epi16(1)));
                acc3 = _mm256_add_epi32(acc3, _mm256_madd_epi16(_mm256_maddubs_epi16(x3, w3), _mm256_set1_epi16(1)));
            }

            __m256i acc = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32(acc0, acc1), acc2), acc3);
            __m256 b = _mm256_loadu_ps(&l1_biases[i]);

            __m256 pre = _mm256_fmadd_ps(_mm256_cvtepi32_ps(acc), _mm256_set1_ps(1.0f / float(QA * QA * QB >> SHIFT)), b);
            pre = _mm256_mul_ps(pre, pre);
            pre = _mm256_min_ps(_mm256_max_ps(pre, _mm256_setzero_ps()), _mm256_set1_ps(1.0f));

            _mm256_storeu_ps(&out[i], pre);
        }

        return out;
    }

    L2Output NNUE::l2_forward(const L1Output& in) const {
        L2Output out = {};

        for (int i = 0; i < L3_WIDTH; i += 4) {
            __m256 acc0 = _mm256_setzero_ps();
            __m256 acc1 = _mm256_setzero_ps();
            __m256 acc2 = _mm256_setzero_ps();
            __m256 acc3 = _mm256_setzero_ps();

            for (int j = 0; j < L2_WIDTH; j += 8) {
                __m256 x = _mm256_loadu_ps(&in[j]);

                __m256 w0 = _mm256_loadu_ps(&l2_weights[i + 0][j]);
                __m256 w1 = _mm256_loadu_ps(&l2_weights[i + 1][j]);
                __m256 w2 = _mm256_loadu_ps(&l2_weights[i + 2][j]);
                __m256 w3 = _mm256_loadu_ps(&l2_weights[i + 3][j]);

                acc0 = _mm256_fmadd_ps(x, w0, acc0);
                acc1 = _mm256_fmadd_ps(x, w1, acc1);
                acc2 = _mm256_fmadd_ps(x, w2, acc2);
                acc3 = _mm256_fmadd_ps(x, w3, acc3);
            }

            auto _mm256_reduce_add_ps = [] (const __m256& vec) {
                __m128 vec0 = _mm256_extractf128_ps(vec, 0);
                __m128 vec1 = _mm256_extractf128_ps(vec, 1);

                __m128 out = _mm_add_ps(vec0, vec1);
                out = _mm_hadd_ps(out, out);
                out = _mm_hadd_ps(out, out);

                return _mm_cvtss_f32(out);
            };

            out[i + 0] = _mm256_reduce_add_ps(acc0) + l2_biases[i + 0];
            out[i + 1] = _mm256_reduce_add_ps(acc1) + l2_biases[i + 1];
            out[i + 2] = _mm256_reduce_add_ps(acc2) + l2_biases[i + 2];
            out[i + 3] = _mm256_reduce_add_ps(acc3) + l2_biases[i + 3];
        }

        return out;
    }

    L3Output NNUE::l3_forward(const L2Output& in) const {
        __m256 acc_ = _mm256_setzero_ps();

        for (int i = 0; i < L3_WIDTH; i += 8) {
            __m256 x = _mm256_loadu_ps(&in[i]);
            __m256 w = _mm256_loadu_ps(&l3_weights[i]);

            acc_ = _mm256_fmadd_ps(x, w, acc_);
        }

        __m128 acc0 = _mm256_extractf128_ps(acc_, 0);
        __m128 acc1 = _mm256_extractf128_ps(acc_, 1);

        __m128 acc = _mm_add_ps(acc0, acc1);
        acc = _mm_hadd_ps(acc, acc);
        acc = _mm_hadd_ps(acc, acc);

        float out = _mm_cvtss_f32(acc); 

        out += l3_bias;
        return static_cast<int32_t>(out * EVAL_SCALE);
    }
#endif
}