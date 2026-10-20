"""Native broker channel through a fake transport. No serial port and no service."""
import json
import struct
import zlib
from pathlib import Path

import pytest

from titan_control_server import (
    BIND_OPCODE,
    NATIVE_OPCODES,
    RestrictedControlPlane,
    hardware_writes_admitted,
)
from titan_transport import FakeSerial, Transport

UID = "545433931bd25436593630352d068363"
BUILD = "4" * 64


def bound():
    return {
        "uid": UID,
        "build": BUILD,
        "schema": "replacement-health-v1",
        "schema_sha256": "replacement-health-v1",
        "protocol": "titan-native",
        "controls_persistent": False,
        "durable_commit": "explicit-v1",
    }


def admit(tmp: Path, monkeypatch):
    monkeypatch.setenv("TITAN_LIVE_CONTROL_ADMITTED", "1")
    (tmp / "accepted-candidate.json").write_text(json.dumps({
        "protocol": "titan-native",
        "uid": UID,
        "build": BUILD,
        "schema_sha256": "replacement-health-v1",
        "controls_persistent": False,
        "durable_commit": "explicit-v1",
        "ops": ["native_identity", "native_health", "native_status", "native_scene", "native_global", "native_durable", "native_durable_read"],
    }))


def identity_body():
    return json.dumps({
        "uid": UID, "contract": "titan-replacement-v1", "controls_persistent": False,
        "durable_commit": "explicit-v1",
        "capabilities": {"controls": "replacement-control-v1", "health": "replacement-health-v1"},
    }).encode()


def health_body(count, now):
    return json.dumps({
        "schema": "replacement-health-v1", "physical_qualified": False,
        "snapshot_us": now, "boot_id": 7,
        "startup": {"ready": 1, "failures": 0, "last_driver_error": 0},
        "capture": {"errors": [0, 0]},
        "output": {"completed_count": count, "completed": {"sequence": count},
                   "first_fault": 0, "failures": 0, "last_completed_us": now - 1000},
    }).encode()


def reply(sequence, body, status=0):
    raw = body if isinstance(body, bytes) else b""
    head = struct.pack("<4sIQIII", b"K1R1", status, sequence, len(raw), 0, zlib.crc32(raw) & 0xFFFFFFFF)
    return head + struct.pack("<I", zlib.crc32(head) & 0xFFFFFFFF) + raw


