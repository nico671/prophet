"""Exercise the deterministic self-play generator on a small fixture."""

from __future__ import annotations

import argparse
import hashlib
import re
import subprocess
import tempfile
from pathlib import Path


GAME_RE = re.compile(
    r"^game (?P<index>\d+) opening (?P<opening>\d+) result (?P<result>\w+) "
    r"terminal (?P<terminal>\w+) moves (?P<moves>\d+) samples (?P<samples>\d+)$"
)
SAMPLE_RE = re.compile(
    r"^sample (?P<ply>\d+) (?P<side>[wb]) (?P<score>-?\d+) "
    r"(?P<result>0\.0|0\.5|1\.0) (?P<depth>\d+) (?P<root_lines>\d+) "
    r"(?P<fen>\S+ \S+ \S+ \S+ \S+ \S+)$"
)
REJECT_RE = re.compile(r"^reject (terminal|in_check|mate_score|partial_search) [1-9]\d*$")


def config_text(openings: Path, opening_count: int, workers: int, root_seed: int) -> str:
    digest = hashlib.sha256(openings.read_bytes()).hexdigest()
    return f"""\
version = 1
openings = \"{openings}\"
openings_sha256 = \"{digest}\"
opening_first = 0
opening_count = {opening_count}
games = 8
workers = {workers}
root_seed = {root_seed}
worker_seed_base = 5678
nodes_per_move = 2000
hash_mb = 1
clear_hash_per_game = true
early_ply_limit = 4
early_multipv = 2
sample_start_ply = 0
sample_interval = 1
sample_offset_seed = 99
max_game_ply = 80
shard_game_limit = 100
color_assignment = alternate
"""


