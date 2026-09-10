from __future__ import annotations

import copy
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from run_pcm1808_target import validate_progress, validate_snapshot  # noqa: E402


def snapshot(hops: int = 20) -> dict:
    return {"pcm1808_target": {
        "contract": "k1-ra8p1-pcm1808-aux-v1",
        "peripheral": "SSIE1", "role": "slave_receiver", "transfer": "DTC",
        "bclk": "P702/U11-24", "lrck": "P701/U11-33", "data": "P700/U11-26",
        "nominal_input_hz": 48000, "canonical_hz": 12800,
        "hop_in": 360, "hop_out": 96, "mono": "MID", "trim_q15": 2048,
        "initialised": True, "running": True, "last_fsp_error": 0,
        "callbacks": hops + 1, "overflow_events": 0, "idle_events": 0,
        "measured_input_hz": 47986, "processed_hops": hops,
        "processed_samples": hops * 96, "sample_hash": hops * 101,
        "sample_min": -100, "sample_max": 120, "sample_peak": 120,
        "sample_square_sum": hops * 5000,
        "channel_map": "UNVALIDATED_ON_TITAN", "clock_gate": "DMA_RATE_ONLY",
        "physical_capture": "NOT_CLAIMED_BY_BUILD",
    }}


def test_valid_pcm1808_capture_and_progress_accept():
    assert validate_snapshot(snapshot()) == []
    assert validate_progress(snapshot(20), snapshot(420)) == []


def test_rate_error_overflow_and_zero_signal_reject():
    observed = snapshot()
    observed["pcm1808_target"].update(
        measured_input_hz=96000, overflow_events=1, sample_peak=0, sample_square_sum=0)
    errors = validate_snapshot(observed)
    assert any("48 kHz gate" in error for error in errors)
    assert any("overflowed" in error for error in errors)
    assert any("no non-zero" in error for error in errors)


def test_no_progress_or_route_drift_rejects():
    before = snapshot()
    after = copy.deepcopy(before)
    after["pcm1808_target"]["data"] = "P000/U18-11"
    errors = validate_progress(before, after)
    assert any("data changed" in error for error in errors)
    assert any("callbacks did not advance" in error for error in errors)
    assert any("hash did not change" in error for error in errors)
