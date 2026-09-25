#!/usr/bin/env python3
"""Sustained FakeSerial live-control path. No CDC, no programming."""
from __future__ import annotations

import json
import os
import socket
import sys
import tempfile
import threading
import time
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
HOST = Path(
    "/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer"
    "/.worktrees/titan-control-001/host/titan_control"
)
sys.path.insert(0, str(HOST))

import live_protocol as proto  # noqa: E402
from smc_titan_control.adapter import Adapter  # noqa: E402
from smc_titan_control.broker_client import BrokerClient, UnixBroker, WholeConfigForbidden  # noqa: E402
from smc_titan_control.live_loop import RateLoop  # noqa: E402
from smc_titan_control.profile import load_profile  # noqa: E402
from titan_broker import Broker  # noqa: E402
from titan_control_server import (  # noqa: E402
    ControlServer,
    RestrictedControlPlane,
    SocketInUseError,
    hardware_writes_admitted,
)
from titan_live_device import BUILD, LiveDeviceSerial, SCHEMA  # noqa: E402
from titan_transport import EXPECTED_UID, FakeSerial, FramingError, Transport  # noqa: E402

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


def _checkpoint(tmp: Path, build: str = BUILD, schema: str = SCHEMA) -> Path:
    evidence = tmp / "evidence"
    evidence.mkdir(parents=True, exist_ok=True)
    rec = {
        "uid": EXPECTED_UID,
        "build": build,
        "schema_sha256": schema,
        "schema": schema,
        "ops": ["patch", "get_config", "snapshot"],
    }
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


def _attach_stack(tmp: Path, device: LiveDeviceSerial):
    evidence = _checkpoint(tmp, build=device.build, schema=device.schema or SCHEMA)
    broker = Broker(evidence, backend=device, control_enabled=True)
    broker.attach(_usb(tmp))
    sock = evidence / "titan-control.sock"
    deadline = time.monotonic() + 2
    while time.monotonic() < deadline and not sock.exists():
        time.sleep(0.01)
    transport = UnixBroker(str(sock))
    adapter = Adapter(BrokerClient(transport), load_profile())
    attached = adapter.attach()
    assert adapter.ready, attached
    return broker, adapter, transport


def test_final_settled_value_without_further_input():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            applied = adapter.flush()
            assert applied
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 240.0) < 1e-4
            before = broker.control_plane.handle({
                "op": "snapshot",
                "session": adapter.client.session,
            })
            assert before["ok"]
            assert before["snapshot"]["accepted_request_id"] == device.cfg["last_patch_request_id"]
            assert before["snapshot"]["last_rendered_request_id"] == 0
            device.render_frame()
            consume = device.consume_events[-1]
            snap = broker.control_plane.handle({
                "op": "snapshot",
                "session": adapter.client.session,
            })
            assert snap["snapshot"]["last_rendered_request_id"] == consume["request_id"]
            assert snap["snapshot"]["last_rendered_hop"] == consume["hop_sequence"]
            later = broker.control_plane.handle({
                "op": "snapshot",
                "session": adapter.client.session,
            })
            assert later["snapshot"]["last_rendered_request_id"] == consume["request_id"]
            assert later["snapshot"]["hop_sequence"] > snap["snapshot"]["hop_sequence"]
        finally:
            transport.close()
            broker.shutdown()


def test_rejected_revision_retries_final():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial(revision=2)
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            device.bump_revision()
            applied = adapter.flush()
            assert applied
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 240.0) < 1e-4
        finally:
            transport.close()
            broker.shutdown()


def test_lost_and_duplicate_outer_frames():
    fake = FakeSerial()
    t = Transport(fake, allowed={1})
    fake.feed_response(0, 1, 1, json.dumps({
        "protocol": 1, "uid": EXPECTED_UID, "build": "abc",
        "source": "s", "contract": "sr24000.hop180.bins80.xover40", "sequence": 1,
    }).encode())
    fake.feed_response(0, 1, 1, b"stale-duplicate")
    got = t.info(timeout=1)
    assert got["ok"]
    fake.feed_response(0, 99, 2, b"wrong-rid")
    try:
        t.transact(1, timeout=0.3)
        raise AssertionError("stale outer request id must fail")
    except FramingError:
        assert t.quarantine


