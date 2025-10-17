#pragma once

#include "nnue.h"
#include "../chess/position.h"
#include "../chess/movegen.h"
#include "../../external/incbin.h"

namespace episteme::eval {
    nn::Accumulator update(const Position& position, const Move& move, nn::Accumulator accum);    
    nn::Accumulator reset(const Position& position);
    int32_t evaluate(nn::Accumulator& accumulator, Color stm);
    bool SEE(const Position& position, const Move& move, int32_t threshold);
}