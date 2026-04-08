# Prophet

Prophet is a chess engine written from scratch in C (C17). It uses bitboards, incremental state updates, and a modern alpha-beta search stack, and currently runs as a UCI engine.

## Features

### Board representation

- **Bitboards (64-bit)** with Little-Endian Rank-File (LERF) mapping
- **Incremental game-state tracking** (side to move, castling rights, en passant square, clocks)
- **Incremental Zobrist hashing** maintained through make/unmake

### Move generation

- **Magic bitboards** for sliding attacks (rook, bishop, queen)
- **Precomputed attack tables** for leapers (knight, king)
- **Pawn handling** with push/capture masks, promotions, and en passant
- **Legal move generation** via pseudo-legal generation + make/unmake king safety filtering

### Search

- **Iterative deepening** root search
- **Negamax + alpha-beta pruning**
- **Quiescence search** at leaf nodes
- **Transposition table (TT)** with PV/CUT/ALL bounds
- **Move ordering** using:
  - TT move priority
  - MVV-LVA-style capture scoring
  - killer moves
  - history heuristic
- **PVS (Principal Variation Search)**
- **Null-move pruning**
- **LMR-style late move reductions**
- **Time/node/depth/mate/movetime constraints** parsed from UCI `go`

### Evaluation

- **Hand-Crafted Evaluation (HCE)** with material and piece-square tables
- **Tapered evaluation** (middlegame/endgame interpolation, PeSTO-inspired)

### UCI support

Implemented core commands include:

- `uci`, `isready`, `ucinewgame`, `quit`
- `position startpos ...` and `position fen ...`
- `go` with standard limits (`wtime`, `btime`, `winc`, `binc`, `movestogo`, `depth`, `nodes`, `mate`, `movetime`, `infinite`, `ponder`, `searchmoves`)
- `stop`, `ponderhit`
- `setoption name Hash value <mb>` (1–1024)
- `setoption name Clear Hash`
- `debug on|off` and debug-only `printboard`

## Build and run

Prophet uses a `Makefile`.

### Requirements

- macOS/Linux
- a C17 compiler (`cc`/`gcc`/`clang`)

### Build targets

- `make` → builds the engine executable: `./prophet`
- `make perft_test` → builds and runs perft tests: `./perft`
- `make magic` → builds and runs magic generator utility: `./magic_gen`

Build modes:

- `make debug` (`-O0 -g3`)
- `make dev` (`-O3 -g1 -DNDEBUG`, default)
- `make release` (`-O3 -DNDEBUG -flto`)

Clean artifacts:

- `make clean`

### Running

Run the engine (UCI mode):

```bash
./prophet
```

Run the perft suite:

```bash
./perft
```

## Technical notes

### Tapered evaluation

The final score interpolates between middlegame and endgame terms:

$$
\mathrm{Score} = \frac{\mathrm{MG} \times \mathrm{Phase} + \mathrm{EG} \times (24 - \mathrm{Phase})}{24}
$$

## Roadmap

Current major priorities:

- Improve time management quality and tuning
- Continue UCI robustness/compliance hardening
- Add richer evaluation terms (mobility, pawn structure, king safety)
- Keep improving search strength and move-ordering quality

## Project structure

```text
prophet/
├── src/
│   ├── attacks/   # Magic bitboards + attack generation utilities
│   ├── board/     # Board state, FEN, and Zobrist hashing
│   ├── core/      # Core chess types and bitboard helpers
│   ├── engine/    # Engine initialization
│   ├── eval/      # Hand-crafted tapered evaluation
│   ├── movegen/   # Move representation, generation, make/unmake
│   ├── search/    # Search, TT, move ordering, pruning
│   ├── tests/     # Perft and test utilities
│   ├── uci/       # UCI loop and command handling
│   ├── utils/     # Misc helpers (e.g., PRNG)
│   └── main.c     # Entry point (starts UCI loop)
├── Makefile
└── README.md
```

## Credits

Inspired by resources from [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page):

- PeSTO-style tapered evaluation ideas
- Magic bitboard techniques for sliding attacks
- RKISS-style PRNG concepts for key generation
