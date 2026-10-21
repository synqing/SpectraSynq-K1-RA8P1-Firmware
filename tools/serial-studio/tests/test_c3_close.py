#!/usr/bin/env python3
"""C3-A/B/C/D on the real Adapter/Unix/Broker/Transport/LiveDeviceSerial path.

InProcessBroker is not used here. No CDC, no programming.
"""
from __future__ import annotations

import json
import math
import os
import socket
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
HOST = Path(
    "/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer"
    "/.worktrees/titan-control-001/host/titan_control"
)
sys.path.insert(0, str(HOST))

from smc_titan_control.adapter import Adapter  # noqa: E402
from smc_titan_control.broker_client import BrokerClient, UnixBroker  # noqa: E402
from smc_titan_control.profile import load_profile  # noqa: E402
from titan_broker import Broker  # noqa: E402
from titan_control_server import ControlServer, RestrictedControlPlane, hardware_writes_admitted  # noqa: E402
from titan_live_device import BUILD, SCHEMA, LiveDeviceSerial  # noqa: E402
from titan_transport import EXPECTED_UID  # noqa: E402

MOTION = "visual_a.vp_waveform_shift_rate"


def _usb(tmp: Path) -> dict:
    device = tmp / "fake-cdc"
    device.write_text("fake")
    return {
        "device": str(device),
        "lsof": [],
        "location": "fake",
        "serial": "x",
        "product": "fake",
        "aliases": [str(device)],
    }


def _checkpoint(tmp: Path, build: str = BUILD, schema: str = SCHEMA, extra: dict | None = None) -> Path:
    evidence = tmp / "evidence"
    evidence.mkdir(parents=True, exist_ok=True)
    rec = {
        "uid": EXPECTED_UID,
        "build": build,
        "schema_sha256": schema,
        "schema": schema,
        "ops": ["patch", "get_config", "snapshot"],
        "fields": [MOTION],
    }
    if extra:
        rec.update(extra)
    (evidence / "accepted-checkpoint.json").write_text(json.dumps(rec) + "\n")
    os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
    return evidence


def _gesture(adapter: Adapter, physical: float) -> None:
    raw = int(round(physical * 16256 / 128) * 128)
    adapter.ingest([{
        "type": "pitchwheel",
        "channel": 6,
        "raw14": raw,
        "physical": raw / 16256,
    }])


def _attach_stack(tmp: Path, device: LiveDeviceSerial, extra: dict | None = None):
    evidence = _checkpoint(tmp, build=device.build, schema=device.schema or SCHEMA, extra=extra)
    broker = Broker(evidence, backend=device, control_enabled=True)
    broker.attach(_usb(tmp))
    sock = evidence / "titan-control.sock"
    deadline = time.monotonic() + 2
    while time.monotonic() < deadline and not sock.exists():
        time.sleep(0.01)
    assert sock.exists(), "control socket missing after attach"
    transport = UnixBroker(str(sock))
    adapter = Adapter(BrokerClient(transport), load_profile())
    attached = adapter.attach()
    assert adapter.ready, attached
    return broker, adapter, transport


def test_c3a_unrelated_revision_keeps_final_value():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial(revision=2)
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            before = int(device.patch_applies)
            device.bump_revision()
            applied = adapter.flush()
            assert applied
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 240.0) < 1e-4
            assert device.patch_applies == before + 1
        finally:
            transport.close()
            broker.shutdown()


def test_c3a_same_target_external_edit_does_not_overwrite():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial(revision=2)
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            assert adapter.values.state(MOTION).acknowledged == 0.0
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            device.apply_external(MOTION, 100.0)
            before = int(device.patch_applies)
            applied = adapter.flush()
            assert applied == []
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 100.0) < 1e-4
            assert device.patch_applies == before
            assert adapter.values.state(MOTION).confirmation == "cancelled"
            _gesture(adapter, 1.0)
            assert adapter.flush() == []
        finally:
            transport.close()
            broker.shutdown()


def test_c3b_schema_drift_via_poll_once_blocks_writes():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            old_session = adapter.client.session
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            device.set_schema("f" * 64)
            broker.poll_i = 0
            broker.poll_once()
            before = int(device.patch_applies)
            applied = adapter.flush()
            assert applied == []
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 0.0) < 1e-4
            assert device.patch_applies == before
            plane_schema = (broker.control_plane.session.bound or {}).get("schema_sha256")
            assert plane_schema == "f" * 64
            assert broker.control_plane.session.token != old_session
            assert broker.bound.get("accepted") is False
        finally:
            transport.close()
            broker.shutdown()


