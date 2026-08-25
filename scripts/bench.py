#!/usr/bin/env python3
"""External UCI benchmark. It never calls Prophet's internal `bench` command."""

from __future__ import annotations

import argparse
import json
import re
import statistics
import sys
import time
from pathlib import Path
from typing import Any

from uci_client import UciEngine, UciError

INFO = re.compile(r"^info depth (\d+) .*\bnodes (\d+) time (\d+)\b")


def load_config(path: Path) -> dict[str, Any]:
    with path.open() as file:
        return json.load(file)


def run_position(engine_path: Path, position: dict[str, Any], depth: int, timeout: float) -> dict[str, Any]:
    engine = UciEngine(engine_path, timeout)
    try:
        engine.initialize()
        command = f"position fen {position['fen']}"
        if position.get("moves"):
            command += f" moves {position['moves']}"
        engine.send(command)
        started = time.monotonic()
        engine.send(f"go depth {depth}")
        bestmove, lines = engine.expect(lambda line: line.startswith("bestmove "), "bestmove", timeout)
        if not re.fullmatch(r"bestmove [a-h][1-8][a-h][1-8][qrbn]?", bestmove):
            raise UciError(f"malformed {bestmove!r}")
        exact = []
        for line in lines:
            match = INFO.match(line)
            if match and int(match.group(1)) == depth:
                exact.append((int(match.group(2)), int(match.group(3))))
        if not exact:
            raise UciError(f"position {position['id']} did not complete depth {depth}")
        nodes, elapsed_ms = exact[-1]
        wall_elapsed_ms = max(1, round((time.monotonic() - started) * 1000))
        if nodes <= 0:
            raise UciError(f"position {position['id']} returned invalid node count")
        engine.quit()
        return {"id": position["id"], "nodes": nodes, "elapsed_ms": wall_elapsed_ms,
                "engine_elapsed_ms": elapsed_ms, "nps": nodes * 1000 / wall_elapsed_ms,
                "bestmove": bestmove.split()[1]}
    except UciError:
        engine.terminate()
        raise


def select_positions(config: dict[str, Any], profile: str, limit: int | None) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    benchmark = config["benchmark"]
    details = benchmark["profiles"][profile]
    if "ids" in details:
        wanted = set(details["ids"])
        positions = [item for item in benchmark["positions"] if item["id"] in wanted]
    else:
        excluded = set(details.get("exclude_ids", []))
        positions = [item for item in benchmark["positions"] if item["id"] not in excluded]
    if limit is not None:
        positions = positions[:limit]
    if not positions:
        raise ValueError(f"profile {profile} has no positions")
    return details, positions


def aggregate(entries: list[dict[str, Any]]) -> dict[str, Any]:
    nodes = sum(entry["nodes"] for entry in entries)
    elapsed = sum(entry["elapsed_ms"] for entry in entries)
    return {"nodes": nodes, "elapsed_ms": elapsed, "nps": nodes * 1000 / elapsed,
            "positions": entries}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", required=True, type=Path)
    parser.add_argument("--candidate", required=True, type=Path)
    parser.add_argument("--config", type=Path, default=Path("config/validation.json"))
    parser.add_argument("--profiles", default="standard,qsearch-stress")
    parser.add_argument("--samples", type=int)
    parser.add_argument("--depth", type=int)
    parser.add_argument("--limit-positions", type=int)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if not args.baseline.is_file() or not args.candidate.is_file():
        parser.error("baseline and candidate must be executable files")
    config = load_config(args.config)
    output: dict[str, Any] = {"baseline": str(args.baseline), "candidate": str(args.candidate),
                              "profiles": {}, "passed": False}
    try:
        for profile in args.profiles.split(","):
            details, positions = select_positions(config, profile, args.limit_positions)
            depth = args.depth or int(details["depth"])
            samples = args.samples or int(details["samples"])
            timeout = float(details.get("timeout_seconds", 120))
            profile_result: dict[str, Any] = {"depth": depth, "samples": samples,
                                               "position_ids": [item["id"] for item in positions],
                                               "baseline": [], "candidate": []}
            for _ in range(samples):
                # Paired order makes each aggregate sample subject to comparable local load.
                for name, binary in (("baseline", args.baseline), ("candidate", args.candidate)):
                    entries = [run_position(binary, item, depth, timeout) for item in positions]
                    profile_result[name].append(aggregate(entries))
            if profile == "standard":
                baseline_median = statistics.median(item["nps"] for item in profile_result["baseline"])
                candidate_median = statistics.median(item["nps"] for item in profile_result["candidate"])
                budget = float(config["benchmark"]["speed_budget_ratio"])
                profile_result.update({"baseline_median_nps": baseline_median,
                                       "candidate_median_nps": candidate_median,
                                       "speed_budget_ratio": budget,
                                       "passed": candidate_median >= baseline_median * budget})
            else:
                # Stress data stays separate and is never part of standard NPS.
                profile_result["passed"] = True
            output["profiles"][profile] = profile_result
        output["passed"] = all(item["passed"] for item in output["profiles"].values())
    except (UciError, ValueError) as error:
        output["error"] = str(error)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n")
    if not output.get("passed"):
        print(f"comparative benchmark failed; evidence: {args.output}", file=sys.stderr)
        return 1
    standard = output["profiles"].get("standard")
    if standard:
        print("comparative benchmark passed: candidate median NPS "
              f"{standard['candidate_median_nps']:.0f}, baseline {standard['baseline_median_nps']:.0f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
