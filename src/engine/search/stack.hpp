#pragma once

#include <cstdint>

#include "../chess/move.hpp"

namespace episteme::stack {
constexpr int32_t INF = 1048576;

struct Entry {
  int32_t eval = -INF;
  int16_t reduction = 0;
  Move move{};
  Piece piece;

  Move excluded{};
  Move killer{};
};

class Stack {
  public:
  void reset() { stack.fill(Entry()); }

  Entry& operator[](int idx) { return stack[idx]; }

  private:
  std::array<Entry, 1024> stack{};
};
}  // namespace episteme::stack
