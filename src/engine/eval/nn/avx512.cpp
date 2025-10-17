#include "common.h"

namespace episteme::eval::nn {
#if defined(USE_AVX512) && defined(USE_VNNI)
    int32_t NNUE::l1_forward(const Accumulator& accum, Color stm) const {
        const auto& accum_stm = (!color_idx(stm)) ? (accum.white) : (accum.black);
        const auto& accum_ntm = (!color_idx(stm)) ? (accum.black) : (accum.white);

        __m512i temp_0 = _mm512_setzero_si512();
        __m512i temp_1 = _mm512_setzero_si512();
        __m512i temp_2 = _mm512_setzero_si512();
        __m512i temp_3 = _mm512_setzero_si512();

        for (int i = 0; i < L1_WIDTH; i += 128) {
            auto partial = [&](__m512i temp, int offset) {
                __m512i stm_ = _mm512_load_si512(reinterpret_cast<const __m512i*>(&accum_stm[i + offset]));
                __m512i ntm_ = _mm512_load_si512(reinterpret_cast<const __m512i*>(&accum_ntm[i + offset]));
                __m512i l1w0 = _mm512_load_si512(reinterpret_cast<const __m512i*>(&l1_weights[i + offset + 0 * L1_WIDTH]));
                __m512i l1w1 = _mm512_load_si512(reinterpret_cast<const __m512i*>(&l1_weights[i + offset + 1 * L1_WIDTH]));

                stm_ = _mm512_min_epi16(_mm512_max_epi16(stm_, _mm512_setzero_si512()), _mm512_set1_epi16(QA));
                ntm_ = _mm512_min_epi16(_mm512_max_epi16(ntm_, _mm512_setzero_si512()), _mm512_set1_epi16(QA));

                __m512i _stm = _mm512_mullo_epi16(stm_, l1w0);
                __m512i _ntm = _mm512_mullo_epi16(ntm_, l1w1);

                temp = _mm512_add_epi32(temp, _mm512_madd_epi16(stm_, _stm));
                temp = _mm512_add_epi32(temp, _mm512_madd_epi16(ntm_, _ntm));

                return temp;
            };

            temp_0 = partial(temp_0, 0);
            temp_1 = partial(temp_1, 32);
            temp_2 = partial(temp_2, 64);
            temp_3 = partial(temp_3, 96);
        }

        int32_t out_0 = _mm512_reduce_add_epi32(temp_0);
        int32_t out_1 = _mm512_reduce_add_epi32(temp_1);
        int32_t out_2 = _mm512_reduce_add_epi32(temp_2);
        int32_t out_3 = _mm512_reduce_add_epi32(temp_3);

        int32_t out = out_0 + out_1 + out_2 + out_3;

        out /= QA;
        out += l1_bias;
        out *= EVAL_SCALE;
        out /= (QA * QB);

        return out;
    }
#endif
}