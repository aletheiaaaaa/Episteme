#include "nn.h"

namespace episteme::eval::nn {
// #if !(defined(__AVX512F__) && defined(__AVX512BW__)) && defined(__AVX2__)
    L0Output NNUE::l0_pairwise(const Accumulator& accum, Color stm) const {
        L0Output out;

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

                    __m256i pair0 = _mm256_mulhi_epi16(_mm256_slli_epi16(acc00, 7), acc01);
                    __m256i pair1 = _mm256_mulhi_epi16(_mm256_slli_epi16(acc10, 7), acc11);

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

        for (int i = 0; i < L2_WIDTH; i += BLOCK_HEIGHT) {
            __m256i acc0 = _mm256_setzero_si256();
            __m256i acc1 = _mm256_setzero_si256();
            __m256i acc2 = _mm256_setzero_si256();
            __m256i acc3 = _mm256_setzero_si256();

            for (int j = 0; j < L1_WIDTH; j += 16) {
                __m256i x0 = _mm256_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 0]));
                __m256i x1 = _mm256_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 4]));
                __m256i x2 = _mm256_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 8]));
                __m256i x3 = _mm256_set1_epi32(*reinterpret_cast<const uint32_t*>(&in[j + 12]));

                __m256i w0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&l1_weights[i / BLOCK_HEIGHT][j + 0]));
                __m256i w1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&l1_weights[i / BLOCK_HEIGHT][j + 1]));
                __m256i w2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&l1_weights[i / BLOCK_HEIGHT][j + 2]));
                __m256i w3 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&l1_weights[i / BLOCK_HEIGHT][j + 3]));

                acc0 = _mm256_add_epi32(acc0, _mm256_madd_epi16(_mm256_maddubs_epi16(x0, w0), _mm256_set1_epi16(1)));
                acc1 = _mm256_add_epi32(acc1, _mm256_madd_epi16(_mm256_maddubs_epi16(x1, w1), _mm256_set1_epi16(1)));
                acc2 = _mm256_add_epi32(acc2, _mm256_madd_epi16(_mm256_maddubs_epi16(x2, w2), _mm256_set1_epi16(1)));
                acc3 = _mm256_add_epi32(acc3, _mm256_madd_epi16(_mm256_maddubs_epi16(x3, w3), _mm256_set1_epi16(1)));
            }

            __m256i b = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&l1_biases[i]));

            __m256i acc = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32(acc0, acc1), acc2), acc3);
            acc = _mm256_add_epi32(acc, b);

            acc = _mm256_min_epi32(_mm256_max_epi32(acc, _mm256_setzero_si256()), _mm256_set1_epi32(QB));
            acc = _mm256_mullo_epi32(acc, acc);

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out[i]), acc);
        }

        return out;
    }

// #endif
}