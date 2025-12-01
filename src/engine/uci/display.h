#pragma once

#include "../chess/position.h"
#include "../search/search.h"
#include "../chess/move.h"

#include <memory>
#include <iostream>

namespace episteme::uci {
    class DisplayMode {
        public:
            virtual ~DisplayMode() = default;

            virtual void show_position(const Position& position, int32_t eval_cp) = 0;

            virtual void on_start(const Position& position) = 0;
            virtual void on_update(const search::Report& report) = 0;
            virtual void on_completion(const search::Report& report, Move best_move) = 0;
            virtual void update_exploring(const search::Line& line) = 0;
            virtual void set_engine(search::Engine* engine) = 0;

            virtual void clear() = 0;
    };

    class PrettyDisplay : public DisplayMode {
        public:
            PrettyDisplay();
            ~PrettyDisplay();

            void show_position(const Position& position, int32_t eval_cp) override;
            void on_start(const Position& position) override;
            void on_update(const search::Report& report) override;
            void on_completion(const search::Report& report, Move best_move) override;
            void update_exploring(const search::Line& line) override;
            void set_engine(search::Engine* engine) override;
            void clear() override;

        private:
            void update_loop();

            std::thread update_thread;
            std::atomic<bool> searching{false};
            std::atomic<uint64_t> live_nodes{0};
            std::chrono::steady_clock::time_point search_start;
            search::Report current_report;
            search::Line current_exploring;
            std::mutex exploring_mutex;
            search::Engine* engine_ptr{nullptr};
    };

    class UCIDisplay : public DisplayMode {
        public:
            void show_position(const Position& position, int32_t eval_cp) override;
            void on_start(const Position& position) override;
            void on_update(const search::Report& report) override;
            void on_completion(const search::Report& report, Move best_move) override;
            void update_exploring(const search::Line& line) override;
            void set_engine(search::Engine* engine) override;
            void clear() override;
    };

    DisplayMode& mode();
    void set_pretty(bool enabled);
}
