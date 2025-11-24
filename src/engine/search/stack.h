#pragma once

#include "../chess/move.h"

#include <cstdint>

namespace episteme::stack {
    constexpr int32_t INF = 1048576;

    struct Entry {
        int32_t eval = -INF;
        int32_t reduction = 0;
        Move move{};
        Piece piece;

        Move excluded{};
        Move killer{};
    };

    class Stack {
        public:
            inline void reset() {
                stack.fill(Entry());
            }

            [[nodiscard]] inline Entry &operator[](int idx) {
                return stack[idx];
            }
        private:
            std::array<Entry, 256> stack{};
    };
}