def test_c3b_same_image_restart_via_poll_once_drops_queue():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            adapter.flush()
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 240.0) < 1e-4
            _gesture(adapter, 1.0)
            device.restart_same_image()
            broker.poll_i = 0
            broker.poll_once()
            before = int(device.patch_applies)
            applied = adapter.flush()
            assert applied == []
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 0.0) < 1e-4
            assert device.patch_applies == before
        finally:
            transport.close()
            broker.shutdown()


def test_c3b_restore_admitted_schema_allows_new_session_write():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            device.set_schema("f" * 64)
            broker.poll_i = 0
            broker.poll_once()
            assert broker.bound.get("accepted") is False
            device.set_schema(SCHEMA)
            broker.poll_i = 0
            broker.poll_once()
            adapter.reconnect()
            assert adapter.ready
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            applied = adapter.flush()
            assert applied
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 240.0) < 1e-4
        finally:
            transport.close()
            broker.shutdown()


def test_c3c_lost_reply_after_apply_does_not_double_apply():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            device.drop_next = True
            first = adapter.flush()
            assert first == []
            assert adapter.in_flight is None or adapter.in_flight.get("outcome") == "unknown"
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 240.0) < 1e-4
            applies = int(device.patch_applies)
            # A serial timeout leaves an uncertain stream: even readback must
            # wait for a newly identified transport. Never replay or invent an
            # acknowledgement from the fake device's out-of-band state.
            assert broker.transport.quarantine is True
            sent = bytes(device.tx)
            second = adapter.flush()
            assert second == []
            assert bytes(device.tx) == sent
            assert device.patch_applies == applies == 1
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 240.0) < 1e-4
            assert adapter.in_flight is not None
            assert adapter.in_flight["outcome"] == "unknown"
        finally:
            transport.close()
            broker.shutdown()


def test_c3c_drop_before_dispatch_does_not_replay_uncertain_request():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            device.drop_before = True
            first = adapter.flush()
            assert first == []
            assert device.patch_applies == 0
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 0.0) < 1e-4
            # The host cannot distinguish a lost request from a lost reply.
            # Both timeouts must quarantine before another serial write.
            assert broker.transport.quarantine is True
            sent = bytes(device.tx)
            second = adapter.flush()
            assert second == []
            assert bytes(device.tx) == sent
            assert device.patch_applies == 0
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 0.0) < 1e-4
            assert adapter.in_flight is not None
            assert adapter.in_flight["outcome"] == "unknown"
        finally:
            transport.close()
            broker.shutdown()


def test_c3c_quiesce_blocks_patch_and_does_not_replay():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            (broker.evidence / "quiesce.flag").write_text("1\n")
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            before = int(device.patch_applies)
            applied = adapter.flush()
            assert applied == []
            assert device.patch_applies == before
            (broker.evidence / "quiesce.flag").unlink()
            applied2 = adapter.flush()
            assert applied2 == []
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 0.0) < 1e-4
            assert device.patch_applies == before
        finally:
            transport.close()
            broker.shutdown()


def test_c3c_delayed_reply_confirms_once():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            device.delay_s = 0.05
            applied = adapter.flush()
            assert applied
            assert device.patch_applies == 1
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 240.0) < 1e-4
        finally:
            transport.close()
            broker.shutdown()


def test_c3c_wrong_applied_is_not_confirmed():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            device.stale_body_once = True
            applied = adapter.flush()
            assert applied == []
            assert device.cfg["visual_a"]["vp_waveform_shift_rate"] == 0.0
            assert adapter.last_error in {"applied_mismatch", "device_request_mismatch"}
        finally:
            transport.close()
            broker.shutdown()


def test_c3c_reconnect_does_not_reopen_serial():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            opens = broker.serial_opens
            adapter.reconnect()
            assert adapter.ready
            assert broker.serial_opens == opens
            assert device.closed_by_test is False
        finally:
            transport.close()
            broker.shutdown()


def test_c3c_stale_socket_file_without_listener_is_replaced():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        evidence = _checkpoint(tmp)
        leftover = evidence / "titan-control.sock"
        leftover.write_text("stale")
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            assert adapter.ready
            hello = transport.request({"op": "hello"})
            assert hello.get("ok") is True
        finally:
            transport.close()
            broker.shutdown()


