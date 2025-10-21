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
#include <iostream>
#include <bitset>

namespace episteme::eval::nn {
    constexpr int SHIFT = 9;
    constexpr int16_t QA = 255;
    constexpr int16_t QB = 64;
    constexpr int16_t EVAL_SCALE = 400;

    constexpr int L1_WIDTH = 1024;
    constexpr int L1_NNZ = 208;
    constexpr int L2_WIDTH = 128;
    constexpr int L3_WIDTH = 32;

    constexpr int BLOCK_HEIGHT = 16;
    constexpr int ALIGNMENT = 64;

    using L0Weights = std::array<std::array<int16_t, L1_WIDTH>, 768>;
    using L1Weights = std::array<std::array<int8_t, L1_NNZ>, L2_WIDTH>;
    using L2Weights = std::array<std::array<float, L2_WIDTH>, L3_WIDTH>;
    using L3Weights = std::array<float, L3_WIDTH>;

    using L0Biases = std::array<int16_t, L1_WIDTH>;
    using L1Biases = std::array<float, L2_WIDTH>;
    using L2Biases = std::array<float, L3_WIDTH>;
    using L3Bias = float;

    using L1Bitmasks = std::array<std::array<uint64_t, L1_WIDTH / 64>, L2_WIDTH / 16>;
    using L1WeightsPerm = std::array<std::array<std::array<int8_t, BLOCK_HEIGHT * 4>, L1_NNZ / 4>, L2_WIDTH / BLOCK_HEIGHT>;

    using L0Output = std::array<uint8_t, L1_WIDTH>;
    using L1Output = std::array<float, L2_WIDTH>;
    using L2Output = std::array<float, L3_WIDTH>;
    using L3Output = int32_t;

    struct Accumulator {
        alignas(ALIGNMENT) std::array<int16_t, L1_WIDTH> white = {};
        alignas(ALIGNMENT) std::array<int16_t, L1_WIDTH> black = {};
    };

    struct NNUEData {
        alignas(ALIGNMENT) L0Weights l0_weights;
        alignas(ALIGNMENT) L0Biases l0_biases;
        alignas(ALIGNMENT) L1Weights l1_weights;
        alignas(ALIGNMENT) L1Bitmasks l1_bitmasks;
        alignas(ALIGNMENT) L1Biases l1_biases;
        alignas(ALIGNMENT) L2Weights l2_weights;
        alignas(ALIGNMENT) L2Biases l2_biases;
        alignas(ALIGNMENT) L3Weights l3_weights;
        alignas(ALIGNMENT) L3Bias l3_bias;

        void init_random();
    };

    class NNUE {
        public:
            Accumulator update_accumulator(const Position& position, const Move& move, Accumulator accum) const;
            Accumulator reset_accumulator(const Position& position) const;

            L0Output l0_pairwise(const Accumulator& accum, Color stm) const;
            L1Output l1_forward(const L0Output& in) const;
            L2Output l2_forward(const L1Output& in) const;
            L3Output l3_forward(const L2Output& in) const;

        private:
            alignas(ALIGNMENT) L0Weights l0_weights;
            alignas(ALIGNMENT) L0Biases l0_biases;
            alignas(ALIGNMENT) L1WeightsPerm l1_weights;
            alignas(ALIGNMENT) L1Bitmasks l1_bitmasks;
            alignas(ALIGNMENT) L1Biases l1_biases;
            alignas(ALIGNMENT) L2Weights l2_weights;
            alignas(ALIGNMENT) L2Biases l2_biases;
            alignas(ALIGNMENT) L3Weights l3_weights;
            alignas(ALIGNMENT) L3Bias l3_bias;
    };
}