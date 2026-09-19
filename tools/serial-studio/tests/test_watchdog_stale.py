#!/usr/bin/env python3
"""Watchdog must go stale on missing/invalid age without rewriting last_valid."""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
from setup_titan_observability import CONTROL  # noqa: E402

DECIDE = r"""
function decide(hasData, ageMs, staleMs) {
  var has = !!hasData;
  var ageRaw = ageMs;
  var age = Number(ageRaw);
  var validAge = has && ageRaw !== null && ageRaw !== undefined && ageRaw !== "" && isFinite(age) && age >= 0;
  if (validAge)
    return {writeTimestamp: true, hostStale: age > staleMs ? 1 : 0};
  return {writeTimestamp: false, hostStale: 1};
}
"""


def decide(has_data, age_ms, stale_ms=2000):
    runner = (
        DECIDE
        + f"console.log(JSON.stringify(decide({json.dumps(has_data)}, {json.dumps(age_ms)}, {stale_ms})));"
    )
    proc = subprocess.run(["node", "-"], input=runner, text=True, capture_output=True, check=False)
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr)
    return json.loads(proc.stdout)


def test_healthy_age_stays_fresh():
    got = decide(True, 100)
    assert got == {"writeTimestamp": True, "hostStale": 0}


def test_old_age_is_stale_and_updates_timestamp():
    got = decide(True, 3000)
    assert got == {"writeTimestamp": True, "hostStale": 1}


def test_missing_frame_sets_stale_without_timestamp_write():
    got = decide(False, 100)
    assert got["writeTimestamp"] is False
    assert got["hostStale"] == 1


def test_invalid_age_sets_stale_without_timestamp_write():
    assert decide(True, None)["hostStale"] == 1
    assert decide(True, None)["writeTimestamp"] is False
    assert decide(True, "nope")["hostStale"] == 1
    assert decide(True, -5)["writeTimestamp"] is False
    assert decide(True, -5)["hostStale"] == 1


def test_control_script_contains_explicit_stale_else():
    assert "validAge" in CONTROL
    assert 'tableSet("titan_watchdog", "host_stale", 1)' in CONTROL
    assert "refreshDashboard" in CONTROL


if __name__ == "__main__":
    test_healthy_age_stays_fresh()
    test_old_age_is_stale_and_updates_timestamp()
    test_missing_frame_sets_stale_without_timestamp_write()
    test_invalid_age_sets_stale_without_timestamp_write()
    test_control_script_contains_explicit_stale_else()
    print("watchdog_stale_tests_ok")
