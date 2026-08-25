#!/usr/bin/env python3
"""Exercise the supported UCI protocol paths that local validation relies on."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from uci_client import UciEngine, UciError


def search(engine: UciEngine, command: str, depth: int, allowed: set[str] | None = None,
           multipv: int | None = None) -> None:
    engine.send(command)
    bestmove, lines = engine.expect(lambda line: line.startswith("bestmove "), "bestmove")
    if not re.fullmatch(r"bestmove [a-h][1-8][a-h][1-8][qrbn]?", bestmove):
        raise UciError(f"malformed {bestmove!r}")
    exact = [line for line in lines if re.match(rf"^info depth {depth} ", line)]
    if not exact:
        raise UciError(f"search did not complete requested depth {depth}")
    if allowed and bestmove.split()[1] not in allowed:
        raise UciError(f"searchmoves was ignored: {bestmove}")
    if multipv:
        ranks = {int(match.group(1)) for line in exact
                 if (match := re.search(r"\bmultipv (\d+)\b", line))}
        if not set(range(1, multipv + 1)).issubset(ranks):
            raise UciError(f"MultiPV {multipv} output incomplete: {exact!r}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True, type=Path)
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args()
    engine = UciEngine(args.engine, args.timeout)
    try:
        engine.initialize()
        engine.send("position startpos moves e2e4 e7e5")
        search(engine, "go depth 2", 2)
        engine.send("position startpos")
        engine.send("go infinite")
        engine.send("stop")
        engine.expect(lambda line: line.startswith("bestmove "), "bestmove after stop")
        engine.send("position startpos")
        search(engine, "go depth 2 searchmoves e2e4 d2d4", 2, {"e2e4", "d2d4"})
        engine.send("setoption name MultiPV value 2")
        engine.send("position startpos")
        search(engine, "go depth 2", 2, multipv=2)
        engine.quit()
    except UciError as error:
        engine.terminate()
        print(f"UCI smoke failed: {error}", file=sys.stderr)
        return 1
    print("UCI smoke passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