def test_idle_gap_keeps_writer():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            time.sleep(3.0)
            hello = transport.request({"op": "hello"})
            assert hello.get("ok") is True
            assert hello.get("error") != "writer_busy"
        finally:
            transport.close()
            broker.shutdown()


def test_mode_change_cancels_and_schema_change_invalidates():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial(mode_a=8)
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            device.set_mode(32)
            device.bump_revision()
            applied = adapter.flush()
            assert applied == []
            assert adapter.mode_a == 32
            _gesture(adapter, 0.5)
            assert adapter.flush() == []
            device.set_schema("deadbeef" * 8)
            broker.poll_i = 0
            broker.poll_once()
            got = transport.request({
                "op": "patch",
                "session": adapter.client.session,
                "request_id": 9,
                "revision": adapter.revision,
                "fields": {MOTION: 40.0},
            })
            assert got.get("ok") is False
        finally:
            transport.close()
            broker.shutdown()


def test_second_writer_refused():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            sock = broker.evidence / "titan-control.sock"
            other = UnixBroker(str(sock))
            hello = other.request({"op": "hello"})
            assert hello.get("error") == "writer_busy"
            other.close()
            try:
                ControlServer(RestrictedControlPlane(), sock)
                raise AssertionError("live socket must refuse a second server")
            except SocketInUseError:
                pass
        finally:
            transport.close()
            broker.shutdown()


def test_malformed_and_whole_config_forbidden():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            transport._ensure().sendall(b"{not json\n")
            buf = b""
            while b"\n" not in buf:
                buf += transport._sock.recv(4096)
            assert b"bad_json" in buf
            try:
                transport.request({"op": "set_config"})
                raise AssertionError("set_config must be refused")
            except WholeConfigForbidden:
                pass
            assert device.whole_config_attempts == 0
        finally:
            transport.close()
            broker.shutdown()


def test_scheduler_serialises_poll_and_patch():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            errors = []

            def poller():
                try:
                    for _ in range(8):
                        broker.poll_once()
                except Exception as exc:  # noqa: BLE001
                    errors.append(exc)

            def writer():
                try:
                    _gesture(adapter, 0.0)
                    _gesture(adapter, 1.0)
                    adapter.flush()
                except Exception as exc:  # noqa: BLE001
                    errors.append(exc)

            t1 = threading.Thread(target=poller)
            t2 = threading.Thread(target=writer)
            t1.start()
            t2.start()
            t1.join()
            t2.join()
            assert not errors, errors
            assert broker.transport.in_flight is None
        finally:
            transport.close()
            broker.shutdown()


def test_25hz_loop_clean_shutdown():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            pending = {"physical": 0.0, "started": False}

            def tick():
                if not pending["started"]:
                    _gesture(adapter, 0.0)
                    pending["started"] = True
                else:
                    pending["physical"] = min(1.0, pending["physical"] + 0.25)
                    _gesture(adapter, pending["physical"])
                adapter.flush()

            loop = RateLoop(25.0, tick)
            loop.start()
            time.sleep(0.4)
            loop.close()
            assert loop.thread is None
            assert abs(device.cfg["visual_a"]["vp_waveform_shift_rate"] - 240.0) < 1e-4
            assert adapter.values.state(MOTION).confirmation == "confirmed"
            assert device.cfg["emit_on"] == 0
        finally:
            transport.close()
            broker.shutdown()


def test_stale_body_through_device_is_not_confirmed():
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
            assert adapter.last_error in {"applied_mismatch", "device_request_mismatch"}
            assert device.cfg["visual_a"]["vp_waveform_shift_rate"] == 0.0
        finally:
            transport.close()
            broker.shutdown()


