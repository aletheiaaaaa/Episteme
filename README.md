# Episteme Chess Engine

A high-performance chess engine with an hopeful rating of ~3500 Elo, capable of outperforming even the strongest humans.

## Overview

Episteme is a modern chess engine designed for strength, efficiency, and competitive play. Named after the Greek concept of knowledge and understanding, Episteme combines advanced search algorithms with sophisticated position evaluation to achieve master-level play.

## Features

- **High Performance**: ~3500 Elo estimated strength
- **UCI Compatible**: Full support for the Universal Chess Interface protocol
- **Advanced Search**: Deep alpha-beta pruning with modern enhancements
- **Sophisticated Evaluation**: Multi-layered position assessment
- **Cross-Platform**: Runs on Windows, Linux, and macOS
- **Tournament Ready**: Suitable for engine tournaments and competitions

## Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/aletheiaaaaa/episteme.git
cd episteme

# Build the engine
make
```

### Basic Usage

```bash
# Run in UCI mode
./episteme

# Example UCI commands
uci
isready
position startpos moves e2e4 e7e5
go depth 15
```

## Technical Details

### Search Algorithm
- Alpha-beta pruning with aspiration windows
- Iterative deepening
- Transposition table
- Move ordering optimizations
- Selective extensions and reductions

### Evaluation Function
- Neural network evaluation trained from scratch over ~700 million positions

## Version History

### v1.0.0
- Initial release
- Core engine functionality
- UCI protocol support
- Basic evaluation and search

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Support

If you find Episteme useful, please consider:
- ⭐ Starring the repository
- 🐛 Reporting bugs and issues
- 💡 Suggesting improvements
- 🤝 Contributing code or documentation

---

*"Knowledge is the only good, and ignorance the only evil." - Socrates*
