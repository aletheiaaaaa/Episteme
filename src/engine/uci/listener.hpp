#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>

#include "../search/search.hpp"

namespace episteme::uci {

enum class Flag : uint8_t {
  Init, Reset, Update, Run, Stop, Quit, Eval, Bench, None 
};

struct Command {
  search::Config config{};
  Flag flag = Flag::None;
};

class Listener {
  public:
  Listener(std::istream& stream) : inputs(stream) {}

  void listen();
  Command next();
  void add(const Command& cmd);

  private:
  std::queue<Command> cmds;
  std::istream& inputs;
  search::Config cfg;
  std::mutex mutex;
  std::condition_variable cond;

  void parse(const std::string& cmd);

  void uci();
  void setoption(const std::string& args);
  void isready();
  void quit();
  void position(const std::string& args);
  void go(const std::string& args);
  void ucinewgame();
  void eval();
  void bench(const std::string& args);
  void perft(const std::string& args);
  void print_tunables();
  void fen();
};

void datagen(const std::string& args);
}  // namespace episteme::uci
