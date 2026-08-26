#!/usr/bin/env python3
"""Validate the stable validation configuration before local or CI checks."""

from __future__ import annotations

import json
import sys
from pathlib import Path

STRESS_FEN = "k7/2n1n3/1nbNbn2/2NbRBn1/1nbRQR2/2NBRBN1/3N1N2/7K w - - 0 1"


def main() -> int:
    path = Path("config/validation.json")
    try:
        config = json.loads(path.read_text())
        benchmark = config["benchmark"]
        positions = benchmark["positions"]
        ids = [item["id"] for item in positions]
        if ids != list(range(len(positions))):
            raise ValueError("position IDs must be stable, unique, and zero-based")
        stress = next(item for item in positions if item["id"] == 37)
        if stress["fen"] != STRESS_FEN:
            raise ValueError("position 37 must be the configured qsearch stress FEN")
        profiles = benchmark["profiles"]
        if profiles["standard"]["depth"] != 12 or 37 not in profiles["standard"]["exclude_ids"]:
            raise ValueError("standard must use depth 12 and exclude position 37")
        if profiles["qsearch-stress"]["ids"] != [37] or profiles["qsearch-stress"]["depth"] != 1:
            raise ValueError("qsearch-stress must contain only position 37 at depth 1")
        if not Path(config["sprt"]["openings"]).is_file():
            raise ValueError("configured tracked opening suite is absent")
    except (KeyError, ValueError, json.JSONDecodeError) as error:
        print(f"validation configuration failed: {error}", file=sys.stderr)
        return 1
    print("validation configuration passed", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
