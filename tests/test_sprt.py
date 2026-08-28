import unittest
from pathlib import Path

from scripts.sprt import fastchess_command, should_display


class FastchessCommandTest(unittest.TestCase):
    def test_candidate_is_the_first_engine(self) -> None:
        settings = {
            "concurrency": 1, "rounds": 2, "seed": 3, "time_control": "10+0.1",
            "openings": "testdata/openings.epd", "elo0": 0, "elo1": 5,
            "alpha": 0.05, "beta": 0.05,
        }
        command = fastchess_command("fastchess", settings, Path("baseline"),
                                    Path("candidate"), Path("output"))
        names = [item.removeprefix("name=") for item in command
                 if item.startswith("name=")]
        self.assertEqual(names, ["candidate", "baseline"])

    def test_display_filter_keeps_results_and_summaries(self) -> None:
        self.assertTrue(should_display("Finished game 3 (candidate vs baseline): 1-0\n"))
        self.assertTrue(should_display("Score of candidate vs baseline: 3 - 2 - 1\n"))
        self.assertTrue(should_display("LLR: 1.20 (-2.94, 2.94) [0.00, 5.00]\n"))
        self.assertTrue(should_display("SPRT ([0.00, 5.00]) completed - H1 was accepted\n"))
        self.assertFalse(should_display("Info; info depth 12 nodes 123\n"))
        self.assertFalse(should_display("Moves; e2e4 e7e5\n"))
        self.assertFalse(should_display("Position; fen rnbqkbnr\n"))


if __name__ == "__main__":
    unittest.main()
