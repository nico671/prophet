import unittest
from pathlib import Path
from unittest.mock import patch

from scripts.bench import paired_comparison, partial_standard_result, run_sample
from scripts.uci_client import UciError


class BenchmarkSampleTest(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
