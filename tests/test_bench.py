import unittest
from pathlib import Path
from unittest.mock import patch

from scripts.bench import paired_comparison, partial_standard_result, run_position, run_sample
from scripts.uci_client import UciError


class BenchmarkSampleTest(unittest.TestCase):
    def test_rejected_fen_never_starts_search(self) -> None:
        with patch("scripts.bench.UciEngine") as create:
            engine = create.return_value
            engine.expect.return_value = ("readyok", ["info string Failed to parse FEN: invalid", "readyok"])
            with self.assertRaisesRegex(UciError, "position rejected"):
                run_position(Path("candidate"), {"id": 20, "fen": "invalid"}, 12, 1, "test")
            self.assertEqual([call.args[0] for call in engine.send.call_args_list],
                             ["position fen invalid", "isready"])
            engine.terminate.assert_called_once()

    def test_null_bestmove_requires_zero_legal_moves(self) -> None:
        for legal_moves in (0, 3):
            with self.subTest(legal_moves=legal_moves), patch("scripts.bench.UciEngine") as create:
                engine = create.return_value
                engine.expect.side_effect = [
                    ("readyok", ["readyok"]),
                    ("bestmove 0000", ["bestmove 0000"]),
                    (f"perft depth 1 nodes {legal_moves} nps 0 time 0.000", []),
                ]
                args = ("candidate", Path("candidate"), [{"id": 47, "fen": "fen"}],
                        12, 1, "standard", 1)
                result = run_sample(*args)
                engine.send.assert_any_call("perft 1")
                self.assertNotIn("nps", result)
                self.assertEqual(result["positions"], [])
                if legal_moves:
                    self.assertIn("3 legal moves", result["failed_positions"][0]["error"])
                    self.assertEqual(result["no_move_positions"], [])
                else:
                    self.assertTrue(result["complete"])
                    self.assertEqual(result["no_move_positions"][0]["id"], 47)
                    self.assertEqual(result["failed_positions"], [])

    def test_engine_failure_does_not_stop_later_positions(self) -> None:
        positions = [{"id": 1}, {"id": 2}, {"id": 3}]

        def fake_run(_binary, position, *_args):
            if position["id"] == 2:
                raise UciError("timed out")
            return {"id": position["id"], "nodes": 100, "elapsed_ms": 10}

        with patch("scripts.bench.run_position", side_effect=fake_run) as mocked:
            result = run_sample("baseline", Path("baseline"), positions, 12, 1,
                                "standard", 1)

        self.assertEqual([call.args[1]["id"] for call in mocked.call_args_list], [1, 2, 3])
        self.assertEqual([item["id"] for item in result["positions"]], [1, 3])
        self.assertEqual(result["failed_positions"][0]["id"], 2)
        self.assertFalse(result["complete"])

    def test_partial_comparison_uses_only_shared_positions(self) -> None:
        baseline = {"positions": [{"id": 1, "nodes": 100, "elapsed_ms": 10}]}
        candidate = {"positions": [{"id": 1, "nodes": 200, "elapsed_ms": 10},
                                    {"id": 2, "nodes": 1, "elapsed_ms": 1}]}
        result = paired_comparison(baseline, candidate, 0.97)
        self.assertEqual(result["position_ids"], [1])
        self.assertTrue(result["passed"])

    def test_baseline_failure_can_pass_on_shared_positions(self) -> None:
        baseline = {"positions": [{"id": 1, "nodes": 100, "elapsed_ms": 10}],
                    "failed_positions": [{"id": 2, "error": "timed out"}]}
        candidate = {"positions": [{"id": 1, "nodes": 100, "elapsed_ms": 10},
                                     {"id": 2, "nodes": 100, "elapsed_ms": 10}],
                     "failed_positions": []}
        result = partial_standard_result([baseline], [candidate], 0.97)
        self.assertTrue(result["passed"])

    def test_candidate_failure_against_baseline_fails(self) -> None:
        baseline = {"positions": [{"id": 1, "nodes": 100, "elapsed_ms": 10}],
                    "failed_positions": []}
        candidate = {"positions": [],
                     "failed_positions": [{"id": 1, "error": "timed out"}]}
        result = partial_standard_result([baseline], [candidate], 0.97)
        self.assertFalse(result["passed"])
        self.assertEqual(result["candidate_failures_against_completed_baseline"][0]["id"], 1)

    def test_candidate_failure_on_baseline_terminal_position_fails(self) -> None:
        baseline = {"positions": [{"id": 1, "nodes": 100, "elapsed_ms": 10}],
                    "no_move_positions": [{"id": 47, "status": "no_move_available"}],
                    "failed_positions": []}
        candidate = {"positions": [{"id": 1, "nodes": 200, "elapsed_ms": 10}],
                     "no_move_positions": [],
                     "failed_positions": [{"id": 47, "error": "timed out"}]}
        result = partial_standard_result([baseline], [candidate], 0.97)
        self.assertTrue(result["partial_comparisons"][0]["passed"])
        self.assertFalse(result["passed"])
        self.assertEqual(result["candidate_failures_against_completed_baseline"],
                         candidate["failed_positions"])


if __name__ == "__main__":
    unittest.main()
