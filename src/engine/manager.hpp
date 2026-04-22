#pragma once

#include "engine.hpp"
#include "uci/listener.hpp"

namespace episteme {

class Manager {
  public:
  Manager(uci::Listener& listener, Engine& engine) : listener(listener), engine(engine) {}

  void run();

  private:
  uci::Listener& listener;
  Engine& engine;
};
}  // namespace episteme
