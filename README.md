# Prophet

Prophet is an experimental UCI chess engine written in C17. It is a personal,
in-development project, not a stable release.

## Current state

The engine currently has:

- bitboard representation, FEN parsing, and Zobrist hashing;
- legal move generation, make/unmake support, and a built-in perft suite;
- iterative-deepening alpha-beta search with quiescence search, principal
  variation search, late-move reductions, null-move pruning, move ordering, and
  a transposition table;
- time controls, pondering, `searchmoves`, and depth, node, mate, and movetime
  search limits;
- UCI options to resize (1-1024 MB) and clear the hash table; and
- a tapered PeSTO-style hand-crafted evaluator.

## Build

Prophet targets a POSIX environment and uses GCC, pthreads, and
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

Supported engine options are `Hash` (1-1024 MB), `Clear Hash`, and `MultiPV`
(1-256). Prophet also provides non-standard `perft` and `printboard` commands
for development.
`printboard` requires `debug on`.

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
├── engine/      Search, evaluation, transposition table, threading, and UCI
    ├── eval/
    ├── search/
    ├── tt/
    ├── uci/
    └── engine.c
└── main.c        Program entry point
refs/                Reference material and table-generation programs
```

## Credits

Prophet is influenced by the [Chess Programming
Wiki](https://www.chessprogramming.org/Main_Page), including its material on
PeSTO-style evaluation, magic bitboards, and Zobrist hashing, and by
[Stockfish](https://github.com/official-stockfish/Stockfish).

### NNUE work-package checks

Run `just check` for the feature contract, direct SearchResult/UCI comparison,
and deterministic data generation checks, including sanitizer builds.
For a separate API/UCI check, run `just search-result debug /absolute/path/to/prophet`.
Data generation requires a new output prefix. It rejects existing shard files.
A shard is complete only when its final `.binpack.manifest.json` exists and passes
validation in `prophet-nnue`.
