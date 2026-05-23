# Prophet

Prophet is a UCI chess engine written in C17.

## Project layout

```text
prophet/
├── src/
│   ├── attacks/   # Attack generation (constant + sliding/magic)
│   ├── board/     # Board state, FEN loading, Zobrist
│   ├── core/      # Core chess types and bitboard helpers
│   ├── engine/    # Engine initialization
│   ├── eval/      # Hand-crafted tapered evaluation
│   ├── movegen/   # Move representation, generation, make/unmake
│   ├── search/    # Search, move ordering, pruning, TT
│   ├── tests/     # Perft harness
│   ├── uci/       # UCI parsing and command loop
│   ├── utils/     # Utility modules (PRNG, etc.)
│   └── main.c     # Program entry point
├── justfile
└── README.md
```

## Rough Roadmap

- [x] Benchmarking speed of search
- [ ] NNUE Training (seperate repo, not yet public)
- [ ] ELO estimation via self-play
- [x] Connection to Lichess API for online play and testing

## Credits

  Influenced by [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page) patterns and references (PeSTO-style evaluation, magic bitboards, Zobrist workflows) and [Stockfish](https://github.com/official-stockfish/Stockfish/).
