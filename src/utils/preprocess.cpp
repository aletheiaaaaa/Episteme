#include <fstream>
#include <array>
#include <vector>
#include <cstdint>
#include <iostream>
#include <bit>
#include <variant>

constexpr int L1_WIDTH = 1024;
constexpr int L2_WIDTH = 128;
constexpr int L3_WIDTH = 32;

constexpr int L1_NNZ = 208;

using L0Weights = std::array<std::array<int16_t, L1_WIDTH>, 768>;
using L1Weights = std::array<std::array<int8_t, L1_WIDTH>, L2_WIDTH>;
using L2Weights = std::array<std::array<float, L2_WIDTH>, L3_WIDTH>;
using L3Weights = std::array<float, L3_WIDTH>;

using L0Biases = std::array<int16_t, L1_WIDTH>;
using L1Biases = std::array<float, L2_WIDTH>;
using L2Biases = std::array<float, L3_WIDTH>;
using L3Bias = float;

using L1Bitmask = std::array<std::array<uint64_t, L1_WIDTH / 64>, L2_WIDTH / 16>;
using L1Nonzeros = std::array<std::array<int8_t, L1_NNZ>, L2_WIDTH>;
using L1Transpose = std::array<std::array<std::array<int8_t, 16 * 4>, L1_NNZ / 4>, L2_WIDTH / 16>;

struct NNUERaw {
    alignas(64) L0Weights l0_weights;
    alignas(64) L0Biases l0_biases;
    alignas(64) L1Weights l1_weights;
    alignas(64) L1Biases l1_biases;
    alignas(64) L2Weights l2_weights;
    alignas(64) L2Biases l2_biases;
    alignas(64) L3Weights l3_weights;
    alignas(64) L3Bias l3_bias;
};

struct NNUEReordered {
    alignas(64) L0Weights l0_weights;
    alignas(64) L0Biases l0_biases;
    alignas(64) L1Weights l1_weights;
    alignas(64) L1Bitmask l1_bitmasks;
    alignas(64) L1Biases l1_biases;
    alignas(64) L2Weights l2_weights;
    alignas(64) L2Biases l2_biases;
    alignas(64) L3Weights l3_weights;
    alignas(64) L3Bias l3_bias;
};

struct NNUECompressed {
    alignas(64) L0Weights l0_weights;
    alignas(64) L0Biases l0_biases;
    alignas(64) L1Nonzeros l1_weights;
    alignas(64) L1Bitmask l1_bitmasks;
    alignas(64) L1Biases l1_biases;
    alignas(64) L2Weights l2_weights;
    alignas(64) L2Biases l2_biases;
    alignas(64) L3Weights l3_weights;
    alignas(64) L3Bias l3_bias;
};

struct NNUETransposed {
    alignas(64) L0Weights l0_weights;
    alignas(64) L0Biases l0_biases;
    alignas(64) L1Transpose l1_weights;
    alignas(64) L1Bitmask l1_bitmasks;
    alignas(64) L1Biases l1_biases;
    alignas(64) L2Weights l2_weights;
    alignas(64) L2Biases l2_biases;
    alignas(64) L3Weights l3_weights;
    alignas(64) L3Bias l3_bias;
};

template <typename T, typename U>
void copy_common_fields(T& dst, const U& src) {
    dst.l0_weights = src.l0_weights;
    dst.l0_biases = src.l0_biases;
    dst.l1_biases = src.l1_biases;
    dst.l2_weights = src.l2_weights;
    dst.l2_biases = src.l2_biases;
    dst.l3_weights = src.l3_weights;
    dst.l3_bias = src.l3_bias;
}

NNUEReordered reorder_net(const NNUERaw& raw) {
    NNUEReordered reordered;
    copy_common_fields(reordered, raw);

    std::array<int, 8> perm = {0, 4, 1, 5, 2, 6, 3, 7};
    
    for (int i = 0; i < L2_WIDTH; ++i) {
        for (int j = 0; j < L1_WIDTH; ++j) {
            int byte_group = j / 8;
            int byte_in_group = j % 8;
            int qword = j / 64;

            int orig_byte_group = perm[byte_group % 8];
            int orig_j = qword * 64 + orig_byte_group * 8 + byte_in_group;
            
            reordered.l1_weights[i][j] = raw.l1_weights[i][orig_j];
        }
    }

    for (int chunk = 0; chunk < L2_WIDTH / 16; ++chunk) {
        std::array<uint64_t, L1_WIDTH / 64> chunk_bitmask{};
        for (int row = 0; row < 16; ++row) {
            int i = chunk * 16 + row;
            for (int j = 0; j < L1_WIDTH; ++j) {
                if (reordered.l1_weights[i][j] != 0) {
                    chunk_bitmask[j / 64] |= (1ULL << (j % 64));
                }
            }
        }

        reordered.l1_bitmasks[chunk] = chunk_bitmask;
    }

    return reordered;
}

NNUECompressed compress_net(const NNUEReordered& reordered) {
    NNUECompressed compressed;
    copy_common_fields(compressed, reordered);
    compressed.l1_bitmasks = reordered.l1_bitmasks;

    for (int chunk = 0; chunk < L2_WIDTH / 16; ++chunk) {
        for (int row = 0; row < 16; ++row) {
            int i = chunk * 16 + row;
            compressed.l1_weights[i].fill(0);
            int nnz_count = 0;

            for (int j = 0; j < L1_WIDTH; ++j) {
                if (reordered.l1_bitmasks[chunk][j / 64] & (1ULL << (j % 64))) {
                    compressed.l1_weights[i][nnz_count++] = reordered.l1_weights[i][j];
                }
            }
        }
    }

    return compressed;
}

NNUETransposed transpose_net(const NNUECompressed& permuted) {
    NNUETransposed transposed;

    copy_common_fields(transposed, permuted);
    transposed.l1_bitmasks = permuted.l1_bitmasks;

    for (int i = 0; i < L2_WIDTH; ++i) {
        for (int j = 0; j < L1_NNZ; ++j) {
            int block_row = i / 16;
            int block_col = j / 4;
            int in_block_row = i % 16;
            int in_block_col = j % 4;
            transposed.l1_weights[block_row][block_col][in_block_row * 4 + in_block_col] = permuted.l1_weights[i][j];
        }
    }

    return transposed;
}

int main(int argc, char* argv[]) {
    std::ifstream infile(argv[1], std::ios::binary);

    NNUERaw raw;
    infile.read(reinterpret_cast<char*>(&raw), sizeof(NNUERaw));
    infile.close();

    NNUETransposed transposed = transpose_net(compress_net(reorder_net(raw)));

    std::ofstream outfile(argv[2], std::ios::binary | std::ios::ate);
    outfile.write(reinterpret_cast<const char*>(&transposed), sizeof(NNUETransposed));
    outfile.close();

    return 0;
}