#pragma once

#include "../engine/chess/movegen.h"
#include "../engine/search/search.h"
#include "format.h"

#include <format>
#include <iostream>
#include <random>
#include <cstdint>
#include <algorithm>
#include <thread>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <sstream>

namespace episteme::datagen {
    constexpr int32_t WIN_SCORE_MIN = 2500;
    constexpr int32_t WIN_PLIES_MIN = 5;
    constexpr int32_t DRAW_SCORE_MAX = 2;
    constexpr int32_t DRAW_PLIES_MIN = 8;
    constexpr int32_t INITIAL_MAX = 300;

    inline std::atomic<bool> stop = false;

    struct Parameters {
        uint32_t soft_limit = 5000;
        uint32_t hard_limit = 1000000;
        int32_t num_games = 100000;

        uint16_t num_threads = 1;
        uint32_t hash_size = 16; 

        std::string out_dir = "data";
    };

    void play_random(Position& position, int32_t num_moves);
    void game_loop(const Parameters& params, std::ostream& stream, uint32_t id);
    void run(Parameters& params);
}