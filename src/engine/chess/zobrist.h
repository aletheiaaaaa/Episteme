#pragma once

#include "core.h"

#include <array>
#include <cstdint>
#include <random>

namespace episteme::hash {
    extern std::array<uint64_t, 768> piecesquares;
    extern std::array<uint64_t, 16> castling_rights;
    extern std::array<uint64_t, 8> ep_files;
    extern uint64_t stm;

    void init();
}