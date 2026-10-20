#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
from titan_transport import (  # noqa: E402
    EXPECTED_UID,
    FakeSerial,
    FramingError,
    ObserveDenied,
    Transport,
)

INFO = {
    "protocol": 1,
    "uid": EXPECTED_UID,
    "build": "abc",
    "source": "def",
    "contract": "sr24000.hop180.bins80.xover40",
    "clock_hz": 1000000000,
    "sequence": 9,
}


def feed_info(fake: FakeSerial, req: int = 1, uid: str | None = None, **extra):
    payload = dict(INFO)
    if uid:
        payload["uid"] = uid
    payload.update(extra)
    fake.feed_response(0, req, 9, json.dumps(payload).encode())


def test_complete_identity_and_wrong_suffix():
    fake = FakeSerial()
    feed_info(fake, 1)
    t = Transport(fake, allowed={1})
    got = t.info()
    assert got["info"]["uid"] == EXPECTED_UID
    fake = FakeSerial()
    feed_info(fake, 1, uid=EXPECTED_UID[:-1] + "0")
    t = Transport(fake, allowed={1})
    got = t.info()
    from titan_snapshot import bind_identity

    assert bind_identity(got["info"])["ok"] is False


def test_timeout_and_wrong_transaction():
    fake = FakeSerial()
    t = Transport(fake, allowed={1})
    try:
        t.transact(1, timeout=0.05)
        raise AssertionError("timeout must fail")
    except FramingError:
        pass
    fake = FakeSerial()
    fake.feed_response(0, 99, 1, json.dumps(INFO).encode())
    t = Transport(fake, allowed={1})
    try:
        t.transact(1, timeout=0.2)
        raise AssertionError("wrong req must fail")
    except FramingError:
        assert t.quarantine


def test_bad_crc_quarantines():
    fake = FakeSerial()
    fake.feed_response(0, 1, 1, json.dumps(INFO).encode(), bad_body_crc=True)
    t = Transport(fake, allowed={1})
    try:
        t.transact(1, timeout=0.2)
        raise AssertionError("bad crc must fail")
    except FramingError:
        assert t.quarantine


def test_forbidden_opcode_does_not_write():
    fake = FakeSerial()
    t = Transport(fake, allowed={1, 6, 17})
    for op in (2, 3, 7, 8, 11, 16, 20, 22):
        before = len(t.writes)
        try:
            t.transact(op)
            raise AssertionError(f"op {op} must be denied")
        except ObserveDenied:
            assert len(t.writes) == before


def test_campaign_payload_ops_require_lease():
    fake = FakeSerial()
    t = Transport(fake, allowed={1, 24, 25, 26}, campaign=False)
    for op in (15, 24, 25, 26):
        try:
            Transport(fake, allowed={1, op}, campaign=False).allow({1, op})
            raise AssertionError(f"op {op} must require campaign")
        except ObserveDenied:
            pass
    t = Transport(fake, allowed={1}, campaign=True)
    t.allow({1, 23, 24, 25, 26})
    before = len(t.writes)
    try:
        t.transact(24, b"")
    except FramingError:
        pass
    assert len(t.writes) > before


def test_partial_reads_succeed():
    fake = FakeSerial(chunk=1)
    feed_info(fake, 1)
    t = Transport(fake, allowed={1})
    got = t.info(timeout=2)
    assert got["ok"]


def test_coalesced_two_responses():
    fake = FakeSerial(chunk=64)
    fake.feed_response(0, 1, 1, json.dumps(INFO).encode())
    fake.feed_response(0, 2, 2, json.dumps(INFO).encode())
    t = Transport(fake, allowed={1})
    a = t.info(timeout=2)
    b = t.info(timeout=2)
    assert a["req"] == 1 and b["req"] == 2


if __name__ == "__main__":
    test_complete_identity_and_wrong_suffix()
    test_timeout_and_wrong_transaction()
    test_bad_crc_quarantines()
    test_forbidden_opcode_does_not_write()
    test_campaign_payload_ops_require_lease()
    test_partial_reads_succeed()
    test_coalesced_two_responses()
    print("ss03_transport_ok")
