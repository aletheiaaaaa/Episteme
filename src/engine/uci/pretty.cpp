#include "pretty.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace episteme::pretty {
    DisplayState state;

    const std::vector<std::string> LOGO_GRADIENT = {
        DIM + std::string("\033[38;5;63m"),
        "\033[38;5;63m",
        "\033[38;5;105m",
        "\033[38;5;111m",
        "\033[38;5;153m",
        "\033[38;5;189m",
        ALMOST_WHITE
    };

    const std::vector<std::string> INFO_GRADIENT = {
        DIM + std::string("\033[38;5;63m"),
        "\033[38;5;63m",
        "\033[38;5;105m",
        "\033[38;5;111m",
        "\033[38;5;153m",
        "\033[38;5;189m",
        ALMOST_WHITE
    };

    void show_banner() {
        std::cout << CLEAR_SCREEN << HIDE_CURSOR;

        std::vector<std::string> possible_paths = {
            "logo.txt",
            "../logo.txt",
            "../../logo.txt",
        };

        std::vector<std::string> logo_lines;
        bool logo_loaded = false;

        for (const auto& path : possible_paths) {
            std::ifstream logo_file(path);
            if (logo_file) {
                std::string line;
                while (std::getline(logo_file, line)) {
                    logo_lines.push_back(line);
                }
                logo_file.close();
                logo_loaded = true;
                break;
            }
        }

        std::vector<std::string> info_lines = {
            "",
            "",
            "",
            "",
            BOLD + std::string("EPISTEME"),
            "Chess Engine",
            "",
            "by aletheia • v1.0",
            "",
            "",
            "Commands:",
            "  position",
            "  go",
            "  eval",
            "  bench",
            "  perft",
            "  uci",
            "  pretty",
            "  quit"
        };

        size_t max_lines = std::max(logo_lines.size(), info_lines.size());
        for (size_t i = 0; i < max_lines; ++i) {

            size_t logo_color_idx = (i * LOGO_GRADIENT.size()) / std::max(logo_lines.size(), size_t(1));
            if (logo_color_idx >= LOGO_GRADIENT.size()) logo_color_idx = LOGO_GRADIENT.size() - 1;
            std::string logo_color = LOGO_GRADIENT[logo_color_idx];

            size_t info_color_idx = (i * INFO_GRADIENT.size()) / std::max(info_lines.size(), size_t(1));
            if (info_color_idx >= INFO_GRADIENT.size()) info_color_idx = INFO_GRADIENT.size() - 1;
            std::string info_color = INFO_GRADIENT[info_color_idx];

            if (i < logo_lines.size()) {
                std::cout << logo_color << logo_lines[i] << RESET;

                if (logo_lines[i].length() < 75) {
                    std::cout << std::string(75 - logo_lines[i].length(), ' ');
                }
            } else {
                std::cout << std::string(75, ' ');
            }

            if (i < info_lines.size()) {
                std::cout << info_color << info_lines[i] << RESET;
            }

            std::cout << "\n";
        }
        std::cout << RESET << "\n";
        std::cout << SHOW_CURSOR;
    }

    std::string get_piece_symbol(Piece piece) {
        static const std::string symbols[] = {
            "P", "p",
            "N", "n",
            "B", "b",
            "R", "r",
            "Q", "q",
            "K", "k" 
        };

        if (piece == Piece::None) return " ";
        return symbols[piece_idx(piece)];
    }

    std::string render_square_line(const Position& position, int rank, int file, int line_type) {
        std::ostringstream ss;

        ss << LIGHT_LAVENDER << "║" << RESET;

        if (line_type == 0 || line_type == 2) {
            ss << "       ";
        } else {

            int square = rank * 8 + file;
            Piece piece = position.mailbox(square);
            std::string piece_symbol = get_piece_symbol(piece);

            ss << "   ";

            if (piece != Piece::None && episteme::color(piece) == Color::White) {
                ss << BOLD << ALMOST_WHITE << piece_symbol << RESET;
            } else if (piece != Piece::None && episteme::color(piece) == Color::Black) {
                ss << BOLD << DEEP_INDIGO << piece_symbol << RESET;
            } else {
                ss << " ";
            }

            ss << "   ";
        }

        return ss.str();
    }

    std::string render_board(const Position& position) {
        std::ostringstream ss;

        ss << "     ╔" << LIGHT_LAVENDER;
        for (int i = 0; i < 8; i++) {
            ss << "═══════";
            if (i < 7) ss << "╦";
        }
        ss << "╗\n" << RESET;

        for (int rank = 7; rank >= 0; rank--) {
            ss << "     ";
            for (int file = 0; file < 8; file++) {
                ss << render_square_line(position, rank, file, 0);
            }
            ss << LIGHT_LAVENDER << "║\n" << RESET;

            ss << "  " << LIGHT_LAVENDER << rank + 1 << "  " << RESET;
            for (int file = 0; file < 8; file++) {
                ss << render_square_line(position, rank, file, 1);
            }
            ss << LIGHT_LAVENDER << "║\n" << RESET;

            ss << "     ";
            for (int file = 0; file < 8; file++) {
                ss << render_square_line(position, rank, file, 2);
            }
            ss << LIGHT_LAVENDER << "║\n" << RESET;

            if (rank > 0) {
                ss << "     ╠";
                for (int i = 0; i < 8; i++) {
                    ss << LIGHT_LAVENDER << "═══════";
                    if (i < 7) ss << "╬";
                }
                ss << "╣\n" << RESET;
            }
        }

        ss << "     ╚" << LIGHT_LAVENDER;
        for (int i = 0; i < 8; i++) {
            ss << "═══════";
            if (i < 7) ss << "╩";
        }
        ss << "╝\n" << RESET;

        ss << "         " << LIGHT_LAVENDER;
        for (char file = 'a'; file <= 'h'; file++) {
            ss << file << "       ";
        }
        ss << RESET << "\n";

        return ss.str();
    }

    std::string format_number(uint64_t num) {
        std::string result = std::to_string(num);
        std::string formatted;
        formatted.reserve(result.length() + result.length() / 3);  

        int count = 0;
        for (auto it = result.rbegin(); it != result.rend(); ++it) {
            if (count == 3) {
                formatted.push_back(',');
                count = 0;
            }
            formatted.push_back(*it);
            count++;
        }

        std::reverse(formatted.begin(), formatted.end());
        return formatted;
    }

    std::string format_time(int64_t ms) {
        if (ms < 1000) return std::to_string(ms) + "ms";
        if (ms < 60000) return std::to_string(ms / 1000) + "." + std::to_string((ms % 1000) / 100) + "s";
        int sec = ms / 1000;
        int min = sec / 60;
        sec = sec % 60;
        return std::to_string(min) + "m" + std::to_string(sec) + "s";
    }

    std::string format_score(int32_t score) {
        constexpr int32_t MATE = 1048575;
        constexpr int32_t MAX_SEARCH_PLY = 256;

        if (std::abs(score) >= MATE - MAX_SEARCH_PLY) {
            int mate_in = score > 0 ? (MATE - score + 1) / 2 : -(MATE + score) / 2;
            return "M" + std::to_string(mate_in);
        }

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << (score > 0 ? "+" : "") << (score / 100.0);
        return ss.str();
    }

    std::string render_metadata(const Position& position) {
        std::ostringstream ss;

        ss << "\n";
        ss << LIGHT_LAVENDER << "    FEN:         " << RESET << MED_INDIGO << position.to_FEN() << RESET << "\n";
        ss << LIGHT_LAVENDER << "    Zobrist:     " << RESET << MED_INDIGO << "0x" << std::hex << position.full_hash() << std::dec << RESET << "\n";

        AllowedCastles castles = position.all_rights();
        ss << LIGHT_LAVENDER << "    Castling:    " << RESET << MED_INDIGO;
        if (castles.rooks[color_idx(Color::White)].is_kingside_set()) ss << "K";
        if (castles.rooks[color_idx(Color::White)].is_queenside_set()) ss << "Q";
        if (castles.rooks[color_idx(Color::Black)].is_kingside_set()) ss << "k";
        if (castles.rooks[color_idx(Color::Black)].is_queenside_set()) ss << "q";
        if (!castles.rooks[color_idx(Color::White)].is_kingside_set() &&
            !castles.rooks[color_idx(Color::White)].is_queenside_set() &&
            !castles.rooks[color_idx(Color::Black)].is_kingside_set() &&
            !castles.rooks[color_idx(Color::Black)].is_queenside_set()) ss << "-";
        ss << RESET << "\n";

        ss << LIGHT_LAVENDER << "    En passant:  " << RESET << MED_INDIGO;
        Square ep = position.ep_square();
        if (ep != Square::None) {
            int file_idx = sq_idx(ep) % 8;
            int rank_idx = sq_idx(ep) / 8;
            ss << char('a' + file_idx) << char('1' + rank_idx);
        } else {
            ss << "-";
        }
        ss << RESET << "\n";
        ss << LIGHT_LAVENDER << "    Halfmoves:   " << RESET << MED_INDIGO << int(position.half_move_clock()) << RESET << "\n";

        return ss.str();
    }

    std::string format_move_sequence(const search::Line& line, size_t max_moves) {
        std::ostringstream ss;
        Position pos = state.current_position;

        uint32_t move_number = pos.full_move_number();
        bool is_white_to_move = (pos.STM() == Color::White);

        for (size_t i = 0; i < std::min(line.length, max_moves); ++i) {
            if (is_white_to_move) {
                ss << move_number << ". ";
            } else if (i == 0) {
                ss << move_number << "... ";
            }

            ss << line.moves[i].to_PGN(pos) << " ";
            pos.make_move(line.moves[i]);

            if (is_white_to_move) {
                is_white_to_move = false;
            } else {
                is_white_to_move = true;
                move_number++;
            }
        }
        if (line.length > max_moves) ss << "...";
        return ss.str();
    }

    std::string render_pv(const search::Line& line) {
        std::ostringstream ss;
        ss << BOLD << LIGHT_LAVENDER << "Best Line: " << RESET << DEEP_INDIGO;
        ss << format_move_sequence(line, 10);
        ss << RESET;
        return ss.str();
    }

    std::string render_stats(const search::Report& report, Move best_move) {
        std::ostringstream ss;

        int64_t time_ms = report.time;
        uint64_t nps = time_ms > 0 ? (report.nodes * 1000) / time_ms : report.nodes;

        ss << "\n";
        ss << LIGHT_LAVENDER << "    Depth:       " << RESET << MED_INDIGO << report.depth << RESET << "\n";
        ss << LIGHT_LAVENDER << "    Seldepth:    " << RESET << MED_INDIGO << report.seldepth << RESET << "\n";
        ss << LIGHT_LAVENDER << "    Nodes:       " << RESET << MED_INDIGO << format_number(report.nodes) << RESET << "\n";
        ss << LIGHT_LAVENDER << "    Time:        " << RESET << MED_INDIGO << format_time(time_ms) << RESET << "\n";
        ss << LIGHT_LAVENDER << "    NPS:         " << RESET << MED_INDIGO << format_number(nps) << RESET << "\n";
        ss << LIGHT_LAVENDER << "    Hashfull:    " << RESET << MED_INDIGO << (report.hashfull / 10.0) << "%" << RESET << "\n";

        if (state.exploring_line.length > 0 && state.searching) {
            ss << "\n";
            ss << LIGHT_LAVENDER << "    Exploring:   " << RESET << DIM << DEEP_INDIGO;
            ss << format_move_sequence(state.exploring_line, 6);
            ss << RESET << "\n";
        }

        ss << "\n";

        for (size_t i = 0; i < state.pv_history.size(); ++i) {
            const auto& entry = state.pv_history[i];
            ss << DIM << LIGHT_LAVENDER << "    Depth " << (entry.depth < 10 ? " " : "") << entry.depth << ":   " << RESET;
            ss << MED_INDIGO << format_score(entry.score) << " " << RESET;
            ss << DEEP_INDIGO << format_move_sequence(entry.line, 6) << RESET << "\n";
        }

        if (report.line.length > 0) {
            ss << RESET << "\n";
        }

        if (!best_move.is_empty()) {
            uint32_t move_number = state.current_position.full_move_number();
            bool is_white_to_move = (state.current_position.STM() == Color::White);

            ss << BOLD << ALMOST_WHITE << "    Best move:   " << RESET << DEEP_INDIGO;

            if (is_white_to_move) {
                ss << move_number << ". ";
            } else {
                ss << move_number << "... ";
            }

            ss << best_move.to_PGN(state.current_position) << RESET << "\n";
        }

        return ss.str();
    }

    void render_two_column_layout(const std::string& board_content, const std::string& right_panel_content, const std::string& header_suffix, const std::string& header_label, const std::string& header_value, bool show_cursor_after) {
        std::cout << CLEAR_SCREEN << HIDE_CURSOR;

        std::istringstream board_stream(board_content);
        std::istringstream panel_stream(right_panel_content);

        std::vector<std::string> board_lines;
        std::vector<std::string> panel_lines;

        std::string line;
        while (std::getline(board_stream, line)) board_lines.push_back(line);
        while (std::getline(panel_stream, line)) panel_lines.push_back(line);

        std::cout << "\n" << BOLD << LIGHT_LAVENDER << "  EPISTEME" << RESET;
        if (!header_suffix.empty()) {
            std::cout << LIGHT_LAVENDER << " - " << header_suffix << RESET;
        }

        size_t padding = header_suffix.empty() ? 61 : 61 - header_suffix.length();
        std::cout << std::string(padding, ' ');

        std::cout << LIGHT_LAVENDER << header_label
                    << BOLD << MED_INDIGO << header_value << RESET << "\n\n";

        size_t max_lines = std::max(board_lines.size(), panel_lines.size());

        for (size_t i = 0; i < max_lines; ++i) {

            if (i < board_lines.size()) {
                std::cout << board_lines[i];
            } else {
                std::cout << std::string(40, ' ');
            }

            if (i < panel_lines.size()) {
                std::cout << panel_lines[i];
            }

            std::cout << "\n";
        }

        std::cout << (show_cursor_after ? "\n" : "") << std::flush;

        if (show_cursor_after) {
            std::cout << SHOW_CURSOR;
        }
    }

    void show_position(const Position& position, int32_t eval_cp) {
        state.current_position = position;
        std::string board = render_board(position);
        std::string metadata = render_metadata(position);
        render_two_column_layout(board, metadata, "Position", "Evaluation: ", format_score(eval_cp), true);
    }

    void show_search_update(const search::Report& report, bool final, Move best_move, const search::Line& exploring) {
        if (report.depth < state.last_report.depth) {
            state.pv_history.clear();
        }

        if (report.depth > state.last_report.depth && report.line.length > 0) {
            PVEntry entry;
            entry.depth = report.depth;
            entry.score = report.score;
            entry.line = report.line;

            if (state.pv_history.size() >= 8) {
                state.pv_history.erase(state.pv_history.begin());
            }
            state.pv_history.push_back(entry);
        }

        state.exploring_line = exploring;

        std::string board = render_board(state.current_position);
        std::string metadata = render_metadata(state.current_position);
        std::string stats = render_stats(report, best_move);
        std::string combined_panel = metadata + stats;
        render_two_column_layout(board, combined_panel, "Searching...", "Score: ", format_score(report.score), final);

        state.last_report = report;
        state.searching = !final;
    }

    void clear() {
        state.searching = false;
        state.last_report = {};
        state.pv_history.clear();
    }
}