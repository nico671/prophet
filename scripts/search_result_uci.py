#!/usr/bin/env python3
"""Compare the synchronous search API with a separate UCI process."""

import argparse
import re
import subprocess
from pathlib import Path

from uci_client import UciEngine


POSITIONS = (
    ("start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", None),
    ("black", "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1", None),
    ("castling", "r3k2r/ppp2ppp/2npbn2/3qp3/8/2NPBN2/PPP2PPP/R2QK2R w KQkq - 0 10", None),
    ("white mate in one", "7k/5Q2/6K1/8/8/8/8/8 w - - 0 1", ("mate", 1)),
    ("black mate in one", "8/8/8/8/8/6k1/5q2/7K b - - 0 1", ("mate", 1)),
    ("checkmate", "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1", ("mate", 0)),
    ("stalemate", "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", ("cp", 0)),
    ("move-rule draw", "7k/8/8/8/8/8/6K1/8 w - - 100 1", ("cp", 0)),
)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    engine = UciEngine(args.engine)
    try:
        engine.initialize()
        engine.send("setoption name Hash value 1")
        engine.send("setoption name MultiPV value 1")
        for name, fen, known_score in POSITIONS:
            for depth in (1, 3):
                output = subprocess.run(
                    [str(args.probe), "--probe", fen, str(depth)],
                    check=True, capture_output=True, text=True, timeout=30,
                ).stdout.split()
                move = output[0]
                score, completed_depth, completed, mate_score, threshold = map(int, output[1:])
                assert completed and completed_depth == depth, (name, output)
                expected_score = ("cp", score)
                if abs(score) >= threshold:
                    distance = (mate_score - abs(score) + 1) // 2
                    expected_score = ("mate", distance if score >= 0 else -distance)
                if known_score is not None:
                    assert expected_score == known_score, (name, expected_score)
                engine.send("ucinewgame")
                engine.send("isready")
                engine.expect(lambda line: line == "readyok", "readyok")
                engine.send(f"position fen {fen}")
                engine.send(f"go depth {depth}")
                bestmove, lines = engine.expect(
                    lambda line: line.startswith("bestmove "), "bestmove"
                )
                assert bestmove == f"bestmove {move}", (name, output, lines)
                scores = [
                    (int(match[1]), match[2], int(match[3]))
                    for line in lines
                    if (match := re.match(
                        r"^info depth (\d+) multipv 1 score (cp|mate) (-?\d+)\b", line
                    ))
                ]
                assert scores and scores[-1] == (depth, *expected_score), (name, output, lines)
        engine.quit()
    finally:
        engine.terminate()
    print(f"Search API/UCI comparison passed: {len(POSITIONS) * 2} searches")


if __name__ == "__main__":
    main()
