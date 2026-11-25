#!/usr/bin/env python3
"""Build and compare Prophet's forced ARM64 NNUE backends."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import platform
import re
import shlex
import socket
import statistics
import subprocess
import sys
import tempfile
import time
from datetime import UTC, datetime
from itertools import permutations
from pathlib import Path
from typing import Any

try:
    from .uci_client import UciEngine, UciError
except ImportError:
    from uci_client import UciEngine, UciError

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = ROOT / "config" / "validation.json"
BACKENDS = ("scalar", "neon", "dotprod")
BACKEND_ORDERS = tuple(permutations(BACKENDS))
BACKEND_FLAGS = {
    "scalar": ["-DPROPHET_NNUE_FORCE_SCALAR"],
    "neon": ["-DPROPHET_NNUE_FORCE_NEON", "-march=armv8-a"],
    "dotprod": ["-DPROPHET_NNUE_FORCE_DOTPROD", "-march=armv8.2-a+dotprod"],
}
INFO = re.compile(r"^info depth (\d+) .*\bnodes (\d+) time (\d+)\b")
BESTMOVE = re.compile(r"bestmove ([a-h][1-8][a-h][1-8][qrbn]?)")
RESULT_FIELDS = (
    "run_id", "sample", "order", "backend", "position_index", "position_id", "depth",
    "nodes", "engine_elapsed_ms", "wall_elapsed_ms", "engine_nps", "wall_nps", "bestmove",
    "status", "error",
)


def now_utc() -> str:
    return datetime.now(UTC).isoformat(timespec="seconds")


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for block in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def command_output(args: list[str], cwd: Path = ROOT) -> str:
    result = subprocess.run(args, cwd=cwd, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, check=False)
    return result.stdout.strip()


def backend_order(sample: int) -> tuple[str, ...]:
    return BACKEND_ORDERS[(sample - 1) % len(BACKEND_ORDERS)]


def load_positions(config: dict[str, Any], limit: int | None) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    benchmark = config["benchmark"]
    profile = benchmark["profiles"]["standard"]
    excluded = set(profile.get("exclude_ids", []))
    positions = [item for item in benchmark["positions"] if item["id"] not in excluded]
    if limit is not None:
        positions = positions[:limit]
    if not positions:
        raise ValueError("the standard benchmark profile has no positions")
    return profile, positions


def source_files() -> list[str]:
    excluded = {"magic_gen.c", "perft_test.c"}
    return [str(path.relative_to(ROOT)) for path in sorted((ROOT / "src").rglob("*.c"))
            if path.name not in excluded]


def build_backends(config: dict[str, Any], build_dir: Path, build_log: Path,
                   git_head: str) -> tuple[dict[str, Path], dict[str, list[str]], str]:
    release = config["release"]
    compiler = os.environ.get("CC", release["compiler"])
    common_flags = shlex.split(release["cflags"]) + ["-I", "src"]
    if git_head:
        common_flags.append(f'-DPROPHET_GIT_COMMIT="{git_head}"')
    sources = source_files()
    binaries: dict[str, Path] = {}
    commands: dict[str, list[str]] = {}
    with build_log.open("w") as log:
        for backend in BACKENDS:
            binary = build_dir / f"prophet-{backend}"
            command = ([compiler] + common_flags + BACKEND_FLAGS[backend] + sources
                       + ["-o", str(binary)])
            commands[backend] = command
            print(f"[nnue-bench] building {backend}", flush=True)
            log.write(f"$ {shlex.join(command)}\n")
            log.flush()
            result = subprocess.run(command, cwd=ROOT, text=True, stdout=log,
                                    stderr=subprocess.STDOUT, check=False)
            if result.returncode != 0:
                raise RuntimeError(f"{backend} build failed; see {build_log}")
            binaries[backend] = binary
    return binaries, commands, compiler


def configure_engine(engine: UciEngine, eval_file: Path, hash_mb: int) -> None:
    engine.initialize()
    engine.send(f"setoption name EvalFile value {eval_file}")
    engine.expect(lambda line: line.startswith("info string EvalFile loaded architecture "),
                  "successful EvalFile load")
    engine.send("setoption name UseNNUE value true")
    engine.expect(lambda line: line == "info string UseNNUE true", "UseNNUE confirmation")
    engine.send(f"setoption name Hash value {hash_mb}")
    engine.expect(lambda line: line == f"info string Hash set to {hash_mb} MB",
                  "Hash confirmation")
    engine.send("isready")
    engine.expect(lambda line: line == "readyok", "readyok")


def clear_engine(engine: UciEngine) -> None:
    engine.send("setoption name Clear Hash")
    engine.expect(lambda line: line == "info string Hash cleared", "Hash cleared")
    engine.send("ucinewgame")


def run_position(engine: UciEngine, position: dict[str, Any], depth: int,
                 timeout: float) -> dict[str, Any]:
    clear_engine(engine)
    command = f"position fen {position['fen']}"
    if position.get("moves"):
        command += f" moves {position['moves']}"
    engine.send(command)
    started = time.monotonic()
    engine.send(f"go depth {depth}")
    bestmove_line, lines = engine.expect(
        lambda line: line.startswith("bestmove "), "bestmove", timeout
    )
    wall_elapsed_ms = max(1, round((time.monotonic() - started) * 1000))
    bestmove_match = BESTMOVE.fullmatch(bestmove_line)
    if not bestmove_match:
        raise UciError(f"malformed {bestmove_line!r}")
    exact = []
    for line in lines:
        match = INFO.match(line)
        if match and int(match.group(1)) == depth:
            exact.append((int(match.group(2)), int(match.group(3))))
    if not exact:
        raise UciError(f"position {position['id']} did not complete depth {depth}")
    nodes, engine_elapsed_ms = exact[-1]
    if nodes <= 0:
        raise UciError(f"position {position['id']} returned invalid node count")
    return {
        "nodes": nodes,
        "engine_elapsed_ms": engine_elapsed_ms,
        "wall_elapsed_ms": wall_elapsed_ms,
        "engine_nps": nodes * 1000 / max(1, engine_elapsed_ms),
        "wall_nps": nodes * 1000 / wall_elapsed_ms,
        "bestmove": bestmove_match.group(1),
    }


def append_result(path: Path, row: dict[str, Any]) -> None:
    exists = path.exists()
    with path.open("a", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=RESULT_FIELDS)
        if not exists:
            writer.writeheader()
        writer.writerow(row)
        file.flush()


def run_backend_sample(run_id: str, sample: int, order: int, backend: str, binary: Path,
                       eval_file: Path, positions: list[dict[str, Any]], depth: int,
                       warmup_depth: int, timeout: float, hash_mb: int,
                       results_path: Path) -> list[dict[str, Any]]:
    engine = UciEngine(binary, timeout)
    rows: list[dict[str, Any]] = []
    try:
        configure_engine(engine, eval_file, hash_mb)
        run_position(engine, positions[0], min(depth, warmup_depth), timeout)
        for index, position in enumerate(positions, 1):
            label = (f"sample {sample} {backend} position {index}/{len(positions)} "
                     f"(id {position['id']})")
            print(f"[nnue-bench] {label}", flush=True)
            try:
                measurement = run_position(engine, position, depth, timeout)
                row = {
                    "run_id": run_id,
                    "sample": sample,
                    "order": order,
                    "backend": backend,
                    "position_index": index,
                    "position_id": position["id"],
                    "depth": depth,
                    **measurement,
                    "status": "ok",
                    "error": "",
                }
            except UciError as error:
                row = {
                    "run_id": run_id,
                    "sample": sample,
                    "order": order,
                    "backend": backend,
                    "position_index": index,
                    "position_id": position["id"],
                    "depth": depth,
                    "nodes": "",
                    "engine_elapsed_ms": "",
                    "wall_elapsed_ms": "",
                    "engine_nps": "",
                    "wall_nps": "",
                    "bestmove": "",
                    "status": "error",
                    "error": str(error),
                }
            rows.append(row)
            append_result(results_path, row)
            if row["status"] != "ok":
                break
        engine.quit()
    except (KeyboardInterrupt, OSError, UciError, subprocess.SubprocessError):
        engine.terminate()
        raise
    return rows


def deterministic_match(rows: list[dict[str, Any]], backend: str) -> bool:
    scalar = {(row["sample"], row["position_id"]): row for row in rows
              if row["backend"] == "scalar" and row["status"] == "ok"}
    candidate = {(row["sample"], row["position_id"]): row for row in rows
                 if row["backend"] == backend and row["status"] == "ok"}
    if scalar.keys() != candidate.keys() or not scalar:
        return False
    return all((row["nodes"], row["bestmove"])
               == (candidate[key]["nodes"], candidate[key]["bestmove"])
               for key, row in scalar.items())


def summarize(rows: list[dict[str, Any]], samples: int,
              positions: int) -> list[dict[str, Any]]:
    sample_results: dict[tuple[str, int], dict[str, float]] = {}
    for backend in BACKENDS:
        for sample in range(1, samples + 1):
            selected = [row for row in rows if row["backend"] == backend
                        and row["sample"] == sample and row["status"] == "ok"]
            if len(selected) != positions:
                continue
            nodes = sum(int(row["nodes"]) for row in selected)
            engine_ms = sum(int(row["engine_elapsed_ms"]) for row in selected)
            wall_ms = sum(int(row["wall_elapsed_ms"]) for row in selected)
            sample_results[(backend, sample)] = {
                "engine_nps": nodes * 1000 / max(1, engine_ms),
                "wall_nps": nodes * 1000 / max(1, wall_ms),
            }
    medians: dict[str, dict[str, float]] = {}
    for backend in BACKENDS:
        completed = [value for (name, _), value in sample_results.items() if name == backend]
        if completed:
            medians[backend] = {
                "engine_nps": statistics.median(item["engine_nps"] for item in completed),
                "wall_nps": statistics.median(item["wall_nps"] for item in completed),
            }
    scalar_nps = medians.get("scalar", {}).get("engine_nps")
    summary = []
    for backend in BACKENDS:
        completed = sum(1 for name, _ in sample_results if name == backend)
        median = medians.get(backend, {})
        engine_nps = median.get("engine_nps")
        speedup = ((engine_nps / scalar_nps - 1) * 100
                   if engine_nps is not None and scalar_nps else None)
        summary.append({
            "backend": backend,
            "samples_completed": completed,
            "positions_per_sample": positions,
            "median_engine_nps": round(engine_nps, 2) if engine_nps is not None else "",
            "median_wall_nps": round(median["wall_nps"], 2) if median else "",
            "speedup_vs_scalar_percent": round(speedup, 3) if speedup is not None else "",
            "deterministic_match": deterministic_match(rows, backend),
        })
    return summary


def write_summary(path: Path, summary: list[dict[str, Any]]) -> None:
    with path.open("w", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=summary[0].keys())
        writer.writeheader()
        writer.writerows(summary)


def self_test() -> None:
    assert backend_order(1) == ("scalar", "neon", "dotprod")
    assert backend_order(2) == ("scalar", "dotprod", "neon")
    assert backend_order(6) == ("dotprod", "neon", "scalar")
    rows = []
    for backend in BACKENDS:
        rows.append({
            "sample": 1, "position_id": 0, "backend": backend, "status": "ok",
            "nodes": 100, "bestmove": "e2e4", "engine_elapsed_ms": 10,
            "wall_elapsed_ms": 11,
        })
    summary = summarize(rows, 1, 1)
    assert all(row["deterministic_match"] for row in summary)
    assert summary[0]["median_engine_nps"] == 10000


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build and compare forced scalar, NEON, and dot-product NNUE backends."
    )
    parser.add_argument("--eval-file", type=Path, help="PNUE file loaded by every backend")
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG,
                        help="validation configuration and standard position set")
    parser.add_argument("--output-dir", type=Path,
                        help="new result directory; defaults under validation-runs")
    parser.add_argument("--samples", type=int, default=6,
                        help="complete rotated samples (default: 6)")
    parser.add_argument("--depth", type=int, help="fixed search depth (default: profile depth)")
    parser.add_argument("--limit-positions", type=int,
                        help="use the first N standard positions")
    parser.add_argument("--timeout", type=float, help="timeout per position in seconds")
    parser.add_argument("--warmup-depth", type=int, default=6,
                        help="unrecorded warmup depth (default: 6)")
    parser.add_argument("--hash-mb", type=int, default=64,
                        help="transposition-table size for every backend (default: 64)")
    parser.add_argument("--self-test", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.self_test:
        self_test()
        print("NNUE backend benchmark self-test passed")
        return 0
    if args.eval_file is None or not args.eval_file.is_file():
        raise SystemExit("--eval-file must name an existing PNUE file")
    if platform.machine() not in ("arm64", "aarch64"):
        raise SystemExit("this benchmark requires native ARM64 hardware")
    if args.samples < 1 or args.warmup_depth < 1 or args.hash_mb < 1:
        raise SystemExit("samples, warmup depth, and hash size must be positive")
    if args.depth is not None and args.depth < 1:
        raise SystemExit("depth must be positive")
    if args.limit_positions is not None and args.limit_positions < 1:
        raise SystemExit("position limit must be positive")
    if args.timeout is not None and args.timeout <= 0:
        raise SystemExit("timeout must be positive")

    config_path = args.config.resolve()
    eval_file = args.eval_file.resolve()
    config = json.loads(config_path.read_text())
    profile, positions = load_positions(config, args.limit_positions)
    depth = args.depth or int(profile["depth"])
    timeout = (args.timeout if args.timeout is not None
               else float(profile.get("timeout_seconds", 120)))
    started = datetime.now(UTC)
    run_id = started.strftime("nnue-backend-benchmark-%Y%m%d-%H%M%S")
    output_dir = (args.output_dir.resolve() if args.output_dir
                  else ROOT / "validation-runs" / run_id)
    output_dir.mkdir(parents=True, exist_ok=False)
    results_path = output_dir / "results.csv"
    summary_path = output_dir / "summary.csv"
    metadata_path = output_dir / "metadata.json"
    build_log = output_dir / "build.log"
    git_head = command_output(["git", "rev-parse", "HEAD"])
    tracked_diff = subprocess.run(["git", "diff", "--binary"], cwd=ROOT,
                                  stdout=subprocess.PIPE, check=True).stdout
    metadata: dict[str, Any] = {
        "run_id": run_id,
        "status": "running",
        "started_at_utc": started.isoformat(timespec="seconds"),
        "command": shlex.join([sys.executable, *sys.argv]),
        "repository": str(ROOT),
        "git_head": git_head,
        "git_branch": command_output(["git", "branch", "--show-current"]),
        "git_status": command_output(["git", "status", "--short", "--branch"]),
        "tracked_diff_sha256": hashlib.sha256(tracked_diff).hexdigest(),
        "benchmark_script_sha256": file_sha256(Path(__file__).resolve()),
        "eval_file": str(eval_file),
        "eval_file_sha256": file_sha256(eval_file),
        "config": str(config_path),
        "config_sha256": file_sha256(config_path),
        "machine": {
            "hostname": socket.gethostname(),
            "platform": platform.platform(),
            "architecture": platform.machine(),
            "processor": platform.processor(),
            "macos_version": platform.mac_ver()[0],
            "python": sys.version.split()[0],
        },
        "benchmark": {
            "backends": list(BACKENDS),
            "samples": args.samples,
            "depth": depth,
            "warmup_depth": min(depth, args.warmup_depth),
            "hash_mb": args.hash_mb,
            "timeout_seconds": timeout,
            "position_ids": [position["id"] for position in positions],
            "order": "balanced six-permutation cycle",
        },
    }
    metadata_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n")

    rows: list[dict[str, Any]] = []
    failed = False
    try:
        with tempfile.TemporaryDirectory(prefix="prophet-nnue-backend-") as directory:
            binaries, build_commands, compiler = build_backends(
                config, Path(directory), build_log, git_head
            )
            metadata["compiler"] = {
                "command": compiler,
                "version": command_output([compiler, "--version"]),
            }
            metadata["build_commands"] = {
                backend: shlex.join(command) for backend, command in build_commands.items()
            }
            metadata_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n")
            for sample in range(1, args.samples + 1):
                order = backend_order(sample)
                print(f"[nnue-bench] sample {sample}/{args.samples}: {', '.join(order)}",
                      flush=True)
                for order_index, backend in enumerate(order, 1):
                    try:
                        new_rows = run_backend_sample(
                            run_id, sample, order_index, backend, binaries[backend], eval_file,
                            positions, depth, args.warmup_depth, timeout, args.hash_mb,
                            results_path,
                        )
                        rows.extend(new_rows)
                        failed = failed or any(row["status"] != "ok" for row in new_rows)
                    except (UciError, subprocess.SubprocessError) as error:
                        print(f"[nnue-bench] {backend} failed: {error}", file=sys.stderr,
                              flush=True)
                        failed = True
    except KeyboardInterrupt:
        metadata["status"] = "interrupted"
        metadata["finished_at_utc"] = now_utc()
        metadata_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n")
        print(f"NNUE backend benchmark interrupted; partial results: {results_path}",
              file=sys.stderr)
        return 130
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        metadata["status"] = "failed"
        metadata["error"] = str(error)
        metadata["finished_at_utc"] = now_utc()
        metadata_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n")
        print(f"NNUE backend benchmark failed: {error}", file=sys.stderr)
        return 1

    summary = summarize(rows, args.samples, len(positions))
    write_summary(summary_path, summary)
    deterministic = all(row["deterministic_match"] for row in summary)
    complete = all(row["samples_completed"] == args.samples for row in summary)
    metadata["status"] = "complete" if complete and deterministic and not failed else "failed"
    metadata["finished_at_utc"] = now_utc()
    metadata["summary"] = summary
    metadata_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n")

    print("\nNNUE backend benchmark results", flush=True)
    for row in sorted(summary, key=lambda item: item["median_engine_nps"] or 0, reverse=True):
        print(f"{row['backend']:8} median {row['median_engine_nps']:>12} NPS  "
              f"vs scalar {row['speedup_vs_scalar_percent']:>8}%  "
              f"deterministic {row['deterministic_match']}", flush=True)
    print(f"results: {results_path}", flush=True)
    print(f"summary: {summary_path}", flush=True)
    print(f"metadata: {metadata_path}", flush=True)
    if metadata["status"] != "complete":
        print("benchmark evidence is incomplete or nondeterministic", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
