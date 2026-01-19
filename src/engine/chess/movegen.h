#pragma once

#include "position.h"

#include <algorithm>
#include <bit>
#include <random>

namespace episteme {
    struct MoveList {
        std::array<Move, 256> list;
        size_t count = 0;

        inline void add(const Move& move) {
            list[count] = move;
            count++;
        }

        inline void clear() {
            count = 0;
        }

        inline void shuffle() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::shuffle(list.begin(), list.begin() + count, gen);
        }

        inline Move operator[](size_t idx) const {
            return list[idx];
        }
    };

    struct PawnAttacks {
        uint64_t push_1st, push_2nd, left_captures, right_captures;
    };

    const std::array<uint64_t, 64> ROOK_MAGICS = {
        0x80024000802910,
        0x100100821004040,
        0x80a000a004840008,
        0x2c08000420802008,
        0x220004a108140200,
        0x220004a108140200,
        0x220004a108140200,
        0x1000080644a0100,
        0x491101020804000,
        0x2000402914c0800,
        0x8001201800402260,
        0x100888002330004,
        0x1012482010080080,
        0xa80042100c010,
        0x880804204004140,
        0x8a0800080004229,
        0x2040a0a001001140,
        0x4000600a002000,
        0x880602004450158,
        0x4001248009000,
        0x4001120011300a00,
        0x100888002330004,
        0x5020018004044122,
        0x940801001020,
        0x240010c8400400e0,
        0x240001902028088,
        0x101020005408,
        0x1800022444002100,
        0x101022002004810,
        0x1820080050400,
        0x90104012800a0003,
        0x800804c0026480,
        0x80802010002009a0,
        0x120005644284000,
        0x2040842000411,
        0x2040842000411,
        0x2040842000411,
        0x2040842000411,
        0x100888002330004,
        0x2e508c480c020402,
        0x8002004000860800,
        0x2020134002042200,
        0x5200109200208,
        0x4001248009000,
        0xc222200100202008,
        0x41004004424a0802,
        0x100888002330004,
        0x120400050a80500,
        0x24802290410100,
        0x3a00802502504e0,
        0x420010a11820c420,
        0x880804204004140,
        0x880804204004140,
        0x204000282190220,
        0x21200380410,
        0x404020940082c020,
        0x22100884288220a,
        0x2801209200205642,
        0x104420008208012,
        0x42002824121042,
        0x8001004010059,
        0x1300800cc000101,
        0x10010028105420c,
        0x1000010020c28412,
    };

    const std::array<uint64_t, 64> BISHOP_MAGICS = {
        0x3a00802502504e0,
        0x81c0890900020a,
        0x81c0890900020a,
        0x880602004450158,
        0x880804204004140,
        0x880602004450158,
        0x3a00802502504e0,
        0xa80042100c010,
        0x880602004450158,
        0x3a00802502504e0,
        0x880804204004140,
        0x880602004450158,
        0x81c0890900020a,
        0x3a00802502504e0,
        0x240001902028088,
        0x880804204004140,
        0x880804204004140,
        0x880602004450158,
        0x6062240086001204,
        0x81c0890900020a,
        0x220102200a48,
        0x1a084010048048,
        0x880804204004140,
        0x880804204004140,
        0x880602004450158,
        0x880602004450158,
        0x2020134002042200,
        0x8080200820002,
        0x101008800400a400,
        0x2020134002042200,
        0x880602004450158,
        0x880602004450158,
        0x4402010000a000,
        0x880602004450158,
        0x2000402914c0800,
        0x902020080480080,
        0x1002410041040140,
        0x3a00802502504e0,
        0x880602004450158,
        0x880602004450158,
        0x7710512080d0c,
        0x7710512080d0c,
        0x4020244008000090,
        0x2000402914c0800,
        0x21200380410,
        0x3a00802502504e0,
        0x81c0890900020a,
        0x880602004450158,
        0x3a00802502504e0,
        0x1000c0100130001,
        0x880602004450158,
        0x880602004450158,
        0x1360004002801402,
        0x880804204004140,
        0x3a00802502504e0,
        0x81c0890900020a,
        0x7710512080d0c,
        0x880804204004140,
        0x1360004002801402,
        0x880602004450158,
        0x140000010020010,
        0x880804204004140,
        0x880602004450158,
        0x3a00802502504e0,
    };

    [[nodiscard]] std::array<uint64_t, 64> fill_king_attacks();
    [[nodiscard]] std::array<uint64_t, 64> fill_knight_attacks();
    [[nodiscard]] std::array<uint64_t, 64> fill_bishop_masks();
    [[nodiscard]] std::array<uint64_t, 64> fill_rook_masks();

    [[nodiscard]] uint64_t slow_bishop_attacks(Square square, uint64_t blockers);
    [[nodiscard]] uint64_t slow_rook_attacks(Square square, uint64_t blockers);

    [[nodiscard]] bool is_square_attacked(Square square, const Position& position, Color stm);

    [[nodiscard]] inline bool in_check(const Position& position, Color color) {
        uint64_t king_bb = position.piece_bb(PieceType::King, color);
        return is_square_attacked(sq_from_idx(std::countr_zero(king_bb)), position, flip(color));
    };

    extern const std::array<uint64_t, 64> KING_ATTACKS;
    extern const std::array<uint64_t, 64> KNIGHT_ATTACKS;

    [[nodiscard]] inline uint64_t get_king_attacks(Square square) {
        return KING_ATTACKS[sq_idx(square)];
    }

    [[nodiscard]] inline uint64_t get_knight_attacks(Square square) {
        return KNIGHT_ATTACKS[sq_idx(square)];
    }

    extern const std::array<uint64_t, 64> ROOK_MASKS;
    extern const std::array<uint64_t, 64> BISHOP_MASKS;

    template<size_t NUM_BITS, typename F>
    extern std::pair<uint64_t, std::array<uint64_t, 1 << NUM_BITS>> find_magics(Square square, std::array<uint64_t, 64> MASKS, F slow_attacks);

    [[nodiscard]] inline std::pair<uint64_t, std::array<uint64_t, 4096>> find_rook_magics(Square square) {
        return find_magics<12>(square, ROOK_MASKS, slow_rook_attacks);
    }

    [[nodiscard]] inline std::pair<uint64_t, std::array<uint64_t, 512>> find_bishop_magics(Square square) {
        return find_magics<9>(square, BISHOP_MASKS, slow_bishop_attacks);
    }

    void print_magics();

    extern const std::array<std::array<uint64_t, 4096>, 64> ROOK_ATTACKS;
    extern const std::array<std::array<uint64_t, 512>, 64> BISHOP_ATTACKS;

    template<size_t NUM_BITS>
    [[nodiscard]] inline uint64_t get_slider_attacks(Square square, const Position& position, const std::array<uint64_t, 64>& MASKS, const std::array<uint64_t, 64>& MAGICS, const std::array<std::array<uint64_t, (1 << NUM_BITS)>, 64>& ATTACKS) {
        size_t sq = sq_idx(square);
        uint64_t blockers = position.total_bb();
        uint64_t rel_blockers = (blockers & MASKS[sq]);
        uint64_t data = rel_blockers * MAGICS[sq];
        uint64_t idx = data >> (64 - NUM_BITS);
        return ATTACKS[sq][idx];
    }

    [[nodiscard]] inline uint64_t get_rook_attacks(Square square, const Position& position) {
        return get_slider_attacks<12>(square, position, ROOK_MASKS, ROOK_MAGICS, ROOK_ATTACKS);
    }

    [[nodiscard]] inline uint64_t get_bishop_attacks(Square square, const Position& position) {
        return get_slider_attacks<9>(square, position, BISHOP_MASKS, BISHOP_MAGICS, BISHOP_ATTACKS);
    }

    [[nodiscard]] inline uint64_t get_queen_attacks(Square square, const Position& position) {
        return (get_bishop_attacks(square, position) | get_rook_attacks(square, position));
    }

    template<size_t NUM_BITS>
    inline uint64_t get_slider_attacks_direct(Square square, uint64_t blockers, const std::array<uint64_t, 64>& MASKS, const std::array<uint64_t, 64>& MAGICS, const std::array<std::array<uint64_t, (1 << NUM_BITS)>, 64>& ATTACKS) {
        size_t sq = sq_idx(square);
        uint64_t rel_blockers = (blockers & MASKS[sq]);
        uint64_t data = rel_blockers * MAGICS[sq];
        uint64_t idx = data >> (64 - NUM_BITS);
        return ATTACKS[sq][idx];
    }

    [[nodiscard]] inline uint64_t get_rook_attacks_direct(Square square, uint64_t blockers) {
        return get_slider_attacks_direct<12>(square, blockers, ROOK_MASKS, ROOK_MAGICS, ROOK_ATTACKS);
    }

    [[nodiscard]] inline uint64_t get_bishop_attacks_direct(Square square, uint64_t blockers) {
        return get_slider_attacks_direct<9>(square, blockers, BISHOP_MASKS, BISHOP_MAGICS, BISHOP_ATTACKS);
    }

    [[nodiscard]] inline uint64_t get_queen_attacks_direct(Square square, uint64_t blockers) {
        return (get_bishop_attacks_direct(square, blockers) | get_rook_attacks_direct(square, blockers));
    }

    extern PawnAttacks get_pawn_attacks_helper(const Position& position, Color stm, bool is_pseudo);

    [[nodiscard]] inline PawnAttacks get_pawn_attacks(const Position& position, Color stm) {
        return get_pawn_attacks_helper(position, stm, false);
    }
    
    [[nodiscard]] inline PawnAttacks get_pawn_pseudo_attacks(const Position& position, Color stm) {
        return get_pawn_attacks_helper(position, stm, true);
    }    

    [[nodiscard]] inline uint64_t get_pawn_sq_attacks(Square square, Color stm) {
        uint64_t sq_bb = (uint64_t)1 << sq_idx(square);

        if (stm == Color::White) return ((sq_bb & ~FILE_A) << 7) | ((sq_bb & ~FILE_H) << 9);
        else return ((sq_bb & ~FILE_A) >> 9) | ((sq_bb & ~FILE_H) >> 7);
    }

    template<PieceType PT, typename F>
    extern void generate_piece_targets(MoveList& move_list, const Position& position, F get_attacks, bool include_quiets);

    extern void generate_pawn_targets(MoveList& move_list, const Position& position, bool include_quiets);

    inline void generate_knight_moves(MoveList& move_list, const Position& position) {
        generate_piece_targets<PieceType::Knight>(move_list, position, get_knight_attacks, true);
    }

    inline void generate_bishop_moves(MoveList& move_list, const Position& position) {
        generate_piece_targets<PieceType::Bishop>(move_list, position, get_bishop_attacks, true);
    }

    inline void generate_rook_moves(MoveList& move_list, const Position& position) {
        generate_piece_targets<PieceType::Rook>(move_list, position, get_rook_attacks, true);
    }

    inline void generate_queen_moves(MoveList& move_list, const Position& position) {
        generate_piece_targets<PieceType::Queen>(move_list, position, get_queen_attacks, true);
    }

    inline void generate_king_moves(MoveList& move_list, const Position& position) {
        generate_piece_targets<PieceType::King>(move_list, position, get_king_attacks, true);
    }

    inline void generate_pawn_moves(MoveList &move_list, const Position &position) {
        generate_pawn_targets(move_list, position, true);
    };

    inline void generate_knight_captures(MoveList& move_list, const Position& position) {
        generate_piece_targets<PieceType::Knight>(move_list, position, get_knight_attacks, false);
    }

    inline void generate_bishop_captures(MoveList& move_list, const Position& position) {
        generate_piece_targets<PieceType::Bishop>(move_list, position, get_bishop_attacks, false);
    }

    inline void generate_rook_captures(MoveList& move_list, const Position& position) {
        generate_piece_targets<PieceType::Rook>(move_list, position, get_rook_attacks, false);
    }

    inline void generate_queen_captures(MoveList& move_list, const Position& position) {
        generate_piece_targets<PieceType::Queen>(move_list, position, get_queen_attacks, false);
    }

    inline void generate_king_captures(MoveList& move_list, const Position& position) {
        generate_piece_targets<PieceType::King>(move_list, position, get_king_attacks, false);
    }

    inline void generate_pawn_captures(MoveList &move_list, const Position &position) {
        generate_pawn_targets(move_list, position, false);
    };

    void generate_en_passant(MoveList& move_list, const Position& position);
    void generate_castles(MoveList& move_list, const Position& position, bool is_kingside);

    void generate_all_moves(MoveList& move_list, const Position& position);
    void generate_all_captures(MoveList& move_list, const Position& position);
}
