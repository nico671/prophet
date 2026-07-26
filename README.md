# Prophet

Prophet is an experimental UCI chess engine written in C17. It is a personal,
in-development project rather than a stable release.

## Current state

The engine currently has:

- bitboard position representation, FEN parsing, and Zobrist hashing;
- legal move generation, make/unmake support, and a built-in perft suite;
- iterative-deepening alpha-beta search with quiescence search, principal
  variation search, late-move reductions, null-move pruning, move ordering, and
  a transposition table;
- time management, pondering, and the main UCI search limits;
- UCI options for resizing and clearing the hash table; and
- both a PeSTO-style classical evaluator and an early custom NNUE
  implementation.

Evaluation is currently in transition. Search uses the experimental NNUE, whose
weights are loaded from a developer-local path and are not distributed with the
repository. Replacing that implementation and returning temporarily to the
classical evaluator is the next major engine change.

## Build

Prophet currently targets a POSIX environment and uses GCC, pthreads, and
[`just`](https://github.com/casey/just).

```sh
just build
```

This creates a development build at:

```text
artifacts/<current-branch>/prophet-dev
```

Other build modes are available:

```sh
just build debug
just build release
```

Because the NNUE weights are not included, starting the engine currently prints
a missing-weights warning. The binary still starts, but its search strength is
not representative of a usable engine until the evaluation work is completed.

## Use

Run the binary directly and communicate with it using UCI commands, or configure
its path in a UCI-compatible chess GUI.

```text
uci
isready
position startpos
go depth 6
quit
```

Supported engine options are `Hash` (1-1024 MB) and `Clear Hash`. Prophet also
provides non-standard `perft` and `printboard` commands for development.

## Validation and formatting

Run the built-in move-generation test suite with:

```sh
just perft
```

Format the C sources with:

```sh
just format
```

## Project layout

```text
src/
├── chess/       Chess types, board state, move generation, hashing, and perft
│   ├── board/
│   ├── core/
│   ├── movegen/
│   ├── perft/
│   └── utils/
└── engine/      Search, evaluation, transposition table, threading, and UCI
    ├── eval/
    ├── search/
    ├── tt/
    ├── uci/
    └── main.c
scripts/
└── bench.py     Early benchmark script
refs/                Reference material and table-generation programs
```

## Rough Roadmap

Note that these are not in order, moreso just a braindump of things to do.

1. Implement a proper NNUE design and rebuild the evaluation from scratch.
2. Implement a data generation mode for training the NNUE.
3. Implement a benchmarking framework for performance testing.
4. Implement a SPRT testing framework for performance testing.
5. Implement multipv support.
6. Implement a proper opening book and endgame tablebase support.
7. Implement multi-threaded search and parallelization.

## Credits

Prophet is influenced by the [Chess Programming
Wiki](https://www.chessprogramming.org/Main_Page), including its material on
PeSTO-style evaluation, magic bitboards, and Zobrist hashing, and by
[Stockfish](https://github.com/official-stockfish/Stockfish).