def test_native_channel_rejects_donor_ops_and_serves_two_scenes(tmp_path, monkeypatch):
    admit(tmp_path, monkeypatch)
    assert hardware_writes_admitted(tmp_path, bound()) == (True, "ok")
    calls = []
    health = {"n": 0}
    scenes = {"n": 0}

    def transact(op, payload):
        calls.append(op)
        if op == BIND_OPCODE:
            version, boot, uid = struct.unpack("<II32s", payload)
            assert version == 1 and boot == 7 and uid == UID.encode()
            return {"ok": True, "status": 0, "body": struct.pack("<4I", 1, 7, 1, 1) + b"\x00" * 96}
        if op == NATIVE_OPCODES["native_identity"]:
            return {"ok": True, "status": 0, "body": identity_body()}
        if op == NATIVE_OPCODES["native_status"]:
            return {"ok": True, "status": 0, "body": struct.pack("<4I", 1, 7, 1, 1) + b"\x00" * 96}
        if op == NATIVE_OPCODES["native_health"]:
            health["n"] += 1
            return {"ok": True, "status": 0, "body": health_body(10 if health["n"] == 1 else 11, 2_000_000)}
        if op == NATIVE_OPCODES["native_scene"]:
            scenes["n"] += 1
            if scenes["n"] == 1:
                return {"ok": False, "status": 5, "body": b""}
            return {"ok": True, "status": 0, "body": b""}
        if op == NATIVE_OPCODES["native_global"]:
            return {"ok": True, "status": 0, "body": b""}
        if op == NATIVE_OPCODES["native_durable"]:
            return {"ok": True, "status": 0, "body": struct.pack("<IIII", 1, 0, 4, 100)}
        if op == NATIVE_OPCODES["native_durable_read"]:
            assert payload == struct.pack("<II", 1, 7)
            raw = bytearray(64)
            struct.pack_into("<III", raw, 0, 2, 1, 4)
            struct.pack_into("<II", raw, 12, 0x50534331, 2)
            struct.pack_into("<IIff", raw, 20, 1, 2, 0.25, 40.0)
            struct.pack_into("<IIff", raw, 36, 7, 4, 0.5, 80.0)
            struct.pack_into("<If", raw, 52, 80, 0.5)
            return {"ok": True, "status": 0, "body": bytes(raw)}
        raise AssertionError(op)

    plane = RestrictedControlPlane(transact=transact, evidence=tmp_path, bound=bound())
    hello = plane.handle({"op": "hello"})
    session = hello["session"]
    denied = plane.handle({"op": "emotiscope_patch", "session": session, "fields": {"scene.gain_a": 0.2}})
    assert denied["ok"] is False and 25 not in calls
    assert plane.handle({"op": "native_identity", "session": session})["identity"]["durable_commit"] == "explicit-v1"
    assert plane.handle({"op": "native_status", "session": session})["boot"] == 7
    assert plane.handle({"op": "native_health", "session": session})["completed_count"] == 11
    stale = plane.handle({"op": "native_scene", "session": session, "boot": 7, "scene": 0, "mode": 1,
                          "palette": 0, "gain": 0.2, "response_ms": 40, "expected_revision": 99})
    assert stale["error"] == "stale" and stale["replayed"] is False
    scene_b = plane.handle({"op": "native_scene", "session": session, "boot": 7, "scene": 1, "mode": 7,
                            "palette": 1, "gain": 0.4, "response_ms": 80, "expected_revision": 3})
    assert scene_b["saved"] is False and scene_b["scene"] == 1
    glob = plane.handle({"op": "native_global", "session": session, "boot": 7, "brightness": 80,
                         "output_limit": 0.5, "expected_revision": 4})
    assert glob["saved"] is False
    saved = plane.handle({"op": "native_durable", "session": session, "boot": 7,
                          "expected_scene_revisions": [2, 3], "expected_global_revision": 4})
    assert saved["saved"] is True and saved["disposition"] == "durable"
    read = plane.handle({"op": "native_durable_read", "session": session, "boot": 7})
    assert read["ok"] is True and read["not_live_status"] is True and read["saved"] is False
    assert read["present"] is True and read["scenes"][0]["mode"] == 1 and read["scenes"][1]["mode"] == 7
    assert len(read["scenes"]) == 2
    assert NATIVE_OPCODES["native_scene"] in calls and NATIVE_OPCODES["native_global"] in calls


def test_durable_timeout_is_not_replayed(tmp_path, monkeypatch):
    admit(tmp_path, monkeypatch)

    def transact(op, payload):
        if op == NATIVE_OPCODES["native_status"]:
            return {"ok": True, "status": 0, "body": struct.pack("<4I", 1, 7, 1, 1) + b"\x00" * 96}
        if op == BIND_OPCODE:
            return {"ok": True, "status": 0, "body": struct.pack("<4I", 1, 7, 1, 1) + b"\x00" * 96}
        raise TimeoutError("reply lost")

    plane = RestrictedControlPlane(transact=transact, evidence=tmp_path, bound=bound())
    session = plane.handle({"op": "hello"})["session"]
    plane.handle({"op": "native_status", "session": session})
    lost = plane.handle({"op": "native_durable", "session": session, "boot": 7,
                         "expected_scene_revisions": [1, 1], "expected_global_revision": 1})
    assert lost["error"] == "ambiguous_not_saved" and lost["replayed"] is False


def test_native_frame_uses_existing_fake_serial_owner():
    serial = FakeSerial()
    transport = Transport(serial, campaign=True)
    body = b"{}"
    # Queue a matching reply after the request is written. The fake has no auto-responder,
    # so pre-load the reply using sequence 1, which is the first native exchange.
    serial.rx.extend(reply(1, body))
    got = transport.native_exchange(0x50524200, b"")
    assert got["ok"] is True and got["body"] == body
    assert serial.tx[:4] == b"K1S1"
    with pytest.raises(Exception):
        transport.native_exchange(25, b"")


