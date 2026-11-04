import copy
import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import MagicMock, patch

from scripts import bench, package_release, validation
from scripts.check_config import check_candidate_nnue
from scripts.uci_client import UciEngine, UciError, probe_engine


class NnueValidationTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.network_file = self.root / "network.pnue"
        self.network_file.write_bytes(b"release network")
        self.manifest_file = self.root / "network.pnue.json"
        self.manifest_file.write_text(
            json.dumps(
                {
                    "architecture_id": "prophet-nnue-v1",
                    "pnue_sha256": hashlib.sha256(
                        self.network_file.read_bytes()
                    ).hexdigest(),
                    "source_checkpoint_manifest_sha256": "a" * 64,
                }
            )
        )
        self.settings = {
            "candidate_nnue": {
                "file": "network.pnue",
                "sha256": hashlib.sha256(self.network_file.read_bytes()).hexdigest(),
            }
        }
        self.identity = check_candidate_nnue(self.settings, self.root)
        self.binaries = {"baseline": "b" * 64, "candidate": "c" * 64}
        self.manifest = {
            "version": "1.0.0",
            "candidate": {"sha": "candidate"},
            "baseline": {"sha": "baseline"},
            "configuration": copy.deepcopy(self.settings),
            "candidate_nnue": self.identity,
            "binaries": self.binaries,
            "checks": {"passed": True},
            "benchmark": {
                "passed": True,
                "result": {
                    "candidate_nnue": self.identity,
                    "binaries": self.binaries,
                },
            },
            "sprt": {
                "passed": True,
                "result": {
                    "candidate_nnue": self.identity,
                    "binaries": self.binaries,
                    "verdict": "accepted",
                },
            },
        }

    def verify(self, speed_override=False):
        validation.verify_release_manifest(
            self.manifest,
            self.settings,
            self.root,
            "candidate",
            "baseline",
            speed_override,
        )

    def test_matching_release_evidence_passes(self):
        self.verify()

    def test_changed_identity_or_configuration_fails(self):
        original = copy.deepcopy(self.manifest)
        for field, changed in (
            ("candidate_nnue", None),
            ("configuration", {}),
            ("candidate", {"sha": "other"}),
            ("baseline", {"sha": "other"}),
            ("binaries", None),
        ):
            with self.subTest(field=field):
                self.manifest = copy.deepcopy(original)
                self.manifest[field] = changed
                with self.assertRaises(RuntimeError):
                    self.verify()

    def test_changed_network_or_export_manifest_fails(self):
        self.network_file.write_bytes(b"changed network")
        with self.assertRaises(ValueError):
            self.verify()
        self.network_file.write_bytes(b"release network")
        self.manifest_file.write_text(self.manifest_file.read_text() + "\n")
        with self.assertRaisesRegex(RuntimeError, "NNUE identity differs"):
            self.verify()

    def test_mixed_benchmark_or_sprt_evidence_fails(self):
        original = copy.deepcopy(self.manifest)
        for stage in ("benchmark", "sprt"):
            for key in ("candidate_nnue", "binaries"):
                with self.subTest(stage=stage, key=key):
                    self.manifest = copy.deepcopy(original)
                    self.manifest[stage]["result"][key] = None
                    with self.assertRaises(RuntimeError):
                        self.verify()

    def test_failed_checks_and_unaccepted_sprt_fail(self):
        original = copy.deepcopy(self.manifest)
        for stage in ("checks", "sprt"):
            self.manifest = copy.deepcopy(original)
            self.manifest[stage]["passed"] = False
            with self.assertRaises(RuntimeError):
                self.verify()
        for verdict in ("identical", "rejected", "inconclusive"):
            self.manifest = copy.deepcopy(original)
            self.manifest["sprt"]["result"]["verdict"] = verdict
            with self.assertRaises(RuntimeError):
                self.verify()

    def test_speed_override_must_be_requested_and_recorded(self):
        self.manifest["benchmark"]["passed"] = False
        with self.assertRaises(RuntimeError):
            self.verify(True)
        self.manifest["speed_override_requested"] = True
        with self.assertRaises(RuntimeError):
            self.verify(False)
        self.verify(True)

    def test_explicit_baseline_is_resolved(self):
        with patch.object(validation, "ref_sha", return_value="pinned") as resolve:
            self.assertEqual(
                validation.choose_baseline({}, "v0.2.0"), ("v0.2.0", "pinned")
            )
        resolve.assert_called_once_with("v0.2.0")

    def test_package_contains_verified_network_and_checksums(self):
        (self.root / "config").mkdir()
        (self.root / "config/validation.json").write_text(json.dumps(self.settings))
        for name in ("README.md", "LICENSE", "THIRD_PARTY_NOTICES.md"):
            (self.root / name).write_text(name)
        manifest_path = self.root / "manifest.json"
        manifest_path.write_text(json.dumps(self.manifest))
        output = self.root / "dist"
        output.mkdir()
        (output / "prophet-macos-arm64").write_bytes(b"binary")
        package_release.package(self.root, manifest_path, output, "candidate")
        self.assertEqual((output / "network.pnue").read_bytes(), b"release network")
        self.assertEqual(
            (output / "network.pnue.json").read_bytes(), self.manifest_file.read_bytes()
        )
        for line in (output / "SHA256SUMS").read_text().splitlines():
            digest, filename = line.split("  ", 1)
            self.assertEqual(
                digest, hashlib.sha256((output / filename).read_bytes()).hexdigest()
            )
        with self.assertRaises(RuntimeError):
            package_release.package(self.root, manifest_path, output, "other")


