#include "nnue.h"

namespace episteme::nn {
Accumulator NNUE::update_accumulator(
    const Position& position, const Move& move, Accumulator accum
) const {
    Square sq_src = move.from_square();
    Square sq_dst = move.to_square();
    std::array<Piece, 64> mailbox = position.mailbox_all();

    Piece pc_src = mailbox[sq_idx(sq_src)];
    Piece pc_dst = mailbox[sq_idx(sq_dst)];

    int w_src = piecesquare(pc_src, sq_src, false);
    int w_dst = piecesquare(pc_src, sq_dst, false);

    int b_src = piecesquare(pc_src, sq_src, true);
    int b_dst = piecesquare(pc_src, sq_dst, true);

    if (pc_dst != Piece::None) {
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
        w_dst = piecesquare(
            piece_type_with_color(move.promo_piece_type(), position.STM()), sq_dst, false
        );
        b_dst = piecesquare(
            piece_type_with_color(move.promo_piece_type(), position.STM()), sq_dst, true
        );
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

int32_t NNUE::l1_forward(const Accumulator& accum, Color stm) const {
    const auto& accum_stm = (!color_idx(stm)) ? (accum.white) : (accum.black);
    const auto& accum_ntm = (!color_idx(stm)) ? (accum.black) : (accum.white);

    __m256i temp_0 = _mm256_setzero_si256();
    __m256i temp_1 = _mm256_setzero_si256();
    __m256i temp_2 = _mm256_setzero_si256();
    __m256i temp_3 = _mm256_setzero_si256();

    for (int i = 0; i < L1_WIDTH; i += 64) {
        auto partial = [&](__m256i temp, int offset) {
            __m256i stm_pre =
                _mm256_load_si256(reinterpret_cast<const __m256i*>(&accum_stm[i + offset]));
            __m256i ntm_pre =
                _mm256_load_si256(reinterpret_cast<const __m256i*>(&accum_ntm[i + offset]));
            __m256i l1w0 =
                _mm256_load_si256(reinterpret_cast<const __m256i*>(&l1_weights[0][i + offset]));
            __m256i l1w1 =
                _mm256_load_si256(reinterpret_cast<const __m256i*>(&l1_weights[1][i + offset]));

            __m256i stm = _mm256_min_epi16(
                _mm256_max_epi16(stm_pre, _mm256_setzero_si256()), _mm256_set1_epi16(QA)
            );
            __m256i ntm = _mm256_min_epi16(
                _mm256_max_epi16(ntm_pre, _mm256_setzero_si256()), _mm256_set1_epi16(QA)
            );

            __m256i temp_stm = _mm256_mullo_epi16(stm, l1w0);
            __m256i temp_ntm = _mm256_mullo_epi16(ntm, l1w1);

            temp += _mm256_madd_epi16(stm, temp_stm);
            temp += _mm256_madd_epi16(ntm, temp_ntm);

            return temp;
        };

        temp_0 = partial(temp_0, 0);
        temp_1 = partial(temp_1, 16);
        temp_2 = partial(temp_2, 32);
        temp_3 = partial(temp_3, 48);
    }

    auto hadd = [&](__m256i temp) {
        __m128i temp_lo = _mm256_castsi256_si128(temp);
        __m128i temp_hi = _mm256_extracti128_si256(temp, 1);

        __m128i out_pre = _mm_hadd_epi32(temp_lo, temp_hi);
        out_pre = _mm_hadd_epi32(out_pre, out_pre);
        out_pre = _mm_hadd_epi32(out_pre, out_pre);

        return _mm_cvtsi128_si32(out_pre);
    };

    int32_t out_0 = hadd(temp_0);
    int32_t out_1 = hadd(temp_1);
    int32_t out_2 = hadd(temp_2);
    int32_t out_3 = hadd(temp_3);

    int32_t out = out_0 + out_1 + out_2 + out_3;

    out /= QA;
    out += l1_bias;
    out *= EVAL_SCALE;
    out /= (QA * QB);

    return out;
}

void NNUE::init_random() {
    std::mt19937 gen(42);
    std::uniform_int_distribution<int16_t> dist(-255, 255);

    for (auto& arr : l0_weights)
        for (auto& val : arr) val = dist(gen);

    for (auto& val : l0_biases) val = dist(gen);

    for (auto& arr : l1_weights)
        for (auto& val : arr) val = dist(gen);

    l1_bias = dist(gen);
}
}  // namespace episteme::nn