@pytest.mark.parametrize('defect', [None, 'wrong_uid', 'wrong_build', 'wrong_schema', 'fault', 'missing_fault', 'no_progress', 'reboot', 'no_transport'])
def test_legacy_readiness_uses_actual_read_only_dispatch(tmp_path, defect):
    legacy = {'uid': UID, 'build': BUILD, 'schema_sha256': 'legacy-schema'}
    calls = []
    counter = [0]
    def transact(op, payload):
        calls.append(op)
        assert payload == b'' and op in (1, 17)
        if op == 1:
            counter[0] += 1
            body = dict(legacy, boot_id=2 if defect == 'reboot' and counter[0] == 2 else 1)
            if defect == 'wrong_uid': body['uid'] = 'other'
            if defect == 'wrong_build': body['build'] = 'other'
            if defect == 'wrong_schema': body['schema_sha256'] = 'other'
        else:
            body = {'emit_errors': 1 if defect == 'fault' else 0,
                    'emitted': 20 if defect == 'no_progress' else 20 + counter[0]}
            if defect == 'missing_fault': del body['emit_errors']
        return {'ok': True, 'body': json.dumps(body).encode()}
    plane = RestrictedControlPlane(transact=None if defect == 'no_transport' else transact,
                                   evidence=tmp_path, bound=legacy)
    session = plane.handle({'op': 'hello'})['session']
    result = plane.handle({'op': 'legacy_health', 'session': session})
    assert result['ok'] is (defect is None)
    if defect is None:
        assert result['completed_count'] == 22 and result['faults'] == 0
        assert calls == [1, 17, 1, 17]
    assert plane.hw_writes == 0


def test_deployment_readiness_through_actual_native_socket_handler(tmp_path, monkeypatch):
    import importlib.util
    import tempfile
    from titan_control_server import ControlServer
    helper = Path('/Users/spectrasynq/Workspace_Management/Software/agent-skills/skills/smc-titan-mixer/scripts/deploy_admission.py')
    spec = importlib.util.spec_from_file_location('actual_deployment_admission', helper)
    deployment = importlib.util.module_from_spec(spec); spec.loader.exec_module(deployment)
    admit(tmp_path, monkeypatch)
    count = [0]
    def transact(op, payload):
        assert not payload
        if op == NATIVE_OPCODES['native_identity']:
            identity = json.loads(identity_body()); identity['source_id'] = BUILD
            return {'ok': True, 'body': json.dumps(identity).encode()}
        if op == NATIVE_OPCODES['native_health']:
            count[0] += 1
            return {'ok': True, 'body': health_body(count[0], 2_000_000)}
        if op == NATIVE_OPCODES['native_status']:
            return {'ok': True, 'body': struct.pack('<4I', 1, 7, 0, 1) + bytes(96)}
        raise AssertionError(op)
    plane = RestrictedControlPlane(transact=transact, evidence=tmp_path, bound=bound())
    with tempfile.TemporaryDirectory(prefix='native-ready-', dir='/tmp') as directory:
        path = Path(directory) / 'control.sock'
        server = ControlServer(plane, path)
        server.start()
        try:
            result = deployment.query_broker(str(path), UID, BUILD, 2, 'native')
            assert result['broker_status']['boot'] == result['broker_health']['boot'] == 7
            assert result['broker_health']['completed_count'] == 2
        finally:
            server.close()
    assert plane.hw_writes == 0


