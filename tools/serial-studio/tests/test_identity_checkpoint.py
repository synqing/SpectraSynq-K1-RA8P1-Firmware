#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
from titan_snapshot import EXPECTED_UID, bind_identity, identity_identified, identity_ok  # noqa: E402


def _info(build: str) -> dict:
    return {
        "protocol": 1,
        "uid": EXPECTED_UID,
        "build": build,
        "source": "6b1e7bc5c9f9871e6ea4e900455bcb37d756304a",
        "contract": "sr24000.hop180.bins80.xover40",
        "clock_hz": 1000000000,
        "sequence": 1,
    }


def test_unbound_builds_are_identified_not_accepted():
    for build in ("superseded-unbound-build", "new-unverified-build"):
        identified, identified_reason = identity_identified(_info(build))
        ok, reason = identity_ok(_info(build))
        assert identified and identified_reason == "identified"
        assert not ok
        assert reason == "no accepted checkpoint bound"


def test_checkpoint_match_and_mismatch():
    expected = "c7f6034a902833e3f8a17f7c5792f90990f647bccfcf7f0cd211036ebaba824c"
    checkpoint = {"build": expected, "uid": EXPECTED_UID}
    ok, reason = identity_ok(_info(expected), checkpoint=checkpoint)
    assert ok and reason == "ok"
    ok, reason = identity_ok(_info("superseded-unbound-build"), checkpoint=checkpoint)
    assert not ok
    assert "not accepted" in reason


def test_bind_keeps_unaccepted_observation_truthful():
    bound = bind_identity(_info("new-unverified-build"), checkpoint={"build": "accepted-build"})
    assert bound["identified"] is True
    assert bound["ok"] is False
    assert bound["accepted"] is False
    assert bound["build"] == "new-unverified-build"


if __name__ == "__main__":
    test_unbound_builds_are_identified_not_accepted()
    test_checkpoint_match_and_mismatch()
    test_bind_keeps_unaccepted_observation_truthful()
    print("identity_checkpoint_ok")
