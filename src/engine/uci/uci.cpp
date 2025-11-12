#include "uci.h"

namespace episteme::uci {

    auto uci() {
        std::cout << "id name Episteme \nid author aletheia\n";
        std::cout << "option name Hash type spin default 32 min 1 max 128\n";
        std::cout << "option name Threads type spin default 1 min 1 max 1\n";
        std::cout << "uciok\n";
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
        } else {
            std::cout << "invalid option" << std::endl;
        }

        engine.set_hash(cfg);
    }

    auto isready() {
        std::cout << "readyok\n";
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
    }

    auto go(const std::string& args, search::Config& cfg, search::Engine& engine) {
        std::istringstream iss(args);
        std::string token;

        while (iss >> token) {
            if (token == "wtime" && iss >> token) cfg.params.time[0] = std::stoi(token);
            else if (token == "btime" && iss >> token) cfg.params.time[1] = std::stoi(token);
            else if (token == "winc" && iss >> token) cfg.params.inc[0] = std::stoi(token);
            else if (token == "binc" && iss >> token) cfg.params.inc[1] = std::stoi(token);
            else if (token == "depth" && iss >> token) cfg.params.depth = std::stoi(token);
            else if (token == "nodes" && iss >> token) cfg.params.nodes = std::stoi(token);
            else {
                std::cout << "invalid command\n"; 
                break;
            }
        }

        engine.reset_go();
        engine.update_params(cfg.params);
        engine.run(cfg.position);
    }

    auto ucinewgame(search::Config& cfg, search::Engine& engine) {
        cfg.params = {};
        cfg.position = {};
        engine.reset_game();
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

    int parse(const std::string& cmd, search::Config& cfg, search::Engine& engine) {
        std::string keyword = cmd.substr(0, cmd.find(' '));

        if (keyword == "uci") uci();
        else if (keyword == "setoption") setoption(cmd.substr(cmd.find(" ")+1), cfg, engine);
        else if (keyword == "isready") isready();
        else if (keyword == "position") position(cmd.substr(cmd.find(" ")+1), cfg);
        else if (keyword == "go") go(cmd.substr(cmd.find(" ")+1), cfg, engine);
        else if (keyword == "ucinewgame") ucinewgame(cfg, engine);
        else if (keyword == "quit") std::exit(0);

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

        else std::cout << "invalid command\n";

        return 0;
    }
}