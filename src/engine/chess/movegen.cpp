#include "movegen.h"

#include <format>

namespace episteme {
    std::array<uint64_t, 64> fill_king_attacks() {
        std::array<uint64_t, 64> king_attacks;
        king_attacks.fill(0);
        for (int i = 0; i < 64; i++) {
            uint64_t square = (uint64_t)1 << i;
            uint64_t pattern = shift_north(square) | shift_north(shift_east(square)) 
                | shift_east(square) | shift_south(shift_east(square)) 
                | shift_south(square)  | shift_south(shift_west(square)) 
                | shift_west(square)  | shift_north(shift_west(square));
            king_attacks[i] = pattern;
        }
        return king_attacks;
    }

    std::array<uint64_t, 64> fill_knight_attacks() {
        std::array<uint64_t, 64> knight_attacks;
        knight_attacks.fill(0);
        for (int i = 0; i < 64; i++) {
            uint64_t square = (uint64_t)1 << i;
            uint64_t pattern = shift_west(shift_north(shift_north(square))) | shift_east(shift_north(shift_north(square)))
                | shift_north(shift_east(shift_east(square)))  | shift_south(shift_east(shift_east(square)))
                | shift_east(shift_south(shift_south(square))) | shift_west(shift_south(shift_south(square)))
                | shift_south(shift_west(shift_west(square)))  | shift_north(shift_west(shift_west(square)));
            knight_attacks[i] = pattern;
        }
        return knight_attacks;
    }

    const std::array<uint64_t, 64> KING_ATTACKS = fill_king_attacks();
    const std::array<uint64_t, 64> KNIGHT_ATTACKS = fill_knight_attacks();

    std::array<uint64_t, 64> fill_rook_masks() {
        std::array<uint64_t, 64> rook_masks;
        rook_masks.fill(0);
        for (int i = 0; i < 64; i++) {
            uint64_t square = 0;
            square |= ((uint64_t)0x7E << 8 * (i / 8)) | ((uint64_t)0x1010101010100 << (i % 8));
            square &= ~((uint64_t)1 << i);
            rook_masks[i] = square;
        }
        return rook_masks;
    }

    std::array<uint64_t, 64> fill_bishop_masks() {
        std::array<uint64_t, 64> bishop_masks;
        bishop_masks.fill(0); 
        for (int i = 0; i < 64; i++) {
            uint64_t diagonal = 0x8040201008040201;
            uint64_t anti_diagonal = 0x0102040810204080;
            uint64_t square = 0;
            int shift = (i / 8) - (i % 8);
            int anti_shift = (i % 8) - (7 - (i / 8));
            diagonal = (shift > 0) ? (diagonal << (shift * 8)) : (diagonal >> -(shift * 8));
            anti_diagonal = (anti_shift > 0) ? (anti_diagonal << (anti_shift * 8)) : (anti_diagonal >> -(anti_shift * 8));
            square |= (diagonal | anti_diagonal);
            square &= ~((uint64_t)1 << i);
            bishop_masks[i] = square & ~(0xFF818181818181FF);
        }
        return bishop_masks;
    }

    const std::array<uint64_t, 64> ROOK_MASKS = fill_rook_masks();
    const std::array<uint64_t, 64> BISHOP_MASKS = fill_bishop_masks();

    uint64_t slow_rook_attacks(Square square, uint64_t blockers) {
        uint64_t rook_attacks = 0;
        size_t sq = sq_idx(square);
        uint64_t sq_bb = (uint64_t)1 << sq;

        auto generate_ray = [sq_bb, &rook_attacks, &blockers](auto shift_dir) {
            uint64_t attack_bb = sq_bb;
            do {
                attack_bb = shift_dir(attack_bb);
                rook_attacks |= attack_bb;
            } while (attack_bb && !(attack_bb & blockers));
        };

        generate_ray(shift_north);
        generate_ray(shift_east);
        generate_ray(shift_south);
        generate_ray(shift_west);

        return rook_attacks;
    }
    
    uint64_t slow_bishop_attacks(Square square, uint64_t blockers) {
        uint64_t bishop_attacks = 0;
        size_t sq = sq_idx(square);
        uint64_t sq_bb = (uint64_t)1 << sq;

        auto generate_ray = [sq_bb, &bishop_attacks, &blockers](auto shift_dir1, auto shift_dir2) {
            uint64_t attack_bb = sq_bb;
            do {
                attack_bb = shift_dir2(shift_dir1(attack_bb));
                bishop_attacks |= attack_bb;
            } while (attack_bb && !(attack_bb & blockers));
        };

        generate_ray(shift_north, shift_east);
        generate_ray(shift_south, shift_east);
        generate_ray(shift_south, shift_west);
        generate_ray(shift_north, shift_west);

        return bishop_attacks;
    }
        
