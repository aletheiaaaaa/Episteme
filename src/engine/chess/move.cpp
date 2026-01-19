#include "move.h"
#include "position.h"

namespace episteme {
    Move::Move() : move_data(0x0000) {};

    Move::Move(uint16_t data) : move_data(data) {};

    Move::Move(Square from_square, Square to_square, MoveType move_type, PromoPiece promo_piece) : move_data(
        static_cast<uint16_t>(from_square)
        | (static_cast<uint16_t>(to_square) << 6)
        | (static_cast<uint16_t>(move_type) << 12)
        | (static_cast<uint16_t>(promo_piece) << 14)
    ) {};

    std::string Move::to_string() const {
        auto square_to_string = [](Square sq) -> std::string {
            if (sq == Square::None) return "--";
            int idx = static_cast<int>(sq);
            char file = 'a' + (idx % 8);
            char rank = '1' + (idx / 8);
            return std::string() + file + rank;
        };
    
        std::string move_str = square_to_string(from_square()) + square_to_string(to_square());
    
        if (move_type() == MoveType::Promotion) {
            char promo_char = '\0';
            switch (promo_piece_type()) {
                case PieceType::Knight: promo_char = 'n'; break;
                case PieceType::Bishop: promo_char = 'b'; break;
                case PieceType::Rook:   promo_char = 'r'; break;
                case PieceType::Queen:  promo_char = 'q'; break;
                default: break;
            }
            move_str += promo_char;
        }

        return move_str;
    }

    std::string Move::to_PGN(const Position& position) const {
        if (is_empty()) return "";

        Square from = from_square();
        Square to = to_square();
        MoveType type = move_type();

        // Handle castling
        if (type == MoveType::Castling) {
            int to_file = sq_idx(to) % 8;
            return (to_file == 6) ? "O-O" : "O-O-O";
        }

        std::string pgn;

        // Get the piece being moved
        Piece moving_piece = position.mailbox(from);
        PieceType piece_type_moved = piece_type(moving_piece);

        // Add piece symbol (uppercase for white/black in PGN)
        if (piece_type_moved != PieceType::Pawn) {
            switch (piece_type_moved) {
                case PieceType::Knight: pgn += 'N'; break;
                case PieceType::Bishop: pgn += 'B'; break;
                case PieceType::Rook:   pgn += 'R'; break;
                case PieceType::Queen:  pgn += 'Q'; break;
                case PieceType::King:   pgn += 'K'; break;
                default: break;
            }
        }

        // Check if destination square is occupied (capture)
        Piece target_piece = position.mailbox(to);
        bool is_capture = (target_piece != Piece::None) || (type == MoveType::EnPassant);

        // For pawn captures, add the file letter
        if (piece_type_moved == PieceType::Pawn && is_capture) {
            int from_file = sq_idx(from) % 8;
            pgn += char('a' + from_file);
        }

        // Add capture symbol
        if (is_capture) {
            pgn += 'x';
        }

        // Add destination square
        int to_file = sq_idx(to) % 8;
        int to_rank = sq_idx(to) / 8;
        pgn += char('a' + to_file);
        pgn += char('1' + to_rank);

        // Add promotion piece
        if (type == MoveType::Promotion) {
            pgn += '=';
            switch (promo_piece_type()) {
                case PieceType::Knight: pgn += 'N'; break;
                case PieceType::Bishop: pgn += 'B'; break;
                case PieceType::Rook:   pgn += 'R'; break;
                case PieceType::Queen:  pgn += 'Q'; break;
                default: break;
            }
        }

        return pgn;
    }
}