@pytest.mark.parametrize('defect', [None, 'uid', 'source_id', 'contract', 'health_boot', 'revoked_profile'])
def test_actual_broker_native_attach_and_poll(tmp_path, monkeypatch, defect):
    """Actual Broker/Transport framing, with only the physical backend replaced."""
    import tempfile
    import titan_broker
    monkeypatch.setattr(titan_broker.cdc_lock, 'acquire', lambda *a: object())
    monkeypatch.setattr(titan_broker.cdc_lock, 'release', lambda *a: None)
    monkeypatch.setattr(titan_broker, 'open_serial', lambda *a: pytest.fail('physical serial reached'))

    class NativeDevice(FakeSerial):
        def __init__(self):
            super().__init__()
            self.ops = []
            self.count = 0
        def write(self, data):
            assert data[:4] == b'K1S1', 'legacy wire protocol reached native image'
            magic, op, seq, size, crc, version, header_crc = struct.unpack('<4sIQIIII', data[:32])
            assert version == 1 and size == 0
            assert zlib.crc32(data[:28]) & 0xffffffff == header_crc
            self.ops.append(op)
            if op == 0x50524200:
                value = json.loads(identity_body()); value['source_id'] = BUILD
                if defect in ('uid', 'source_id', 'contract'): value[defect] = 'wrong'
                body = json.dumps(value).encode()
            elif op == 0x50524205:
                self.count += 1
                body = health_body(self.count, 2_000_000 + self.count * 1000)
            elif op == 0x50524201:
                body = struct.pack('<4I', 1, 8 if defect == 'health_boot' else 7, 0, 1) + bytes(96)
            else:
                pytest.fail(f'Unexpected native operation {op:x}')
            self.rx.extend(reply(seq, body))
            return len(data)

    with tempfile.TemporaryDirectory(prefix='native-attach-', dir='/tmp') as directory:
        evidence = Path(directory)
        admit(evidence, monkeypatch)
        if defect == 'revoked_profile':
            record = json.loads((evidence / 'accepted-candidate.json').read_text())
            record['schema_sha256'] = 'wrong'
            (evidence / 'accepted-candidate.json').write_text(json.dumps(record))
        device = NativeDevice()
        broker = titan_broker.Broker(evidence, backend=device, control_enabled=True)
        try:
            if defect:
                with pytest.raises(RuntimeError):
                    broker.attach({'device': str(evidence / 'fake-cdc'), 'lsof': []})
                assert broker.transport is None and broker.control_server is None
            else:
                bound_identity = broker.attach({'device': str(evidence / 'fake-cdc'), 'lsof': []})
                assert bound_identity['build'] == BUILD and bound_identity['protocol'] == 'titan-native'
                assert (evidence / 'titan-control.sock').exists()
                assert broker.control_plane.isolated_execute == broker._isolated_capture
                broker.poll_once()
                assert not broker.errors and broker.last_progress == 4
                assert device.ops == [0x50524200, 0x50524205, 0x50524205, 0x50524201] * 2
                # Execute the production loop too: legacy request handling must stay unreachable.
                monkeypatch.setattr(broker, '_service_emit_requests', lambda: pytest.fail('legacy emit reached'))
                import threading
                import socket
                subscriber = socket.create_connection(broker.publisher.sock.getsockname(), timeout=1)
                deadline = threading.Timer(0.25, lambda: setattr(broker, 'stop', True))
                deadline.start()
                try:
                    broker.loop()  # Real snapshot, encoder, publisher and recorder.
                    published = subscriber.recv(16384).decode()
                finally:
                    deadline.cancel()
                    subscriber.close()
                assert BUILD in published and 'titan-replacement-v1' in published
                assert 'UNAVAILABLE' in published and published.endswith('\n')
                assert not broker.errors and broker.publisher.skips == 0
                row = broker.last_row
                assert row['protocol'] == 'titan-native' and row['source'] == BUILD
                assert row['contract'] == 'titan-replacement-v1'
                assert row['hop_samples'] == row['admitted_rate_hz'] == 'UNAVAILABLE'
                assert row['mic_a_valid'] == row['mic_b_valid'] == row['rate_valid'] == row['led_valid'] == 0
                assert row['native_health']['completed_count'] == broker.last_progress
                assert row['native_status']['boot'] == 7 and row['physical_qualified'] is False
                assert titan_broker.encode(row)
                stats = json.loads((evidence / 'live-stats.json').read_text())
                assert stats['native']['health']['completed_count'] == broker.last_progress
                assert all(op >= 0x50524200 for op in device.ops)
        finally:
            broker.shutdown()


@pytest.mark.parametrize('stalled', [False, True])
def test_native_health_observes_elapsed_output_frames(tmp_path, monkeypatch, stalled):
    """The real framed consumer must wait for DMA time, not count requests."""
    import time
    admit(tmp_path, monkeypatch)
    class FrameClockDevice(FakeSerial):
        def __init__(self):
            super().__init__()
            self.started = time.monotonic()
            self.observed = []
        def write(self, frame):
            magic, op, seq, size, crc, version, head_crc = struct.unpack('<4sIQIIII', frame[:32])
            assert magic == b'K1S1' and op == 0x50524205 and size == 0
            now = time.monotonic()
            count = 100 if stalled else 100 + int((now - self.started) / .010334)
            self.observed.append((now, count))
            self.rx.extend(reply(seq, health_body(count, 2_000_000 + int((now-self.started)*1_000_000))))
            return len(frame)
    device = FrameClockDevice()
    transport = Transport(device, campaign=True)
    plane = RestrictedControlPlane(transact=transport.native_exchange, evidence=tmp_path, bound=bound())
    session = plane.handle({'op': 'hello'})['session']
    result = plane.handle({'op': 'native_health', 'session': session})
    assert len(device.observed) == 2
    assert device.observed[1][0] - device.observed[0][0] >= .025
    if stalled:
        assert result['ok'] is False and result['error'] == 'output_not_advancing'
        assert result['completed_count'] == [100, 100]
    else:
        assert result['ok'] is True and result['boot'] == 7 and result['faults'] == 0
        assert result['completed_count'] == device.observed[-1][1] > device.observed[0][1]


