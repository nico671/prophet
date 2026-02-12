# Prophet

Prophet is a chess engine written from scratch in C (C17). It uses a bitboard representation and fast move generation techniques. Currently, it serves as a robust foundation with a working search, evaluation, and perft testing suite—ready for further expansion into a fully UCI-compatible engine.

## Features

### Board representation

- **Bitboards (64-bit)** with Little-Endian Rank-File (LERF) mapping
- **Incremental game-state tracking** (side to move, castling rights, en passant square, etc.)
- **Zobrist hashing** maintained incrementally during make/unmake

### Move generation

- **Magic bitboards** for sliding piece attacks (rook, bishop, queen)
- **Lookup tables** for leapers (knight, king)
- **Pawns** via bitwise shifts for pushes and bitwise attack masks (including en passant)
- **Legality filtering** via pseudo-legal generation + make/unmake verification

### Search

- **Negamax** (simplified minimax for zero-sum games)
- **Alpha-beta pruning** to cut irrelevant branches
- **Iterative deepening** (framework in place)
- **Quiescence search**: planned (see roadmap)

### Evaluation

- **Hand-Crafted Evaluation (HCE)** using material weights + piece-square tables (PST)
- **Tapered evaluation**: smoothly interpolates between middlegame and endgame using PeSTO-style tables

### Testing

- **Perft suite** to validate move generation correctness on standard positions (initial, Kiwipete, etc.)

## Build & usage

Prophet uses a `Makefile` for compilation.

### Requirements

- macOS/Linux
- `gcc` (C17)

### Compilation

Build the main engine executable:

```bash
make
```

Build the Perft test suite:

```bash
make perft_test
```

Build the magic-number generator (utility):

```bash
make magic
```

### Running

Run the engine:

```bash
./chess
```

At the moment, `src/main.c` runs a hardcoded search on a set of positions.

Run the perft tests:

```bash
./perft
```

## Technical implementation details

### 1) Board representation (`src/board`)

The engine uses bitboards (64-bit integers) to represent piece placement and occupancy.

- **`CBoard` struct**: maintains bitboards for piece types/colors and game-state metadata.
- **Zobrist hashing**: an incrementally updated key used to identify positions (and to support a future transposition table).

### 2) Move generation (`src/movegen`, `src/attacks`)

- **Sliding pieces**: magic bitboards provide $O(1)$ attack lookup for rooks and bishops (queens combine both), based on occupancy.
- **Leapers**: king/knight attacks come from precomputed lookup tables.
- **Pawns**:
  - pushes via bitwise shifts
  - captures via precomputed/derived attack masks
  - en passant handled in move generation + make/unmake

The generator produces **pseudo-legal** moves; the search filters illegal moves by making/unmaking and verifying king safety.

### 3) Evaluation (`src/hcevaluation`)

Prophet uses a tapered evaluation inspired by PeSTO.

- **Game phase** is computed from remaining material.
- The final score interpolates between middlegame and endgame terms:

$$
  ext{Score} = \frac{\text{MG} \times \text{Phase} + \text{EG} \times (24 - \text{Phase})}{24}
$$

### 4) Search (`src/search`)

The current search is a standard negamax with alpha-beta pruning.

- Leaf nodes stop at depth 0 (no quiescence yet), which can lead to the *horizon effect* in sharp tactical positions.

## Roadmap

Prophet is functional, but it’s missing several standard features needed for competitive play and GUI integration.

### High priority

- [ ] **UCI protocol**: implement the UCI loop so Prophet can run in GUIs (Arena, CuteChess, etc.). See `engine-interface.txt`.
- [ ] **Time management**: allocate time per move from `wtime`, `btime`, `movestogo`, increments, etc.
- [ ] **Quiescence search**: extend leaf evaluation to resolve capture sequences and reduce tactical blunders.

### Search enhancements

- [ ] **Transposition table (TT)**: cache results using the existing Zobrist keys.
- [ ] **Move ordering**:
  - [ ] MVV-LVA (Most Valuable Victim / Least Valuable Attacker)
  - [ ] Killer heuristic
  - [ ] History heuristic
- [ ] **Selectivity**:
  - [ ] Null-move pruning
  - [ ] Late move reductions (LMR)
  - [ ] Principal variation search (PVS)

### Evaluation improvements

- [ ] Mobility
- [ ] Pawn structure (backward, doubled, isolated pawns)
- [ ] King safety (more advanced patterns)

## Project structure

```text
prophet/
├── src/
│   ├── attacks/      # Magic bitboards and precomputed attack tables
│   ├── board/        # Board representation, bitboards, Zobrist hashing
│   ├── core/         # Typedefs and bit manipulation helpers
│   ├── engine/       # Engine initialization / plumbing
│   ├── hcevaluation/ # Static evaluation (PeSTO-style tapered eval)
│   ├── movegen/      # Move generation + make/unmake
│   ├── search/       # Negamax + alpha-beta search
│   ├── tests/        # Perft testing suite
│   ├── utils/        # PRNG and helpers
│   └── main.c        # Entry point
├── Makefile          # Build configuration
└── README.md         # Documentation
```

## Credits
All from [The Chess Programming Wiki](https://www.chessprogramming.org/Main_Page)
- **PeSTO evaluation**: piece-square tables adapted from the PeSTO-style tapered evaluation approach.
- **Magic bitboards**: standard magic bitboard techniques for sliding attacks.
- **PRNG**: Bob Jenkins’ RKISS-style PRNG for generating Zobrist keys.


