#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
import tempfile
import threading
import time
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
import live_protocol as proto  # noqa: E402
from titan_control_server import (  # noqa: E402
    ControlServer,
    RestrictedControlPlane,
    SocketInUseError,
    hardware_writes_admitted,
    load_approval_record,
)

UID = "545433931bd25436593630352d068363"
BUILD = "test-build"
SCHEMA = "05198c7a0ad56c06a9ed29b13c8845a8a10da90d3dbff0f68098d2aa5d70f817"


class FakeTransport:
    def __init__(self):
        self.ops: list[tuple[int, bytes]] = []
        self.in_flight = 0
        self.max_in_flight = 0
        self.lock = threading.Lock()
        self.cfg = proto.unpack_config(proto.pack_config({
            "version": 2,
            "revision": 3,
            "emit_on": 0,
            "boot_id": 1,
            "flags": 1,
            "mode_a": 8,
            "visual_a": {"hue_position": 0.1, "vp_waveform_shift_rate": 120.0},
            "visual_b": {"hue_position": 0.2, "vp_waveform_shift_rate": 10.0},
        }))
        self.cfg_blob = proto.pack_config(self.cfg)

    def transact(self, op: int, payload: bytes) -> dict:
        with self.lock:
            self.in_flight += 1
            self.max_in_flight = max(self.max_in_flight, self.in_flight)
        try:
            time.sleep(0.05)
            self.ops.append((op, payload))
            if op != 25:
                return {"ok": False, "status": 3, "body": b""}
            sub = proto._u32(payload, 0)
            if sub in (proto.CFG_SET_CONFIG, proto.CFG_BEGIN_SET, proto.CFG_COMMIT_SET):
                raise AssertionError("whole-config setter used")
            if sub == proto.CFG_GET_CONFIG:
                return {"ok": True, "status": 0, "body": self.cfg_blob}
            if sub == proto.CFG_PATCH:
                patch = proto.unpack_patch(payload)
                if int(patch.get("boot_id") or 0) != int(self.cfg.get("boot_id") or 0):
                    return {"ok": False, "status": 6, "body": b""}
                if int(patch["revision"]) != int(self.cfg["revision"]):
                    return {"ok": False, "status": 5, "body": b""}
                channel = "visual_a" if patch["channel"] == 0 else "visual_b"
                vis = dict(self.cfg.get(channel) or {})
                vis.update({k.split(".", 1)[1]: v for k, v in patch["fields"].items()})
                self.cfg[channel] = vis
                self.cfg["revision"] = int(self.cfg["revision"]) + 1
                self.cfg["emit_on"] = 0
                self.cfg["last_patch_request_id"] = int(patch["request_id"])
                self.cfg_blob = proto.pack_config(self.cfg)
                return {"ok": True, "status": 0, "body": self.cfg_blob}
            return {"ok": False, "status": 3, "body": b""}
        finally:
            with self.lock:
                self.in_flight -= 1


def _bound():
    return {"uid": UID, "build": BUILD, "schema_sha256": SCHEMA, "schema": SCHEMA}


