#include "format.h"

namespace episteme::datagen {
    Format::Format() {
        moves.reserve(256);
    }

    void Format::init(const Position& position) {
        initial = PackedBoard::pack(position, 0);
        moves.clear();
        moves.shrink_to_fit();
        moves.reserve(256);
    }

    void Format::push(Move move, int32_t score) {
        uint16_t to_idx = move.to_idx();
        if (move.move_type() == MoveType::Castling) {
            bool is_kingside = move.to_idx() > move.from_idx();
            to_idx += (is_kingside ? 1 : -2);
        }

        uint16_t viri_move = 0;
        viri_move |= (move.from_idx() | (to_idx << 6) | (move.promo_idx() << 12) | (move.type_idx() << 14));

        moves.push_back({viri_move, static_cast<int16_t>(score)});
    }

    size_t Format::write(std::ostream& stream, uint8_t wdl) {
        static constexpr std::array<uint8_t, sizeof(ScoredMove)> NULL_TERMINATOR{};

        initial.wdl = wdl;

        stream.write(reinterpret_cast<const char*>(&initial), sizeof(PackedBoard));
        stream.write(reinterpret_cast<const char*>(moves.data()), sizeof(ScoredMove) * moves.size());
        stream.write(reinterpret_cast<const char*>(NULL_TERMINATOR.data()), sizeof(ScoredMove));

        return moves.size() + 1;
    }
}