# Prophet

Prophet is a UCI chess engine written in C17. It uses bitboards, incremental board state, and a modern alpha-beta search pipeline with iterative deepening.

## Current engine status

The engine currently includes:

- **Bitboard board model** with incremental make/unmake and Zobrist hashing.
- **Move generation** with precomputed leaper attacks + magic-bitboard sliding attacks.
- **Search stack** with iterative deepening, negamax + alpha-beta, quiescence, transposition table, PVS, null-move pruning, and LMR-style reductions.
- **Move ordering** using TT move, capture scoring, killer moves, and history heuristic.
- **Evaluation** via tapered PeSTO-style hand-crafted evaluation.
- **UCI interface** with practical options/limits for GUI play and testing.

## UCI support

Implemented commands/options:

- Commands: `uci`, `isready`, `ucinewgame`, `position`, `go`, `stop`, `ponderhit`, `debug`, `quit`
- `position startpos ...` and `position fen ...`
- `go` tokens: `searchmoves`, `ponder`, `wtime`, `btime`, `winc`, `binc`, `movestogo`, `depth`, `nodes`, `mate`, `movetime`, `infinite`
- Options:
  - `setoption name Hash value <mb>` (clamped to `1..1024`)
  - `setoption name Clear Hash`
- Debug helper: `printboard` (when `debug on`)

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
│   ├── tests/     # Perft harness and test helpers
│   ├── uci/       # UCI parsing and command loop
│   ├── utils/     # Utility modules (PRNG, etc.)
│   └── main.c     # Program entry point
├── docs/
│   └── workflow.md
├── justfile
└── README.md
```

## Rough Roadmap

- [ ] Benchmarking speed of search
- [ ] NNUE Training (seperate repo, not yet public)
- [ ] ELO estimation via self-play
- [ ] Connection to Lichess API for online play and testing

## Credits

Influenced by [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page) patterns and references (PeSTO-style evaluation, magic bitboards, Zobrist workflows).
