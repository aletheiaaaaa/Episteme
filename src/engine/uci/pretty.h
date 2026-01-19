#pragma once

#include "../chess/position.h"
#include "../search/search.h"

#include <sstream>
#include <string>
#include <vector>

namespace episteme::pretty {
    constexpr const char* DEEP_INDIGO = "\033[38;5;63m";
    constexpr const char* MED_INDIGO = "\033[38;5;105m";
    constexpr const char* LIGHT_LAVENDER = "\033[38;5;189m";
    constexpr const char* ALMOST_WHITE = "\033[38;5;231m";

    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* DIM = "\033[2m";

    constexpr const char* CLEAR_SCREEN = "\033[2J\033[3J\033[H";  

    constexpr const char* CLEAR_LINE = "\033[2K";
    constexpr const char* SAVE_CURSOR = "\033[s";
    constexpr const char* RESTORE_CURSOR = "\033[u";
    constexpr const char* HIDE_CURSOR = "\033[?25l";
    constexpr const char* SHOW_CURSOR = "\033[?25h";

    inline std::string move_cursor(int row, int col) {
        return "\033[" + std::to_string(row) + ";" + std::to_string(col) + "H";
    }

    inline std::string move_up(int n) {
        return "\033[" + std::to_string(n) + "A";
    }

    inline std::string move_down(int n) {
        return "\033[" + std::to_string(n) + "B";
    }

    struct PVEntry {
        int depth;
        int32_t score;
        search::Line line;
    };

    struct DisplayState {
        bool searching = false;
        search::Report last_report = {};
        Position current_position = {};
        std::vector<PVEntry> pv_history;
        search::Line exploring_line;
    };

    extern DisplayState state;

    void show_banner();
    void show_position(const Position& position, int32_t eval_cp = 0);
    void show_search_update(const search::Report& report, bool final = false, Move best_move = Move(), const search::Line& exploring = search::Line());
    void clear();

    std::string render_board(const Position& position);
    std::string render_metadata(const Position& position);
    std::string render_stats(const search::Report& report, Move best_move = Move());
    std::string render_pv(const search::Line& line);

    std::string get_piece_symbol(Piece piece);
    std::string format_number(uint64_t num);
    std::string format_time(int64_t ms);
    std::string format_score(int32_t score);
}

