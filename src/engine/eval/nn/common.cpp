#include "common.h"

#include <random>

namespace episteme::eval::nn {
    Accumulator NNUE::update_accumulator(const Position& position, const Move& move, Accumulator accum) const {
        Square sq_src = move.from_square();
        Square sq_dst = move.to_square();
        std::array<Piece, 64> mailbox = position.mailbox_all();

        Piece pc_src = mailbox[sq_idx(sq_src)];
        Piece pc_dst = mailbox[sq_idx(sq_dst)];

        int w_src = piecesquare(pc_src, sq_src, false);
        int w_dst = piecesquare(pc_src, sq_dst, false);

        int b_src = piecesquare(pc_src, sq_src, true);
        int b_dst = piecesquare(pc_src, sq_dst, true);

        if (pc_dst != Piece::None){
            int w_capt = piecesquare(pc_dst, sq_dst, false);
            int b_capt = piecesquare(pc_dst, sq_dst, true);

            for (int i = 0; i < L1_WIDTH; i++) {
                accum.white[i] -= l0_weights[w_capt][i];
                accum.black[i] -= l0_weights[b_capt][i];
            }

        } else if (move.move_type() == MoveType::Castling) {
            Square rook_src, rook_dst;
            bool is_kingside = sq_dst == Square::G1 || sq_dst == Square::G8;
    
            if (is_kingside) { 
                rook_src = position.castling_rights(position.STM()).kingside;
                rook_dst = (position.STM() == Color::White) ? Square::F1 : Square::F8;
            } else { 
                rook_src = position.castling_rights(position.STM()).queenside;
                rook_dst = (position.STM() == Color::White) ? Square::D1 : Square::D8;
            }
            
            Piece rook = mailbox[sq_idx(rook_src)];
            
            int w_rook_src = piecesquare(rook, rook_src, false);
            int w_rook_dst = piecesquare(rook, rook_dst, false);

            int b_rook_src = piecesquare(rook, rook_src, true);
            int b_rook_dst = piecesquare(rook, rook_dst, true);
            
            for (int i = 0; i < L1_WIDTH; i++) {
                accum.white[i] -= l0_weights[w_rook_src][i];
                accum.white[i] += l0_weights[w_rook_dst][i];
                
                accum.black[i] -= l0_weights[b_rook_src][i];
                accum.black[i] += l0_weights[b_rook_dst][i];
            }

        } else if (move.move_type() == MoveType::EnPassant) {
            Square sq_ep = position.ep_square();
            int idx_ep = (position.STM() == Color::White) ? (sq_idx(sq_ep) - 8) : (sq_idx(sq_ep) + 8);
            Piece pc_ep = mailbox[idx_ep];

            int w_capt = piecesquare(pc_ep, sq_from_idx(idx_ep), false);
            int b_capt = piecesquare(pc_ep, sq_from_idx(idx_ep), true);

            for (int i = 0; i < L1_WIDTH; i++) {
                accum.white[i] -= l0_weights[w_capt][i];
                accum.black[i] -= l0_weights[b_capt][i];
            }
        }

        if (move.move_type() == MoveType::Promotion) {
            w_dst = piecesquare(piece_type_with_color(move.promo_piece_type(), position.STM()), sq_dst, false);
            b_dst = piecesquare(piece_type_with_color(move.promo_piece_type(), position.STM()), sq_dst, true);
        }

        for (int i = 0; i < L1_WIDTH; i++) {
            accum.white[i] -= l0_weights[w_src][i];
            accum.white[i] += l0_weights[w_dst][i];

            accum.black[i] -= l0_weights[b_src][i];
            accum.black[i] += l0_weights[b_dst][i];
        }

        return accum;
    }

    Accumulator NNUE::reset_accumulator(const Position& position) const {
        Accumulator accum = {};
        std::array<Piece, 64> mailbox = position.mailbox_all();

        for (size_t i = 0; i < 64; i++) {
            int w_psq = piecesquare(mailbox[i], sq_from_idx(i), false);
            int b_psq = piecesquare(mailbox[i], sq_from_idx(i), true);

            if (w_psq != -1) {
                for (int j = 0; j < L1_WIDTH; j++) {
                    accum.white[j] += l0_weights[w_psq][j];
                }
            }

            if (b_psq != -1) {
                for (int j = 0; j < L1_WIDTH; j++) {
                    accum.black[j] += l0_weights[b_psq][j];
                }
            }
        }
        
        for (int i = 0; i < L1_WIDTH; i++) {
            accum.white[i] += l0_biases[i];
            accum.black[i] += l0_biases[i];
        }
        
        return accum;
    }

    void NNUE::init_random() {
        std::mt19937 gen(42);
        std::uniform_int_distribution<int16_t> dist(-255, 255);

        for (auto& arr : l0_weights)
            for (auto& val : arr)
                val = dist(gen);

        for (auto& val : l0_biases)
            val = dist(gen);

        for (auto& val : l1_weights)
            val = dist(gen);

        l1_bias = dist(gen);
    }
}