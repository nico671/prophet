#!/usr/bin/env python3
"""Verify tagged validation evidence and package the matching network."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from pathlib import Path

try:
    from .check_config import check_candidate_nnue
    from .uci_client import UciEngine
    from .validation import verify_release_manifest
except ImportError:
    from check_config import check_candidate_nnue
    from uci_client import UciEngine
    from validation import verify_release_manifest

ROOT = Path(__file__).resolve().parents[1]


def package(root: Path, manifest_path: Path, output: Path, candidate_sha: str) -> None:
    manifest = json.loads(manifest_path.read_text())
    settings = json.loads((root / "config/validation.json").read_text())
    verify_release_manifest(manifest, settings, root, candidate_sha,
                            manifest["baseline"]["sha"],
                            bool(manifest.get("speed_override_requested")))
    identity = check_candidate_nnue(settings, root)
    output.mkdir(parents=True, exist_ok=True)
    if identity:
        network_file = root / identity["file"]
        shutil.copy2(network_file, output / network_file.name)
        shutil.copy2(network_file.with_suffix(network_file.suffix + ".json"),
                     output / (network_file.name + ".json"))
    shutil.copy2(manifest_path, output / "validation-manifest.json")
    for source, target in (("README.md", "README.txt"), ("LICENSE", "LICENSE"),
                           ("THIRD_PARTY_NOTICES.md", "THIRD_PARTY_NOTICES.txt")):
        shutil.copy2(root / source, output / target)
    files = sorted(path for path in output.iterdir() if path.is_file() and path.name != "SHA256SUMS")
    (output / "SHA256SUMS").write_text("".join(
        f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n" for path in files
    ))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--candidate-sha", required=True)
    args = parser.parse_args()
    package(ROOT, args.manifest, args.output_dir, args.candidate_sha)
    settings = json.loads((ROOT / "config/validation.json").read_text())
    identity = check_candidate_nnue(settings, ROOT)
    engine = UciEngine(args.output_dir / "prophet-macos-arm64")
    try:
        engine.initialize()
        engine.configure(args.output_dir.resolve() / Path(identity["file"]).name if identity else None,
                         identity.get("model_identifier") if identity else None)
        engine.send("position startpos")
        engine.send("go depth 2")
        bestmove, _ = engine.expect(lambda line: line.startswith("bestmove "), "packaged engine search")
        if bestmove == "bestmove 0000":
            raise RuntimeError("packaged engine returned no move")
        engine.quit()
    finally:
        engine.terminate()


if __name__ == "__main__":
    main()
