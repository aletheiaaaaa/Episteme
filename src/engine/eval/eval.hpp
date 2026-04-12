#pragma once

#include "../../external/incbin.hpp"
#include "../chess/movegen.hpp"
#include "../chess/position.hpp"
#include "nn/common.hpp"

namespace episteme::eval {
nn::Accumulator update(const Position& position, const Move& move, nn::Accumulator accum);
nn::Accumulator reset(const Position& position);
int32_t evaluate(nn::Accumulator& accumulator, Color stm);
bool SEE(const Position& position, const Move& move, int32_t threshold);
}  // namespace episteme::eval
