#!/usr/bin/env python3
from __future__ import annotations

import json
import socket
import sys
import tempfile
import threading
import time
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
from titan_broker import BIND, Broker  # noqa: E402
from titan_snapshot import ORIGIN_LIVE, ORIGIN_REPLAY  # noqa: E402
from titan_transport import EXPECTED_UID, FakeSerial  # noqa: E402


def test_payload_never_writes_serial():
    evidence = Path(tempfile.mkdtemp(prefix="ss03-broker-"))
    broker = Broker(evidence, replay_only=True)
    spy = FakeSerial()
    broker.backend = spy
    thread = threading.Thread(target=broker.loop, daemon=True)
    thread.start()
    time.sleep(0.2)
    client = socket.create_connection(BIND, timeout=2)
    client.sendall(b"K1S1" + b"\x00" * 28)
    time.sleep(0.4)
    client.close()
    deadline = time.monotonic() + 2
    while time.monotonic() < deadline and broker.publisher.rejects == 0:
        time.sleep(0.05)
    broker.stop = True
    thread.join(timeout=2)
    broker.shutdown()
    assert broker.publisher.rejects >= 1
    assert spy.tx == b""
    assert broker.hw_writes == 0
    assert broker.serial_opens == 0


def test_replay_only_origin():
    evidence = Path(tempfile.mkdtemp(prefix="ss03-replay-"))
    broker = Broker(evidence, replay_only=True)
    line = broker.publish_row(ORIGIN_REPLAY)
    assert line
    assert line.split(",")[0] == ORIGIN_REPLAY
    assert line.split(",")[1] == "REPLAY"
    broker.shutdown()


def test_stale_device_age_advances_without_new_sample():
    evidence = Path(tempfile.mkdtemp(prefix="ss03-age-"))
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
    broker.sample_mono = time.monotonic() - 3.0
    row = broker.current_row(ORIGIN_LIVE)
    assert row["device_age_ms"] >= 2000
    broker.shutdown()


def test_display_sequence_does_not_clear_freeze():
    evidence = Path(tempfile.mkdtemp(prefix="ss03-freeze-"))
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
    broker.last_metrics = {"pdm_target": {"ap_hops": 12, "measured_hz": 24000, "rate_locked": 1}}
    broker.last_progress = 12
    broker.same_progress = 8
    broker.sample_mono = time.monotonic()
    a = broker.current_row()
    b = broker.current_row()
    assert int(b["sequence"]) > int(a["sequence"])
    assert a["frozen_device"] == 1 and b["frozen_device"] == 1
    assert a["device_progress"] == 12
    broker.shutdown()


if __name__ == "__main__":
    test_payload_never_writes_serial()
    test_replay_only_origin()
    test_stale_device_age_advances_without_new_sample()
    test_display_sequence_does_not_clear_freeze()
    print("ss03_broker_ok")
