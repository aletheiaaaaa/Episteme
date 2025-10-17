#pragma once

#include "../../chess/core.h"
#include "../../chess/move.h"
#include "../../chess/position.h"

#include <cstdint>
#include <cmath>
#include <array>
#include <algorithm>
#include <cstring>
#include <immintrin.h>
#include <random>

namespace episteme::eval::nn {
    constexpr int16_t QA = 255;
    constexpr int16_t QB = 64;
    constexpr int16_t EVAL_SCALE = 400;

    constexpr int L1_WIDTH = 512;

#if defined(USE_AVX512) && defined(USE_VNNI)
    constexpr int ALIGNMENT = 64;
#elif defined(USE_AVX2)
    constexpr int ALIGNMENT = 32;
#else
    constexpr int ALIGNMENT = 16;
#endif

    struct Accumulator {
        alignas(ALIGNMENT) std::array<int16_t, L1_WIDTH> white = {};
        alignas(ALIGNMENT) std::array<int16_t, L1_WIDTH> black = {};
    };

    class NNUE {
        public:
            Accumulator update_accumulator(const Position& position, const Move& move, Accumulator accum) const;
            Accumulator reset_accumulator(const Position& position) const;
            int32_t l1_forward(const Accumulator& accum, Color stm) const; 

            void init_random();

        private:
            using L0Weights = std::array<std::array<int16_t, L1_WIDTH>, 768>;
            using L0Biases = std::array<int16_t, L1_WIDTH>;
            using L1Weights = std::array<std::array<int16_t, L1_WIDTH>, 2>;
            using L1Bias = int16_t;

            alignas(ALIGNMENT) L0Weights l0_weights;
            alignas(ALIGNMENT) L0Biases l0_biases;
            alignas(ALIGNMENT) L1Weights l1_weights;
            alignas(ALIGNMENT) L1Bias l1_bias;
    };
}