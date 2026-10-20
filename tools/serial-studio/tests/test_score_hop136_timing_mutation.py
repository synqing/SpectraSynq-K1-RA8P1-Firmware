#!/usr/bin/env python3
"""Host negatives for hop-136 five-millisecond injection detection."""
from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parents[1] / "host_scorers"
sys.path.insert(0, str(HERE))
from score_hop136_timing_mutation import detect_hop136_injection  # noqa: E402

CLOCK = 1_000_000_000  # convenient: 5 ms == 5_000_000 cycles
MIN_CYCLES = CLOCK // 200


def write_trace(path: Path, rows: list) -> None:
    with path.open("w") as handle:
        for row in rows:
            handle.write(json.dumps(row) + "\n")


class Hop136DetectionTests(unittest.TestCase):
    def test_detects_valid_injection(self):
        with tempfile.TemporaryDirectory() as tmp:
            raw = Path(tmp) / "raw-stage-trace.jsonl"
            rows = [{"hop": i, "injected_delay_cycles": 0, "timing_mutation": False} for i in range(140)]
            rows[136] = {
                "hop": 136,
                "injected_delay_cycles": MIN_CYCLES,
                "timing_mutation": True,
            }
            write_trace(raw, rows)
            result = detect_hop136_injection(raw, CLOCK)
            self.assertTrue(result["ok"], result)
            self.assertEqual(result["code"], "DETECTED")

    def test_misses_when_injection_absent(self):
        with tempfile.TemporaryDirectory() as tmp:
            raw = Path(tmp) / "raw-stage-trace.jsonl"
            rows = [{"hop": i, "injected_delay_cycles": 0, "timing_mutation": False} for i in range(140)]
            write_trace(raw, rows)
            result = detect_hop136_injection(raw, CLOCK)
            self.assertFalse(result["ok"])
            self.assertEqual(result["code"], "INJECTION_COUNT")

    def test_fails_wrong_hop(self):
        with tempfile.TemporaryDirectory() as tmp:
            raw = Path(tmp) / "raw-stage-trace.jsonl"
            rows = [{"hop": i, "injected_delay_cycles": 0, "timing_mutation": False} for i in range(140)]
            rows[50] = {"hop": 50, "injected_delay_cycles": MIN_CYCLES, "timing_mutation": True}
            write_trace(raw, rows)
            result = detect_hop136_injection(raw, CLOCK)
            self.assertFalse(result["ok"])
            self.assertEqual(result["code"], "WRONG_HOP")

    def test_fails_too_short(self):
        with tempfile.TemporaryDirectory() as tmp:
            raw = Path(tmp) / "raw-stage-trace.jsonl"
            rows = [{"hop": i, "injected_delay_cycles": 0, "timing_mutation": False} for i in range(140)]
            rows[136] = {"hop": 136, "injected_delay_cycles": MIN_CYCLES - 1, "timing_mutation": True}
            write_trace(raw, rows)
            result = detect_hop136_injection(raw, CLOCK)
            self.assertFalse(result["ok"])
            self.assertEqual(result["code"], "TOO_SHORT")

    def test_existing_non_mutation_trace_must_fail_detector(self):
        existing = Path(
            "/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/"
            "k1-ra8p1-002/g4-empty-tcm-raw-hops-run-20260920-01/raw-stage-trace.jsonl"
        )
        if not existing.is_file():
            self.skipTest("existing raw trace absent")
        result = detect_hop136_injection(existing, 480_000_000)
        self.assertFalse(result["ok"], "non-mutation trace must not pass hop-136 detector")


if __name__ == "__main__":
    unittest.main(verbosity=2)
