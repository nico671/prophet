#!/usr/bin/env python3
"""Run one configuration-driven, paired-opening Fastchess SPRT."""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", required=True, type=Path)
    parser.add_argument("--candidate", required=True, type=Path)
    parser.add_argument("--config", type=Path, default=Path("config/validation.json"))
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    fastchess = shutil.which("fastchess")
    if not fastchess:
        parser.error("fastchess is required for SPRT")
    config: dict[str, Any] = json.loads(args.config.read_text())
    settings = config["sprt"]
    opening = Path(settings["openings"])
    if not opening.is_file():
        parser.error(f"tracked opening suite is missing: {opening}")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    pgn = args.output_dir / "games.pgn"
    log = args.output_dir / "fastchess.log"
    command = [
        fastchess, "-concurrency", str(settings["concurrency"]), "-rounds", str(settings["rounds"]),
        "-repeat", "-srand", str(settings["seed"]),
        "-engine", f"cmd={args.baseline}", "name=baseline", "proto=uci",
        "-engine", f"cmd={args.candidate}", "name=candidate", "proto=uci",
        "-each", f"tc={settings['time_control']}",
        "-openings", f"file={opening}", "format=epd", "order=random",
        "-sprt", f"elo0={settings['elo0']}", f"elo1={settings['elo1']}",
        f"alpha={settings['alpha']}", f"beta={settings['beta']}",
        "-pgnout", f"file={pgn}", "append=false",
    ]
    completed = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.write_text(completed.stdout)
    lower = completed.stdout.lower()
    if re.search(r"\bh1\b.*\baccepted\b|\baccepted\b.*\bh1\b", lower):
        verdict = "accepted"
    elif re.search(r"\bh0\b.*\baccepted\b|\baccepted\b.*\bh0\b|\brejected\b", lower):
        verdict = "rejected"
    else:
        verdict = "inconclusive"
    evidence = {"command": command, "fastchess_exit_code": completed.returncode,
                "log": str(log), "pgn": str(pgn), "verdict": verdict}
    (args.output_dir / "sprt.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(f"SPRT {verdict}; evidence: {args.output_dir}")
    return 0 if completed.returncode == 0 and verdict == "accepted" else 1


if __name__ == "__main__":
    raise SystemExit(main())
