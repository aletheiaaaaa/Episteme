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

    constexpr int L1_WIDTH = 1024;
    constexpr int L2_WIDTH = 16;
    constexpr int L3_WIDTH = 32;

#if defined(__AVX512F__) && defined(__AVX512BW__)
    constexpr int BLOCK_HEIGHT = 16;
#elif defined(__AVX2__)
    constexpr int BLOCK_HEIGHT = 8;
#endif

    using L0Weights = std::array<std::array<int16_t, L1_WIDTH>, 768>;
    using L1WeightsPre = std::array<std::array<int8_t, L1_WIDTH>, L2_WIDTH>;
    using L1Weights = std::array<std::array<std::array<int8_t, BLOCK_HEIGHT * 4>, L1_WIDTH / 4>, L2_WIDTH / BLOCK_HEIGHT>;
    using L2Weights = std::array<std::array<int8_t, L3_WIDTH>, L2_WIDTH>;
    using L3Weights = std::array<std::array<int8_t, 1>, L3_WIDTH>;

    using L0Biases = std::array<int16_t, L1_WIDTH>;
    using L1Biases = std::array<int32_t, L2_WIDTH>;
    using L2Biases = std::array<int16_t, L3_WIDTH>;
    using L3Bias = int32_t;

    using L0Output = std::array<uint8_t, L1_WIDTH>;
    using L1Output = std::array<int16_t, L2_WIDTH>;
    using L2Output = std::array<int16_t, L3_WIDTH>;
    using L3Output = int32_t;

    struct Accumulator {
        alignas(64) std::array<int16_t, L1_WIDTH> white = {};
        alignas(64) std::array<int16_t, L1_WIDTH> black = {};
    };

    struct NNUEData {
        alignas(64) L0Weights l0_weights;
        alignas(64) L0Biases l0_biases;
        alignas(64) L1WeightsPre l1_weights_pre;
        alignas(64) L1Biases l1_biases;
        alignas(64) L2Weights l2_weights;
        alignas(64) L2Biases l2_biases;
        alignas(64) L3Weights l3_weights;
        alignas(64) L3Bias l3_bias;

        void init_random();
    };

    class NNUE {
        public:
            NNUE(const NNUEData& data);

            Accumulator update_accumulator(const Position& position, const Move& move, Accumulator accum) const;
            Accumulator reset_accumulator(const Position& position) const;
            L0Output l0_pairwise(const Accumulator& accum, Color stm) const;
            L1Output l1_forward(const L0Output& in) const;

        private:
            alignas(64) L0Weights l0_weights;
            alignas(64) L0Biases l0_biases;
            alignas(64) L1Weights l1_weights;
            alignas(64) L1Biases l1_biases;
            alignas(64) L2Weights l2_weights;
            alignas(64) L2Biases l2_biases;
            alignas(64) L3Weights l3_weights;
            alignas(64) L3Bias l3_bias;
    };
}