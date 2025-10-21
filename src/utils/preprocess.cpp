#include <fstream>
#include <array>
#include <vector>
#include <cstdint>
#include <iostream>
#include <bit>

constexpr int L1_WIDTH = 1024;
constexpr int L2_WIDTH = 128;
constexpr int L3_WIDTH = 32;

struct NNUERaw {
    alignas(64) std::array<std::array<int16_t, L1_WIDTH>, 768> l0_weights;
    alignas(64) std::array<int16_t, L1_WIDTH> l0_biases;
    alignas(64) std::array<std::array<int8_t, L1_WIDTH>, L2_WIDTH> l1_weights;
    alignas(64) std::array<float, L2_WIDTH> l1_biases;
    alignas(64) std::array<std::array<float, L2_WIDTH>, L3_WIDTH> l2_weights;
    alignas(64) std::array<float, L3_WIDTH> l2_biases;
    alignas(64) std::array<float, L3_WIDTH> l3_weights;
    alignas(64) float l3_bias;
};

constexpr int MAX_NNZ = 208;

struct NNUECompressed {
    alignas(64) std::array<std::array<int16_t, L1_WIDTH>, 768> l0_weights;
    alignas(64) std::array<int16_t, L1_WIDTH> l0_biases;
    alignas(64) std::array<std::array<int8_t, MAX_NNZ>, L2_WIDTH> l1_weights;
    alignas(64) std::array<std::array<uint64_t, L1_WIDTH / 64>, L2_WIDTH> l1_bitmasks;
    alignas(64) std::array<float, L2_WIDTH> l1_biases;
    alignas(64) std::array<std::array<float, L2_WIDTH>, L3_WIDTH> l2_weights;
    alignas(64) std::array<float, L3_WIDTH> l2_biases;
    alignas(64) std::array<float, L3_WIDTH> l3_weights;
    alignas(64) float l3_bias;
};

NNUECompressed process_net(const NNUERaw& raw) {
    NNUECompressed compressed;

    compressed.l0_weights = raw.l0_weights;
    compressed.l0_biases = raw.l0_biases;
    compressed.l1_biases = raw.l1_biases;
    compressed.l2_weights = raw.l2_weights;
    compressed.l2_biases = raw.l2_biases;
    compressed.l3_weights = raw.l3_weights;
    compressed.l3_bias = raw.l3_bias;

    // Process in chunks of 16 rows
    for (int chunk = 0; chunk < L2_WIDTH / 16; ++chunk) {
        // Compute OR'd bitmask across 16 rows in this chunk
        std::array<uint64_t, L1_WIDTH / 64> chunk_bitmask{};
        for (int row = 0; row < 16; ++row) {
            int i = chunk * 16 + row;
            for (int j = 0; j < L1_WIDTH; ++j) {
                if (raw.l1_weights[i][j] != 0) {
                    chunk_bitmask[j / 64] |= (1ULL << (j % 64));
                }
            }
        }

        // Print the first bitmask (first 64 bits) for each row in this chunk
        std::cout << "Chunk " << chunk << " bitmask[0]: 0x" << std::hex << chunk_bitmask[0] << std::dec << std::endl;
        for (int row = 0; row < 16; ++row) {
            int i = chunk * 16 + row;
            uint64_t row_bitmask_0 = 0;
            for (int j = 0; j < 64; ++j) {
                if (raw.l1_weights[i][j] != 0) {
                    row_bitmask_0 |= (1ULL << j);
                }
            }
            std::cout << "  Row " << i << " bitmask[0]: 0x" << std::hex << row_bitmask_0 << std::dec << std::endl;
        }

        // Compress each row using the chunk bitmask
        for (int row = 0; row < 16; ++row) {
            int i = chunk * 16 + row;
            compressed.l1_weights[i].fill(0);
            int nnz_count = 0;

            for (int j = 0; j < L1_WIDTH; ++j) {
                if (chunk_bitmask[j / 64] & (1ULL << (j % 64))) {
                    compressed.l1_weights[i][nnz_count++] = raw.l1_weights[i][j];
                }
            }

            compressed.l1_bitmasks[i] = chunk_bitmask;
        }
    }

    return compressed;
}

int main() {
    std::ifstream infile("aquamarine_raw.bin", std::ios::binary);
    NNUERaw raw;
    infile.read(reinterpret_cast<char*>(&raw), sizeof(NNUERaw));
    infile.close();

    NNUECompressed compressed = process_net(raw);

    std::ofstream outfile("aquamarine_1024_128.bin", std::ios::binary);
    outfile.write(reinterpret_cast<const char*>(&compressed), sizeof(NNUECompressed));
    outfile.close();

    return 0;
}