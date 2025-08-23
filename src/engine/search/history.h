#pragma once

#include "../chess/move.h"
#include "stack.h"

#include <array>
#include <cstdint>
#include <algorithm>

namespace episteme::hist {
    constexpr int MAX_HIST = 16384;
    constexpr int MAX_CORR_HIST = 128;

    [[nodiscard]] inline int16_t bonus(int16_t depth) {
        return static_cast<int16_t>(std::clamp(depth * 300, 0, 2500));
    }

    struct Entry {
        int32_t value = 0;

        inline void update(int16_t bonus, int32_t max) {
            value += bonus - value * std::abs(bonus) / max;
        }
    };

    class Table {
        public:
            [[nodiscard]] inline int32_t get_quiet_hist(Color stm, Move move) {
                return quiet_hist[color_idx(stm)][sq_idx(move.from_square())][sq_idx(move.to_square())].value;
            }

            [[nodiscard]] inline int32_t get_cont_hist(stack::Stack& stack, Piece piece, Move move, int16_t ply) {
                int32_t value = 0;

                auto get_hist = [&](int16_t diff) {
                    int16_t ply_value = 0;
                    if (ply > diff - 1) {
                        Move prev_move = stack[ply - diff].move;
                        Piece prev_piece = stack[ply - diff].piece;

                        if (prev_piece != Piece::None) ply_value += cont_hist[piece_idx(piece)][sq_idx(move.to_square())][piece_idx(prev_piece)][sq_idx(prev_move.to_square())].value;
                    }
                    return ply_value;
                };

                value += get_hist(1);
                value += get_hist(2);

                return value;
            }

            [[nodiscard]] inline int32_t get_hist(stack::Stack& stack, Piece attacker, Piece victim, Move move, Color stm, int16_t ply) {
                int32_t value = 0;

                value += get_quiet_hist(stm, move);
                value += get_cont_hist(stack, attacker, move, ply);
                if (victim != Piece::None) value += get_capt_hist(attacker, move, victim);

                return value;
            }

            [[nodiscard]] inline int32_t get_capt_hist(Piece attacker, Move move, Piece victim) {
                return capt_hist[piece_idx(attacker)][sq_idx(move.to_square())][piece_type_idx(victim)].value;
            }

            [[nodiscard]] inline int32_t get_pawn_corr_hist(uint64_t pawn_hash, Color stm) {
                return corr_hist[color_idx(stm)][pawn_hash % 16384].value;
            }

            inline void update_quiet_hist(Color stm, Move move, int16_t bonus) {
                quiet_hist[color_idx(stm)][sq_idx(move.from_square())][sq_idx(move.to_square())].update(bonus, MAX_HIST);
            }

            inline void update_cont_hist(stack::Stack& stack, Piece piece, Move move, int16_t bonus, int16_t ply) {
                auto update_hist = [&](int16_t diff) {
                    if (ply > diff - 1) {
                        Move prev_move = stack[ply - diff].move;
                        Piece prev_piece = stack[ply - diff].piece;

                        if (prev_piece != Piece::None) cont_hist[piece_idx(piece)][sq_idx(move.to_square())][piece_idx(prev_piece)][sq_idx(prev_move.to_square())].update(bonus, MAX_HIST);
                    }
                };

                update_hist(1);
                update_hist(2);
            }

            inline void update_capt_hist(Piece attacker, Move move, Piece victim, int16_t bonus) {
                capt_hist[piece_idx(attacker)][sq_idx(move.to_square())][piece_type_idx(victim)].update(bonus, MAX_HIST);
            }

            inline void update_corr_hist(uint64_t pawn_hash, Color stm, int16_t diff) {
                corr_hist[color_idx(stm)][pawn_hash % 16384].update(diff, MAX_CORR_HIST);
            }

            inline void reset() {
                quiet_hist = {};
                cont_hist = {};
                capt_hist = {};
                corr_hist = {};
            }

        private:
            std::array<std::array<std::array<Entry, 64>, 64>, 2> quiet_hist{};
            std::array<std::array<std::array<std::array<Entry, 64>, 12>, 64>, 12> cont_hist{};
            std::array<std::array<std::array<Entry, 6>, 64>, 12> capt_hist{};
            std::array<std::array<Entry, 16384>, 2> corr_hist{};
    };
}