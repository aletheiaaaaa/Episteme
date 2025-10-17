#include "evaluate.h"

INCBIN(NNUE, EVALFILE);

namespace episteme::eval {
    using namespace nn;

    const NNUE* nnue = reinterpret_cast<const NNUE*>(gNNUEData);

    Accumulator update(const Position& position, const Move& move, Accumulator accum) {
        accum = nnue->update_accumulator(position, move, accum);
        return accum;
    }

    Accumulator reset(const Position& position) {
        Accumulator accum = nnue->reset_accumulator(position);
        return accum;
    }

    int32_t evaluate(Accumulator& accum, Color stm) {
        int32_t out = nnue->l1_forward(accum, stm);
        return out;
    }

    bool SEE(const Position& position, const Move& move, int32_t threshold) {
        const Square from_sq = move.from_square();
        const Square to_sq = move.to_square();

        const uint64_t from_bb = (uint64_t)1 << sq_idx(from_sq);
        const uint64_t to_bb = (uint64_t)1 << sq_idx(to_sq);

        if (move.move_type() == MoveType::EnPassant) return (threshold <= 0);

        int32_t score = piece_vals[piece_type_idx(position.mailbox(to_sq))] - threshold;
        if (score < 0) return false;

        score = piece_vals[piece_type_idx(position.mailbox(from_sq))] - score;
        if (score <= 0) return true;

        const uint64_t pawn_bb = position.piece_type_bb(PieceType::Pawn);
        const uint64_t knight_bb = position.piece_type_bb(PieceType::Knight);
        const uint64_t bishop_bb = position.piece_type_bb(PieceType::Bishop);
        const uint64_t rook_bb = position.piece_type_bb(PieceType::Rook);
        const uint64_t queen_bb = position.piece_type_bb(PieceType::Queen);
        const uint64_t king_bb = position.piece_type_bb(PieceType::King);

        uint64_t occupied_bb = position.total_bb();
        occupied_bb &= ~(from_bb | to_bb);

        uint64_t pawn_threats = (
            (get_pawn_sq_attacks(to_sq, Color::White) & position.piece_bb(PieceType::Pawn, Color::Black)) | 
            (get_pawn_sq_attacks(to_sq, Color::Black) & position.piece_bb(PieceType::Pawn, Color::White))
        );

        uint64_t knight_threats = get_knight_attacks(to_sq) & knight_bb;
        uint64_t bishop_threats = get_bishop_attacks_direct(to_sq, occupied_bb) & bishop_bb;
        uint64_t rook_threats = get_rook_attacks_direct(to_sq, occupied_bb) & rook_bb;
        uint64_t queen_threats = get_queen_attacks_direct(to_sq, occupied_bb) & queen_bb;
        uint64_t king_threats = get_king_attacks(to_sq) & king_bb;

        uint64_t all_threats = (
            pawn_threats | knight_threats | bishop_threats | rook_threats | queen_threats | king_threats 
        );

        all_threats &= occupied_bb;

        Color stm = position.STM();
        Color win = position.STM();

        while (true) {
            stm = flip(stm);
            all_threats &= occupied_bb;

            uint64_t our_threats = all_threats & position.color_bb(stm);
            if (!our_threats) break;

            win = flip(win);

            uint64_t next_threat;
            int32_t threat_val;

            if ((next_threat = our_threats & pawn_bb)) {
                threat_val = piece_vals[piece_type_idx(PieceType::Pawn)];
                occupied_bb &= ~((uint64_t)1 << std::countr_zero(next_threat));
                all_threats |= get_bishop_attacks_direct(to_sq, occupied_bb) & (bishop_bb | queen_bb);
            } else if ((next_threat = our_threats & knight_bb)) {
                threat_val = piece_vals[piece_type_idx(PieceType::Knight)];
                occupied_bb &= ~((uint64_t)1 << std::countr_zero(next_threat));
            } else if ((next_threat = our_threats & bishop_bb)) {
                threat_val = piece_vals[piece_type_idx(PieceType::Bishop)];
                occupied_bb &= ~((uint64_t)1 << std::countr_zero(next_threat));
                all_threats |= get_bishop_attacks_direct(to_sq, occupied_bb) & (bishop_bb | queen_bb);
            } else if ((next_threat = our_threats & rook_bb)) {
                threat_val = piece_vals[piece_type_idx(PieceType::Rook)];
                occupied_bb &= ~((uint64_t)1 << std::countr_zero(next_threat));
                all_threats |= get_rook_attacks_direct(to_sq, occupied_bb) & (rook_bb | queen_bb);
            } else if ((next_threat = our_threats & queen_bb)) {
                threat_val = piece_vals[piece_type_idx(PieceType::Queen)];
                occupied_bb &= ~((uint64_t)1 << std::countr_zero(next_threat));
                all_threats |= (get_bishop_attacks_direct(to_sq, occupied_bb) & (bishop_bb | queen_bb)) | (get_rook_attacks_direct(to_sq, occupied_bb) & (rook_bb | queen_bb));
            } else {
                return (all_threats & position.color_bb(flip(stm))) ? position.STM() != win : position.STM() == win;
            }

            score = threat_val - score + 1;
            if (score <= 0) break;
        }

        return position.STM() == win;
    }
}