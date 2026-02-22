#pragma once

#include "../engine/chess/position.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <ostream>

namespace episteme::datagen {
    class U4 {
        public:
            U4(uint8_t& value, bool high) : value{value}, high{high} {};

            constexpr U4& operator=(uint8_t x) {
                if (high) value = ((value & 0x0F) | (x << 4));
                else value = ((value & 0xF0) | (x & 0x0F));
                return *this;
            }

        private:
            uint8_t& value;
            bool high;
    };

    template<size_t SIZE>
    class U4Array {
        public:
            constexpr U4 operator[](size_t i) {
                return U4(data[i / 2], (i % 2) == 1);
            }

        private:
            std::array<uint8_t, SIZE / 2> data{};
    };

    typedef std::pair<uint16_t, int16_t> ScoredMove;

    struct PackedBoard {
        uint64_t bitboard;
        U4Array<32> pieces;
        uint8_t stm_ep_square;
        uint8_t half_move_clock;
        uint16_t full_move_number;
        int16_t score;
        uint8_t wdl;
        uint8_t unused;

        [[nodiscard]] static PackedBoard pack(const Position& position, int16_t score) {
            uint64_t bitboard = position.total_bb();
            bool stm = static_cast<bool>(position.STM());
            U4Array<32> pieces{};

            uint64_t temp_bb = bitboard;
            size_t i = 0;
            while (temp_bb) {
                Square square = sq_from_idx(std::countr_zero(temp_bb));
                Piece piece = position.mailbox(square);

                uint8_t type = static_cast<uint8_t>(piece_type_idx(piece));
                bool color = static_cast<bool>(color_idx(piece));

                if (piece_type(piece) == PieceType::Rook && position.all_rights().is_castling(square)) type = 0b0110;

                pieces[i++] = (color << 3) | type;

                temp_bb &= (temp_bb - 1);
            }

            uint8_t stm_ep_square = static_cast<uint8_t>(position.ep_square()) | (stm << 7);
            uint8_t half_move_clock = position.half_move_clock();
            uint16_t full_move_number = position.full_move_number();

            PackedBoard packed{
                .bitboard = bitboard,
                .pieces = pieces,
                .stm_ep_square = stm_ep_square,
                .half_move_clock = half_move_clock,
                .full_move_number = full_move_number,
                .score = score,
                .wdl = UINT8_MAX,
                .unused = 0
            };

            return packed;
        }
    };

    class Format {
        public:
            static constexpr std::string_view EXTENSION = "vf";

            Format();

            void init(const Position& position);
            void push(Move move, int32_t score);
            size_t write(std::ostream& stream, uint8_t wdl);
        private:
            PackedBoard initial{};
            std::vector<ScoredMove> moves;
    };
}