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
import emotiscope_control as donor  # noqa: E402
import live_protocol as proto  # noqa: E402
from titan_control_server import (  # noqa: E402
    ControlServer,
    RestrictedControlPlane,
    SocketInUseError,
    hardware_writes_admitted,
    load_approval_record,
    _decode_donor_body,
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


def test_snapshot_ml_witness_fixed_route_and_default_snapshot_are_separate(tmp_path):
    import struct
    calls=[];raw=bytearray(592);raw[:4]=b'MLR1'
    struct.pack_into('<II9I',raw,4,1,10000,8,7,9900,1,0x12345678,11,20,25,2)
    struct.pack_into('<64f',raw,48,*([.25]*64));struct.pack_into('<8f',raw,304,*([.5]*8))
    struct.pack_into('<64f',raw,336,*([.75]*64))
    live=bytearray(1184);struct.pack_into('<IH',live,0,proto.SNAP_MAGIC,1)
    def transact(op,payload):
        calls.append((op,bytes(payload)))
        return {'ok':True,'status':0,'body':bytes(raw) if op==17 else bytes(live)}
    plane=RestrictedControlPlane(transact=transact,evidence=Path(tmp_path),bound=_bound())
    session=plane.handle({'op':'hello'})['session']
    result=plane.handle({'op':'snapshot','kind':'emotiscope_ml','session':session})
    assert result['ok'] is True
    assert result['body_hex']==raw.hex()
    witness=result['witness']
    assert witness['sequence']==8 and witness['input_sequence']==7 and witness['inference_count']==11
    assert witness['latent']==[.5]*8 and witness['reconstruction']==[.75]*64
    assert calls==[(17,b'MLST')]
    result=plane.handle({'op':'snapshot','session':session})
    assert result['ok'] is True and 'snapshot' in result and 'witness' not in result
    assert calls[-1]==(23,b'')
    before=list(calls)
    result=plane.handle({'op':'snapshot','kind':'arbitrary','payload':'4d4c5354','session':session})
    assert result['ok'] is False and calls==before


def test_snapshot_ml_witness_real_broker_transport_admission(tmp_path):
    """Use the production socket, broker and framing; emulate only device bytes."""
    import struct
    import pytest
    from test_live_control_path import _checkpoint, _usb, UnixBroker
    from titan_broker import Broker
    from titan_live_device import LiveDeviceSerial
    from titan_transport import ObserveDenied

    raw = bytearray(592)
    raw[:4] = b"MLR1"
    struct.pack_into("<II9I", raw, 4, 1, 10000, 8, 7, 9900, 1, 0x12345678, 11, 20, 25, 2)
    struct.pack_into("<64f", raw, 48, *([0.25] * 64))
    struct.pack_into("<8f", raw, 304, *([0.5] * 8))
    struct.pack_into("<64f", raw, 336, *([0.75] * 64))

    class DeviceBytes(LiveDeviceSerial):
        def __init__(self):
            super().__init__()
            self.requests = []

        def _handle(self, op, req, payload):
            self.requests.append((op, bytes(payload)))
            if op == 17 and payload == b"MLST":
                self._respond(0, req, bytes(raw))
            else:
                super()._handle(op, req, payload)

    device = DeviceBytes()
    evidence = _checkpoint(_short_dir(), build=device.build, schema=device.schema)
    broker = Broker(evidence, backend=device, control_enabled=True)
    client = None
    try:
        broker.attach(_usb(Path(tmp_path)))  # Injected backend; no physical CDC open.
        client = UnixBroker(str(evidence / "titan-control.sock"))
        session = client.request({"op": "hello"})["session"]
        device.requests.clear()
        result = client.request({"op": "snapshot", "kind": "emotiscope_ml", "session": session})
        assert result["ok"] is True and result["body_hex"] == raw.hex()
        assert result["witness"]["input_sequence"] == 7
        assert result["witness"]["latent"] == [0.5] * 8
        assert device.requests == [(17, b"MLST")]

        before = bytes(device.tx), broker.transport.next_req
        for payload in (b"MLSU", b"MLS", b"MLST\x00", b"mlst"):
            with pytest.raises(ObserveDenied, match="payload not admitted"):
                broker.transact(17, payload)
            assert (bytes(device.tx), broker.transport.next_req) == before
        bad_kind = client.request({"op": "snapshot", "kind": "arbitrary", "session": session})
        assert bad_kind["ok"] is False
        assert (bytes(device.tx), broker.transport.next_req) == before

        ordinary = client.request({"op": "snapshot", "session": session})
        assert ordinary["ok"] is True and "snapshot" in ordinary and "witness" not in ordinary
        assert device.requests == [(17, b"MLST"), (23, b"")]
        assert broker.control_plane.hw_writes == 0 and device.patch_applies == 0
    finally:
        if client is not None:
            client.close()
        broker.shutdown()


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


def _short_dir():
    import tempfile
    return Path(tempfile.mkdtemp(prefix="tcs", dir="/tmp"))


def test_unix_socket_one_writer(tmp_path):
    evidence = _short_dir()
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
    evidence = _short_dir()
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

    evidence = _short_dir()
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


def _donor_state(request_id, boot_id=0x11, revision=4, brightness=0.5, mode=1, flags=39, output=100, accepted=4):
    import struct
    body = bytearray(donor.STATE_BYTES)
    def pu(off, value):
        struct.pack_into("<I", body, off, int(value) & 0xFFFFFFFF)
    def pf(off, value):
        struct.pack_into("<f", body, off, float(value))
    pu(0, donor.STATE_MAGIC)
    pu(4, 1)
    pu(8, 1)
    pu(12, boot_id)
    pu(16, revision)
    pu(20, request_id)
    pf(24, brightness)
    pf(28, 0.25)
    pf(32, 0.33)
    pf(56, 0.33)
    struct.pack_into("<i", body, 60, mode)
    pu(64, flags)
    pu(68, output)
    pu(72, 0xDFDF)
    pu(76, accepted)
    return bytes(body)


def test_donor_gain_readback_version_2_and_legacy_support():
    import struct
    legacy = _decode_donor_body(_donor_state(1))
    assert legacy["version"] == 1
    assert legacy["output_intensity_supported"] is False
    assert legacy["output_intensity_a"] is None and legacy["output_intensity_b"] is None
    body = bytearray(_donor_state(2))
    struct.pack_into("<I", body, 4, 2)
    struct.pack_into("<ff", body, 144, 0.25, 1.0)
    state = _decode_donor_body(bytes(body))
    assert state["version"] == 2
    assert state["output_intensity_supported"] is True
    assert state["output_intensity_a"] == 0.25 and state["output_intensity_b"] == 1.0
    body[152] = 1
    assert _decode_donor_body(bytes(body)) is None


def test_legacy_patch_still_subcommand_8_and_donor_is_separate(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["patch","emotiscope_patch"]}\n'
        % (UID, BUILD, SCHEMA)
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    fake = FakeTransport()
    plane = RestrictedControlPlane(transact=fake.transact, evidence=evidence, bound=_bound())
    hello = plane.handle({"op": "hello"})
    legacy = plane.handle({
        "op": "patch",
        "session": hello["session"],
        "request_id": 7,
        "revision": 3,
        "boot_id": 1,
        "fields": {"visual_a.vp_waveform_shift_rate": 40.0},
    })
    assert legacy["ok"] is True
    assert proto._u32(fake.ops[-1][1], 0) == proto.CFG_PATCH
    calls = []

    def donor_transact(op, payload):
        calls.append((op, bytes(payload)))
        return {"ok": True, "status": 0, "body": _donor_state(3, accepted=3, revision=9, brightness=0.5)}

    plane.transact = donor_transact
    scene = {name: 0.5 for name in (
        "brightness", "softness", "warmth", "speed", "background")}
    scene.update({
        "current_mode": 1,
        "mirror_mode": 1,
        "screensaver": 1,
        "temporal_dithering": 1,
        "auto_color_cycle": 0,
        "reverse_color_range": 0,
        "hardware_output_brightness": 100,
    })
    got = plane.handle({
        "op": "emotiscope_patch",
        "session": hello["session"],
        "request_id": 3,
        "revision": 8,
        "boot_id": 0x11,
        "fields": scene,
    })
    assert got["ok"] is True
    assert got["state"]["brightness"] == 0.5
    assert got["state"]["hardware_output_brightness"] == 100
    assert got["state"]["emission_enabled"] is True
    assert calls[0][0] == 25
    assert calls[0][1] == donor.encode_patch(3, 0x11, 8, scene)
    assert proto._u32(calls[0][1], 0) == donor.PATCH_SUB
    assert len(calls[0][1]) == 24 + 12 * 8


def _top_state(request_id, capabilities=3):
    import struct
    body = bytearray(_donor_state(request_id))
    body.extend(b"\0" * (donor.TOP_STATE_BYTES - len(body)))
    struct.pack_into("<I", body, 4, 3)
    struct.pack_into("<ffffII", body, 144, 1.0, 1.0, 1.0, 0.25,
                     int(bool(capabilities & donor.CAP_SECONDARY_OUTPUT)), capabilities)
    return bytes(body)


def test_board_control_readback_rejects_malformed_profile():
    import struct
    state = _decode_donor_body(_top_state(1))
    assert state["board_capabilities"] == 3
    assert state["primary_intensity_supported"] is True
    assert state["secondary_output_supported"] is True
    assert state["primary_intensity"] == 1.0
    assert state["secondary_intensity"] == 0.25
    assert state["secondary_output_enabled"] is True
    legacy = _decode_donor_body(_donor_state(2))
    assert legacy["board_capabilities"] == 0
    assert legacy["secondary_intensity"] is None
    assert legacy["secondary_output_supported"] is False
    for offset, value in ((164, 4), (168, 1), (160, 2)):
        bad = bytearray(_top_state(1))
        struct.pack_into("<I", bad, offset, value)
        assert _decode_donor_body(bytes(bad)) is None
    assert _decode_donor_body(_top_state(1)[:160]) is None


def test_board_controls_require_observed_same_boot_capability(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["emotiscope_patch"]}\n'
        % (UID, BUILD, SCHEMA)
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    calls = []
    reply_body = _top_state(1)
    def transact(op, wire):
        calls.append((op, wire))
        return {"ok": True, "status": 0, "body": reply_body}
    plane = RestrictedControlPlane(transact=transact, evidence=evidence, bound=_bound())
    hello = plane.handle({"op": "hello"})
    request = {"op": "emotiscope_patch", "session": hello["session"],
               "request_id": 5, "boot_id": 0x11, "revision": 4,
               "fields": {"primary_intensity": 0.75, "secondary_intensity": 0.25,
                          "secondary_output_enabled": True},
               "state_version": 3, "board_capabilities": 3}
    # Caller assertions cannot grant a device capability.
    assert plane.handle(request)["ok"] is False
    assert calls == []
    get = {"op": "emotiscope_get", "session": hello["session"],
           "request_id": 1, "boot_id": 0x11, "revision": 4}
    assert plane.handle(get)["ok"] is True
    calls.clear()
    assert plane.handle(request)["ok"] is True
    assert calls == [(25, donor.encode_patch(5, 0x11, 4, request["fields"],
                                          state_version=3, board_capabilities=3))]
    calls.clear()
    assert plane.handle({**request, "boot_id": 0x12})["ok"] is False
    assert calls == []
    for reply_body in (_donor_state(1), _top_state(1, capabilities=1)):
        assert plane.handle(get)["ok"] is True
        calls.clear()
        assert plane.handle(request)["ok"] is False
        assert calls == []
    reply_body = _top_state(1)
    assert plane.handle(get)["ok"] is True
    plane.on_reconnect(_bound())
    new_hello = plane.handle({"op": "hello"})
    calls.clear()
    assert plane.handle({**request, "session": new_hello["session"]})["ok"] is False
    assert calls == []


def _dual_state(request_id, board=0, capabilities=7):
    import struct
    body = bytearray(_top_state(request_id, capabilities & 3))
    body.extend(b"\0" * (donor.DUAL_STATE_BYTES - len(body)))
    struct.pack_into("<I", body, 4, 4)
    struct.pack_into("<I", body, 140, 1)
    struct.pack_into("<I", body, 72, 0xDFDF & ~((1 << 2) | (1 << 4) | (1 << 6)))
    struct.pack_into("<I", body, 164, capabilities)
    struct.pack_into("<IfIf", body, 176, board, 1.5, 2, 0.5)
    return bytes(body)


def test_independent_scene_selection_requires_device_capability(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["emotiscope_patch"]}\n'
        % (UID, BUILD, SCHEMA)
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    calls = []
    reply_body = _dual_state(1)

    def transact(op, wire):
        calls.append((op, wire))
        return {"ok": True, "status": 0, "body": reply_body}

    plane = RestrictedControlPlane(transact=transact, evidence=evidence, bound=_bound())
    hello = plane.handle({"op": "hello"})
    get = {"op": "emotiscope_get", "session": hello["session"],
           "request_id": 1, "boot_id": 0x11, "revision": 4}
    secondary = {**get, "board": 1, "state_version": 4, "board_capabilities": 7}
    patch = {"op": "emotiscope_patch", "session": hello["session"],
             "request_id": 5, "boot_id": 0x11, "revision": 4,
             "fields": {"board": 1, "response_gain": 1.5,
                        "edge_response": 2, "edge_depth": 0.5},
             "state_version": 4, "board_capabilities": 7}
    assert plane.handle(secondary)["ok"] is False
    assert plane.handle(patch)["ok"] is False
    assert calls == []
    for reply_body in (_donor_state(1), _top_state(1)):
        assert plane.handle(get)["ok"] is True
        calls.clear()
        assert plane.handle(secondary)["ok"] is False
        assert plane.handle(patch)["ok"] is False
        assert calls == []
    reply_body = _dual_state(1)
    primary = plane.handle(get)
    assert primary["ok"] is True
    assert primary["state"]["selected_board"] == 0
    assert primary["state"]["independent_scenes_supported"] is True
    assert primary["state"]["response_gain"] == 1.5
    calls.clear()
    reply_body = _dual_state(1, board=1)
    selected = plane.handle(secondary)
    assert selected["ok"] is True and selected["state"]["selected_board"] == 1
    assert calls == [(25, donor.encode_get(1, 0x11, 1, state_version=4, board_capabilities=7))]
    calls.clear()
    assert plane.handle(patch)["ok"] is True
    assert calls == [(25, donor.encode_patch(5, 0x11, 4, patch["fields"],
                                            state_version=4, board_capabilities=7))]
    calls.clear()
    reply_body = _dual_state(1, board=1, capabilities=5)
    faulted = plane.handle(secondary)
    assert faulted["ok"] is True and faulted["state"]["selected_board"] == 1
    assert faulted["state"]["independent_scenes_supported"] is True
    assert faulted["state"]["secondary_output_supported"] is False
    calls.clear()
    # A transport fault does not remove the independent secondary scene.
    assert plane.handle(patch)["ok"] is True
    assert calls == [(25, donor.encode_patch(5, 0x11, 4, patch["fields"],
                                            state_version=4, board_capabilities=5))]
    calls.clear()
    # Claimed request capabilities cannot admit either physical output write.
    for physical in ({"secondary_output_enabled": True}, {"secondary_intensity": 0.5}):
        assert plane.handle({**patch, "fields": {"board": 1, **physical}})["ok"] is False
        assert calls == []
    assert plane.handle({**secondary, "boot_id": 0x12})["ok"] is False
    assert plane.handle({**patch, "boot_id": 0x12})["ok"] is False
    assert calls == []
    plane.transact = None
    assert plane.handle(secondary)["cached"] is True
    assert plane.handle(get)["ok"] is False
    plane.on_reconnect(_bound())
    new_hello = plane.handle({"op": "hello"})
    assert plane.handle({**secondary, "session": new_hello["session"]})["ok"] is False


def test_donor_patch_rejected_by_legacy_only_approval(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["patch"]}\n' % (UID, BUILD, SCHEMA)
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    calls = []
    plane = RestrictedControlPlane(
        transact=lambda op, payload: calls.append(op) or {"ok": True, "status": 0, "body": b""},
        evidence=evidence,
        bound=_bound(),
    )
    hello = plane.handle({"op": "hello"})
    got = plane.handle({
        "op": "emotiscope_patch",
        "session": hello["session"],
        "request_id": 1,
        "revision": 0,
        "boot_id": 1,
        "fields": {"brightness": 0.5},
    })
    assert got["ok"] is False
    assert got["error"] == "ops_denied"
    assert calls == []


def test_legacy_patch_rejected_by_donor_only_approval(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["emotiscope_patch"]}\n' % (UID, BUILD, SCHEMA)
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    assert hardware_writes_admitted(evidence, _bound()) == (True, "ok")
    calls = []
    plane = RestrictedControlPlane(
        transact=lambda op, payload: calls.append(op) or {"ok": True, "status": 0, "body": b""},
        evidence=evidence,
        bound=_bound(),
    )
    hello = plane.handle({"op": "hello"})
    got = plane.handle({
        "op": "patch",
        "session": hello["session"],
        "request_id": 1,
        "revision": 0,
        "boot_id": 1,
        "fields": {"visual_a.hue_position": 0.2},
    })
    assert got["ok"] is False
    assert got["admit_reason"] == "legacy_patch_not_admitted"
    assert calls == []


def test_donor_invalid_values_do_not_transact(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["emotiscope_patch"]}\n' % (UID, BUILD, SCHEMA)
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    calls = []
    plane = RestrictedControlPlane(
        transact=lambda op, payload: calls.append(payload) or {"ok": False, "status": 3, "body": b""},
        evidence=evidence,
        bound=_bound(),
    )
    hello = plane.handle({"op": "hello"})
    base = {"op": "emotiscope_patch", "session": hello["session"], "request_id": 4, "revision": 1, "boot_id": 2}
    for fields in ({"current_mode": 7}, {"current_mode": 8}, {"brightness": float("nan")},
                   {"hardware_output_brightness": 256}, {"mirror_mode": 2}):
        got = plane.handle({**base, "fields": fields})
        assert got["ok"] is False
        assert got["error"] == "forbidden_or_invalid_field"
    zero = plane.handle({**base, "request_id": 0, "fields": {"brightness": 0.5}})
    assert zero["ok"] is False
    assert calls == []


def test_approval_without_either_write_capability_denied(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["get_config"]}\n' % (UID, BUILD, SCHEMA)
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    ok, reason = hardware_writes_admitted(evidence, _bound())
    assert ok is False
    assert reason == "ops_denied"


def test_donor_get_uses_get_discriminator(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    calls = []

    def transact(op, payload):
        calls.append((op, bytes(payload)))
        return {"ok": True, "status": 0, "body": _donor_state(9, revision=5)}

    plane = RestrictedControlPlane(transact=transact, evidence=evidence, bound=_bound())
    hello = plane.handle({"op": "hello"})
    got = plane.handle({
        "op": "emotiscope_get",
        "session": hello["session"],
        "request_id": 9,
        "boot_id": 0x11,
    })
    assert got["ok"] is True
    assert got["state"]["revision"] == 5
    assert calls == [(25, donor.encode_get(9, 0x11))]
    assert proto._u32(calls[0][1], 0) == donor.GET_SUB


def test_donor_conflict_returns_state_without_counting_a_write(tmp_path):
    evidence = Path(tmp_path)
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["emotiscope_patch"]}\n' % (UID, BUILD, SCHEMA)
    )
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    plane = RestrictedControlPlane(
        transact=lambda op, payload: {"ok": False, "status": 7, "body": _donor_state(1, revision=3, brightness=0.25)},
        evidence=evidence,
        bound=_bound(),
    )
    hello = plane.handle({"op": "hello"})
    before = plane.hw_writes
    got = plane.handle({
        "op": "emotiscope_patch",
        "session": hello["session"],
        "request_id": 2,
        "revision": 3,
        "boot_id": 0x11,
        "fields": {"brightness": 0.2},
    })
    assert got["ok"] is False
    assert got["error"] == "request_conflict"
    assert got["state"]["brightness"] == 0.25
    assert plane.hw_writes == before


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
        test_legacy_patch_still_subcommand_8_and_donor_is_separate(root / "donor-separate")
        test_donor_patch_rejected_by_legacy_only_approval(root / "donor-legacy-only")
        test_legacy_patch_rejected_by_donor_only_approval(root / "legacy-donor-only")
        test_donor_invalid_values_do_not_transact(root / "donor-invalid")
        test_approval_without_either_write_capability_denied(root / "neither-write")
        test_donor_get_uses_get_discriminator(root / "donor-get")
        test_donor_conflict_returns_state_without_counting_a_write(root / "donor-conflict")
    print("TITAN_CONTROL_SERVER_PASS")


