#include "common.h"

namespace episteme::eval::nn {
#if !(defined(USE_AVX512) && defined(USE_VNNI)) && defined(USE_AVX2)
        int32_t NNUE::l1_forward(const Accumulator& accum, Color stm) const {

        const auto& accum_stm = (!color_idx(stm)) ? (accum.white) : (accum.black);
        const auto& accum_ntm = (!color_idx(stm)) ? (accum.black) : (accum.white);

        __m256i temp_0 = _mm256_setzero_si256();
        __m256i temp_1 = _mm256_setzero_si256();

        for (int i = 0; i < L1_WIDTH; i += 32) {
            auto partial = [&](__m256i temp, int offset) {
                __m256i stm_pre = _mm256_load_si256(reinterpret_cast<const __m256i*>(&accum_stm[i + offset]));
                __m256i ntm_pre = _mm256_load_si256(reinterpret_cast<const __m256i*>(&accum_ntm[i + offset]));
                __m256i l1w0 = _mm256_load_si256(reinterpret_cast<const __m256i*>(&l1_weights[0][i + offset]));
                __m256i l1w1 = _mm256_load_si256(reinterpret_cast<const __m256i*>(&l1_weights[1][i + offset]));

                __m256i stm = _mm256_min_epi16(_mm256_max_epi16(stm_pre, _mm256_setzero_si256()), _mm256_set1_epi16(QA));
                __m256i ntm = _mm256_min_epi16(_mm256_max_epi16(ntm_pre, _mm256_setzero_si256()), _mm256_set1_epi16(QA));

                __m256i temp_stm = _mm256_mullo_epi16(stm, l1w0);
                __m256i temp_ntm = _mm256_mullo_epi16(ntm, l1w1);

                temp = _mm256_add_epi16(temp, _mm256_madd_epi16(stm, temp_stm));
                temp = _mm256_add_epi16(temp, _mm256_madd_epi16(ntm, temp_ntm));

                return temp;
            };

            temp_0 = partial(temp_0, 0);
            temp_1 = partial(temp_1, 16);
        }

        auto _mm256_reduce_add_epi32 = [&](__m256i vec) {
            __m128i vec0 = _mm256_extracti128_si256(vec, 0);
            __m128i vec1 = _mm256_extracti128_si256(vec, 1);

            __m128i out = _mm_hadd_epi32(vec0, vec1);
            out = _mm_hadd_epi32(out, out);
            out = _mm_hadd_epi32(out, out);

            return _mm_cvtsi128_si32(out);
        };

        int32_t out_0 = _mm256_reduce_add_epi32(temp_0);
        int32_t out_1 = _mm256_reduce_add_epi32(temp_1);

        int32_t out = out_0 + out_1;

        out /= QA;
        out += l1_bias;
        out *= EVAL_SCALE;
        out /= (QA * QB);

        return out;
    }
#endif
}