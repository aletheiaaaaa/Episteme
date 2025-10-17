#include "common.h"

namespace episteme::eval::nn {
#if !(defined(USE_AVX512) && defined(USE_VNNI)) && !defined(USE_AVX2) && defined(USE_SSSE3)
        int32_t NNUE::l1_forward(const Accumulator& accum, Color stm) const {

        const auto& accum_stm = (!color_idx(stm)) ? (accum.white) : (accum.black);
        const auto& accum_ntm = (!color_idx(stm)) ? (accum.black) : (accum.white);

        __m128i temp = _mm_setzero_si128();

        for (int i = 0; i < L1_WIDTH; i += 8) {
            __m128i stm_pre = _mm_load_si128(reinterpret_cast<const __m128i*>(&accum_stm[i]));
            __m128i ntm_pre = _mm_load_si128(reinterpret_cast<const __m128i*>(&accum_ntm[i]));
            __m128i l1w0 = _mm_load_si128(reinterpret_cast<const __m128i*>(&l1_weights[0][i]));
            __m128i l1w1 = _mm_load_si128(reinterpret_cast<const __m128i*>(&l1_weights[1][i]));

            __m128i stm = _mm_min_epi16(_mm_max_epi16(stm_pre, _mm_setzero_si128()), _mm_set1_epi16(QA));
            __m128i ntm = _mm_min_epi16(_mm_max_epi16(ntm_pre, _mm_setzero_si128()), _mm_set1_epi16(QA));

            __m128i temp_stm = _mm_mullo_epi16(stm, l1w0);
            __m128i temp_ntm = _mm_mullo_epi16(ntm, l1w1);

            temp = _mm_add_epi32(temp, _mm_madd_epi16(stm, temp_stm));
            temp = _mm_add_epi32(temp, _mm_madd_epi16(ntm, temp_ntm));
        }

        auto _mm_reduce_add_epi32 = [&](__m128i vec) {
            __m128i out = _mm_hadd_epi32(vec, vec);
            out = _mm_hadd_epi32(out, out);

            return _mm_cvtsi128_si32(out);
        };

        int32_t out = _mm_reduce_add_epi32(temp);

        out /= QA;
        out += l1_bias;
        out *= EVAL_SCALE;
        out /= (QA * QB);

        return out;
    }
#endif
}