#include "engine.hpp"

#include <memory>

#include "search/search.hpp"

namespace episteme {

void Engine::run(Position& position) {
  time::Config cfg{
    .nodes = params.nodes,
    .move_time = params.move_time,
    .time_left = params.time[color_idx(position.STM())],
    .increment = params.inc[color_idx(position.STM())],
    .stop = false,
  };

  search::Report last_report;
  int32_t last_score = 0;

  reset_nodes();

  limiter.set_config(cfg);
  limiter.start();

  for (int depth = 1; depth <= params.depth; depth++) {
    search::Parameters iter_params = params;
    iter_params.depth = depth;

    reset_seldepth();

    search::Report report = workers[0]->run(last_score, iter_params, position, false);
    if (workers[0]->stopped()) break;

    last_report = report;
    last_score = report.score;

    report.hashfull = ttable.hashfull();

    bool is_mate = std::abs(report.score) >= search::MATE - search::MAX_SEARCH_PLY;
    int32_t display_score =
      is_mate ? (1 + search::MATE - std::abs(report.score)) / 2 : report.score;
    int64_t nps = report.time > 0 ? 1000 * report.nodes / report.time : 0;

    std::string pv;
    for (size_t i = 0; i < report.line.length; ++i) {
      pv += report.line.moves[i].to_string() + ' ';
    }
    std::println(
      "info depth {} time {} nodes {} nps {} hashfull {} score {} {} pv {}",
      report.depth,
      report.time,
      report.nodes,
      nps,
      report.hashfull,
      is_mate ? "mate" : "cp",
      display_score,
      pv
    );

    if (
      limiter.time_approaching(report.line.moves[0], workers[0]->node_count()) ||
      limiter.time_exceeded() || limiter.abort()
    )
      break;
  }

  Move best = last_report.line.moves[0];
  std::println("bestmove {}", best.to_string());
}

search::ScoredMove Engine::datagen_search(Position& position) {
  time::Config cfg{
    .nodes = params.nodes,
    .soft_nodes = params.soft_nodes,
  };

  search::Report last_report;
  int32_t last_score = 0;

  reset_nodes();

  limiter.set_config(cfg);
  limiter.start();

  for (int depth = 1; depth <= 10; depth++) {
    search::Parameters iter_params = params;
    iter_params.depth = depth;

    search::Report report = workers[0]->run(last_score, iter_params, position, true);
    if (workers[0]->stopped()) break;

    last_report = report;
    last_score = report.score;

    if (limiter.nodes_approaching(workers[0]->node_count())) break;
  }

  search::ScoredMove best{.move = last_report.line.moves[0], .score = last_report.score};

  return best;
}

void Engine::eval(Position& position) {
  int32_t eval_cp = workers[0]->eval(position);
  std::println("info score cp {}", eval_cp);
}

void Engine::bench(int depth) { workers[0]->bench(depth); }
}  // namespace episteme