    template<size_t NUM_BITS, typename F>
    std::array<uint64_t, (1 << NUM_BITS)> fill_sq_attack (Square square, std::array<uint64_t, 64> MASKS, F slow_attacks) {
        constexpr size_t ARR_SIZE = 1 << NUM_BITS;
        std::array<uint64_t, ARR_SIZE> attacks; 
        uint64_t mask = MASKS[sq_idx(square)];

        uint64_t submask = 0;
        size_t num_moves = 0;
        do {
            attacks[num_moves] = slow_attacks(square, submask);
            submask = (submask - mask) & mask;
            num_moves++;
        } while (submask);

        while (num_moves < ARR_SIZE) {
            attacks[num_moves] = 0;
            num_moves++; 
        }

        return attacks;
    }

    template<size_t NUM_BITS, typename F>
    std::pair<uint64_t, std::array<uint64_t, (1 << NUM_BITS)>> find_magics(Square square, std::array<uint64_t, 64> MASKS, F slow_attacks) {
        constexpr size_t ARR_SIZE = 1 << NUM_BITS;
        auto attacks = fill_sq_attack<NUM_BITS>(square, MASKS, slow_attacks);
        std::array<uint64_t, ARR_SIZE> submasks;
        std::array<uint64_t, ARR_SIZE> used_indices;
        uint64_t mask = MASKS[sq_idx(square)];

        uint64_t submask = 0;
        int num_moves = 0;
        do {
            submasks[num_moves] = submask;
            submask = (submask - mask) & mask;
            num_moves++;
        } while (submask);

        std::mt19937 gen(42);
        std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
        bool fail;
        uint64_t magic;
        do {
            used_indices.fill(0);
            magic = dist(gen) & dist(gen) & dist(gen);
            fail = false;
            for (int i = 0; (!fail) && (i < num_moves); i++) {
                const auto magic_idx = (submasks[i] * magic) >> (64 - NUM_BITS);
                if (used_indices[magic_idx] == 0) {
                    used_indices[magic_idx] = attacks[i];
                } else if (used_indices[magic_idx] != attacks[i]) {
                    fail = true;
                }
            }
        } while (fail);

        return {magic, used_indices};
    }

    void print_magics() {
        std::cout << "const std::array<uint64_t, 64> ROOK_MAGICS = {";
        for (int i = 0; i < 64; i++) {
            uint64_t rook_magic = find_rook_magics(sq_from_idx(i)).first;
            std::cout << std::format("0x{:x},\n", rook_magic);
        }
        std::cout << "}\nconst std::array<uint64_t, 64> BISHOP_MAGICS = {";
        for (int i = 0; i < 64; i++) {
            uint64_t bishop_magic = find_bishop_magics(sq_from_idx(i)).first;
            std::cout << std::format("0x{:x},\n", bishop_magic);
        }
        std::cout << "}";
    }

    template<size_t NUM_BITS, typename F>
    std::array<std::array<uint64_t, (1 << NUM_BITS)>, 64> fill_attacks(const std::array<uint64_t, 64>& MAGICS, const std::array<uint64_t, 64>& MASKS, F slow_attacks) {
        constexpr size_t ARR_SIZE = 1 << NUM_BITS;
        std::array<std::array<uint64_t, ARR_SIZE>, 64> attack_tables{};
    
        for (int sq = 0; sq < 64; ++sq) {
            const uint64_t mask = MASKS[sq];
            std::array<uint64_t, ARR_SIZE> table{};
            uint64_t submask = 0;
            do {
                const uint64_t index = (submask * MAGICS[sq]) >> (64 - NUM_BITS);
                table[index] = slow_attacks(sq_from_idx(sq), submask);
                submask = (submask - mask) & mask;
            } while (submask);
            attack_tables[sq] = table;
        }
    
        return attack_tables;
    }
    
    std::array<std::array<uint64_t, 4096>, 64> fill_rook_attacks() {
        return fill_attacks<12>(ROOK_MAGICS, ROOK_MASKS, slow_rook_attacks);
    }

    std::array<std::array<uint64_t, 512>, 64> fill_bishop_attacks() {
        return fill_attacks<9>(BISHOP_MAGICS, BISHOP_MASKS, slow_bishop_attacks);
    }

    const std::array<std::array<uint64_t, 4096>, 64> ROOK_ATTACKS = fill_rook_attacks();
    const std::array<std::array<uint64_t, 512>, 64> BISHOP_ATTACKS = fill_bishop_attacks();