@pytest.mark.parametrize('age_us', [7847, 10_000_001, -1])
def test_native_health_microsecond_age_from_framed_snapshot(tmp_path, monkeypatch, age_us):
    # Firmware uses the same 64-bit microsecond clock for both timestamps.
    # Long uptime must stay in microseconds, not host time or milliseconds.
    admit(tmp_path, monkeypatch)
    serial = FakeSerial()
    for sequence in (1, 2):
        now = 700_000_000 + sequence * 25_000
        snapshot = json.loads(health_body(33794 + sequence, now))
        snapshot['boot_id'] = 55
        snapshot['output']['last_completed_us'] = now - age_us
        serial.rx.extend(reply(sequence, json.dumps(snapshot).encode()))
    transport = Transport(serial, campaign=True)
    plane = RestrictedControlPlane(transact=transport.native_exchange, evidence=tmp_path, bound=bound())
    session = plane.handle({'op': 'hello'})['session']
    result = plane.handle({'op': 'native_health', 'session': session})
    if age_us == 7847:
        assert result['ok'] is True and result['boot'] == 55
        assert result['completed_count'] == 33796
    else:
        assert result['ok'] is False and result['error'] == 'stale_output'
        assert result['completion_age_us'] == age_us and result['boot'] == 55
        assert result['snapshot_us'] == 700_025_000
        assert result['last_completed_us'] == 700_025_000 - age_us


def test_full_health_snapshot_logged_once_per_minute(monkeypatch):
    # Soak evidence (21 Oct 2026): the second health snapshot of each poll is kept and one full
    # copy is recorded at most once per 60 s; no extra device traffic.
    import tempfile
    import titan_broker
    monkeypatch.setattr(titan_broker.cdc_lock, 'acquire', lambda *a: object())
    monkeypatch.setattr(titan_broker.cdc_lock, 'release', lambda *a: None)
    monkeypatch.setattr(titan_broker, 'open_serial', lambda *a: pytest.fail('physical serial reached'))

    class NativeDevice(FakeSerial):
        def __init__(self):
            super().__init__()
            self.ops = []
            self.count = 0
        def write(self, data):
            magic, op, seq, size, crc, version, header_crc = struct.unpack('<4sIQIIII', data[:32])
            self.ops.append(op)
            if op == 0x50524200:
                value = json.loads(identity_body()); value['source_id'] = BUILD
                body = json.dumps(value).encode()
            elif op == 0x50524205:
                self.count += 1
                body = health_body(self.count, 2_000_000 + self.count * 1000)
            elif op == 0x50524201:
                body = struct.pack('<4I', 1, 7, 0, 1) + bytes(96)
            else:
                pytest.fail(f'Unexpected native operation {op:x}')
            self.rx.extend(reply(seq, body))
            return len(data)

    with tempfile.TemporaryDirectory(prefix='native-full-', dir='/tmp') as directory:
        evidence = Path(directory)
        admit(evidence, monkeypatch)
        device = NativeDevice()
        broker = titan_broker.Broker(evidence, backend=device, control_enabled=True)
        try:
            broker.attach({'device': str(evidence / 'fake-cdc'), 'lsof': []})
            rows = []
            monkeypatch.setattr(broker.recorder, 'put', rows.append)
            broker.poll_once(); broker.poll_once()
            full = [r for r in rows if r.get('kind') == 'native_health_full']
            assert len(full) == 1 and full[0]['snapshot']['schema'] == 'replacement-health-v1'
            ops_per_poll = len(device.ops)
            broker._full_health_mono -= 61.0
            broker.poll_once()
            assert len([r for r in rows if r.get('kind') == 'native_health_full']) == 2
            assert not broker.errors
        finally:
            broker.shutdown()
