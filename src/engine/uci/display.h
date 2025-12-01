#pragma once

#include "../chess/position.h"
#include "../search/search.h"
#include "../chess/move.h"

namespace episteme::uci {
    void show_position(const Position& position, int32_t eval_cp);

    void on_start(const Position& position);
    void on_update(const search::Report& report);
    void on_completion(const search::Report& report, Move best_move);
    void update_exploring(const search::Line& line);
    void set_engine(search::Engine* engine);

    void clear();
    void set_pretty(bool enabled);
    bool is_pretty();
}
