#pragma once

#include "../chess/move.h"
#include "stack.h"

#include <array>
#include <cstdint>
#include <algorithm>

namespace episteme::hist {
    constexpr int MAX_HIST = 16384;
    constexpr int MAX_CORR_HIST = 1024;

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

            [[nodiscard]] inline int32_t get_hist(stack::Stack& stack, Piece attacker, Piece victim, Move move, Color stm, int16_t ply, const Position& position) {
                int32_t value = 0;

                if (victim != Piece::None) {
                    value += get_capt_hist(attacker, move, victim);
                } else {
                    value += get_quiet_hist(stm, move);
                    value += get_cont_hist(stack, attacker, move, ply);
                    value += get_pawn_hist(stm, position.pawn_hash(), attacker, move);
                }

                return value;
            }

            [[nodiscard]] inline int32_t get_capt_hist(Piece attacker, Move move, Piece victim) {
                return capt_hist[piece_idx(attacker)][sq_idx(move.to_square())][piece_type_idx(victim)].value;
            }

            [[nodiscard]] inline int32_t get_pawn_hist(Color stm, uint64_t pawn_hash, Piece piece, Move move) {
                return pawn_hist[color_idx(stm)][pawn_hash % 1024][piece_type_idx(piece)][sq_idx(move.to_square())].value;
            }

            [[nodiscard]] inline int32_t get_pawn_corr_hist(uint64_t pawn_hash, Color stm) {
                return pawn_corr_hist[color_idx(stm)][pawn_hash % 16384].value;
            }

            [[nodiscard]] inline int32_t get_major_corr_hist(uint64_t major_hash, Color stm) {
                return major_corr_hist[color_idx(stm)][major_hash % 16384].value;
            }

            [[nodiscard]] inline int32_t get_minor_corr_hist(uint64_t minor_hash, Color stm) {
                return minor_corr_hist[color_idx(stm)][minor_hash % 16384].value;
            }

            [[nodiscard]] inline int32_t get_non_pawn_stm_corr_hist(uint64_t non_pawn_stm_hash, Color stm) {
                return non_pawn_stm_corr_hist[color_idx(stm)][non_pawn_stm_hash % 16384].value;
            }

            [[nodiscard]] inline int32_t get_non_pawn_ntm_corr_hist(uint64_t non_pawn_ntm_hash, Color stm) {
                return non_pawn_ntm_corr_hist[color_idx(stm)][non_pawn_ntm_hash % 16384].value;
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

            inline void update_pawn_hist(Color stm, uint64_t pawn_hash, Piece piece, Move move, int16_t bonus) {
                pawn_hist[color_idx(stm)][pawn_hash % 1024][piece_type_idx(piece)][sq_idx(move.to_square())].update(bonus, MAX_HIST);
            }

            inline void update_corr_hist(const Position& position, Color stm, int16_t diff) {
                pawn_corr_hist[color_idx(stm)][position.pawn_hash() % 16384].update(diff, MAX_CORR_HIST);
                // major_corr_hist[color_idx(stm)][position.major_hash() % 16384].update(diff, MAX_CORR_HIST);
                minor_corr_hist[color_idx(stm)][position.minor_hash() % 16384].update(diff, MAX_CORR_HIST);
                non_pawn_stm_corr_hist[color_idx(stm)][position.non_pawn_stm_hash() % 16384].update(diff, MAX_CORR_HIST);
                non_pawn_ntm_corr_hist[color_idx(stm)][position.non_pawn_ntm_hash() % 16384].update(diff, MAX_CORR_HIST);
            }

            inline void reset() {
                quiet_hist = {};
                cont_hist = {};
                capt_hist = {};
                pawn_hist = {};

                pawn_corr_hist = {};
                major_corr_hist = {};
                minor_corr_hist = {};
                non_pawn_stm_corr_hist = {};
                non_pawn_ntm_corr_hist = {};
            }

        private:
            std::array<std::array<std::array<Entry, 64>, 64>, 2> quiet_hist{};
            std::array<std::array<std::array<std::array<Entry, 64>, 12>, 64>, 12> cont_hist{};
            std::array<std::array<std::array<Entry, 6>, 64>, 12> capt_hist{};
            std::array<std::array<std::array<std::array<Entry, 64>, 6>, 1024>, 2> pawn_hist{};

            std::array<std::array<Entry, 16384>, 2> pawn_corr_hist{};
            std::array<std::array<Entry, 16384>, 2> major_corr_hist{};
            std::array<std::array<Entry, 16384>, 2> minor_corr_hist{};
            std::array<std::array<Entry, 16384>, 2> non_pawn_stm_corr_hist{};
            std::array<std::array<Entry, 16384>, 2> non_pawn_ntm_corr_hist{};
    };
}