class UciConfigurationTest(unittest.TestCase):
    def engine(self, replies):
        engine = object.__new__(UciEngine)
        engine.send = MagicMock()
        engine.expect = MagicMock(side_effect=[(line, []) for line in replies])
        return engine

    def test_nnue_load_precedes_enable_and_confirms_model(self):
        engine = self.engine(
            [
                "info string EvalFile loaded architecture prophet-nnue-v1 model expected",
                "info string UseNNUE true",
                "readyok",
            ]
        )
        engine.configure(Path("network.pnue"), "expected")
        sent = [call.args[0] for call in engine.send.call_args_list]
        self.assertEqual(
            sent[:2],
            [
                "setoption name EvalFile value network.pnue",
                "setoption name UseNNUE value true",
            ],
        )
        self.assertEqual(
            sent[2:],
            [
                "setoption name Hash value 64",
                "setoption name MultiPV value 1",
                "isready",
            ],
        )

    def test_wrong_model_never_enables_nnue(self):
        engine = self.engine(
            ["info string EvalFile loaded architecture prophet-nnue-v1 model wrong"]
        )
        with self.assertRaises(UciError):
            engine.configure(Path("network.pnue"), "expected")
        engine.send.assert_called_once_with(
            "setoption name EvalFile value network.pnue"
        )

    def test_baseline_receives_no_nnue_options(self):
        engine = self.engine(["readyok"])
        engine.configure()
        self.assertFalse(
            any(
                "NNUE" in call.args[0] or "EvalFile" in call.args[0]
                for call in engine.send.call_args_list
            )
        )

    def test_probe_stops_on_configuration_failure(self):
        with patch("scripts.uci_client.UciEngine") as create:
            engine = create.return_value
            engine.configure.side_effect = UciError("load failed")
            with self.assertRaises(UciError):
                probe_engine(Path("candidate"), Path("network.pnue"))
            engine.terminate.assert_called_once()

    def test_benchmark_alternates_order_and_configures_only_candidate(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("baseline", "candidate"):
                (root / name).write_bytes(name.encode())
            (root / "network.pnue").write_bytes(b"network")
            digest = hashlib.sha256(b"network").hexdigest()
            (root / "network.pnue.json").write_text(
                json.dumps(
                    {
                        "architecture_id": "prophet-nnue-v1",
                        "pnue_sha256": digest,
                    }
                )
            )
            config = {
                "candidate_nnue": {"file": "network.pnue", "sha256": digest},
                "benchmark": {
                    "positions": [{"id": 0, "fen": "fen"}],
                    "speed_budget_ratio": 0.97,
                    "profiles": {"standard": {"depth": 12, "samples": 2}},
                },
            }
            (root / "config.json").write_text(json.dumps(config))
            argv = [
                "bench",
                "--baseline",
                str(root / "baseline"),
                "--candidate",
                str(root / "candidate"),
                "--config",
                str(root / "config.json"),
                "--candidate-root",
                str(root),
                "--profiles",
                "standard",
                "--output",
                str(root / "result.json"),
            ]
            sample = {
                "complete": True,
                "nps": 100,
                "positions": [],
                "failed_positions": [],
            }
            with (
                patch("sys.argv", argv),
                patch.object(bench, "probe_engine"),
                patch.object(bench, "run_sample", return_value=sample) as run,
            ):
                self.assertEqual(bench.main(), 0)
            self.assertEqual(
                [call.args[0] for call in run.call_args_list],
                ["baseline", "candidate", "candidate", "baseline"],
            )
            for call in run.call_args_list:
                self.assertEqual(
                    call.args[7],
                    (root / "network.pnue").resolve()
                    if call.args[0] == "candidate"
                    else None,
                )
            result = json.loads((root / "result.json").read_text())
            self.assertEqual(result["candidate_nnue"]["sha256"], digest)

    def test_benchmark_excludes_startup_from_search_timing(self):
        with patch.object(bench, "UciEngine") as create:
            engine = create.return_value
            engine.expect.return_value = (
                "bestmove e2e4",
                ["info depth 2 multipv 1 score cp 1 nodes 100 time 900 pv e2e4"],
            )
            with patch.object(bench.time, "monotonic", side_effect=[0, 10, 10, 11]):
                result = bench.run_position(
                    Path("candidate"),
                    {"id": 0, "fen": "fen"},
                    2,
                    30,
                    "test",
                    Path("network.pnue"),
                    "model",
                )
            engine.configure.assert_called_once_with(Path("network.pnue"), "model")
            self.assertEqual(result["startup_elapsed_ms"], 10000)
            self.assertEqual(result["elapsed_ms"], 1000)
            self.assertEqual(result["nps"], 100)


if __name__ == "__main__":
    unittest.main()
