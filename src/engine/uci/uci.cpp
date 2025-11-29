#include "uci.h"

namespace episteme::uci {
    using namespace tunable;

    auto uci() {
        set_pretty(false);

        std::cout << "id name Episteme \nid author aletheia\n";
        std::cout << "option name Hash type spin default 32 min 1 max 128\n";
        std::cout << "option name Threads type spin default 1 min 1 max 1\n";
#if ENABLE_TUNING
        for (const auto& tunable : tunables()) {
            std::cout << "option name " << tunable.name << " type spin default "
                      << tunable.value << " min " << tunable.min << " max " << tunable.max << "\n";
        }
#endif
        std::cout << "uciok\n";
    }

    auto pretty_cmd() {
        set_pretty(true);
    }

    auto setoption(const std::string& args, search::Config& cfg, search::Engine& engine) {
        std::istringstream iss(args);
        std::string name, option_name, value, option_value;
    
        iss >> name >> option_name >> value >> option_value;
    
        if (name != "name" || value != "value") {
            std::cout << "invalid command" << std::endl;
        }
    
        if (option_name == "Hash") {
            cfg.hash_size = std::stoi(option_value);
        } else if (option_name == "Threads") {
            cfg.num_threads = std::stoi(option_value);
        } 
#if ENABLE_TUNING
        else if (!tunables().empty()) {
            for (auto& tunable : tunables()) {
                if (tunable.name == option_name) {
                    int32_t new_value = std::stoi(option_value);
                    if (new_value < tunable.min || new_value > tunable.max) {
                        std::cout << "value out of range" << std::endl;
                        return;
                    }
                    tunable.value = new_value;
                    if (tunable.setter) tunable.setter();
                    break;
                }
            }
        }
#endif
        else {
            std::cout << "invalid option" << std::endl;
        }

        engine.set_hash(cfg);
    }

    auto isready() {
        std::cout << "readyok" << std::endl;
    }

    auto position(const std::string& args, search::Config& cfg) {
        Position position;
        std::istringstream iss(args);
        std::string token;

        iss >> token;

        if (token == "startpos") {
            position.from_startpos();

        } else if (token == "fen") {
            std::string fen;
            for (int i = 0; i < 6 && iss >> token; ++i) {
                if (!fen.empty()) fen += " ";
                fen += token;
            }
            position.from_FEN(fen);

        } else {
            std::cout << "invalid command\n";
        }

        if (iss >> token && token == "moves") {
            while (iss >> token) {
                position.make_move(from_UCI(position, token));
            }
        }

        cfg.position = position;

        mode().show_position(position, 0);
    }

    auto go(const std::string& args, search::Config& cfg, search::Engine& engine) {
        std::istringstream iss(args);
        std::string token;

        cfg.params = {};

        while (iss >> token) {
            if (token == "wtime" && iss >> token) cfg.params.time[0] = std::stoi(token);
            else if (token == "btime" && iss >> token) cfg.params.time[1] = std::stoi(token);
            else if (token == "winc" && iss >> token) cfg.params.inc[0] = std::stoi(token);
            else if (token == "binc" && iss >> token) cfg.params.inc[1] = std::stoi(token);
            else if (token == "depth" && iss >> token) cfg.params.depth = std::stoi(token);
            else if (token == "nodes" && iss >> token) cfg.params.nodes = std::stoi(token);
            else if (token == "movetime" && iss >> token) cfg.params.move_time = std::stoi(token);
            else {
                std::cout << "invalid command\n";
                break;
            }
        }

        engine.reset_go();
        engine.update_params(cfg.params);
        mode().set_engine(&engine);
        engine.run(cfg.position);
    }

    auto ucinewgame(search::Config& cfg, search::Engine& engine) {
        cfg.params = {};
        cfg.position = {};
        engine.reset_game();

        mode().clear();

        Position empty_position;
        mode().show_position(empty_position, 0);
    }
    
    auto eval(search::Config& cfg, search::Engine& engine) {
        engine.eval(cfg.position);
    }
    
    auto bench(const std::string& args, search::Config& cfg) {
        int depth = (args.empty()) ? 10 : std::stoi(args);
        if (!cfg.hash_size) cfg.hash_size = 32;

        search::Engine engine(cfg);
        engine.bench(depth);
    }

    auto perft(const std::string& args, search::Config& cfg) {
        int depth = (args.empty()) ? 6 : std::stoi(args);
        Position& position = cfg.position;

        time_perft(position, depth);
    }

    auto datagen(const std::string& args) {
        std::istringstream iss(args);
        std::string token;

        datagen::Parameters params;

        while (iss >> token) {
            if (token == "softnodes" && iss >> token) params.soft_limit = std::stoi(token);
            else if (token == "hardnodes" && iss >> token) params.hard_limit = std::stoi(token);
            else if (token == "games" && iss >> token) params.num_games = std::stoi(token);
            else if (token == "threads" && iss >> token) params.num_threads = std::stoi(token);
            else if (token == "hash" && iss >> token) params.hash_size = std::stoi(token);
            else if (token == "dir" && iss >> token) params.out_dir = token;
            else {
                std::cout << "invalid command" << std::endl;
            }
        }

        datagen::run(params);
    }

#if ENABLE_TUNING
    auto print_tunables() {
        for (const auto& tunable : tunables()) {
            std::cout << tunable.name << ", int, " << tunable.value << ".0, "
                      << tunable.min << ".0, " << tunable.max << ".0, "
                      << tunable.step << ", 0.002\n";
        }
    }
#endif

    auto fen(search::Config& cfg) {
        std::cout << cfg.position.to_FEN() << std::endl;
    }

    int parse(const std::string& cmd, search::Config& cfg, search::Engine& engine) {
        std::string keyword = cmd.substr(0, cmd.find(' '));

        if (keyword == "uci") uci();
        else if (keyword == "pretty") pretty_cmd();
        else if (keyword == "setoption") setoption(cmd.substr(cmd.find(" ")+1), cfg, engine);
        else if (keyword == "isready") isready();
        else if (keyword == "position") position(cmd.substr(cmd.find(" ")+1), cfg);
        else if (keyword == "go") go(cmd.substr(cmd.find(" ")+1), cfg, engine);
        else if (keyword == "ucinewgame") ucinewgame(cfg, engine);
        else if (keyword == "quit") {
            if (dynamic_cast<PrettyDisplay*>(&mode())) {
                std::cout << "\033[2J\033[3J\033[H" << "\033[?25h" << std::flush;
            }
            std::exit(0);
        }

        else if (keyword == "bench") {
            size_t space = cmd.find(' ');
            std::string arg = (space != std::string::npos) ? cmd.substr(space+1) : "";
            bench(arg, cfg);
        }
        else if (keyword == "perft") {
            size_t space = cmd.find(' ');
            std::string arg = (space != std::string::npos) ? cmd.substr(space+1) : "";
            perft(arg, cfg);
        }

        else if (keyword == "eval") eval(cfg, engine);
        else if (keyword == "datagen") datagen(cmd.substr(cmd.find(" ")+1));
#if ENABLE_TUNING
        else if (keyword == "printob") print_tunables();
#endif 
        else if (keyword == "fen") fen(cfg);

        else std::cout << "invalid command\n";

        return 0;
    }
}