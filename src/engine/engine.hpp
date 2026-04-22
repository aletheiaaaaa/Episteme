#pragma once

#include "../engine/search/search.hpp"

namespace episteme {
class Engine {
  public:
  Engine(const search::Config& cfg) : ttable(cfg.hash_size), params(cfg.params), limiter() {}

  void set_hash(const search::Config& cfg) { ttable.resize(cfg.hash_size); }

  void update_params(search::Parameters& new_params) { this->params = new_params; }

  void init_workers(const search::Config& cfg) {
    size_t num = cfg.num_threads;

    workers.clear();
    workers.reserve(num);
    for (uint16_t i = 0; i < num; ++i) {
      workers.emplace_back(std::make_unique<search::Worker>(ttable, limiter));
    }
  }

  void abort() { limiter.set_stop(); }

  void reset_nodes() {
    for (auto& worker : workers) {
      worker->reset_nodes();
    }
  }

  void reset_seldepth() {
    for (auto& worker : workers) {
      worker->reset_seldepth();
    }
  }

  void reset_go() {
    for (auto& worker : workers) {
      worker->reset_stop();
      worker->reset_stack();
    }
  }

  void reset_game() {
    ttable.reset();
    for (auto& worker : workers) {
      worker->reset_history();
      worker->reset_accum();
      worker->reset_stop();
      worker->reset_stack();
    }
  }

  void run(Position& position);
  search::ScoredMove datagen_search(Position& position);
  void eval(Position& position);
  void bench(int depth);

  search::Worker* get_worker(size_t idx = 0) {
    return (idx < workers.size()) ? workers[idx].get() : nullptr;
  }

  private:
  tt::Table ttable;
  search::Parameters params;
  time::Limiter limiter;

  std::vector<std::unique_ptr<search::Worker>> workers;
};
}  // namespace episteme