    PawnAttacks get_pawn_attacks_helper(const Position& position, Color stm, bool is_pseudo) {
        uint64_t us_bb = position.color_bb(stm);
        uint64_t them_bb = position.color_bb(flip(stm));
        uint64_t pawn_bb = position.piece_type_bb(PieceType::Pawn) & us_bb;
        
        uint64_t occupied = us_bb | (is_pseudo ? 0ULL : them_bb); 
        uint64_t captures_mask = is_pseudo ? ~0ULL : them_bb;
    
        PawnAttacks attacks;
    
        if (stm == Color::White) {
            attacks.push_1st = (pawn_bb << 8) & ~occupied;
            attacks.push_2nd = ((attacks.push_1st & RANK_3) << 8) & ~occupied;
            attacks.left_captures = ((pawn_bb & ~FILE_A) << 7) & captures_mask;
            attacks.right_captures = ((pawn_bb & ~FILE_H) << 9) & captures_mask;
        } else {
            attacks.push_1st = (pawn_bb >> 8) & ~occupied;
            attacks.push_2nd = ((attacks.push_1st & RANK_6) >> 8) & ~occupied;
            attacks.left_captures = ((pawn_bb & ~FILE_A) >> 9) & captures_mask;
            attacks.right_captures = ((pawn_bb & ~FILE_H) >> 7) & captures_mask;
        }
    
        return attacks;
    }

    bool is_square_attacked(Square square, const Position& position, Color nstm) {
        uint64_t square_bb = uint64_t(1) << sq_idx(square);
        uint64_t them_bb = position.color_bb(nstm);
        
        uint64_t knights = position.piece_type_bb(PieceType::Knight);
        uint64_t knight_attacks = get_knight_attacks(square) & knights & them_bb;

        uint64_t queens = position.piece_type_bb(PieceType::Queen);
        uint64_t bishops_and_queens = position.piece_type_bb(PieceType::Bishop) | queens;
        uint64_t bishop_attacks = get_bishop_attacks(square, position) & bishops_and_queens & them_bb;
        uint64_t rooks_and_queens = position.piece_type_bb(PieceType::Rook) | queens;
        uint64_t rook_attacks = get_rook_attacks(square, position) & rooks_and_queens & them_bb;
        
        uint64_t kings = position.piece_type_bb(PieceType::King);
        uint64_t king_attacks = get_king_attacks(square) & kings & them_bb;
    
        if ((knight_attacks | bishop_attacks | rook_attacks | king_attacks) != 0) {
            return true;
        }
    
        PawnAttacks pawn_attacks = get_pawn_pseudo_attacks(position, nstm);
        if (((pawn_attacks.left_captures | pawn_attacks.right_captures) & square_bb) != 0) {
            return true;
        }

        return false;
    }
    
    template<PieceType PT, typename F>
    void generate_piece_targets(MoveList& move_list, const Position& position, F get_attacks, bool include_quiets) {
        uint64_t us_bb = position.color_bb(position.STM());
        uint64_t them_bb = position.color_bb(position.NTM());
        uint64_t piece_bb = position.piece_type_bb(PT) & us_bb;

        while (piece_bb != 0) {
            Square from_sq = sq_from_idx(std::countr_zero(piece_bb));

            uint64_t attacks_bb;
            if constexpr (std::is_invocable_r<uint64_t, F, Square, const Position&>::value) {
                attacks_bb = get_attacks(from_sq, position);
            } else {
                attacks_bb = get_attacks(from_sq);
            }

            uint64_t targets_bb = (include_quiets) 
                ? (attacks_bb & ~us_bb)
                : (attacks_bb & ~us_bb & them_bb);

            while (targets_bb != 0) {
                Square to_sq = sq_from_idx(std::countr_zero(targets_bb));
                move_list.add({from_sq, to_sq});
                targets_bb &= targets_bb - 1;
            }

            piece_bb &= piece_bb - 1;
        }
    }

    void generate_pawn_targets(MoveList& move_list, const Position& position, bool include_quiets) {
        Color stm = position.STM();
        PawnAttacks attacks = get_pawn_attacks(position, stm);
        uint64_t promo_rank = (stm == Color::White) ? (RANK_8) : (RANK_1);
        constexpr std::array<size_t, 2> left_capture_shift{ 7, 9 };
        constexpr std::array<size_t, 2> right_capture_shift{ 9, 7 };
        constexpr size_t push1st_shift = 8;
        constexpr size_t push2nd_shift = 16;

        auto get_from_sq = [stm](uint64_t attacks_bb, size_t shift) {
            if (stm == Color::White) {
                return sq_from_idx(std::countr_zero(attacks_bb) - shift);
            } else {
                return sq_from_idx(std::countr_zero(attacks_bb) + shift);
            }
        };

        auto add_move = [promo_rank, &move_list](Square from_sq, Square to_sq) {
            if ((((uint64_t)1 << sq_idx(to_sq)) & promo_rank) != 0) {
                for (auto promo_piece : {PromoPiece::Knight, PromoPiece::Bishop, PromoPiece::Rook, PromoPiece::Queen}) {
                    move_list.add({from_sq, to_sq, MoveType::Promotion, promo_piece});
                }
            } else {
                move_list.add({from_sq, to_sq});
            };
        };

        auto attacks2Moves = [&](uint64_t attacks_bb, size_t shift) {
            while (attacks_bb != 0) {
                Square from_sq = get_from_sq(attacks_bb, shift);
                Square to_sq = sq_from_idx(std::countr_zero(attacks_bb));
                add_move(from_sq, to_sq);
                attacks_bb &= attacks_bb - 1;
            }
        };

        if (include_quiets) {
            attacks2Moves(attacks.push_1st, push1st_shift);
            attacks2Moves(attacks.push_2nd, push2nd_shift);
        }

        attacks2Moves(attacks.left_captures, left_capture_shift[color_idx(stm)]);
        attacks2Moves(attacks.right_captures, right_capture_shift[color_idx(stm)]);
    }

