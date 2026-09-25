#!/usr/bin/env python3
"""SS-03 host Historian binding + stale-to-fresh recovery. Never opens CDC."""
from __future__ import annotations

import hashlib
import json
import sqlite3
import sys
import tempfile
import threading
import time
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))

from titan_broker import Broker  # noqa: E402
from titan_snapshot import ORIGIN_LIVE, ORIGIN_REPLAY  # noqa: E402
from titan_transport import EXPECTED_UID  # noqa: E402

SS02_PROJ = HERE / "titan.ssproj"
SS03_LIVE_PROJ = HERE / "titan-live.ssproj"
SS02_HISTORIAN = Path(
    "/Users/spectrasynq/Documents/Serial Studio/Session Databases/"
    "Titan Mini Observability/Titan Mini Observability.db"
)
SS03_LIVE_HISTORIAN = Path(
    "/Users/spectrasynq/Documents/Serial Studio/Session Databases/"
    "Titan Mini Live Observability/Titan Mini Live Observability.db"
)
SS02_PROOF = HERE / "ss-02-proofs" / "ss-02-proof-run2-b9a8562e4259a741.json"


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def test_ss02_current_manifest_matches_historian_session():
    """Retained SS-02 Historian session is bound to the current titan.ssproj."""
    assert SS02_PROJ.is_file(), f"missing current SS-02 manifest: {SS02_PROJ}"
    assert SS02_HISTORIAN.is_file(), f"missing Historian DB: {SS02_HISTORIAN}"
    file_sha = _sha256(SS02_PROJ)
    assert file_sha == "b9a8562e4259a741bb9e2261ee6826d9df73d85d72ce01e57f8237d4848d3a84"
    live = json.loads(SS02_PROJ.read_text())
    con = sqlite3.connect(str(SS02_HISTORIAN))
    try:
        row = con.execute(
            "SELECT session_id, project_json FROM sessions WHERE session_id = 14"
        ).fetchone()
        assert row is not None, "session 14 missing from Historian"
        session_id, project_json = row
        hist = json.loads(project_json)
        assert json.dumps(live, sort_keys=True) == json.dumps(hist, sort_keys=True)
        raw_count = con.execute(
            "SELECT COUNT(*) FROM raw_bytes WHERE session_id = ?", (session_id,)
        ).fetchone()[0]
    finally:
        con.close()
    proof = json.loads(SS02_PROOF.read_text())
    assert proof["saved_project_sha256"] == file_sha
    assert proof["recorded_session"]["session_id"] == 14
    assert proof["replay"]["replaying"] is True
    assert raw_count >= proof["recorded_session"]["raw_count"]
    # Offline field presence required for actual Historian replay assets.
    con = sqlite3.connect(str(SS02_HISTORIAN))
    try:
        blobs = [
            r[0]
            for r in con.execute(
                "SELECT data FROM raw_bytes WHERE session_id = 14 ORDER BY raw_id"
            )
        ]
    finally:
        con.close()
    joined = b"\n".join(blobs)
    assert b"UNIDENTIFIED" in joined or b"IDENTIFYING" in joined
    assert EXPECTED_UID.encode() in joined
    assert b"SIMULATED" in joined


def test_ss03_live_historian_not_bound_to_current_live_manifest():
    """Document the exact live-manifest Historian gap without claiming SS-03 closed."""
    assert SS03_LIVE_PROJ.is_file(), f"missing current SS-03 live manifest: {SS03_LIVE_PROJ}"
    live_sha = _sha256(SS03_LIVE_PROJ)
    assert live_sha == "aa96c9eae69cac05056ee113488221def2c6e6206de70a85ea70ef4c1f1decf8"
    assert SS03_LIVE_HISTORIAN.is_file(), f"missing live Historian DB: {SS03_LIVE_HISTORIAN}"
    live = json.loads(SS03_LIVE_PROJ.read_text())
    con = sqlite3.connect(str(SS03_LIVE_HISTORIAN))
    try:
        row = con.execute(
            "SELECT session_id, project_json FROM sessions ORDER BY session_id DESC LIMIT 1"
        ).fetchone()
        assert row is not None
        hist = json.loads(row[1])
        readings = con.execute("SELECT COUNT(*) FROM readings").fetchone()[0]
    finally:
        con.close()
    # Current live manifest has drifted from the only stored live Historian session.
    assert json.dumps(live, sort_keys=True) != json.dumps(hist, sort_keys=True)
    assert live.get("nextUniqueId") != hist.get("nextUniqueId")
    # No numeric readings available for field-for-field live replay.
    assert readings == 0


def test_stale_to_fresh_recovery_without_cdc():
    """Restart / stale-to-fresh recovery on the host broker path (replay-only)."""
    evidence = Path(tempfile.mkdtemp(prefix="ss03-stale-fresh-"))
    broker = Broker(evidence, replay_only=True)
    broker.bound = {
        "ok": True,
        "uid": EXPECTED_UID,
        "build": "b",
        "source": "s",
        "contract": "sr24000.hop180.bins80.xover40",
        "hop_samples": 180,
        "admitted_rate_hz": 24000,
    }
    broker.epoch = 1
    broker.sample_mono = time.monotonic() - 5.0
    stale = broker.current_row(ORIGIN_LIVE)
    assert stale["device_age_ms"] >= 2000
    # Fresh sample after "restart" of observation clock.
    broker.sample_mono = time.monotonic()
    broker.last_metrics = {"pdm_target": {"ap_hops": 20, "measured_hz": 24000, "rate_locked": 1}}
    broker.last_progress = 20
    broker.same_progress = 0
    fresh = broker.current_row(ORIGIN_LIVE)
    assert fresh["device_age_ms"] < 500
    # Replay origin stays distinct after recovery.
    replay_line = broker.publish_row(ORIGIN_REPLAY)
    assert replay_line.split(",")[0] == ORIGIN_REPLAY
    broker.shutdown()


def test_replay_only_broker_survives_restart_flag_cycle():
    """Broker restart recovery: stop/start replay-only without touching hardware."""
    evidence = Path(tempfile.mkdtemp(prefix="ss03-broker-restart-"))
    first = Broker(evidence, replay_only=True)
    first.bound = {
        "ok": True,
        "uid": EXPECTED_UID,
        "build": "b",
        "source": "s",
        "contract": "sr24000.hop180.bins80.xover40",
        "hop_samples": 180,
        "admitted_rate_hz": 24000,
    }
    first.epoch = 7
    line = first.publish_row(ORIGIN_REPLAY)
    assert "REPLAY" in line
    first.shutdown()
    second = Broker(evidence, replay_only=True)
    second.bound = first.bound
    second.epoch = first.epoch
    thread = threading.Thread(target=second.loop, daemon=True)
    thread.start()
    time.sleep(0.2)
    second.stop = True
    thread.join(timeout=2)
    second.shutdown()
    assert second.hw_writes == 0
    assert second.serial_opens == 0


if __name__ == "__main__":
    test_ss02_current_manifest_matches_historian_session()
    test_ss03_live_historian_not_bound_to_current_live_manifest()
    test_stale_to_fresh_recovery_without_cdc()
    test_replay_only_broker_survives_restart_flag_cycle()
    print("ss03_historian_host_ok")
