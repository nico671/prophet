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

## Build & run (justfile-based)

This repo is driven by `just` recipes (not a Makefile).

### Requirements

- macOS or Linux
- C17 compiler (`cc`, `clang`, or `gcc`)
- [`just`](https://github.com/casey/just)

### Common commands

- `just all [build]` — build engine (`build`: `debug`, `dev` (default), `release`)
- `just run [build]` — build and run `./prophet`
- `just perft_test [build]` — build and run perft harness (`./perft`)
- `just magic [build]` — build and run magic-number generator (`./magic_gen`)
- `just debug` — clean + debug build
- `just dev` — clean + dev build
- `just release-build` — clean + release build
- `just clean` — remove build outputs

### Artifacts and versioning

- `just version` resolves version via `git describe --tags --always --dirty`.
- `just archive [build]`, `just archive-perft [build]`, `just archive-magic [build]` archive binaries under `artifacts/<version>/`.
- `just list-artifacts` lists all archived binaries.

## Development strategy

Branching model:

- `main` = stable releases only
- `dev` = integration branch
- `feature/*` = short-lived branches for focused work

Helper recipes exist for lifecycle operations:

- `just start-feature <name> [base]`
- `just publish-feature <name>`
- `just finish-feature <name> [delete_local]`
- `just release <ver>`

See `docs/workflow.md` for the full branch/release flow.

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

## Technical note: tapered evaluation

The interpolation form is:

$$
\mathrm{Score} = \frac{\mathrm{MG} \times \mathrm{Phase} + \mathrm{EG} \times (24 - \mathrm{Phase})}{24}
$$

## Near-term focus

- Strength/tuning work in search and move ordering
- Evaluation improvements (mobility, pawn/king structure, etc.)
- UCI robustness and testing coverage

## Credits

Influenced by [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page) patterns and references (PeSTO-style evaluation, magic bitboards, Zobrist workflows).
