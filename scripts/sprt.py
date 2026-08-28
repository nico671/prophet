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


DISPLAY_LINE = re.compile(
    r"^(Finished game|Score of |Elo:|nElo:|LOS:|DrawRatio:|Ptnml\(|LLR:|"
    r"SPRT \(|Finished match)"
)


def should_display(line: str) -> bool:
    """Keep terminal output to game results and Fastchess summaries."""
    return bool(DISPLAY_LINE.match(line))


def fastchess_command(fastchess: str, settings: dict[str, Any], baseline: Path,
                      candidate: Path, output_dir: Path) -> list[str]:
    """Build a command that tests the candidate against the baseline."""
    pgn = output_dir / "games.pgn"
    return [
        fastchess, "-concurrency", str(settings["concurrency"]), "-rounds", str(settings["rounds"]),
        "-repeat", "-srand", str(settings["seed"]),
        # Fastchess applies the SPRT hypotheses to its first engine.  The
        # candidate must therefore be first: H1 means candidate > baseline.
        "-engine", f"cmd={candidate}", "name=candidate", "proto=uci",
        "-engine", f"cmd={baseline}", "name=baseline", "proto=uci",
        "-each", f"tc={settings['time_control']}",
        "-openings", f"file={settings['openings']}", "format=epd", "order=random",
        "-sprt", f"elo0={settings['elo0']}", f"elo1={settings['elo1']}",
        f"alpha={settings['alpha']}", f"beta={settings['beta']}",
        "-config", f"outname={output_dir / 'fastchess-config.json'}",
        "-pgnout", f"file={pgn}", "append=false",
    ]


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
    command = fastchess_command(fastchess, settings, args.baseline, args.candidate,
                                args.output_dir)
    print(f"[sprt] starting Fastchess: {settings['rounds']} rounds, concurrency {settings['concurrency']}", flush=True)
    with log.open("w") as output:
        process = subprocess.Popen(command, text=True, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, bufsize=1)
        assert process.stdout is not None
        for line in process.stdout:
            output.write(line)
            output.flush()
            if should_display(line):
                sys.stdout.write(line)
                sys.stdout.flush()
        process.wait()
    completed_output = log.read_text()
    lower = completed_output.lower()
    if re.search(r"\bh1\b.*\baccepted\b|\baccepted\b.*\bh1\b", lower):
        verdict = "accepted"
    elif re.search(r"\bh0\b.*\baccepted\b|\baccepted\b.*\bh0\b|\brejected\b", lower):
        verdict = "rejected"
    else:
        verdict = "inconclusive"
    evidence = {"command": command, "fastchess_exit_code": process.returncode,
                "tested_engine": "candidate", "reference_engine": "baseline",
                "engine_order": ["candidate", "baseline"],
                "log": str(log), "pgn": str(pgn), "verdict": verdict}
    (args.output_dir / "sprt.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(f"SPRT {verdict}; evidence: {args.output_dir}", flush=True)
    return 0 if process.returncode == 0 and verdict == "accepted" else 1


if __name__ == "__main__":
    raise SystemExit(main())
