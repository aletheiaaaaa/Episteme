#include "datagen.h"

#include <cassert>
#include <format>

namespace episteme::datagen {
    using namespace std::chrono;

    void play_random(Position& position, int32_t num_moves) {
        for (int i = 0; i < num_moves; i++) {
            MoveList move_list;
            generate_all_moves(move_list, position); 
            move_list.shuffle();

            bool found_legal = false;

            for (size_t j = 0; j < move_list.count; j++) {
                position.make_move(move_list[j]);
                if (!in_check(position, position.NTM())) {
                    found_legal = true;
                    break;
                }

                position.unmake_move();
            }

            if (!found_legal) break;
        }
    }

    void game_loop(const Parameters& params, std::ostream& stream, uint32_t id) {
        Position position;
        position.from_startpos();

        Format formatter{};

        search::Parameters search_params{
            .nodes = params.hard_limit,
            .soft_nodes = params.soft_limit,
        };

        search::Config cfg{
            .params = search_params,
            .hash_size = params.hash_size,
            .num_threads = params.num_threads,
            .position = {}
        };

        search::Engine engine(cfg);
        time_point start = steady_clock::now();
        size_t positions = 0;
        size_t games = 0;
        
        int workload = params.num_games / params.num_threads;
        for (int i = 0; i < workload && !stop; i++) {
            engine.reset_game();
            play_random(position, 8);
            formatter.init(position);

            search::ScoredMove initial = engine.datagen_search(position);
            if (initial.score >= INITIAL_MAX) {
                i--;
                position.from_startpos();
                continue;
            };

            uint64_t win_plies = 0, draw_plies = 0, loss_plies = 0;
            std::optional<uint8_t> wdl{};

            while (!stop) {
                const search::ScoredMove scored_move = engine.datagen_search(position);

                if (!scored_move.move) {
                    wdl = in_check(position, position.STM()) ? (position.STM() == Color::Black ? 2 : 0) : 1;
                    break;
                } else {
                    if (std::abs(scored_move.score) >= search::MATE - search::MAX_SEARCH_PLY) wdl = 2 * (scored_move.score > 0);
                    else {
                        if (scored_move.score >= WIN_SCORE_MIN) win_plies++, draw_plies = loss_plies = 0;
                        else if (scored_move.score <= -WIN_SCORE_MIN) loss_plies++, win_plies = draw_plies = 0;
                        else if (std::abs(scored_move.score) <= DRAW_SCORE_MAX && position.half_move_clock() >= 100) draw_plies++, win_plies = loss_plies = 0;

                        if (win_plies >= WIN_PLIES_MIN) wdl = 2;
                        else if (loss_plies >= WIN_PLIES_MIN) wdl = 0;
                        else if (draw_plies >= DRAW_PLIES_MIN) wdl = 1;    
                    }
                }

                if (wdl) break;

                position.make_move(scored_move.move);

                if (position.is_threefold() || position.is_insufficient()) {
                    formatter.push(scored_move.move, 0);
                    wdl = 1;
                    break;
                }

                formatter.push(scored_move.move, scored_move.score);
                engine.reset_go();
            }

            games++;
            positions += formatter.write(stream, static_cast<uint8_t>(*wdl));

            position.from_startpos();

            if ((i + 1) % 16 == 0 || (i + 1) == workload) {
                time_point end = steady_clock::now();
                int32_t elapsed = duration_cast<milliseconds>(end - start).count() / 1000;
                std::cout << "Thread " << id << ": " << games << "/" << workload << " games completed at "
                          << (positions / (elapsed > 0 ? elapsed : 1)) << " pos/sec\n";

                start = steady_clock::now();
                positions = 0;
            }
        }
    }

    void run(Parameters& params) {
        std::cout << "Beginning datagen.\n";

        std::signal(SIGINT, []([[maybe_unused]] int signum){
            stop = true;
            std::cout << "Datagen interrupted.\n";
            return;
        });

        params.num_games = params.num_threads * (params.num_games / params.num_threads);

        std::vector<std::thread> threads;
        std::vector<std::string> files;

        std::filesystem::create_directory(std::filesystem::path(params.out_dir));

        for (size_t i = 0; i < params.num_threads; i++) {

            std::ostringstream oss;
            oss << params.out_dir << "/temp_" << i << "." << Format::EXTENSION;
            const auto file = std::filesystem::path(oss.str());
            files.push_back(file.string());

            threads.emplace_back(
                [&params, file = std::move(file), i]() {
                    std::ofstream stream(file, std::ios::binary | std::ios::app);
                    if (!stream) {
                        std::cout << "Failed to open " << file.string() << " on thread " << i << "\n";
                        return;
                    }

                    game_loop(params, stream, i);

                    if (!stream.good()) {
                        std::cout << "Error encountered on thread " << i << " when closing\n";
                    }
                }
            );
        }

        for (auto& thread : threads) thread.join();

        std::cout << "Generated " << params.num_threads << " data files. Concatenating.\n";

        std::ostringstream oss;
        oss << params.out_dir << "/data." << Format::EXTENSION;
        const auto output = std::filesystem::path(oss.str());

        std::ofstream final(output, std::ios::binary);
        if (!final) {
            std::cout << "Failed to open output file " << output.string() << "\n";
        }

        int32_t concats = 0;
        for (const auto& file : files) {
            std::ifstream input(file, std::ios::binary);
            if (input) {
                final << input.rdbuf();
                input.close();

                if (std::remove(file.c_str()) == 0) concats++;
                else std::cout << "Error removing file " << file << " after concatenation\n";
            } else {
                std::cout << "Error opening file " << file << " during concatenation\n";
            }
        }

        std::cout << "Concatenated " << concats << " files\n";
        std::cout << "Datagen complete.\n";
    }
}