def test_composed_v6_capability_and_negotiation(tmp_path):
    import struct
    evidence = Path(tmp_path)
    (evidence / "accepted-checkpoint.json").write_text(
        '{"uid":"%s","build":"%s","schema_sha256":"%s","ops":["emotiscope_patch"]}\n'
        % (UID, BUILD, SCHEMA))
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    body = bytearray(_dual_state(1)) + bytearray(424 - 208)
    struct.pack_into("<I", body, 4, 6)
    struct.pack_into("<I", body, 164, 127)
    struct.pack_into("<f", body, 248, 40.)
    struct.pack_into("<f", body, 252, 160.)
    values = (0,.14,1400.,350.,450.,.1,2200.,.45,120.,1,.3,.5,500.,0,0,180.,0,0,1)
    # Literal offsets/types, independent of codec field table.
    struct.pack_into("<I8fI3fIIfIII", body, 320, *values)
    calls = []
    response = {"ok": True, "status": 0, "body": bytes(body)}
    def transact(op, wire):
        calls.append((op, wire))
        return response
    plane = RestrictedControlPlane(transact=transact, evidence=evidence, bound=_bound())
    hello = plane.handle({"op":"hello"})
    get = {"op":"emotiscope_get", "session":hello["session"], "request_id":1,
           "boot_id":0x11,"revision":4,"state_version":6}
    patch = {**get,"op":"emotiscope_patch","fields":{"scene":1,"scene_outgoing_ms":1800.},"board_capabilities":127}
    assert plane.handle(patch)["ok"] is False  # caller caps grant nothing
    assert not calls
    got = plane.handle(get)
    assert got["ok"] is True, got
    assert got["state"]["scene_outgoing_ms"] == 1400.
    assert got["state"]["composed_scenes_supported"] is True
    assert got["state"]["active_scene"] == 0
    assert plane.handle(patch)["ok"] is True
    assert calls[-1][1] == donor.encode_patch(1,0x11,4,patch["fields"],state_version=6,board_capabilities=127)
    calls.clear()
    assert plane.handle({**get,"board":1})["ok"] is True
    assert calls[-1][1] == donor.encode_get(1,0x11,1,state_version=6,board_capabilities=127)
    response = {"ok":False,"status":3,"body":b""}
    assert plane.handle(get)["error"] == "unsupported_state_version"
