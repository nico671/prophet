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


def progress(message: str, error: bool = False) -> None:
    print(f"[bench] {message}", file=sys.stderr if error else sys.stdout, flush=True)


def load_config(path: Path) -> dict[str, Any]:
    with path.open() as file:
        return json.load(file)


def run_position(engine_path: Path, position: dict[str, Any], depth: int, timeout: float,
                 label: str) -> dict[str, Any]:
    engine = UciEngine(engine_path, timeout)
    started = time.monotonic()
    last_report = started

    def report(line: str) -> None:
        nonlocal last_report
        match = INFO.match(line)
        now = time.monotonic()
        if match and now - last_report >= 5:
            progress(f"{label}: depth {match.group(1)}, nodes {match.group(2)}, "
                     f"engine time {match.group(3)} ms")
            last_report = now

    progress(f"{label}: starting")
    try:
        engine.initialize()
        command = f"position fen {position['fen']}"
        if position.get("moves"):
            command += f" moves {position['moves']}"
        engine.send(command)
        engine.send(f"go depth {depth}")
        bestmove, lines = engine.expect(lambda line: line.startswith("bestmove "), "bestmove", timeout,
                                        on_line=report)
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
        progress(f"{label}: complete, {nodes} nodes, {wall_elapsed_ms} ms")
        return {"id": position["id"], "nodes": nodes, "elapsed_ms": wall_elapsed_ms,
                "engine_elapsed_ms": elapsed_ms, "nps": nodes * 1000 / wall_elapsed_ms,
                "bestmove": bestmove.split()[1]}
    except UciError as error:
        engine.terminate()
        progress(f"{label}: failed: {error}", error=True)
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
    for profile in args.profiles.split(","):
        details, positions = select_positions(config, profile, args.limit_positions)
        depth = args.depth or int(details["depth"])
        samples = args.samples or int(details["samples"])
        timeout = float(details.get("timeout_seconds", 120))
        progress(f"profile {profile}: {len(positions)} positions, depth {depth}, {samples} samples, timeout {timeout:g}s")
        profile_result: dict[str, Any] = {"depth": depth, "samples": samples,
                                           "position_ids": [item["id"] for item in positions],
                                           "baseline": [], "candidate": []}
        try:
            for sample in range(1, samples + 1):
                # Paired order makes each aggregate sample subject to comparable local load.
                for name, binary in (("baseline", args.baseline), ("candidate", args.candidate)):
                    entries = []
                    for index, item in enumerate(positions, 1):
                        label = f"{profile} sample {sample}/{samples} {name} position {index}/{len(positions)} (id {item['id']})"
                        entries.append(run_position(binary, item, depth, timeout, label))
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
        except (UciError, ValueError) as error:
            profile_result["error"] = str(error)
            profile_result["passed"] = False
            progress(f"profile {profile} failed: {error}", error=True)
        output["profiles"][profile] = profile_result

    # The stress profile is diagnostic. A timeout is retained in the evidence but
    # does not invalidate the standard comparative speed gate.
    gating_profiles = [result for name, result in output["profiles"].items()
                       if name != "qsearch-stress"]
    if not gating_profiles:
        gating_profiles = list(output["profiles"].values())
    output["passed"] = all(item.get("passed", False) for item in gating_profiles)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n")
    if not output.get("passed"):
        print(f"comparative benchmark failed; evidence: {args.output}", file=sys.stderr)
        return 1
    standard = output["profiles"].get("standard")
    if standard:
        print("comparative benchmark passed: candidate median NPS "
              f"{standard['candidate_median_nps']:.0f}, baseline {standard['baseline_median_nps']:.0f}", flush=True)
    stress = output["profiles"].get("qsearch-stress")
    if stress and stress.get("error"):
        progress(f"qsearch-stress recorded a diagnostic failure: {stress['error']}", error=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
