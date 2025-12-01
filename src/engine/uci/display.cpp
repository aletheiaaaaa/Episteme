#include "display.h"
#include "pretty.h"

#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>

namespace episteme::uci {
    using namespace std::chrono;

    namespace {
        enum class DisplayMode {
            PRETTY,
            UCI
        };

        DisplayMode current_mode = DisplayMode::PRETTY;

        // PrettyDisplay state
        std::thread update_thread;
        std::atomic<bool> searching{false};
        std::atomic<uint64_t> live_nodes{0};
        steady_clock::time_point search_start;
        search::Report current_report;
        search::Line current_exploring;
        std::mutex exploring_mutex;
        search::Engine* engine_ptr{nullptr};

        // Forward declarations
        void pretty_update_loop();
        void cleanup_pretty();
    }

    void set_pretty(bool enabled) {
        cleanup_pretty();

        if (enabled) {
            current_mode = DisplayMode::PRETTY;
            pretty::show_banner();
        } else {
            current_mode = DisplayMode::UCI;
            std::cout << pretty::CLEAR_SCREEN << std::flush;
        }
    }

    bool is_pretty() {
        return current_mode == DisplayMode::PRETTY;
    }

    void show_position(const Position& position, int32_t eval_cp) {
        if (current_mode == DisplayMode::PRETTY) {
            pretty::show_position(position, eval_cp);
        }
        // UCI mode doesn't show position
    }

    void on_start(const Position& position) {
        if (current_mode == DisplayMode::PRETTY) {
            searching = true;
            search_start = steady_clock::now();
            live_nodes = 0;
            current_exploring.clear();

            if (engine_ptr) {
                auto* worker = engine_ptr->get_worker(0);
                if (worker) {
                    worker->set_live_update_callback([](uint64_t nodes, const search::Line& line) {
                        live_nodes.store(nodes);
                        std::lock_guard<std::mutex> lock(exploring_mutex);
                        current_exploring = line;
                    });
                }
            }

            update_thread = std::thread(pretty_update_loop);
        }
        // UCI mode doesn't need start handling
    }

    void on_update(const search::Report& report) {
        if (current_mode == DisplayMode::PRETTY) {
            current_report = report;
            live_nodes = report.nodes;
        } else {
            // UCI mode
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
    }

    void on_completion(const search::Report& report, Move best_move) {
        if (current_mode == DisplayMode::PRETTY) {
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
        } else {
            // UCI mode
            std::cout << "bestmove " << best_move.to_string() << "\n";
        }
    }

    void update_exploring(const search::Line& line) {
        if (current_mode == DisplayMode::PRETTY) {
            std::lock_guard<std::mutex> lock(exploring_mutex);
            current_exploring = line;
        }
        // UCI mode doesn't track exploring
    }

    void set_engine(search::Engine* engine) {
        engine_ptr = engine;
    }

    void clear() {
        if (current_mode == DisplayMode::PRETTY) {
            pretty::clear();
        }
        // UCI mode doesn't need clearing
    }

    namespace {
        void pretty_update_loop() {
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

        void cleanup_pretty() {
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
    }
}
