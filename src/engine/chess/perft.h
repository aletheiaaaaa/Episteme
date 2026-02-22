#pragma once

#include "movegen.h"

#include <string>

namespace episteme {
    Position fen_to_position(const std::string& FEN);
    uint64_t perft(Position &position, int32_t depth);
    void split_perft(Position &position, int32_t depth);
    void time_perft(Position& position, int32_t depth);
}
