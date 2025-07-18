#pragma once

#include "../chess/move.h"
#include "stack.h"

#include <array>
#include <cstdint>
#include <algorithm>

namespace episteme::hist {
    constexpr int MAX_HISTORY = 16384;

    [[nodiscard]] inline int16_t bonus(int16_t depth) {
        return static_cast<int16_t>(std::clamp(depth * 300, 0, 2500));
    }

    struct Entry {
        int16_t value = 0;

        inline void update(int16_t bonus) {
            value += bonus - value * std::abs(bonus) / MAX_HISTORY;
        }
    };

    class Table {
        public:
            [[nodiscard]] inline int16_t get_quiet_hist(Color stm, Move move) {
                return quiet_hist[color_idx(stm)][sq_idx(move.from_square())][sq_idx(move.to_square())].value;
            }

            [[nodiscard]] inline int16_t get_cont_hist(stack::Stack& stack, Piece piece, Move move, int16_t ply) {
                int16_t value = 0;

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

            inline void update_quiet_hist(Color stm, Move move, int16_t bonus) {
                quiet_hist[color_idx(stm)][sq_idx(move.from_square())][sq_idx(move.to_square())].update(bonus);
            }

            inline void update_cont_hist(stack::Stack& stack, Piece piece, Move move, int16_t bonus, int16_t ply) {
                auto update_hist = [&](int16_t diff) {
                    if (ply > diff - 1) {
                        Move prev_move = stack[ply - diff].move;
                        Piece prev_piece = stack[ply - diff].piece;

                        if (prev_piece != Piece::None) cont_hist[piece_idx(piece)][sq_idx(move.to_square())][piece_idx(prev_piece)][sq_idx(prev_move.to_square())].update(bonus);
                    }
                };

                update_hist(1);
                update_hist(2);
            }

            inline void reset() {
                quiet_hist = {};
                cont_hist = {};
            }

        private:
            std::array<std::array<std::array<Entry, 64>, 64>, 2> quiet_hist{};
            std::array<std::array<std::array<std::array<Entry, 64>, 12>, 64>, 12> cont_hist{};
    };
}