#include "display.h"
#include "pretty.h"

#include <mutex>

namespace episteme::uci {
    using namespace std::chrono;

    namespace {
        std::unique_ptr<DisplayMode> current_mode;
    }

    DisplayMode& mode() {
        if (!current_mode) {

            current_mode = std::make_unique<PrettyDisplay>();
        }
        return *current_mode;
    }

    void set_pretty(bool enabled) {
        if (enabled) {
            current_mode = std::make_unique<PrettyDisplay>();
            pretty::show_banner();
        } else {
            current_mode = std::make_unique<UCIDisplay>();

            std::cout << pretty::CLEAR_SCREEN << std::flush;
        }
    }

    PrettyDisplay::PrettyDisplay() = default;

    PrettyDisplay::~PrettyDisplay() {
        if (update_thread.joinable()) {
            searching = false;
            update_thread.join();
        }

        if (engine_ptr) {
            auto* worker = engine_ptr->get_worker(0);
            if (worker) {
                worker->clear_live_update_callback();
            }
        }
    }

    void PrettyDisplay::show_position(const Position& position, int32_t eval_cp) {
        pretty::show_position(position, eval_cp);
    }

    void PrettyDisplay::on_start(const Position& position) {
        searching = true;
        search_start = steady_clock::now();
        live_nodes = 0;
        current_exploring.clear();

        if (engine_ptr) {
            auto* worker = engine_ptr->get_worker(0);
            if (worker) {
                worker->set_live_update_callback([this](uint64_t nodes, const search::Line& line) {
                    live_nodes.store(nodes);
                    std::lock_guard<std::mutex> lock(exploring_mutex);
                    current_exploring = line;
                });
            }
        }

        update_thread = std::thread([this]() { update_loop(); });
    }

    void PrettyDisplay::on_update(const search::Report& report) {
        current_report = report;
        live_nodes = report.nodes;
    }

    void PrettyDisplay::on_completion(const search::Report& report, Move best_move) {
        if (engine_ptr) {
            auto* worker = engine_ptr->get_worker(0);
            if (worker) {
                worker->clear_live_update_callback();
            }
        }

        searching = false;
        if (update_thread.joinable()) {
            update_thread.join();
        }

        search::Report final_report = report;
        auto elapsed = steady_clock::now() - search_start;
        final_report.time = duration_cast<milliseconds>(elapsed).count();

        pretty::show_search_update(final_report, true, best_move);
        std::cout << pretty::SHOW_CURSOR;
    }

    void PrettyDisplay::update_exploring(const search::Line& line) {
        std::lock_guard<std::mutex> lock(exploring_mutex);
        current_exploring = line;
    }

    void PrettyDisplay::set_engine(search::Engine* engine) {
        engine_ptr = engine;
    }

    void PrettyDisplay::clear() {
        pretty::clear();
    }

    void PrettyDisplay::update_loop() {
        while (searching) {
            if (current_report.depth > 0) {

                search::Report live_report = current_report;
                live_report.nodes = live_nodes.load();

                auto elapsed = steady_clock::now() - search_start;
                live_report.time = duration_cast<milliseconds>(elapsed).count();

                search::Line exploring_line;
                {
                    std::lock_guard<std::mutex> lock(exploring_mutex);
                    exploring_line = current_exploring;
                }

                pretty::show_search_update(live_report, false, Move(), exploring_line);
            }
            std::this_thread::sleep_for(milliseconds(50));
        }
    }

    void UCIDisplay::show_position(const Position& position, int32_t eval_cp) {}
    void UCIDisplay::on_start(const Position& position) {}

    void UCIDisplay::on_update(const search::Report& report) {

        constexpr int32_t MATE = 1048575;
        constexpr int32_t MAX_SEARCH_PLY = 256;

        bool is_mate = std::abs(report.score) >= MATE - MAX_SEARCH_PLY;
        int32_t display_score = is_mate ?
            (report.score > 0 ? (MATE - report.score + 1) / 2 : -(MATE + report.score) / 2) :
            report.score;

        int32_t nps = report.time > 0 ? (report.nodes * 1000) / report.time : report.nodes;

        std::string pv;
        for (size_t i = 0; i < report.line.length; ++i) {
            pv += report.line.moves[i].to_string() + " ";
        }

        std::cout << "info depth " << report.depth
                  << " seldepth " << report.seldepth
                  << " time " << report.time
                  << " nodes " << report.nodes
                  << " nps " << nps
                  << " hashfull " << report.hashfull
                  << " score " << (is_mate ? "mate" : "cp") << " " << display_score
                  << " pv " << pv << "\n";
    }

    void UCIDisplay::on_completion(const search::Report& report, Move best_move) {
        std::cout << "bestmove " << best_move.to_string() << "\n";
    }

    void UCIDisplay::update_exploring(const search::Line& line) {}
    void UCIDisplay::set_engine(search::Engine* engine) {}
    void UCIDisplay::clear() {}

}

