#include <iostream>
#include <sstream>

#include "engine/engine.hpp"
#include "engine/search/search.hpp"
#include "engine/uci/listener.hpp"

using namespace episteme;

int main(int argc, char* argv[]) {
  hash::init();
  tunable::init_lmr_table();

  std::istringstream stream;
  if (argc > 1) {
    std::string cmd;
    for (int i = 1; i < argc; ++i) {
      cmd += argv[i];
      if (i < argc - 1) cmd += ' ';
    }
    stream.str(cmd);
  }

  std::istream& input = (argc > 1) ? static_cast<std::istream&>(stream) : std::cin;

  search::Config cfg;
  Engine engine(cfg);
  engine.init_workers(cfg);

  uci::Listener listener(input, engine);
  listener.start();
  listener.join();

  return 0;
}
