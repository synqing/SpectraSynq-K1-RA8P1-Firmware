#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
from titan_decode import ORIGIN_LIVE, ORIGIN_REPLAY, decode, cells_to_csv  # noqa: E402
from titan_snapshot import bind_identity, encode, snapshot  # noqa: E402

UID = "545433931bd25436593630352d068363"
BUILD = "c65c6fcaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
SOURCE = "deadbeefdeadbeefdeadbeefdeadbeefdeadbeef"
CONTRACT = "sr24000.hop180.bins80.xover40"


def base(**extra):
    row = snapshot(
        origin=ORIGIN_LIVE,
        bound={
            "ok": True,
            "uid": UID,
            "build": BUILD,
            "source": SOURCE,
            "contract": CONTRACT,
            "hop_samples": 180,
            "admitted_rate_hz": 24000,
        },
        epoch=1,
        display_seq=3,
        device_age_ms=40,
        device_progress=100,
        frozen=0,
        metrics={"pdm_target": {"measured_hz": 41410, "rate_locked": 1, "ap_hops": 100}},
        palette={"emit_errors": 0, "emitted": 50, "frames": 50},
    )
    row.update(extra)
    return row


def test_schema2_unchanged():
    from titan_decode import decode as d

    obj = {
        "schema": 2,
        "mode": 3,
        "identity_ok": 1,
        "protocol": 1,
        "simulated": 1,
        "health_valid": 1,
        "mic_a_valid": 1,
        "mic_b_valid": 1,
        "uid": UID,
        "build": "x",
        "source": "y",
        "contract": CONTRACT,
        "hop_max_us": 1,
        "late_starts": 0,
        "deadlines": 0,
        "crc_mismatches": 0,
        "dma_irqs": 1,
        "latched_frames": 1,
        "led_faults": 0,
        "mic_a_rms_dbfs": -28,
        "mic_b_rms_dbfs": -27,
        "capture_rate_hz": 24000,
        "hop_samples": 180,
        "admitted_rate_hz": 24000,
        "sequence": "1",
    }
    cells, _, _ = d(obj, "", 0)
    assert cells[0].startswith("SIMULATED")
    assert len(cells) == 27


def test_independent_unavailable():
    cells, _, _ = decode(base(), "", 0)
    assert cells is not None
    assert cells[8] == "UNAVAILABLE"
    assert cells[9] == "UNAVAILABLE"
    assert cells[13] == "UNAVAILABLE"
    assert cells[14] == "UNAVAILABLE"
    assert cells[17] == "OK"
    assert cells[12] == 41410
    assert cells[27] == ORIGIN_LIVE
    line = cells_to_csv(cells)
    assert "UNAVAILABLE" in line
    assert "," in line


def test_replay_overrides_live_state():
    row = base(origin=ORIGIN_REPLAY)
    cells, _, _ = decode(row, "", 0)
    assert cells[0] == ORIGIN_REPLAY
    assert cells[1] == "REPLAY"
    assert cells[28] == "HISTORICAL"


def test_identity_same_frame():
    info = {"protocol": 1, "uid": UID, "build": BUILD, "source": SOURCE, "contract": CONTRACT}
    unbound = bind_identity(info)
    assert unbound["identified"] is True
    assert unbound["ok"] is False
    matched = bind_identity(info, checkpoint={"build": BUILD, "uid": UID})
    assert matched["ok"] is True
    bad = dict(info)
    bad["uid"] = UID[:-1] + "0"
    assert bind_identity(bad, checkpoint={"build": BUILD, "uid": UID})["identified"] is False
    bad = dict(info)
    bad["build"] = "nope"
    rebound = bind_identity(bad, checkpoint={"build": BUILD, "uid": UID})
    assert rebound["identified"] is True
    assert rebound["ok"] is False
    bad = dict(info)
    bad["protocol"] = 2
    assert bind_identity(bad, checkpoint={"build": BUILD})["ok"] is False


def test_last_emit_cycles_is_not_hop_compute():
    emit_only = base()
    emit_only["last_emit_cycles"] = 3872400
    assert "hop_max_us" not in emit_only
    cells, _, _ = decode(emit_only, "", 0)
    assert cells[13] == "UNAVAILABLE"

    both = base()
    both["hop_max_us"] = 5600
    both["last_emit_cycles"] = 3872400
    cells, _, _ = decode(both, "", 0)
    assert cells[13] == 5600
    assert cells[13] != 3872400


def test_csv_rejects_comma():
    cells, _, _ = decode(base(), "", 0)
    cells[3] = "a,b"
    assert cells_to_csv(cells) == ""


if __name__ == "__main__":
    test_schema2_unchanged()
    test_independent_unavailable()
    test_replay_overrides_live_state()
    test_identity_same_frame()
    test_csv_rejects_comma()
    test_last_emit_cycles_is_not_hop_compute()
    print("ss03_decode_ok")
