#include "listener.hpp"

#include <cstdlib>
#include <mutex>
#include <print>
#include <sstream>
#include <string>

#include "../../utils/datagen.hpp"
#include "../../utils/tunable.hpp"
#include "../chess/perft.hpp"
#include "../chess/position.hpp"

namespace episteme::uci {
using namespace tunable;

void Listener::listen() {
  std::string line;
  while (std::getline(inputs, line)) {
    parse(line);
    if (line == "quit") return;
  }
  quit();
}

void Listener::add(const Command& cmd) {
  {
    std::lock_guard<std::mutex> lock(mutex);
    cmds.push(cmd);
  }
  cond.notify_one();
}

Command Listener::next() {
  std::unique_lock<std::mutex> lock(mutex);
  cond.wait(lock, [this] { return !cmds.empty(); });
  Command cmd = cmds.front();
  cmds.pop();

  return cmd;
}

void Listener::parse(const std::string& cmd) {
  size_t space = cmd.find(' ');
  std::string keyword = cmd.substr(0, space);
  std::string args = (space != std::string::npos) ? cmd.substr(space + 1) : "";

  if (keyword == "uci")
    uci();
  else if (keyword == "setoption")
    setoption(args);
  else if (keyword == "isready")
    isready();
  else if (keyword == "position")
    position(args);
  else if (keyword == "go")
    go(args);
  else if (keyword == "ucinewgame")
    ucinewgame();
  else if (keyword == "bench")
    bench(args);
  else if (keyword == "perft")
    perft(args);
  else if (keyword == "eval")
    eval();
  else if (keyword == "printob")
    print_tunables();
  else if (keyword == "fen")
    fen();
  else if (keyword == "quit") {
    quit();
    return;
  }
  else
    std::println("invalid command");
}
void Listener::uci() {
  std::println("id name Episteme \nid author aletheia");
  std::println("option name Hash type spin default 32 min 1 max 128");
  std::println("option name Threads type spin default 1 min 1 max 1");
  for (int i = 0; i < Tunable<int>::registry.size(); i++) {
    Tunable<int>::registry[i]->print();
  }
  std::println("uciok");
}

void Listener::setoption(const std::string& args) {
  std::istringstream iss(args);
  std::string name, option_name, value, option_value;

  iss >> name >> option_name >> value >> option_value;

  if (name != "name" || value != "value") {
    std::println("invalid command");
  }

  if (option_name == "Hash") {
    cfg.hash_size = std::stoi(option_value);
  } else if (option_name == "Threads") {
    cfg.num_threads = std::stoi(option_value);
  } else if (!Tunable<int>::registry.empty()) {
    for (int i = 0; i < Tunable<int>::registry.size(); i++) {
      if (Tunable<int>::registry[i]->name == option_name) {
        Tunable<int>::registry[i]->set(std::stoi(option_value));
        break;
      }
    }
  } else {
    std::println("invalid option");
  }

  add({.config = cfg, .flag = Flag::Init});
}

void Listener::isready() { std::println("readyok"); }

void Listener::quit() { add({.config = cfg, .flag = Flag::Quit}); }

void Listener::position(const std::string& args) {
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
    std::println("invalid command");
  }

  if (iss >> token && token == "moves") {
    while (iss >> token) {
      position.make_move(from_UCI(position, token));
    }
  }

  cfg.position = position;
}

void Listener::go(const std::string& args) {
  std::istringstream iss(args);
  std::string token;

  cfg.params = {};

  while (iss >> token) {
    if (token == "wtime" && iss >> token)
      cfg.params.time[0] = std::max(std::stoi(token) - 100, 0);
    else if (token == "btime" && iss >> token)
      cfg.params.time[1] = std::max(std::stoi(token) - 100, 0);
    else if (token == "winc" && iss >> token)
      cfg.params.inc[0] = std::stoi(token);
    else if (token == "binc" && iss >> token)
      cfg.params.inc[1] = std::stoi(token);
    else if (token == "depth" && iss >> token)
      cfg.params.depth = std::stoi(token);
    else if (token == "nodes" && iss >> token)
      cfg.params.nodes = std::stoi(token);
    else if (token == "movetime" && iss >> token)
      cfg.params.move_time = std::stoi(token);
    else {
      std::println("invalid command");
      break;
    }
  }

  add({.config = cfg, .flag = Flag::Run});
}

void Listener::ucinewgame() {
  cfg.params = {};
  cfg.position = {};
  
  add({.config = cfg, .flag = Flag::Reset});
}

void Listener::eval() { add({.config = cfg, .flag = Flag::Eval}); }

void Listener::bench(const std::string& args) {
  int depth = (args.empty()) ? 10 : std::stoi(args);
  if (!cfg.hash_size) cfg.hash_size = 32;

  cfg.params.depth = depth;
  add({.config = cfg, .flag = Flag::Bench});
}

void Listener::perft(const std::string& args) {
  int depth = (args.empty()) ? 6 : std::stoi(args);
  Position& position = cfg.position;

  perft_timed(position, depth);
}

void Listener::print_tunables() {
  for (int i = 0; i < Tunable<int>::registry.size(); i++) {
    Tunable<int>::registry[i]->print(true);
  }
}

void Listener::fen() { std::println("{}", cfg.position.to_FEN()); }

void datagen(const std::string& args) {
  std::istringstream iss(args);
  std::string token;

  datagen::Parameters params;

  while (iss >> token) {
    if (token == "softnodes" && iss >> token)
      params.soft_limit = std::stoi(token);
    else if (token == "hardnodes" && iss >> token)
      params.hard_limit = std::stoi(token);
    else if (token == "games" && iss >> token)
      params.num_games = std::stoi(token);
    else if (token == "threads" && iss >> token)
      params.num_threads = std::stoi(token);
    else if (token == "hash" && iss >> token)
      params.hash_size = std::stoi(token);
    else if (token == "dir" && iss >> token)
      params.out_dir = token;
    else {
      std::println("invalid command");
    }
  }

  datagen::run(params);
}
}  // namespace episteme::uci
