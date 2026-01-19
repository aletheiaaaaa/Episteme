#include "engine/uci/uci.h"
#include "engine/uci/display.h"

#include <iostream>

using namespace episteme;

int main(int argc, char *argv[]) {
    hash::init();
    tunable::init_lmr_table();

    search::Config cfg;
    search::Engine engine(cfg);

    if (argc > 1) {
        std::string cmd;
        for (int i = 1; i < argc; ++i) {
            cmd += argv[i];
            if (i < argc - 1) cmd += ' ';
        }
        uci::parse(cmd, cfg, engine);

    } else {
        uci::set_pretty(true);

        std::string line;
        while (std::getline(std::cin, line)) {
            uci::parse(line, cfg, engine);
        }
    }

    return 0;
}

