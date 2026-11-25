import contextlib
import hashlib
import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from scripts import check_config


class CheckConfigTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.config = json.loads((check_config.ROOT / "config/validation.json").read_text())
        self.config.pop("candidate_nnue", None)
        self.config["sprt"]["openings"] = "openings.epd"
        (self.root / "openings.epd").write_text("test opening")
        (self.root / "config").mkdir()
        self.network_file = self.root / "networks/prophet-v1.0.0.pnue"
        self.network_file.parent.mkdir()
        self.network_file.write_bytes(b"test network")
        self.manifest_file = self.network_file.with_suffix(".pnue.json")
        self.sha256 = hashlib.sha256(self.network_file.read_bytes()).hexdigest()
        self.manifest = {
            "pnue_sha256": self.sha256,
            "architecture_id": "prophet-nnue-v1",
        }
        self.write_manifest()

    def write_manifest(self) -> None:
        self.manifest_file.write_text(json.dumps(self.manifest))

    def enable_nnue(self) -> None:
        self.config["candidate_nnue"] = {
            "file": "networks/prophet-v1.0.0.pnue",
            "sha256": self.sha256,
        }

    def run_check(self, expected: int, message: str) -> None:
        (self.root / "config/validation.json").write_text(json.dumps(self.config))
        output = io.StringIO()
        with patch.object(check_config, "ROOT", self.root):
            with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
                self.assertEqual(check_config.main(), expected)
        self.assertIn(message, output.getvalue())

    def test_hce_configuration_needs_no_network(self) -> None:
        self.network_file.unlink()
        self.manifest_file.unlink()
        self.run_check(0, "validation configuration passed")

    def test_valid_nnue_configuration(self) -> None:
        self.enable_nnue()
        self.run_check(0, "validation configuration passed")

    def test_missing_network_or_manifest_fails(self) -> None:
        self.enable_nnue()
        self.network_file.unlink()
        self.run_check(1, "network is absent")
        self.network_file.write_bytes(b"test network")
        self.manifest_file.unlink()
        self.run_check(1, "export manifest is absent")

    def test_configuration_hash_mismatch_fails(self) -> None:
        self.enable_nnue()
        self.config["candidate_nnue"]["sha256"] = "0" * 64
        self.run_check(1, "SHA-256 differs from configuration")

    def test_manifest_hash_mismatch_fails(self) -> None:
        self.enable_nnue()
        self.manifest["pnue_sha256"] = "0" * 64
        self.write_manifest()
        self.run_check(1, "SHA-256 differs from export manifest")

    def test_wrong_architecture_fails(self) -> None:
        self.enable_nnue()
        self.manifest["architecture_id"] = "other-network"
        self.write_manifest()
        self.run_check(1, "unsupported architecture")

    def test_malformed_candidate_settings_fail(self) -> None:
        for candidate in (None, False, [], "network", {}, {"file": None},
                          {"file": ""}, {"file": "  "}, {"file": []},
                          {"file": "networks/prophet-v1.0.0.pnue"}):
            with self.subTest(candidate=candidate):
                self.config["candidate_nnue"] = candidate
                self.run_check(1, "validation configuration failed")

    def test_malformed_or_incomplete_manifest_fails(self) -> None:
        self.enable_nnue()
        for manifest in ("{", "null", "[]", "{}",
                         json.dumps({"pnue_sha256": self.sha256})):
            with self.subTest(manifest=manifest):
                self.manifest_file.write_text(manifest)
                self.run_check(1, "validation configuration failed")

    def test_network_must_stay_inside_candidate_repository(self) -> None:
        self.enable_nnue()
        for filename in (str(self.network_file), "../outside.pnue"):
            with self.subTest(filename=filename):
                self.config["candidate_nnue"]["file"] = filename
                self.run_check(1, "candidate_nnue.file must")

    def test_unreadable_network_fails(self) -> None:
        self.enable_nnue()
        with patch.object(Path, "read_bytes", side_effect=PermissionError("unreadable network")):
            self.run_check(1, "unreadable network")


if __name__ == "__main__":
    unittest.main()
