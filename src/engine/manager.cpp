#include "manager.hpp"
#include "uci/listener.hpp"

namespace episteme {

void Manager::run() {
  while (true) {
    uci::Command cmd = listener.next();
    switch (cmd.flag) {
      case uci::Flag::Init:
        engine.set_hash(cmd.config);
        engine.init_workers(cmd.config);
        break;
      case uci::Flag::Reset:
        engine.reset_game();
        break;
      case uci::Flag::Run:
        engine.reset_go();
        engine.update_params(cmd.config.params);
        engine.run(cmd.config.position);
        break;
      case uci::Flag::Stop:
        engine.abort();
        break;
      case uci::Flag::Eval:
        engine.eval(cmd.config.position);
        break;
      case uci::Flag::Bench:
        engine.bench(cmd.config.params.depth);
        break;
      case uci::Flag::Quit:
        engine.abort();
        return;
      default:
        break;
    }
  }
};
}