def test_empty_approval_wrong_id_denied(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-candidate.json").write_text("{}\n")
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    fake = FakeTransport()
    plane = RestrictedControlPlane(transact=fake.transact, evidence=evidence, bound=_bound())
    hello = plane.handle({"op": "hello"})
    assert hello["admitted"] is False
    got = plane.handle({
        "op": "patch",
        "session": hello["session"],
        "request_id": 1,
        "revision": 3,
        "boot_id": 1,
        "fields": {"visual_a.vp_waveform_shift_rate": 40.0},
    })
    assert got["ok"] is False
    assert got["error"] == "ADMISSION_HOLD"


def test_wrong_uid_denied(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"deadbeef","build":"test-build","schema_sha256":"%s"}\n' % SCHEMA
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    ok, reason = hardware_writes_admitted(evidence, _bound())
    assert ok is False
    assert reason == "uid_mismatch"


def test_matching_checkpoint_admits(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["patch","get_config"]}\n'
        % (UID, BUILD, SCHEMA)
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    fake = FakeTransport()
    plane = RestrictedControlPlane(transact=fake.transact, evidence=evidence, bound=_bound())
    hello = plane.handle({"op": "hello"})
    got = plane.handle({
        "op": "patch",
        "session": hello["session"],
        "request_id": 7,
        "revision": 3,
        "boot_id": 1,
        "fields": {"visual_a.vp_waveform_shift_rate": 40.0},
    })
    assert got["ok"] is True
    assert got["request_id"] == 7
    assert got["last_patch_request_id"] == 7
    assert got["emit_on"] == 0
    assert abs(got["applied"]["visual_a.vp_waveform_shift_rate"] - 40.0) < 1e-5


def test_patch_requires_session():
    plane = RestrictedControlPlane(bound=_bound())
    got = plane.handle({"op": "patch", "revision": 0, "request_id": 1,
                        "fields": {"visual_a.vp_waveform_shift_rate": 40.0}})
    assert got["ok"] is False
    assert got["error"] == "stale_session"


def test_sessions_are_unique():
    a = RestrictedControlPlane(bound=_bound()).session.token
    b = RestrictedControlPlane(bound=_bound()).session.token
    assert a != b
    assert a != "sess-1"


def test_channel_b_not_silently_a():
    packed = proto.pack_patch(1, {"visual_b.hue_position": 0.5}, request_id=9)
    decoded = proto.unpack_patch(packed)
    assert decoded["channel"] == 1
    assert "visual_b.hue_position" in decoded["fields"]
    assert "visual_a.hue_position" not in decoded["fields"]


def test_unknown_prefix_rejected():
    try:
        proto.pack_patch(1, {"unrelated.hue_position": 0.5}, request_id=1)
    except ValueError:
        return
    raise AssertionError("unknown prefix accepted")


def test_mixed_channels_rejected():
    try:
        proto.pack_patch(1, {"visual_a.hue_position": 0.1, "visual_b.hue_position": 0.2}, request_id=1)
    except ValueError:
        return
    raise AssertionError("mixed channels accepted")


def test_serialised_transport(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["patch","get_config"]}\n'
        % (UID, BUILD, SCHEMA)
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    fake = FakeTransport()
    plane = RestrictedControlPlane(transact=fake.transact, evidence=evidence, bound=_bound())
    hello = plane.handle({"op": "hello"})
    errors = []

    def worker(rid):
        try:
            plane.handle({
                "op": "patch",
                "session": hello["session"],
                "request_id": rid,
                "revision": 3,
                "boot_id": 1,
                "fields": {"visual_a.vp_waveform_shift_rate": 40.0},
            })
        except Exception as exc:  # noqa: BLE001
            errors.append(exc)

    t1 = threading.Thread(target=worker, args=(1,))
    t2 = threading.Thread(target=worker, args=(2,))
    t1.start()
    t2.start()
    t1.join()
    t2.join()
    assert not errors
    assert fake.max_in_flight == 1


def test_unix_socket_one_writer(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    plane = RestrictedControlPlane(bound=_bound())
    sock_path = evidence / "titan-control.sock"
    server = ControlServer(plane, sock_path)
    server.start()
    time.sleep(0.05)
    mode = sock_path.stat().st_mode & 0o777
    assert mode == 0o600
    server.close()


def test_schema_required_when_approval_names_it(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s"}\n' % (UID, BUILD, SCHEMA)
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    ok, reason = hardware_writes_admitted(evidence, {"uid": UID, "build": BUILD})
    assert ok is False
    assert reason in {"schema_missing", "ops_missing"}
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["patch"]}\n' % (UID, BUILD, SCHEMA)
    )
    ok, reason = hardware_writes_admitted(evidence, _bound())
    assert ok is True
    assert reason == "ok"


def test_load_approval_rejects_empty(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-candidate.json").write_text("{}\n")
    assert load_approval_record(evidence) is None


def test_second_server_refuses_live_socket(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    plane = RestrictedControlPlane(bound=_bound())
    sock_path = evidence / "titan-control.sock"
    first = ControlServer(plane, sock_path)
    first.start()
    time.sleep(0.05)
    try:
        raised = False
        try:
            ControlServer(RestrictedControlPlane(bound=_bound()), sock_path)
        except SocketInUseError:
            raised = True
        assert raised
        assert sock_path.exists()
    finally:
        first.close()


def test_one_connection_many_requests(tmp_path):
    import json
    import socket as sockmod

    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    plane = RestrictedControlPlane(bound=_bound())
    sock_path = evidence / "titan-control.sock"
    server = ControlServer(plane, sock_path)
    server.start()
    time.sleep(0.05)
    conn = sockmod.socket(sockmod.AF_UNIX, sockmod.SOCK_STREAM)
    conn.settimeout(2)
    try:
        conn.connect(str(sock_path))
        replies = []
        for op in ("hello", "get_config"):
            conn.sendall((json.dumps({"op": op, "session": plane.session.token}) + "\n").encode())
            buf = b""
            while b"\n" not in buf:
                buf += conn.recv(4096)
            replies.append(json.loads(buf.split(b"\n", 1)[0].decode()))
        assert replies[0]["ok"] is True
        assert replies[1]["ok"] is False  # no transport: ADMISSION_HOLD or get_config_rejected
    finally:
        conn.close()
        server.close()


if __name__ == "__main__":
    with tempfile.TemporaryDirectory() as d:
        root = Path(d)
        test_empty_approval_wrong_id_denied(root / "empty")
        test_wrong_uid_denied(root / "uid")
        test_matching_checkpoint_admits(root / "ok")
        test_patch_requires_session()
        test_sessions_are_unique()
        test_channel_b_not_silently_a()
        test_unknown_prefix_rejected()
        test_mixed_channels_rejected()
        test_serialised_transport(root / "serial")
        test_unix_socket_one_writer(root / "unix")
        test_schema_required_when_approval_names_it(root / "schema")
        test_load_approval_rejects_empty(root / "blank")
        test_second_server_refuses_live_socket(root / "busy")
        test_one_connection_many_requests(root / "persist")
        test_schema_required_when_approval_names_it(root / "schema")
    print("TITAN_CONTROL_SERVER_PASS")
