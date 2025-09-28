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

    struct Hashes {
        uint64_t full_hash = 0;

        uint64_t pawn_hash = 0;

        uint64_t krp_hash = 0;
        uint64_t krn_hash = 0;
        uint64_t krb_hash = 0;
        uint64_t major_hash = 0;
        uint64_t minor_hash = 0;

        uint64_t non_pawn_white_hash = 0;
        uint64_t non_pawn_black_hash = 0;
    };

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

        Hashes hashes;
    };

    class Position {
        public:
            Position();

            [[nodiscard]] inline uint64_t total_bb() const {
                return (current.bitboards[color_idx(Color::White) + COLOR_OFFSET] | current.bitboards[color_idx(Color::Black) + COLOR_OFFSET]);
            }

            [[nodiscard]] inline uint64_t piece_bb(PieceType piece_type, Color color) const {
                return (current.bitboards[piece_type_idx(piece_type)] & current.bitboards[color_idx(color) + COLOR_OFFSET]);
            }

            [[nodiscard]] inline uint64_t piece_type_bb(PieceType piece_type) const {
                return current.bitboards[piece_type_idx(piece_type)];
            }

            [[nodiscard]] inline uint64_t color_bb(Color color) const {
                return current.bitboards[color_idx(color) + COLOR_OFFSET];
            }

            [[nodiscard]] inline std::array<uint64_t, 8> bitboards_all() const {
                return current.bitboards;
            }

            [[nodiscard]] inline uint64_t bitboard(int index) const {
                return current.bitboards[index];
            }
        
            [[nodiscard]] inline Color STM() const {
                return static_cast<Color>(current.stm);
            }
        
            [[nodiscard]] inline Color NTM() const {
                return static_cast<Color>(!current.stm);
            }
        
            [[nodiscard]] inline uint8_t half_move_clock() const {
                return current.half_move_clock; 
            }
        
            [[nodiscard]] inline uint32_t full_move_number() const {
                return current.full_move_number;
            }
        
            [[nodiscard]] inline Square ep_square() const {
                return current.ep_square;    
            }

            [[nodiscard]] inline AllowedCastles all_rights() const {
                return current.allowed_castles;
            }
        
            [[nodiscard]] inline AllowedCastles::RookPair castling_rights(Color stm) const {
                return current.allowed_castles.rooks[color_idx(stm)];
            }

            [[nodiscard]] inline Piece mailbox(Square square) const {
                return current.mailbox[sq_idx(square)];
            }

            [[nodiscard]] inline Piece mailbox(int index) const {
                return current.mailbox[index];
            }

            [[nodiscard]] inline std::array<Piece, 64> mailbox_all() const {
                return current.mailbox;
            }

            [[nodiscard]] inline uint64_t full_hash() const {
                return current.hashes.full_hash;
            }

            [[nodiscard]] inline uint64_t pawn_hash() const {
                return current.hashes.pawn_hash;
            }

            [[nodiscard]] inline uint64_t krp_hash() const {
                return current.hashes.krp_hash;
            }

            [[nodiscard]] inline uint64_t krn_hash() const {
                return current.hashes.krn_hash;
            }

            [[nodiscard]] inline uint64_t krb_hash() const {
                return current.hashes.krb_hash;
            }

            [[nodiscard]] inline uint64_t major_hash() const {
                return current.hashes.major_hash;
            }

            [[nodiscard]] inline uint64_t minor_hash() const {
                return current.hashes.minor_hash;
            }

            [[nodiscard]] inline uint64_t non_pawn_stm_hash() const {
                return (!current.stm) ? current.hashes.non_pawn_white_hash : current.hashes.non_pawn_black_hash;
            }

            [[nodiscard]] inline uint64_t non_pawn_ntm_hash() const {
                return (!current.stm) ? current.hashes.non_pawn_black_hash : current.hashes.non_pawn_white_hash;
            }

            void from_FEN(const std::string& FEN);
            void from_startpos();

            void make_move(const Move& move);
            void make_null();
            void unmake_move();

            bool is_threefold();
            bool is_insufficient();

            std::string to_FEN() const; 

            Hashes explicit_hashes();
        public:
            static const uint16_t COLOR_OFFSET = 6;

        private:
            std::vector<PositionState> history;
            PositionState current;
    };

    Move from_UCI(const Position& position, const std::string& move);
}
