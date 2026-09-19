#!/usr/bin/env python3
"""Replay must reject partial identity matches."""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
from prove_ss02 import IDENTIFYING_EXPECT, LIVE_EXPECT, checkpoint_matches  # noqa: E402


def test_live_requires_full_identity():
    snap = dict(LIVE_EXPECT)
    assert checkpoint_matches(snap, LIVE_EXPECT)
    snap["UID"] = LIVE_EXPECT["UID"][:-1] + "0"
    assert checkpoint_matches(snap, LIVE_EXPECT) is False
    snap = dict(LIVE_EXPECT)
    snap["Build"] = "deadbeef"
    assert checkpoint_matches(snap, LIVE_EXPECT) is False
    snap = dict(LIVE_EXPECT)
    snap["Source pin"] = "not-the-source"
    assert checkpoint_matches(snap, LIVE_EXPECT) is False
    snap = dict(LIVE_EXPECT)
    snap["Contract"] = "sr1.hop1.bins1.xover1"
    assert checkpoint_matches(snap, LIVE_EXPECT) is False


def test_identifying_is_all_or_nothing():
    snap = dict(IDENTIFYING_EXPECT)
    assert checkpoint_matches(snap, IDENTIFYING_EXPECT)
    snap["UID"] = LIVE_EXPECT["UID"]
    assert checkpoint_matches(snap, IDENTIFYING_EXPECT) is False
    snap = dict(IDENTIFYING_EXPECT)
    snap["Lane A RMS"] = "-28"
    assert checkpoint_matches(snap, IDENTIFYING_EXPECT) is False


if __name__ == "__main__":
    test_live_requires_full_identity()
    test_identifying_is_all_or_nothing()
    print("checkpoint_match_tests_ok")
