# Prophet

Prophet is an experimental UCI chess engine written in C17. It is a personal,
in-development project, not a stable release.

Prophet is licensed under the GNU General Public License version 3. See
[LICENSE](LICENSE). Third-party material and provenance are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

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

Supported engine options are `Hash` (1-1024 MB), `Clear Hash`, `MultiPV`
(1-256), `EvalFile`, and `UseNNUE`. Prophet also provides non-standard `perft` and `printboard` commands
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

## NNUE releases and validation

NNUE release assets include the engine, PNUE network, export manifest,
validation manifest, and `SHA256SUMS`. Download the assets into one directory.
On macOS, run `shasum -a 256 -c SHA256SUMS` there to verify the files.
The release workflow rebuilds the validated source on macOS ARM64; the download
checksum identifies that build, while the validation manifest identifies the
locally tested binaries.

NNUE is disabled at startup. In your chess GUI, set `EvalFile` to the absolute
path of `prophet-v1.0.0.pnue`, then set `UseNNUE` to true. For a manual UCI session:

```text
uci
setoption name EvalFile value /absolute/path/to/prophet-v1.0.0.pnue
setoption name UseNNUE value true
isready
position startpos
go depth 6
```

Wait for the search to finish before sending `quit`. Require a successful
`EvalFile loaded` message and `UseNNUE true`. Loading a network alone does not
enable it.

`config/validation.json` selects the candidate network and pins its SHA-256.
Benchmarks and matches enable that network only for the candidate. Both engines
use 64 MB hash and MultiPV 1. Benchmark search timing excludes startup and network
loading; startup time is reported separately. Standard samples alternate engine
order. Qsearch stress results remain diagnostic and separate from the speed gate.

For release validation against v0.2.0, commit the complete candidate, including
its network and export manifest, then run:

```fish
python3 -u scripts/validation.py validate 1.0.0 --baseline v0.2.0
```

This runs strict checks, the benchmark, and SPRT. The manifest records the source,
network, export manifest, configuration, binary hashes, and match evidence.
SPRT uses the configured model explicitly; `normalized` means normalized Elo.
A failed speed gate needs an explicit, recorded `--speed-override`; it never
replaces correctness checks or an accepted NNUE SPRT result.

After the exact validated commit is on clean, current `main`, publish with:

```fish
python3 -u scripts/validation.py release 1.0.0 --baseline v0.2.0 --manifest /absolute/path/to/manifest.json
```

This creates and pushes the release tag. Add `--speed-override` only when the
validation run records the accepted speed override. A changed commit requires
new validation. Standalone `just bench v0.2.0` and `just sprt v0.2.0` are diagnostic
runs and do not create a release manifest.
