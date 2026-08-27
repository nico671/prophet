#!/usr/bin/env python3
"""Check UCI search scoring at draw-rule thresholds."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from uci_client import UciEngine, UciError


def expect_draw_score(engine: UciEngine, position: str, label: str,
                      go_command: str = "go depth 1") -> None:
    engine.send(position)
    engine.send(go_command)
    bestmove, lines = engine.expect(lambda line: line.startswith("bestmove "), "bestmove")
    if not re.fullmatch(r"bestmove [a-h][1-8][a-h][1-8][qrbn]?", bestmove):
        raise UciError(f"{label}: malformed {bestmove!r}")
    if not any(re.match(r"^info depth 1 .* score cp 0\b", line) for line in lines):
        raise UciError(f"{label}: expected a draw score, got {lines!r}")


def expect_mate_score(engine: UciEngine, position: str, label: str) -> None:
    engine.send(position)
    engine.send("go depth 1")
    bestmove, lines = engine.expect(lambda line: line.startswith("bestmove "), "bestmove")
    if not re.fullmatch(r"bestmove [a-h][1-8][a-h][1-8][qrbn]?", bestmove):
        raise UciError(f"{label}: malformed {bestmove!r}")
    if not any(re.match(r"^info depth 1 .* score mate [0-9]+", line) for line in lines):
        raise UciError(f"{label}: expected a mate score, got {lines!r}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True, type=Path)
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args()

    engine = UciEngine(args.engine, args.timeout)
    try:
        engine.initialize()
        base = "7k/8/8/8/8/8/R7/K7 w - -"
        expect_draw_score(engine, f"position fen {base} 100 1", "fifty-move current position")
        expect_draw_score(engine, f"position fen {base} 99 1", "fifty-move intended move")
        expect_draw_score(engine, f"position fen {base} 150 1", "seventy-five-move current position")
        expect_draw_score(engine, f"position fen {base} 149 1", "seventy-five-move final move")
        expect_mate_score(engine, "position fen 7k/8/5KQ1/8/8/8/8/8 w - - 149 1",
                          "seventy-five-move checkmate exception")

        cycle = "g1f3 g8f6 f3g1 f6g8"
        expect_draw_score(engine,
                          "position startpos moves g1f3 g8f6 f3g1 f6g8 g1f3 g8f6 f3g1",
                          "threefold repetition intended move",
                          "go depth 1 searchmoves f6g8")
        expect_draw_score(engine, f"position startpos moves {cycle} {cycle} {cycle} {cycle}",
                          "fivefold repetition")
        engine.quit()
    except UciError as error:
        engine.terminate()
        print(f"Draw-rule smoke failed: {error}", file=sys.stderr)
        return 1

    print("Draw-rule smoke passed", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
