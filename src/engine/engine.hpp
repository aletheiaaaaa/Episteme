#pragma once

#include <print>
#include <vector>

#include "search/latch.hpp"
#include "search/search.hpp"

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
    reports.clear();
    reports.resize(num);
    for (uint16_t i = 0; i < num; ++i) {
      workers.emplace_back(std::make_unique<search::Worker>(i, ttable, limiter, latch, reports));
    }
  }

  void abort() { limiter.set_stop(); }

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

  void run(Position& position) {
    time::Config cfg{
      .nodes = params.nodes,
      .move_time = params.move_time,
      .time_left = params.time[color_idx(position.STM())],
      .increment = params.inc[color_idx(position.STM())],
      .stop = false,
    };

    limiter.set_config(cfg);

    latch.wait();
    limiter.start();
    latch.arm(workers.size());

    for (auto& worker : workers) {
      worker->start(position, params);
    }
  }

  search::ScoredMove datagen_search(Position& position) {
    return workers[0]->datagen_search(params, position);
  }

  void eval(Position& position) { std::println("info score cp {}", workers[0]->eval(position)); }

  void bench(int depth) { workers[0]->bench(depth); }

  search::Worker* get_worker(size_t idx = 0) {
    return (idx < workers.size()) ? workers[idx].get() : nullptr;
  }

  private:
  search::Parameters params;
  tt::Table ttable;
  time::Limiter limiter;
  latch::Latch latch;
  std::vector<search::Report> reports;

  std::vector<std::unique_ptr<search::Worker>> workers;
};
}  // namespace episteme
