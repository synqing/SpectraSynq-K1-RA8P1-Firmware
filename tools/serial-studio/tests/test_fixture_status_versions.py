"""Fixture-status version/length admission through a fake transport. No serial port.

Kx-Titan status protocol v7 (21 Oct 2026) is the v6 field map with 3,840-bit lanes and the
same 768-byte length. The broker reads only the header and DMA ACT bits, so v7 is one more
accepted (version, length) pair; every other pair stays refused.
"""
import struct

import pytest

from titan_control_server import BIND_OPCODE, NATIVE_OPCODES, RestrictedControlPlane
from test_native_broker_channel import admit, bound, identity_body

BOOT, EPOCH = 7, 1


def control_body():
    return struct.pack("<4I", 1, BOOT, 1, 1) + b"\x00" * 96


def transact(op, payload):
    if op == BIND_OPCODE:
        return {"ok": True, "status": 0, "body": control_body()}
    if op == NATIVE_OPCODES["native_identity"]:
        return {"ok": True, "status": 0, "body": identity_body()}
    if op == NATIVE_OPCODES["native_status"]:
        return {"ok": True, "status": 0, "body": control_body()}
    raise AssertionError(op)


def fixture_status(version, length, *, epoch=EPOCH, state=6, act=False):
    raw = bytearray(length)
    struct.pack_into("<4I", raw, 0, version, BOOT, epoch, state)
    if act:
        raw[73 * 4] = 0x80
    return bytes(raw)


def read_status(tmp_path, monkeypatch, raw, epoch=EPOCH):
    admit(tmp_path, monkeypatch)
    plane = RestrictedControlPlane(transact=transact, evidence=tmp_path, bound=bound(),
                                   fixture_status_read=lambda boot, ep: raw)
    session = plane.handle({"op": "hello"})["session"]
    return plane._handle({"op": "native_fixture_status", "session": session, "boot": BOOT, "epoch": epoch})


@pytest.mark.parametrize("version,length", [(4, 640), (5, 768), (6, 768), (7, 768)])
def test_known_status_versions_are_accepted(tmp_path, monkeypatch, version, length):
    reply = read_status(tmp_path, monkeypatch, fixture_status(version, length))
    assert reply["ok"] is True and reply["physical_qualified"] is False
    assert bytes.fromhex(reply["raw_hex"])[:4] == struct.pack("<I", version)


def test_v7_idle_bootstrap_is_accepted(tmp_path, monkeypatch):
    reply = read_status(tmp_path, monkeypatch, fixture_status(7, 768, epoch=0, state=0), epoch=0)
    assert reply["ok"] is True


@pytest.mark.parametrize("version,length", [(7, 640), (8, 768), (3, 768), (6, 640)])
def test_unknown_or_mislengthed_versions_are_refused(tmp_path, monkeypatch, version, length):
    reply = read_status(tmp_path, monkeypatch, fixture_status(version, length))
    assert reply == {"ok": False, "error": "fixture_status_identity", "replayed": False}


def test_v7_active_dma_is_refused(tmp_path, monkeypatch):
    reply = read_status(tmp_path, monkeypatch, fixture_status(7, 768, act=True))
    assert reply["ok"] is False and reply["error"] == "fixture_dma_act"
