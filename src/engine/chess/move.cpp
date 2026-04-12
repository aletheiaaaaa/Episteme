#include "move.hpp"

namespace episteme {
Move::Move() : move_data(0x0000) {};

Move::Move(Square from_square, Square to_square, MoveType move_type, PromoPiece promo_piece)
  : move_data(
      static_cast<uint16_t>(from_square) | (static_cast<uint16_t>(to_square) << 6) |
      (static_cast<uint16_t>(move_type) << 12) | (static_cast<uint16_t>(promo_piece) << 14)
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
      case PieceType::Knight:
        promo_char = 'n';
        break;
      case PieceType::Bishop:
        promo_char = 'b';
        break;
      case PieceType::Rook:
        promo_char = 'r';
        break;
      case PieceType::Queen:
        promo_char = 'q';
        break;
      default:
        break;
    }
    move_str += promo_char;
  }

  return move_str;
}
}  // namespace episteme