def test_c3c_unix_patch_does_not_retry_after_close():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        sock_path = Path(raw) / "once.sock"
        connections = {"n": 0}
        server_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        if sock_path.exists():
            sock_path.unlink()
        server_sock.bind(str(sock_path))
        server_sock.listen(2)
        server_sock.settimeout(1.0)

        def serve():
            try:
                conn, _ = server_sock.accept()
            except OSError:
                return
            connections["n"] += 1
            conn.close()

        import threading
        thr = threading.Thread(target=serve, daemon=True)
        thr.start()
        time.sleep(0.05)
        client = UnixBroker(str(sock_path), timeout=0.4)
        try:
            reply = client.request({
                "op": "patch",
                "session": "s",
                "request_id": 1,
                "revision": 2,
                "fields": {MOTION: 240.0},
            })
            assert reply.get("ok") is False
            assert reply.get("error") == "broker_unavailable"
            time.sleep(0.2)
            assert connections["n"] == 1
        finally:
            client.close()
            server_sock.close()
            sock_path.unlink(missing_ok=True)


def test_c3d_missing_schema_or_ops_denies_and_nan_expiry_denies(tmp_path=None):
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        evidence = tmp / "evidence"
        evidence.mkdir()
        os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
        bound = {
            "uid": EXPECTED_UID,
            "build": BUILD,
            "schema_sha256": SCHEMA,
            "schema": SCHEMA,
        }
        (evidence / "accepted-checkpoint.json").write_text(json.dumps({
            "uid": EXPECTED_UID,
            "build": BUILD,
        }) + "\n")
        ok, reason = hardware_writes_admitted(evidence, bound)
        assert ok is False
        assert reason in {"schema_missing", "ops_missing"}
        (evidence / "accepted-checkpoint.json").write_text(json.dumps({
            "uid": EXPECTED_UID,
            "build": BUILD,
            "schema_sha256": SCHEMA,
            "ops": ["patch"],
            "expires_at": math.nan,
        }) + "\n")
        ok, reason = hardware_writes_admitted(evidence, bound)
        assert ok is False
        assert reason in {"expires_type", "expires_nonfinite"}


def test_c3d_wrong_scope_and_ambiguous_records_zero_backend_writes():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        extra = {"fields": ["visual_a.hue_position"]}
        broker, adapter, transport = _attach_stack(tmp, device, extra=extra)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            before = int(device.patch_applies)
            applied = adapter.flush()
            assert applied == []
            assert device.patch_applies == before
        finally:
            transport.close()
            broker.shutdown()
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        evidence = tmp / "evidence"
        evidence.mkdir()
        os.environ["TITAN_LIVE_CONTROL_ADMITTED"] = "1"
        rec_a = {
            "uid": EXPECTED_UID,
            "build": BUILD,
            "schema_sha256": SCHEMA,
            "ops": ["patch"],
        }
        rec_b = dict(rec_a)
        rec_b["ops"] = ["get_config"]
        (evidence / "accepted-checkpoint.json").write_text(json.dumps(rec_a) + "\n")
        (evidence / "accepted-candidate.json").write_text(json.dumps(rec_b) + "\n")
        bound = {"uid": EXPECTED_UID, "build": BUILD, "schema_sha256": SCHEMA, "schema": SCHEMA}
        ok, reason = hardware_writes_admitted(evidence, bound)
        assert ok is False
        assert reason == "ambiguous_approval"


def test_c3d_expired_while_waiting_zero_writes():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        extra = {"expires_at": time.time() + 0.05}
        broker, adapter, transport = _attach_stack(tmp, device, extra=extra)
        try:
            time.sleep(0.08)
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            before = int(device.patch_applies)
            applied = adapter.flush()
            assert applied == []
            assert device.patch_applies == before
        finally:
            transport.close()
            broker.shutdown()


if __name__ == "__main__":
    test_c3a_unrelated_revision_keeps_final_value()
    test_c3a_same_target_external_edit_does_not_overwrite()
    test_c3b_schema_drift_via_poll_once_blocks_writes()
    test_c3b_same_image_restart_via_poll_once_drops_queue()
    test_c3b_restore_admitted_schema_allows_new_session_write()
    test_c3c_lost_reply_after_apply_does_not_double_apply()
    test_c3c_drop_before_dispatch_does_not_replay_uncertain_request()
    test_c3c_quiesce_blocks_patch_and_does_not_replay()
    test_c3c_delayed_reply_confirms_once()
    test_c3c_wrong_applied_is_not_confirmed()
    test_c3c_reconnect_does_not_reopen_serial()
    test_c3c_stale_socket_file_without_listener_is_replaced()
    test_c3c_unix_patch_does_not_retry_after_close()
    test_c3d_missing_schema_or_ops_denies_and_nan_expiry_denies()
    test_c3d_wrong_scope_and_ambiguous_records_zero_backend_writes()
    test_c3d_expired_while_waiting_zero_writes()
    print("C3_CLOSE_PASS")
