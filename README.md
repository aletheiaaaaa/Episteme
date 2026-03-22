# Episteme

Episteme is a chess engine that I wrote over the span of a year to learn how to code C++ while building something technical. It currently features a small (~512 hidden layer neurons) efficiently updateable neural network to evaluate position, and searches moves using a combination of most well known heuristics such as late move reduction and null move pruning. I estimate it to be around ~3400 elo in strength, vastly stronger than even the strongest humans (though endgames are a known weakness of this engine) but mid-tier in the realm of top chess engines. I hope you enjoy!

## Quick Start

```bash
# Clone the repository
git clone https://github.com/aletheiaaaaa/Episteme.git
cd Episteme

# Build the engine
make
```

## Version History

### v1.0.0
- Initial release
- UCI protocol support
- Basic 512HL smallnet NNUE
- Search w/ 90% of furypasta

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

---

*"Knowledge is the only good, and ignorance the only evil." - Socrates*