def test_r3_1_schema_change_discovered_through_info():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            before = broker.control_plane.session.token
            device.set_schema("f" * 64)
            broker.poll_i = 0
            broker.poll_once()
            bound = broker.control_plane.session.bound
            ok, reason = hardware_writes_admitted(broker.evidence, bound)
            assert ok is False
            assert reason in {"schema_mismatch", "schema_missing"}
            assert broker.control_plane.session.token != before
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            applied = adapter.flush()
            assert applied == []
            assert device.cfg["visual_a"]["vp_waveform_shift_rate"] == 0.0
        finally:
            transport.close()
            broker.shutdown()


def test_r3_2_same_image_restart_does_not_replay_queue():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 0.25)
            assert adapter.flush()
            _gesture(adapter, 1.0)
            device.restart()
            broker.poll_i = 0
            broker.poll_once()
            applied = adapter.flush()
            assert applied == []
            assert device.cfg["visual_a"]["vp_waveform_shift_rate"] == 0.0
            assert adapter.ready is False or adapter.blocked
        finally:
            transport.close()
            broker.shutdown()


def test_r3_3_quiesce_rejects_writes_and_does_not_apply_later():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            (broker.evidence / "quiesce.flag").write_text("1\n")
            assert broker.quiesced()
            _gesture(adapter, 0.0)
            _gesture(adapter, 1.0)
            applied = adapter.flush()
            assert applied == []
            assert device.cfg["visual_a"]["vp_waveform_shift_rate"] == 0.0
            (broker.evidence / "quiesce.flag").unlink()
            assert adapter.flush() == []
            assert device.cfg["visual_a"]["vp_waveform_shift_rate"] == 0.0
        finally:
            transport.close()
            broker.shutdown()


def test_r3_4_duplicate_frame_quarantine_disconnects():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            _gesture(adapter, 0.0)
            _gesture(adapter, 0.1)
            device.duplicate_next = True
            adapter.flush()
            _gesture(adapter, 0.25)
            adapter.flush()
            assert adapter.ready is False
            assert adapter.connection_state == "disconnected"
            before = device.cfg["visual_a"]["vp_waveform_shift_rate"]
            for _ in range(3):
                adapter.flush()
            assert device.cfg["visual_a"]["vp_waveform_shift_rate"] == before
            assert adapter.last_error in {"quarantined", "transport"}
        finally:
            transport.close()
            broker.shutdown()


def test_observe_only_does_not_start_control():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        evidence = _checkpoint(tmp)
        broker = Broker(evidence, backend=device, control_enabled=False)
        try:
            broker.attach(_usb(tmp))
            assert broker.control_server is None
            assert 25 not in broker.transport.allowed
        finally:
            broker.shutdown()


def test_idle_beyond_writer_timeout():
    with tempfile.TemporaryDirectory() as raw:
        tmp = Path(raw)
        device = LiveDeviceSerial()
        broker, adapter, transport = _attach_stack(tmp, device)
        try:
            time.sleep(31.0)
            hello = transport.request({"op": "hello"})
            assert hello.get("ok") is True
            assert hello.get("error") != "writer_busy"
        finally:
            transport.close()
            broker.shutdown()


if __name__ == "__main__":
    test_final_settled_value_without_further_input()
    test_rejected_revision_retries_final()
    test_lost_and_duplicate_outer_frames()
    test_idle_gap_keeps_writer()
    test_mode_change_cancels_and_schema_change_invalidates()
    test_second_writer_refused()
    test_malformed_and_whole_config_forbidden()
    test_scheduler_serialises_poll_and_patch()
    test_25hz_loop_clean_shutdown()
    test_stale_body_through_device_is_not_confirmed()
    test_r3_1_schema_change_discovered_through_info()
    test_r3_2_same_image_restart_does_not_replay_queue()
    test_r3_3_quiesce_rejects_writes_and_does_not_apply_later()
    test_r3_4_duplicate_frame_quarantine_disconnects()
    test_observe_only_does_not_start_control()
    test_idle_beyond_writer_timeout()
    print("LIVE_CONTROL_PATH_PASS")
