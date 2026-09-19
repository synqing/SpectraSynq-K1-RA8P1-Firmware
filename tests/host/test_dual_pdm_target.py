from __future__ import annotations

import copy
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from run_dual_pdm_target import validate_progress, validate_snapshot  # noqa: E402


def snapshot(slots: int = 20) -> dict:
    lanes = []
    identities = [
        ("U14", "LOW", "RISE", 2, 0, "programme", 101),
        ("U13", "HIGH", "FALL", 0, 1, "measurement", 202),
    ]
    for microphone, select, edge, channel, dma, role, sample_hash in identities:
        lanes.append({
            "microphone": microphone,
            "select": select,
            "edge": edge,
            "pdm_channel": channel,
            "dma_channel": dma,
            "role": role,
            "data_callbacks": slots + 1,
            "error_callbacks": 0,
            "error_flags": 0,
            "processed_slots": slots,
            "processed_samples": slots * 120,
            "sample_hash": sample_hash,
            "sample_min": -12,
            "sample_max": 14,
            "sample_peak": 14,
            "sample_square_sum": 5000,
            "overflow_events": 0,
            "drop_events": 0,
            "recovery_count": 0,
        })
    return {
        "heap_used": 40640,
        "pdm_target": {
            "mpn": "LMD2718T261-OA1",
            "profile": "diagnostic_16k",
            "sample_rate_hz": 16000,
            "working_source_sample_rate_hz": 12800,
            "sample_rate_match": False,
            "slot_elements": 120,
            "slot_duration_us": 7500,
            "shared_clock_and_data": True,
            "programme_lane": 0,
            "initialised": True,
            "running": True,
            "last_fsp_error": 0,
            "rearm_denied": 0,
            "paired_slots": slots + 1,
            "pair_skew_drops": 0,
            "startup_discard_pairs": 1,
            "max_pair_skew_us": 4,
            "lanes": lanes,
        },
    }


def test_valid_dual_capture_and_progress_accept():
    before = snapshot(20)
    after = snapshot(420)
    assert validate_snapshot(before) == []
    assert validate_progress(before, after) == []


def test_zero_or_duplicated_lane_rejects():
    observed = snapshot()
    observed["pdm_target"]["lanes"][1]["sample_peak"] = 0
    observed["pdm_target"]["lanes"][1]["sample_square_sum"] = 0
    observed["pdm_target"]["lanes"][1]["sample_min"] = 0
    observed["pdm_target"]["lanes"][1]["sample_max"] = 0
    observed["pdm_target"]["lanes"][1]["sample_hash"] = 101
    errors = validate_snapshot(observed)
    assert any("no non-zero microphone signal" in error for error in errors)
    assert any("identical sample hashes" in error for error in errors)


def test_lane_swap_rate_lie_and_capture_fault_reject():
    observed = snapshot()
    observed["pdm_target"]["sample_rate_match"] = True
    observed["pdm_target"]["lanes"][0], observed["pdm_target"]["lanes"][1] = (
        observed["pdm_target"]["lanes"][1], observed["pdm_target"]["lanes"][0]
    )
    observed["pdm_target"]["lanes"][0]["overflow_events"] = 1
    errors = validate_snapshot(observed)
    assert any("must not claim" in error for error in errors)
    assert any("identity/role contract changed" in error for error in errors)
    assert any("overflow_events" in error for error in errors)


def test_no_progress_or_heap_growth_rejects():
    before = snapshot(20)
    after = copy.deepcopy(before)
    after["heap_used"] += 32
    errors = validate_progress(before, after)
    assert any("did not advance" in error for error in errors)
    assert "heap usage changed during bounded capture" in errors
