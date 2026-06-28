#pragma once

#include <string>
#include <thread>

#include "../engine.hpp"
#include "../search/search.hpp"

namespace episteme::uci {

class Listener {
public:
    Listener(std::istream& stream, Engine& engine) : inputs(stream), engine(engine) {}

    void start() { thread = std::thread(&Listener::listen, this); }
    void join() {
        if (thread.joinable()) thread.join();
    }
    ~Listener() { join(); }

private:
    std::thread thread;
    std::istream& inputs;
    Engine& engine;
    search::Config cfg;

    void listen();

    bool parse(const std::string& cmd);

    void uci();
    void setoption(const std::string& args);
    void isready();
    void stop();
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
