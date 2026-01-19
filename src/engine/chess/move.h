#pragma once

#include "core.h"
#include <array>
#include <string>

namespace episteme {
    enum class MoveType : uint16_t {
        Normal, EnPassant, Castling, Promotion, 
        None
    };

    enum class PromoPiece : uint16_t {
        Knight, Bishop, Rook, Queen, 
        None
    };

    class Move {
        public:
            Move();
            Move(Square from_square, Square to_square, MoveType move_type = MoveType::Normal, PromoPiece promo_piece = PromoPiece::None);

            [[nodiscard]] inline uint16_t data() const {
                return move_data;
            }

            [[nodiscard]] inline Square from_square() const {
                return static_cast<Square>(move_data & 0b111111);
            }
        
            [[nodiscard]] inline Square to_square() const {
                return static_cast<Square>((move_data >> 6) & 0b111111);
            }
        
            [[nodiscard]] inline MoveType move_type() const {
                return static_cast<MoveType>((move_data >> 12) & 0b11);
            }
        
            [[nodiscard]] inline PromoPiece promo_piece() const {
                return static_cast<PromoPiece>((move_data >> 14) & 0b11);
            }
        
            [[nodiscard]] inline PieceType promo_piece_type() const {
                return static_cast<PieceType>(((move_data >> 14) & 0b11) + 1);
            }

            [[nodiscard]] inline uint16_t from_idx() const {
                return move_data & 0b111111;
            }

            [[nodiscard]] inline uint16_t to_idx() const {
                return (move_data >> 6) & 0b111111;
            }

            [[nodiscard]] inline uint16_t type_idx() const {
                return (move_data >> 12) & 0b11;
            }

            [[nodiscard]] inline uint16_t promo_idx() const {
                return (move_data >> 14) & 0b11;
            }

            [[nodiscard]] inline bool is_empty() const {
                return (move_data == 0x0000);
            }

            [[nodiscard]] inline bool operator==(const Move& other) const {
                return move_data == other.move_data;
            }

            [[nodiscard]] inline bool operator!=(const Move& other) const {
                return move_data != other.move_data;
            }

            [[nodiscard]] inline bool operator!() const {
                return move_data == 0x0000;
            }

            std::string to_string() const;
            std::string to_PGN(const class Position& position) const;
        private:
            uint16_t move_data;
    };
}
