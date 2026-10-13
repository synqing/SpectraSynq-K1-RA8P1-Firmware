#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

import pytest

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


def test_bloom_witness_requires_exact_board_and_campaign():
    for campaign in (False, True):
        for board in (0, 1, 2):
            fake = FakeSerial()
            t = Transport(fake, allowed={17}, campaign=campaign)
            payload = b"BLM1" + board.to_bytes(4, "little")
            if campaign and board < 2:
                fake.feed_response(0, 1, 1, b"bounded-render-records")
                assert t.transact(17, payload)["body"] == b"bounded-render-records"
            else:
                try:
                    t.transact(17, payload)
                    raise AssertionError("unadmitted trace must be denied")
                except ObserveDenied:
                    assert not t.writes


def test_model_witness_admits_only_fixed_readonly_payload():
    for campaign in (False, True):
        fake = FakeSerial()
        t = Transport(fake, allowed={17}, campaign=campaign)
        for payload in (b"MLS", b"MLST\x00", b"mlst", b"MLSU", b"arbitrary"):
            try:
                t.transact(17, payload)
                raise AssertionError("non-witness payload must be denied")
            except ObserveDenied:
                assert not t.writes and t.next_req == 0
        fake.feed_response(0, 1, 1, b"numerical-witness")
        got = t.transact(17, b"MLST")
        assert got["body"] == b"numerical-witness" and len(t.writes) == 1
        # Opcode admission still applies to the fixed query.
        denied = Transport(FakeSerial(), allowed={1}, campaign=campaign)
        try:
            denied.transact(17, b"MLST")
            raise AssertionError("opcode admission must still be required")
        except ObserveDenied:
            assert not denied.writes


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


@pytest.mark.parametrize("serial_reader", [False, True])
@pytest.mark.parametrize("received_bytes", [0, 8, 35])
def test_read_timeout_quarantines_before_late_response_or_next_write(serial_reader, received_bytes):
    # A subclass takes the same run_led_smoke.read_exact path as real serial;
    # plain FakeSerial also exercises the transport's local reader.
    class SerialReader(FakeSerial):
        pass

    fake = (SerialReader if serial_reader else FakeSerial)(chunk=4096, timeout=0.001)
    fake.feed_response(0, 1, 0, b'{"generation":7}')
    response = bytes(fake.rx)
    fake.rx[:] = response[:received_bytes]
    transport = Transport(fake, allowed={1, 21, 25}, campaign=True)
    with pytest.raises(RuntimeError, match="timeout"):
        transport.transact(21, timeout=0.01)
    assert transport.quarantine
    assert transport.in_flight is None
    assert transport.next_req == 1
    sent = bytes(fake.tx)
    assert len(transport.writes) == 1

    # The old response may arrive after the deadline. No new request can be
    # sent or consume it on this uncertain stream, including an INFO request.
    fake.feed(response[received_bytes:])
    pending = bytes(fake.rx)
    for op in (25, 1):
        with pytest.raises(FramingError, match="quarantined until re-identify"):
            transport.transact(op, timeout=0.01)
        assert bytes(fake.tx) == sent
        assert bytes(fake.rx) == pending
        assert len(transport.writes) == 1
        assert transport.next_req == 1


def test_waveform_v8_transport_admits_valid_and_rejects_malformed_before_write():
    import struct
    import emotiscope_control as wire
    valid = wire.encode_get(1, 42, state_version=8, board=1, board_capabilities=511)
    fake = FakeSerial()
    fake.feed_response(0, 1, 1, b"v8-readback")
    transport = Transport(fake, allowed={25}, campaign=True)
    assert transport.transact(25, valid)["body"] == b"v8-readback"
    for malformed in (struct.pack("<I", wire.WAVEFORMS_GET_SUB),
                      struct.pack("<I", wire.WAVEFORMS_PATCH_SUB) + bytes(28)):
        transport = Transport(FakeSerial(), allowed={25}, campaign=True)
        with pytest.raises(ObserveDenied, match="malformed in-house"):
            transport.transact(25, malformed)
        assert not transport.writes
