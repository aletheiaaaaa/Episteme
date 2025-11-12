#pragma once

#include "../chess/position.h"
#include "../chess/perft.h"
#include "../search/search.h"
#include "../../utils/datagen.h"

#include <string>
#include <sstream>
#include <iostream>
#include <array>

namespace episteme::uci {
    int parse(const std::string& cmd, search::Config& cfg, search::Engine& engine);

    auto uci();
    auto setoption(const std::string& args, search::Config& cfg, search::Engine& engine);
    auto isready();
    auto position(const std::string& args, search::Config& cfg);
    auto go(const std::string& args, search::Config& cfg, search::Engine& engine);
    auto ucinewgame(search::Config& cfg, search::Engine& engine);
    auto eval(search::Config& cfg, search::Engine& engine);
    auto bench(const std::string& args, search::Config& cfg);
    auto perft(const std::string& args, search::Config& cfg);
    auto datagen(const std::string& args);
}