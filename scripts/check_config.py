#!/usr/bin/env python3
"""Validate the stable validation configuration before local or CI checks."""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

STRESS_FEN = "k7/2n1n3/1nbNbn2/2NbRBn1/1nbRQR2/2NBRBN1/3N1N2/7K w - - 0 1"


def check_candidate_nnue(config: dict, root: Path | None = None) -> dict | None:
    root = ROOT if root is None else root
    if "candidate_nnue" not in config:
        return
    candidate = config["candidate_nnue"]
    if not isinstance(candidate, dict):
        raise ValueError("candidate_nnue must be an object")
    filename = candidate["file"]
    if not isinstance(filename, str) or not filename.strip():
        raise ValueError("candidate_nnue.file must be a nonempty path")
    if Path(filename).is_absolute():
        raise ValueError("candidate_nnue.file must be relative to the candidate repository")
    network_file = root / filename
    if not network_file.resolve().is_relative_to(root.resolve()):
        raise ValueError("candidate_nnue.file must stay inside the candidate repository")
    manifest_file = network_file.with_suffix(network_file.suffix + ".json")
    if not network_file.is_file():
        raise ValueError(f"candidate NNUE network is absent: {network_file}")
    if not manifest_file.is_file():
        raise ValueError(f"candidate NNUE export manifest is absent: {manifest_file}")
    manifest = json.loads(manifest_file.read_text())
    if not isinstance(manifest, dict):
        raise ValueError("candidate NNUE export manifest must be an object")
    actual_sha256 = hashlib.sha256(network_file.read_bytes()).hexdigest()
    if candidate["sha256"] != actual_sha256:
        raise ValueError("candidate NNUE SHA-256 differs from configuration")
    if manifest["pnue_sha256"] != actual_sha256:
        raise ValueError("candidate NNUE SHA-256 differs from export manifest")
    if manifest["architecture_id"] != "prophet-nnue-v1":
        raise ValueError("candidate NNUE export manifest has an unsupported architecture")

    return {
        "file": filename,
        "sha256": actual_sha256,
        "manifest_sha256": hashlib.sha256(manifest_file.read_bytes()).hexdigest(),
        "architecture_id": manifest["architecture_id"],
        "model_identifier": manifest.get("source_checkpoint_manifest_sha256"),
    }


def main() -> int:
    path = ROOT / "config/validation.json"
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
        if not (ROOT / config["sprt"]["openings"]).is_file():
            raise ValueError("configured tracked opening suite is absent")
        check_candidate_nnue(config)
        if config["sprt"].get("model", "normalized") not in {"normalized", "logistic", "bayesian"}:
            raise ValueError("unsupported SPRT model")
    except (KeyError, OSError, TypeError, ValueError) as error:
        print(f"validation configuration failed: {error}", file=sys.stderr)
        return 1
    print("validation configuration passed", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
