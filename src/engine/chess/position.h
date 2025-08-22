#pragma once

#include "move.h"
#include "zobrist.h"

#include <array>
#include <vector>
#include <string>
#include <ranges>
#include <cstdint>
#include <cstdlib>
#include <sstream>

namespace episteme {
    static constexpr std::array<Piece, 64> empty_mailbox() {
        std::array<Piece, 64> mailbox{};
        mailbox.fill(Piece::None);
        return mailbox;
    }

    struct PositionState {
        std::array<uint64_t, 8> bitboards{};
        std::array<Piece, 64> mailbox = empty_mailbox();
    
        AllowedCastles allowed_castles{
            .rooks{
                {{.kingside = Square::None, .queenside = Square::None},
                {.kingside = Square::None, .queenside = Square::None}}
            }
        };
    
        bool stm = color_idx(Color::White);
        uint8_t half_move_clock = 0;
        uint16_t full_move_number = 0;
        Square ep_square = Square::None;

        uint64_t hash = 0;
        uint64_t pawn_hash = 0;
    };

    class Position {
        public:
            Position();

            [[nodiscard]] inline uint64_t total_bb() const {
                return (state.bitboards[color_idx(Color::White) + COLOR_OFFSET] | state.bitboards[color_idx(Color::Black) + COLOR_OFFSET]);
            }

            [[nodiscard]] inline uint64_t piece_bb(PieceType piece_type, Color color) const {
                return (state.bitboards[piece_type_idx(piece_type)] & state.bitboards[color_idx(color) + COLOR_OFFSET]);
            }

            [[nodiscard]] inline uint64_t piece_type_bb(PieceType piece_type) const {
                return state.bitboards[piece_type_idx(piece_type)];
            }

            [[nodiscard]] inline uint64_t color_bb(Color color) const {
                return state.bitboards[color_idx(color) + COLOR_OFFSET];
            }

            [[nodiscard]] inline std::array<uint64_t, 8> bitboards_all() const {
                return state.bitboards;
            }

            [[nodiscard]] inline uint64_t bitboard(int index) const {
                return state.bitboards[index];
            }
        
            [[nodiscard]] inline Color STM() const {
                return static_cast<Color>(state.stm);
            }
        
            [[nodiscard]] inline Color NTM() const {
                return static_cast<Color>(!state.stm);
            }
        
            [[nodiscard]] inline uint8_t half_move_clock() const {
                return state.half_move_clock; 
            }
        
            [[nodiscard]] inline uint32_t full_move_number() const {
                return state.full_move_number;
            }
        
            [[nodiscard]] inline Square ep_square() const {
                return state.ep_square;    
            }

            [[nodiscard]] inline AllowedCastles all_rights() const {
                return state.allowed_castles;
            }
        
            [[nodiscard]] inline AllowedCastles::RookPair castling_rights(Color stm) const {
                return state.allowed_castles.rooks[color_idx(stm)];
            }

            [[nodiscard]] inline Piece mailbox(Square square) const {
                return state.mailbox[sq_idx(square)];
            }

            [[nodiscard]] inline Piece mailbox(int index) const {
                return state.mailbox[index];
            }

            [[nodiscard]] inline std::array<Piece, 64> mailbox_all() const {
                return state.mailbox;
            }

            [[nodiscard]] inline uint64_t hash() const {
                return state.hash;
            }

            [[nodiscard]] inline uint64_t pawn_hash() const {
                return state.pawn_hash;
            }

            void from_FEN(const std::string& FEN);
            void from_startpos();

            void make_move(const Move& move);
            void make_null();
            void unmake_move();

            bool is_threefold();
            bool is_insufficient();

            std::string to_FEN() const; 
            uint64_t explicit_hash();
            uint64_t explicit_pawn_hash();
        public:
            static const uint16_t COLOR_OFFSET = 6;

        private:
            std::vector<PositionState> position_history;
            PositionState state;
    };

    Move from_UCI(const Position& position, const std::string& move);
}
