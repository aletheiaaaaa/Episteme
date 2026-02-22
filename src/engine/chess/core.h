#pragma once

#include <unordered_map>
#include <cstdint>
#include <cstddef>
#include <array>

namespace episteme {
    constexpr std::array<int32_t, 7> piece_vals = {100, 300, 300, 500, 900, -1, 0}; 

    constexpr uint8_t WHITE_KINGSIDE = 1;
    constexpr uint8_t WHITE_QUEENSIDE = 1 << 1;
    constexpr uint8_t BLACK_KINGSIDE = 1 << 2;
    constexpr uint8_t BLACK_QUEENSIDE = 1 << 3;

    constexpr uint64_t FILE_A = 0x0101010101010101;
    constexpr uint64_t FILE_B = 0x0202020202020202;
    constexpr uint64_t FILE_C = 0x0404040404040404;
    constexpr uint64_t FILE_D = 0x0808080808080808;
    constexpr uint64_t FILE_E = 0x1010101010101010;
    constexpr uint64_t FILE_F = 0x2020202020202020;
    constexpr uint64_t FILE_G = 0x4040404040404040;
    constexpr uint64_t FILE_H = 0x8080808080808080;

    constexpr uint64_t RANK_1 = 0x00000000000000FF;
    constexpr uint64_t RANK_2 = 0x000000000000FF00;
    constexpr uint64_t RANK_3 = 0x0000000000FF0000;
    constexpr uint64_t RANK_4 = 0x00000000FF000000;
    constexpr uint64_t RANK_5 = 0x000000FF00000000;
    constexpr uint64_t RANK_6 = 0x0000FF0000000000;
    constexpr uint64_t RANK_7 = 0x00FF000000000000;
    constexpr uint64_t RANK_8 = 0xFF00000000000000;

    constexpr size_t DOUBLE_PUSH = 16;

    enum class Square : uint16_t {
        A1, B1, C1, D1, E1, F1, G1, H1, 
        A2, B2, C2, D2, E2, F2, G2, H2,
        A3, B3, C3, D3, E3, F3, G3, H3,
        A4, B4, C4, D4, E4, F4, G4, H4,
        A5, B5, C5, D5, E5, F5, G5, H5,
        A6, B6, C6, D6, E6, F6, G6, H6,
        A7, B7, C7, D7, E7, F7, G7, H7,
        A8, B8, C8, D8, E8, F8, G8, H8,
        None
    };

    enum class Piece : uint16_t {
        WhitePawn,   BlackPawn, 
        WhiteKnight, BlackKnight,
        WhiteBishop, BlackBishop,
        WhiteRook,   BlackRook, 
        WhiteQueen,  BlackQueen,
        WhiteKing,   BlackKing,
        None
    };

    enum class PieceType : uint16_t {
        Pawn, Knight, Bishop, Rook, Queen, King, 
        None
    };

    enum class Color : uint16_t {
        White, Black, 
        None
    };

    enum class BBIndex : uint16_t {
        Pawn, Knight, Bishop, Rook, Queen, King, White, Black,
        None
    };

    struct AllowedCastles {
        struct RookPair {
            Square kingside{Square::None};
            Square queenside{Square::None};

            [[nodiscard]] inline bool is_kingside_set() const {
                if (kingside != Square::None) return true;
                return false;            
            };

            [[nodiscard]] inline bool is_queenside_set() const {
                if (queenside != Square::None) return true;
                return false;
            };

            inline void clear() {
                kingside = Square::None; 
                queenside = Square::None;            
            };

            inline void unset(bool is_kingside) {
                if (is_kingside) {
                    kingside = Square::None;
                } else {
                    queenside = Square::None;
                }            
            };
        };
        std::array<RookPair, 2> rooks{};
        
        [[nodiscard]] inline uint8_t as_mask() {
            size_t mask = 0;
            if (rooks[0].is_kingside_set()) mask |= WHITE_KINGSIDE; 
            if (rooks[0].is_queenside_set()) mask |= WHITE_QUEENSIDE; 
            if (rooks[1].is_kingside_set()) mask |= BLACK_KINGSIDE; 
            if (rooks[1].is_queenside_set()) mask |= BLACK_QUEENSIDE;  

            return mask;
        }