def run_datagen(engine: Path, config: Path, output: Path, cwd: Path) -> list[Path]:
    completed = subprocess.run(
        [str(engine), "datagen", "--config", str(config), "--output", str(output)],
        cwd=cwd,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise AssertionError(
            f"datagen failed with {completed.returncode}\n"
            f"stdout: {completed.stdout}\nstderr: {completed.stderr}"
        )
    files = sorted(output.parent.glob(output.name + ".worker-*.part-*.games"))
    if not files:
        raise AssertionError("datagen produced no trace files")
    return files


def audit_traces(validator: Path, files: list[Path]) -> None:
    completed = subprocess.run(
        [str(validator), *(str(path) for path in files)],
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise AssertionError(
            f"trace audit failed with {completed.returncode}\n"
            f"stdout: {completed.stdout}\nstderr: {completed.stderr}"
        )


def read_games(files: list[Path]) -> list[tuple[int, str, str, str]]:
    games: list[tuple[int, str, str, str]] = []
    game_indices: set[int] = set()
    for path in files:
        lines = path.read_text(encoding="utf-8").splitlines()
        if len(lines) < 5 or lines[:1] != ["prophet-datagen-v1"]:
            raise AssertionError(f"invalid trace header in {path}")
        if not re.fullmatch(r"worker \d+", lines[1]):
            raise AssertionError(f"invalid worker header in {path}")
        if not re.fullmatch(r"worker_seed \d+", lines[2]):
            raise AssertionError(f"invalid worker seed header in {path}")
        if not re.fullmatch(r"openings_sha256 [0-9a-fA-F]{64}", lines[3]):
            raise AssertionError(f"invalid openings hash header in {path}")
        if lines[4] != "":
            raise AssertionError(f"invalid trace header separator in {path}")

        index = 5
        while index < len(lines):
            if lines[index] == "":
                index += 1
                continue

            match = GAME_RE.fullmatch(lines[index])
            if not match:
                raise AssertionError(f"unexpected trace line in {path}: {lines[index]}")
            game_index = int(match.group("index"))
            declared_moves = int(match.group("moves"))
            declared_samples = int(match.group("samples"))
            if index + 2 >= len(lines) or not lines[index + 1].startswith("opening_fen "):
                raise AssertionError(f"missing opening FEN in {path}")
            opening_fen = lines[index + 1][len("opening_fen ") :]
            if not opening_fen:
                raise AssertionError(f"empty opening FEN in {path}")

            moves_line = lines[index + 2]
            if not re.fullmatch(r"moves(?: [a-h][1-8][a-h][1-8][nbrq]?)*", moves_line):
                raise AssertionError(f"invalid moves line in {path}: {moves_line}")
            move_text = moves_line.removeprefix("moves").strip()
            actual_moves = [] if not move_text else move_text.split()
            if len(actual_moves) != declared_moves:
                raise AssertionError(f"move count mismatch in {path}")

            sample_count = 0
            index += 3
            while index < len(lines) and lines[index] != "endgame":
                sample_line = lines[index]
                if sample_line.startswith("sample "):
                    if not SAMPLE_RE.fullmatch(sample_line):
                        raise AssertionError(f"invalid sample line in {path}: {sample_line}")
                    sample_count += 1
                elif sample_line.startswith("reject "):
                    if not REJECT_RE.fullmatch(sample_line):
                        raise AssertionError(f"invalid rejection line in {path}: {sample_line}")
                else:
                    raise AssertionError(f"unexpected game line in {path}: {sample_line}")
                index += 1

            if index >= len(lines) or lines[index] != "endgame":
                raise AssertionError(f"missing endgame marker in {path}")
            if sample_count != declared_samples:
                raise AssertionError(f"sample count mismatch in {path}")
            if game_index in game_indices:
                raise AssertionError(f"duplicate game index in {path}: {game_index}")
            game_indices.add(game_index)
            games.append((game_index, opening_fen, move_text, match.group("result")))
            index += 1
            if index < len(lines) and lines[index] != "":
                raise AssertionError(f"missing trace block separator in {path}")
            while index < len(lines) and lines[index] == "":
                index += 1
    if not games:
        raise AssertionError("trace contains no complete games")
    return games


def replay_games(engine: Path, games: list[tuple[int, str, str, str]]) -> None:
    commands = ["uci", "isready"]
    for _game_index, opening_fen, moves, _result in games:
        commands.append(f"position fen {opening_fen} moves{moves}")
    commands.append("quit")
    completed = subprocess.run(
        [str(engine)], input="\n".join(commands) + "\n", text=True,
        capture_output=True, check=False,
    )
    if completed.returncode != 0 or "info string error:" in completed.stdout:
        raise AssertionError(
            f"recorded game replay failed\nstdout: {completed.stdout}\nstderr: {completed.stderr}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--validator", type=Path, required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    openings = repo / "testdata/datagen/openings.epd"

    with tempfile.TemporaryDirectory(prefix="prophet-datagen-") as temporary:
        root = Path(temporary)
        config = root / "datagen.conf"
        config.write_text(config_text(openings, 3, 1, 1234), encoding="utf-8")
        first = run_datagen(args.engine, config, root / "first", repo)
        first_bytes = b"".join(path.read_bytes() for path in first)
        first_hash = hashlib.sha256(first_bytes).hexdigest()
        second = run_datagen(args.engine, config, root / "second", repo)
        second_bytes = b"".join(path.read_bytes() for path in second)
        assert first_bytes == second_bytes, "same seed did not reproduce the trace"
        assert first_hash == hashlib.sha256(second_bytes).hexdigest()

        games = read_games(first)
        audit_traces(args.validator, first)
        replay_games(args.engine, games)

        config.write_text(config_text(openings, 3, 1, 9876), encoding="utf-8")
        different = run_datagen(args.engine, config, root / "different", repo)
        different_bytes = b"".join(path.read_bytes() for path in different)
        assert different_bytes != first_bytes, "different seed did not change the trace"
        different_games = read_games(different)
        audit_traces(args.validator, different)
        assert len(different_games) == len(games), "different seed changed the emitted game count"
        assert [game[1] for game in different_games] != [game[1] for game in games], \
            "different seed did not change any opening"
        assert [game[2] for game in different_games] != [game[2] for game in games], \
            "different seed did not change any move sequence"

        config.write_text(config_text(openings, 3, 2, 1234), encoding="utf-8")
        workers = run_datagen(args.engine, config, root / "workers", repo)
        assert len(workers) == 2, f"expected two worker files, got {workers}"
        worker_games = read_games(workers)
        audit_traces(args.validator, workers)
        assert len(worker_games) == len(games), "worker sharding changed the emitted game count"
        assert {game[0]: game[1:] for game in worker_games} == {
            game[0]: game[1:] for game in games
        }, "worker sharding changed emitted game contents"

    print("Datagen smoke test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
