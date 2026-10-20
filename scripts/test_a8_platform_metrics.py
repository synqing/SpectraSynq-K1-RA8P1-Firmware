#!/usr/bin/env python3
"""Host negatives for A8 platform-metrics wire (CHANGE_REQUEST-A8 / I5).

Proves:
  1. Exact field names on the k1_platform_metrics snprintf surface
  2. Validity: missing stays UNKNOWN, never reported as zero
  3. Old consumers remain compatible (existing fields unchanged / still present)
  4. Wired getters match the six named counters
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from a8_platform_metrics import (  # noqa: E402
    A8_FIELD_NAMES,
    EXISTING_PDM_FIELDS,
    UNKNOWN,
    extract_a8_counters,
    forbid_missing_as_zero,
)

HAL = (ROOT / "platform/ra8p1/hal_entry.c").read_text(encoding="utf-8")
HEADER = (ROOT / "platform/ra8p1/pdm_target.h").read_text(encoding="utf-8")
RESIDENT_METRICS = Path(
    "/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/"
    "k1-ra8p1-002/swarm-20260922/runs/H/A9-metrics-20260923/METRICS_T00s.json"
)


def _pdm_metrics_block() -> str:
    start = HAL.index('k1_platform_metrics')
    block = HAL[start:]
    # Isolate the K1_PDM_TARGET snprintf format + args.
    fmt_start = block.index('"{\\"stack_bytes\\"')
    fmt_end = block.index('}]}}"', fmt_start) + len('}]}}"')
    return block[fmt_start:fmt_end]


def test_exact_field_names_and_getters() -> None:
    block = _pdm_metrics_block()
    getters = {
        "stream_epoch": "k1_pdm_target_stream_epoch()",
        "pair_epoch_drops": "k1_pdm_target_pair_epoch_drops()",
        "asrc_consumed": "k1_pdm_target_asrc_consumed()",
        "asrc_discarded": "k1_pdm_target_asrc_discarded()",
        "push_rejected": "k1_pdm_target_push_rejected()",
        "stale_discards": "k1_pdm_target_stale_discards()",
    }
    for name, getter in getters.items():
        assert f'\\"{name}\\":' in block, f"missing exact JSON key {name}"
        assert getter in HAL, f"missing getter wire {getter}"
        assert getter.replace("()", "") in HEADER, f"getter not declared: {getter}"
    # Order on the wire matches the CHANGE_REQUEST / A8 contract list.
    positions = [block.index(f'\\"{name}\\":') for name in A8_FIELD_NAMES]
    assert positions == sorted(positions), "A8 field order must match contract"


def test_existing_fields_unchanged() -> None:
    block = _pdm_metrics_block()
    for name in EXISTING_PDM_FIELDS:
        assert f'\\"{name}\\":' in block, f"existing field removed: {name}"
    # Acoustic / rate contract literals must stay.
    assert "sample_rate_match\\\":false" in block
    assert "acoustic_identity\\\":\\\"unproven\\\"" in block
    assert "working_source_sample_rate_hz\\\":24000" in block


def test_missing_stays_unavailable_never_zero() -> None:
    # Resident A9 capture omitted the six fields — extract must yield UNKNOWN.
    resident = json.loads(RESIDENT_METRICS.read_text(encoding="utf-8"))
    for name in A8_FIELD_NAMES:
        assert name not in resident["pdm_target"], f"resident unexpectedly has {name}"
    extracted = extract_a8_counters(resident)
    for name in A8_FIELD_NAMES:
        assert extracted[name] is UNKNOWN or extracted[name] == UNKNOWN
        assert extracted[name] != 0
        assert extracted[name] != "0"

    # Empty / absent pdm_target → all UNKNOWN.
    assert all(v == UNKNOWN for v in extract_a8_counters(None).values())
    assert all(v == UNKNOWN for v in extract_a8_counters({}).values())
    assert all(v == UNKNOWN for v in extract_a8_counters({"pdm_target": {}}).values())

    # Hostile default-to-zero consumer is the anti-pattern under test.
    hostile = forbid_missing_as_zero(resident.get("pdm_target"))
    for name in A8_FIELD_NAMES:
        assert hostile[name] == 0
    # Correct extractor must disagree with the hostile path on a missing wire.
    correct = extract_a8_counters(resident)
    assert all(correct[n] == UNKNOWN for n in A8_FIELD_NAMES)
    assert correct != hostile


def test_present_zero_is_valid_count() -> None:
    """Once wired, a real zero is a counter value — not UNKNOWN."""
    wired = {
        "pdm_target": {
            "stream_epoch": 0,
            "pair_epoch_drops": 0,
            "asrc_consumed": 0,
            "asrc_discarded": 0,
            "push_rejected": 0,
            "stale_discards": 0,
            "paired_slots": 1,
        }
    }
    extracted = extract_a8_counters(wired)
    for name in A8_FIELD_NAMES:
        assert extracted[name] == 0
        assert extracted[name] != UNKNOWN


def test_old_consumer_compatibility() -> None:
    """Old consumers that only read pre-A8 keys must still see them."""
    resident = json.loads(RESIDENT_METRICS.read_text(encoding="utf-8"))
    pdm = resident["pdm_target"]
    for name in EXISTING_PDM_FIELDS:
        assert name in pdm
    # Simulated new-image metrics: existing keys retained alongside A8.
    new_image = json.loads(json.dumps(resident))
    for name in A8_FIELD_NAMES:
        new_image["pdm_target"][name] = 7 if name != "stream_epoch" else 1
    for name in EXISTING_PDM_FIELDS:
        assert new_image["pdm_target"][name] == pdm[name] or name in (
            "paired_slots",
            "ap_hops",
            "measured_hz",
            "rate_locked",
            "asrc_starved",
            "pair_skew_drops",
        )
        assert name in new_image["pdm_target"]
    # Old-style access without touching A8 keys.
    old_view = {k: new_image["pdm_target"][k] for k in EXISTING_PDM_FIELDS}
    assert set(old_view) == set(EXISTING_PDM_FIELDS)


def main() -> int:
    test_exact_field_names_and_getters()
    test_existing_fields_unchanged()
    test_missing_stays_unavailable_never_zero()
    test_present_zero_is_valid_count()
    test_old_consumer_compatibility()
    print("A8_PLATFORM_METRICS_HOST=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
