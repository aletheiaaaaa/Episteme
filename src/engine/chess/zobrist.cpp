#include "zobrist.h"

namespace episteme::hash {
    std::array<uint64_t, 768> piecesquares;
    std::array<uint64_t, 16> castling_rights;
    std::array<uint64_t, 8> ep_files;
    uint64_t stm;

    void init() {
        std::mt19937 gen(42);
        std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

        for (int i = 0; i < 12; i++) {
            for (int j = 0; j < 64; j++) {
                piecesquares[piecesquare(pc_from_idx(i), sq_from_idx(j), false)] = dist(gen);
            }
        }

        for (int i = 0; i < 8; i++) {
            ep_files[i] = dist(gen);
        }

        castling_rights[0] = 0;
        castling_rights[WHITE_KINGSIDE] = dist(gen);
        castling_rights[WHITE_QUEENSIDE] = dist(gen);
        castling_rights[BLACK_KINGSIDE] = dist(gen);
        castling_rights[BLACK_QUEENSIDE] = dist(gen);

        for (uint8_t i = 0; i < 16; i++) {
            if (std::popcount(i) < 2) continue;

            uint64_t delta = 0;

            if (i & WHITE_KINGSIDE) delta ^= castling_rights[WHITE_KINGSIDE];
            if (i & WHITE_QUEENSIDE) delta ^= castling_rights[WHITE_QUEENSIDE];
            if (i & BLACK_KINGSIDE) delta ^= castling_rights[BLACK_KINGSIDE];
            if (i & BLACK_QUEENSIDE) delta ^= castling_rights[BLACK_QUEENSIDE];

            castling_rights[i] = delta;
        }

        stm = dist(gen);
    }
}