        bool is_castling(Square square) {
            if (rooks[0].kingside == square || rooks[0].queenside == square || rooks[1].kingside == square || rooks[1].queenside == square) return true;
            return false;
        }
    };

    static const std::unordered_map<char, std::pair<PieceType, Color>> piece_map = {
        {'P', {PieceType::Pawn, Color::White}}, {'N', {PieceType::Knight, Color::White}}, 
        {'B', {PieceType::Bishop, Color::White}}, {'R', {PieceType::Rook, Color::White}}, 
        {'Q', {PieceType::Queen, Color::White}}, {'K', {PieceType::King, Color::White}}, 
        {'p', {PieceType::Pawn, Color::Black}}, {'n', {PieceType::Knight, Color::Black}}, 
        {'b', {PieceType::Bishop, Color::Black}}, {'r', {PieceType::Rook, Color::Black}}, 
        {'q', {PieceType::Queen, Color::Black}}, {'k', {PieceType::King, Color::Black}}
    };
    
    [[nodiscard]] inline PieceType piece_type(Piece piece) {
        return static_cast<PieceType>(static_cast<uint16_t>(piece) >> 1);
    };

    [[nodiscard]] inline Color color(Piece piece) {
        return static_cast<Color>(static_cast<uint16_t>(piece) & 0b1);
    };

    [[nodiscard]] inline Color flip(Color color) {
        return static_cast<Color>(!static_cast<bool>(color));
    }

    [[nodiscard]] inline Square flip(Square square) {
        return static_cast<Square>(static_cast<int16_t>(square) ^ 56);
    }

    [[nodiscard]] inline Piece piece_type_with_color(PieceType piece_type, Color color) {
        return static_cast<Piece>(2 * static_cast<uint16_t>(piece_type) + static_cast<uint16_t>(color));
    }

    [[nodiscard]] inline int16_t piecesquare(Piece piece, Square square, bool flip_color) {
        if (piece == Piece::None) {
            return -1;
        };

        Color stm  = flip_color ? flip(color(piece)) : color(piece);
        Square location = flip_color ? flip(square) : square;
        
        return static_cast<int16_t>(stm) * 384 + static_cast<int16_t>(piece_type(piece)) * 64 + static_cast<int16_t>(location);
    }

    [[nodiscard]] inline Piece pc_from_idx(uint16_t index) {
        return static_cast<Piece>(index);
    }

    [[nodiscard]] inline Square sq_from_idx(uint16_t index) {
        return static_cast<Square>(index);
    }

    [[nodiscard]] inline uint16_t sq_idx(Square square) {
        return static_cast<uint16_t>(square);
    }

    [[nodiscard]] inline uint16_t piece_idx(Piece piece) {
        return static_cast<uint16_t>(piece);
    }

    [[nodiscard]] inline uint16_t piece_type_idx(PieceType piece_type) {
        return static_cast<uint16_t>(piece_type);
    }

    [[nodiscard]] inline uint16_t piece_type_idx(Piece piece) {
        return static_cast<uint16_t>(piece) >> 1;
    }

    [[nodiscard]] inline uint16_t color_idx(Color color) {
        return static_cast<uint16_t>(color);
    }

    [[nodiscard]] inline uint16_t color_idx(Piece piece) {
        return static_cast<uint16_t>(piece) & 0b1;
    }

    [[nodiscard]] inline uint16_t file(Square square) {
        return sq_idx(square) % 8;
    }

    [[nodiscard]] inline uint16_t rank(Square square) {
        return sq_idx(square) / 8;
    }

    [[nodiscard]] inline uint64_t shift_west(uint64_t bitboard) {
        return (bitboard & ~FILE_A) >> 1; 
    }

    [[nodiscard]] inline uint64_t shift_east(uint64_t bitboard) {
        return (bitboard & ~FILE_H) << 1;
    }

    [[nodiscard]] inline uint64_t shift_north(uint64_t bitboard) {
        return (bitboard & ~RANK_8) << 8;
    }

    [[nodiscard]] inline uint64_t shift_south(uint64_t bitboard) {
        return (bitboard & ~RANK_1) >> 8;
    }
}
