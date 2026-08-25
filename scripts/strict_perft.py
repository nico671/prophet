#!/usr/bin/env python3
"""Make Prophet's human-readable full perft suite a strict gate."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from uci_client import UciEngine, UciError

EXPECTED_CASES = {
    "Initial Position": 6, "Kiwipete Position": 5, "Position 3": 6,
    "Position 4": 5, "Position 5": 5, "Position 6": 5,
}
DEPTH = re.compile(r"^Depth (\d+): (\d+) \([^)]*\) PASS$")
RESULT = re.compile(r"^Results: (\d+) passed, (\d+) failed$")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True, type=Path)
    parser.add_argument("--timeout", type=float, default=180.0)
    args = parser.parse_args()
    if not args.engine.is_file():
        print(f"missing engine: {args.engine}", file=sys.stderr)
        return 1
    engine = UciEngine(args.engine, args.timeout)
    try:
        engine.initialize()
        engine.send("perft suite")
        _, lines = engine.expect(lambda line: bool(RESULT.match(line)), "perft result", args.timeout)
        engine.quit()
    except UciError as error:
        engine.terminate()
        print(f"strict perft failed: {error}", file=sys.stderr)
        return 1
    output = "\n".join(lines)
    if "Failed to parse FEN" in output or " FAIL" in output:
        print("strict perft failed: engine reported a FEN or node-count failure", file=sys.stderr)
        return 1
    current = None
    found: dict[str, set[int]] = {name: set() for name in EXPECTED_CASES}
    for line in lines:
        if line.startswith("=== ") and line.endswith(" ==="):
            current = line[4:-4]
        match = DEPTH.match(line)
        if match and current in found:
            found[current].add(int(match.group(1)))
    for name, max_depth in EXPECTED_CASES.items():
        if found[name] != set(range(1, max_depth + 1)):
            print(f"strict perft failed: malformed or incomplete output for {name}", file=sys.stderr)
            return 1
    match = next((RESULT.match(line) for line in lines if RESULT.match(line)), None)
    if not match or (int(match.group(1)), int(match.group(2))) != (32, 0):
        print("strict perft failed: expected `Results: 32 passed, 0 failed`", file=sys.stderr)
        return 1
    print("strict perft passed: 32 expected depths, 0 failures")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