    void generate_en_passant(MoveList& move_list, const Position& position) {
        Square ep_sq = position.ep_square();
        Color stm = position.STM();
        uint64_t ep_bb = (uint64_t)1 << sq_idx(ep_sq);
        uint64_t pawn_bb = position.piece_bb(PieceType::Pawn, position.STM());

        uint64_t left_attacks = (stm == Color::White) ? (((ep_bb & ~FILE_A) >> 9) & pawn_bb) : (((ep_bb & ~FILE_A) << 7) & pawn_bb);
        uint64_t right_attacks = (stm == Color::White) ? (((ep_bb & ~FILE_H) >> 7) & pawn_bb) : (((ep_bb & ~FILE_H) << 9) & pawn_bb);

        if (left_attacks != 0) {
            Square from_sq = sq_from_idx(std::countr_zero(left_attacks));
            move_list.add({from_sq, ep_sq, MoveType::EnPassant});
        }
        if (right_attacks != 0) {
            Square from_sq = sq_from_idx(std::countr_zero(right_attacks));
            move_list.add({from_sq, ep_sq, MoveType::EnPassant});
        }
    }

    void generate_castles(MoveList& move_list, const Position& position, bool is_kingside) {
        Color stm = position.STM();
        uint64_t king_bb = position.piece_bb(PieceType::King, stm);
        constexpr std::array<std::array<Square, 2>, 2> king_ends = {{
            {Square::C1, Square::G1},
            {Square::C8, Square::G8}
        }};

        Square king_src = sq_from_idx(std::countr_zero(king_bb));
        Square king_dst = king_ends[color_idx(stm)][is_kingside];

        Square rook_src = is_kingside ? position.castling_rights(stm).kingside : position.castling_rights(stm).queenside;

        if (is_square_attacked(king_src, position, position.NTM())) {
            return;
        }

        size_t king_start = std::min(sq_idx(king_src), sq_idx(king_dst));
        size_t king_end = std::max(sq_idx(king_src), sq_idx(king_dst));

        size_t start = std::min(sq_idx(rook_src), sq_idx(king_src));
        size_t end = std::max(sq_idx(rook_src), sq_idx(king_src));

        for (size_t sq = (start + 1); sq <= (end - 1); sq++) {
            Piece piece = position.mailbox(sq);
            if (piece != Piece::None) {
                return;
            }
        }

        for (size_t sq = king_start; sq <= king_end; sq++) {
            if ((sq != sq_idx(king_src)) && is_square_attacked(sq_from_idx(sq), position, position.NTM())) {
                return;
            }
        }

        move_list.add({king_src, king_dst, MoveType::Castling});
    }

    void generate_all_moves(MoveList& move_list, const Position& position) {
        generate_pawn_moves(move_list, position);
        generate_knight_moves(move_list, position);
        generate_bishop_moves(move_list, position);
        generate_rook_moves(move_list, position);
        generate_queen_moves(move_list, position);
        generate_king_moves(move_list, position);

        if (position.ep_square() != Square::None) {
            generate_en_passant(move_list, position);
        }

        Square kingside_castle = position.castling_rights(position.STM()).kingside;
        Square queenside_castle = position.castling_rights(position.STM()).queenside;

        if (kingside_castle != Square::None) {
            generate_castles(move_list, position, true);
        }
        if (queenside_castle != Square::None) {
            generate_castles(move_list, position, false);
        }
    }

    void generate_all_captures(MoveList& move_list, const Position& position) {
        generate_pawn_captures(move_list, position);
        generate_knight_captures(move_list, position);
        generate_bishop_captures(move_list, position);
        generate_rook_captures(move_list, position);
        generate_queen_captures(move_list, position);
        generate_king_captures(move_list, position);

        if (position.ep_square() != Square::None) {
            generate_en_passant(move_list, position);
        }
    }
}
