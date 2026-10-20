#!/usr/bin/env python3
"""Titan observe/broker: one CDC owner, TCP snapshots on 127.0.0.1:7778."""
from __future__ import annotations

import argparse
import json
import os
import queue
import signal
import socket
import struct
import sys
import threading
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parents[1] / "scripts"))

import cdc_lock  # noqa: E402
from serial.tools import list_ports  # noqa: E402
from titan_snapshot import (  # noqa: E402
    ORIGIN_LIVE,
    ORIGIN_REPLAY,
    STALE_MS,
    bind_identity,
    encode,
    identity_identified,
    identity_ok,
    snapshot,
)
import emotiscope_control as donor_wire  # noqa: E402
import live_protocol as proto  # noqa: E402
from titan_control_server import (  # noqa: E402
    ControlServer,
    RestrictedControlError,
    RestrictedControlPlane,
    hardware_writes_admitted,
    load_approval_record,
)
from titan_transport import (  # noqa: E402
    EXPECTED_UID,
    FORBIDDEN_OPS,
    OBSERVE_OPS,
    ObserveDenied,
    Transport,
    close_serial,
    open_serial,
)

BIND = ("127.0.0.1", 7778)
MAX_SUBS = 4
MAX_PENDING = 2
MAX_LINE = 4096
PUBLISH_HZ = 5.0
POLL_HZ = 1.0
REC_MAX = 20000
REC_MAX_BYTES = 32 * 1024 * 1024
PID_PATH = HERE / ".locks" / "titan-broker.pid"
OWNER = "ss03-broker"


def identity_tuple_changed(previous: dict, rebound: dict) -> bool:
    keys = (
        "uid",
        "build",
        "schema",
        "schema_sha256",
        "source",
        "runtime_kind",
        "contract",
        "accepted",
        "identified",
        "boot_id",
    )
    for key in keys:
        if rebound.get(key) != previous.get(key):
            return True
    prev_seq = previous.get("device_sequence")
    new_seq = rebound.get("device_sequence")
    try:
        if prev_seq is not None and new_seq is not None:
            prev_i = int(prev_seq)
            new_i = int(new_seq)
            if new_i < prev_i:
                return True
            if new_i == 1 and prev_i > 1:
                return True
    except (TypeError, ValueError):
        return True
    return False


def load_checkpoint(evidence: Path) -> dict | None:
    path = evidence / "accepted-checkpoint.json"
    if path.is_file():
        data = json.loads(path.read_text())
        if isinstance(data, dict) and data.get("build"):
            return data
    build = os.environ.get("TITAN_ACCEPTED_BUILD", "").strip()
    if not build:
        return None
    checkpoint = {"build": build, "uid": EXPECTED_UID}
    source = os.environ.get("TITAN_ACCEPTED_SOURCE", "").strip()
    contract = os.environ.get("TITAN_ACCEPTED_CONTRACT", "").strip()
    if source:
        checkpoint["source"] = source
    if contract:
        checkpoint["contract"] = contract
    return checkpoint


def find_titan(*, inventory_lsof: bool = True) -> dict:
    matches = [p for p in list_ports.comports() if (p.vid, p.pid) == cdc_lock.VIDPID]
    if len(matches) != 1:
        raise RuntimeError(f"expected one Titan 045b:5310, found {len(matches)}")
    port = matches[0]
    usb = {
        "device": port.device,
        "location": port.location,
        "serial": port.serial_number,
        "product": port.product,
        "aliases": cdc_lock.alias_paths(port.device),
    }
    if inventory_lsof:
        usb["lsof"] = cdc_lock.lsof_owners(port.device)
    else:
        # macOS lsof on this CDC node can sit in uninterruptible wait. A timeout
        # is not a live holder. Cooperative flock remains the owner gate.
        usb["lsof"] = []
        usb["lsof_skipped"] = "timed_lsof_hangs_on_this_cdc_node; flock is owner gate"
    return usb


class Recorder:
    def __init__(self, path: Path, max_items: int = REC_MAX):
        self.path = path
        self.max_items = max_items
        self.q: queue.Queue = queue.Queue(max_items)
        self.dropped = 0
        self.high = 0
        self.failed = False
        self._io_lock = threading.Lock()
        path.parent.mkdir(parents=True, exist_ok=True)
        self._fp = path.open("a", encoding="utf-8")
        self._stop = False
        self._thr = threading.Thread(target=self._drain, daemon=True)
        self._thr.start()

    def put(self, item: dict) -> None:
        item = dict(item)
        item["t"] = time.monotonic()
        try:
            self.q.put_nowait(item)
            self.high = max(self.high, self.q.qsize())
        except queue.Full:
            self.dropped += 1
            self.failed = True
            # M2: gap marker when observation queue cannot accept the sample.
            self._write_gap_unlocked({"kind": "gap", "gap_reason": "queue_full", "dropped": self.dropped, "t": item["t"]})

    def _write_gap_unlocked(self, row: dict) -> None:
        with self._io_lock:
            try:
                self._rotate_if_needed_locked()
                self._fp.write(json.dumps(row) + "\n")
                self._fp.flush()
            except OSError:
                self.failed = True

    def _rotate_if_needed_locked(self) -> None:
        try:
            size = self._fp.tell()
        except OSError:
            return
        if size < REC_MAX_BYTES:
            return
        self._fp.close()
        stamp = time.strftime("%Y%m%dT%H%M%S")
        rotated = self.path.with_name(self.path.name + "." + stamp)
        self.path.replace(rotated)
        self._fp = self.path.open("a", encoding="utf-8")
        # M2: rotation itself is a continuity gap in the active file.
        try:
            self._fp.write(
                json.dumps({"kind": "gap", "gap_reason": "rotation", "rotated_to": rotated.name, "t": time.monotonic()})
                + "\n"
            )
            self._fp.flush()
        except OSError:
            self.failed = True

    def _drain(self) -> None:
        while not self._stop:
            try:
                item = self.q.get(timeout=0.2)
            except queue.Empty:
                continue
            with self._io_lock:
                try:
                    self._rotate_if_needed_locked()
                    self._fp.write(json.dumps(item) + "\n")
                    self._fp.flush()
                except OSError:
                    self.failed = True
                    self.dropped += 1

    def close(self) -> None:
        self._stop = True
        self._thr.join(timeout=2)
        with self._io_lock:
            try:
                self._fp.close()
            except OSError:
                pass


class Client:
    def __init__(self, sock: socket.socket, addr):
        self.sock = sock
        self.addr = addr
        self.pending: list[bytes] = []
        self.slow = False
        self.dead = False
        self.sent = 0
        sock.setblocking(False)

    def queue(self, line: bytes) -> str:
        if self.dead:
            return "dead"
        if len(self.pending) >= MAX_PENDING:
            if self.pending:
                self.pending[-1] = line
            else:
                self.pending.append(line)
            return "coalesce"
        self.pending.append(line)
        return "queued"

    def flush(self) -> None:
        if self.dead:
            return
        try:
            inbound = self.sock.recv(256)
        except BlockingIOError:
            inbound = None
        except OSError:
            self.dead = True
            return
        if inbound == b"":
            self.dead = True
            return
        if inbound:
            self.dead = True
            self.reject_payload = inbound[:64]
            return
        while self.pending:
            blob = self.pending[0]
            try:
                sent = self.sock.send(blob)
            except BlockingIOError:
                self.slow = True
                return
            except OSError:
                self.dead = True
                return
            if sent == 0:
                self.dead = True
                return
            if sent < len(blob):
                self.pending[0] = blob[sent:]
                self.slow = True
                return
            self.pending.pop(0)
            self.sent += 1


class Publisher:
    def __init__(self, recorder: Recorder):
        self.recorder = recorder
        self.clients: list[Client] = []
        self.skips = 0
        self.rejects = 0
        self.disconnects = 0
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(BIND)
        self.sock.listen(8)
        self.sock.setblocking(False)

    def accept(self) -> None:
        while len(self.clients) < MAX_SUBS:
            try:
                conn, addr = self.sock.accept()
            except BlockingIOError:
                return
            self.clients.append(Client(conn, addr))
            self.recorder.put({"kind": "subscriber", "event": "connect", "addr": list(addr)})
        try:
            conn, addr = self.sock.accept()
            conn.close()
            self.disconnects += 1
            self.recorder.put({"kind": "subscriber", "event": "refused", "addr": list(addr)})
        except BlockingIOError:
            return

    def publish(self, csv_line: str) -> None:
        line = (csv_line + "\n").encode("utf-8")
        if len(line) > MAX_LINE:
            self.skips += 1
            return
        live = []
        for client in self.clients:
            if client.dead:
                self.disconnects += 1
                if getattr(client, "reject_payload", None):
                    self.rejects += 1
                    self.recorder.put(
                        {
                            "kind": "subscriber",
                            "event": "payload_rejected",
                            "addr": list(client.addr),
                            "sample": client.reject_payload.hex(),
                        }
                    )
                try:
                    client.sock.close()
                except OSError:
                    pass
                continue
            action = client.queue(line)
            if action == "coalesce":
                self.skips += 1
            live.append(client)
        self.clients = live
        for client in self.clients:
            client.flush()
            if getattr(client, "reject_payload", None) and client.dead:
                self.rejects += 1
                self.recorder.put(
                    {
                        "kind": "subscriber",
                        "event": "payload_rejected",
                        "addr": list(client.addr),
                        "sample": client.reject_payload.hex(),
                    }
                )

    def close(self) -> None:
        for client in self.clients:
            try:
                client.sock.close()
            except OSError:
                pass
        self.clients = []
        try:
            self.sock.close()
        except OSError:
            pass


def identity_key(bound: dict | None) -> tuple:
    bound = bound or {}
    return (
        bound.get("uid"),
        bound.get("build"),
        bound.get("schema_sha256") or bound.get("schema"),
        bound.get("source"),
        bound.get("contract"),
        bool(bound.get("accepted")),
        bool(bound.get("identified")),
    )


class Broker:
    def __init__(self, evidence: Path, replay_only: bool = False, backend=None,
                 control_enabled: bool = False, continuous_output: bool = False,
                 emit_window_seconds: float = 8.0):
        self.evidence = evidence
        self.replay_only = replay_only
        self.backend = backend
        self.control_enabled = bool(control_enabled)
        self.continuous_output = bool(continuous_output)
        if not 1.0 <= emit_window_seconds <= 120.0:
            raise ValueError("diagnostic emit window must be within 1..120 seconds")
        self.emit_window_seconds = float(emit_window_seconds)
        self.recorder = Recorder(evidence / "broker.jsonl")
        self.publisher = Publisher(self.recorder)
        self.lock_handle = None
        self.port = None
        self.transport = None
        self.bound = None
        self.epoch = 0
        self.display_seq = 0
        self.last_progress = None
        self.same_progress = 0
        self.sample_mono = None
        self.topic_mono = {}
        self.emit_deadline = None
        self.last_row = None
        self.last_metrics = None
        self.last_palette = None
        self.poll_i = 0
        self.last_info_sequence = None
        self.stop = False
        self.device_path = None
        self.device_lost = None
        self.serial_opens = 0
        self.hw_writes = 0
        self.rtt = []
        self.errors = []
        self.poll_ops = []
        self._sched = threading.Lock()
        self.control_plane = None
        self.control_server = None
        evidence.mkdir(parents=True, exist_ok=True)

    def log_tx(self, **kwargs) -> None:
        if kwargs.get("dir") == "TX":
            self.hw_writes += 1
        self.recorder.put({"kind": "wire", **kwargs})

    def attach(self, usb: dict | None = None) -> dict:
        if self.replay_only:
            raise RuntimeError("replay-only cannot open hardware")
        usb = usb or find_titan()
        if usb["lsof"]:
            raise RuntimeError(f"CDC already owned: {usb['lsof']}")
        self.lock_handle = cdc_lock.acquire(usb["device"], OWNER)
        self.device_path = usb["device"]
        if self.backend is None:
            self.port = open_serial(usb["device"], self.lock_handle)
            self.serial_opens += 1
            backend = self.port
        else:
            backend = self.backend
            self.serial_opens += 1
        self.transport = Transport(
            backend, allowed={1}, log=self.log_tx, campaign=bool(self.control_enabled)
        )
        approval = load_approval_record(self.evidence)
        if approval and approval.get("protocol") == "titan-native":
            return self._attach_native(approval, usb)
        info = self.transport.info(timeout=2.0)
        raw_path = self.evidence / f"info-epoch-{self.epoch + 1}.json"
        checkpoint = load_checkpoint(self.evidence)
        raw_path.write_text(json.dumps({"usb": usb, "info": info["info"], "header_sequence": info["sequence"], "checkpoint": checkpoint}, indent=2) + "\n")
        bound = bind_identity(info["info"], checkpoint=checkpoint)
        self.topic_mono["identity"] = time.monotonic()
        self.last_info_sequence = info["info"].get("sequence")
        if not bound["identified"]:
            self.release_hardware()
            raise RuntimeError(f"identity rejected: {bound['reason']}")
        self.epoch += 1
        self.bound = bound
        self.last_progress = info["info"].get("sequence")
        self.same_progress = 0
        self.sample_mono = time.monotonic()
        admitted = {1}
        for op in (6, 17):
            self.transport.allow(admitted | {op})
            try:
                got = self.transact(op, b"", timeout=2.0)
            except (ObserveDenied, json.JSONDecodeError, RuntimeError) as exc:
                self.errors.append(f"opcode {op} probe: {exc}")
                continue
            if got["ok"]:
                admitted.add(op)
                body = json.loads(got["body"])
                if op == 6:
                    self.last_metrics = body
                    self.topic_mono["pdm"] = time.monotonic()
                else:
                    self.last_palette = body
                    self.topic_mono["palette"] = time.monotonic()
                self.poll_ops.append(op)
        if self.control_enabled:
            admitted |= {16, 23, 25}
            capabilities = info["info"].get("capabilities")
            opcodes = info["info"].get("opcodes")
            # INFO has passed protocol/UID identification; admit only the
            # live runtime's explicitly advertised, empty-payload MIR read.
            if (info["info"].get("runtime_kind") == "live"
                    and isinstance(capabilities, dict)
                    and capabilities.get("legacy_mir_json") is True
                    and isinstance(opcodes, dict)
                    and opcodes.get("legacy_mir") == 21):
                admitted.add(21)
            if (self.last_palette or {}).get("wire_profile") in (3, 4):
                admitted.add(19)
            self.transport.allow(admitted)
            self._bind_control_plane(bound)
        else:
            self.transport.allow(admitted)
        if 1 in admitted:
            try:
                refreshed = self.transport.info(timeout=2.0)
                seq = refreshed["info"].get("sequence")
                if seq is not None:
                    self.last_info_sequence = seq
                rebound = bind_identity(refreshed["info"], checkpoint=checkpoint)
                if rebound.get("identified"):
                    self.bound = rebound
            except Exception as exc:  # noqa: BLE001
                self.errors.append(f"opcode 1 watermark: {exc}")
        (self.evidence / "bind.json").write_text(
            json.dumps({
                "epoch": self.epoch,
                "bound": bound,
                "checkpoint": checkpoint,
                "admitted": sorted(admitted),
                "poll_ops": self.poll_ops,
            }, indent=2)
            + "\n"
        )
        self.recorder.put({
            "kind": "bind",
            "epoch": self.epoch,
            "uid": bound["uid"],
            "build": bound["build"],
            "accepted": bound["accepted"],
            "reason": bound["reason"],
        })
        return bound

    def _native_observation(self, plane: RestrictedControlPlane) -> dict:
        identity = plane._native_identity()
        if (not identity.get("ok")
                or identity["identity"].get("source_id") != self.bound["build"]
                or identity["identity"].get("contract") != "titan-replacement-v1"):
            raise RuntimeError(f"native identity rejected: {identity}")
        health = plane._native_health()
        status = plane._native_status()
        if (not health.get("ok") or not status.get("ok")
                or status.get("target_matches") is not True
                or health.get("boot") != status.get("boot")):
            raise RuntimeError(f"native health/status rejected: {health}; {status}")
        return {"identity": identity["identity"], "health": health, "status": status}

    def _attach_native(self, approval: dict, usb: dict) -> dict:
        # Select the wire protocol before sending any legacy INFO probe.
        try:
            if approval.get("uid") != EXPECTED_UID or not self.control_enabled:
                raise RuntimeError("native target/control admission required")
            bound = dict(approval, schema="replacement-health-v1",
                         schema_sha256="replacement-health-v1", identified=True,
                         accepted=True, ok=True, reason="native_admitted",
                         runtime_kind="live", contract="titan-replacement-v1")
            admitted, reason = hardware_writes_admitted(self.evidence, bound)
            if not admitted:
                raise RuntimeError(f"native admission rejected: {reason}")
            self.bound = bound
            self.transport.allowed.clear()
            plane = RestrictedControlPlane(transact=self.transact, evidence=self.evidence,
                                           bound=self._control_bound(bound))
            observed = self._native_observation(plane)
            self.epoch += 1
            self.native_boot = observed["health"]["boot"]
            self.last_progress = observed["health"]["completed_count"]
            self.bound["device_sequence"] = self.last_progress
            self.sample_mono = time.monotonic()
            self.topic_mono.update({k: self.sample_mono for k in ("identity", "native_health", "native_status")})
            self.native_observation = observed
            (self.evidence / "bind.json").write_text(json.dumps({
                "epoch": self.epoch, "bound": self.bound, "usb": usb,
                "native": observed, "poll_ops": [0x50524200, 0x50524205, 0x50524201]}, indent=2) + "\n")
            self._bind_control_plane(self.bound)
            self.recorder.put({"kind": "bind", "epoch": self.epoch, "bound": self.bound})
            return self.bound
        except BaseException:
            self.bound = None
            self.release_hardware()
            raise

    def _poll_native(self) -> None:
        try:
            observed = self._native_observation(self.control_plane)
            boot = observed["health"]["boot"]
            if boot != self.native_boot:
                self.epoch += 1
                self.control_plane.on_reconnect(self._control_bound(self.bound))
                self.native_boot = boot
            progress = observed["health"]["completed_count"]
            self.same_progress = self.same_progress + 1 if progress == self.last_progress else 0
            self.last_progress = progress
            self.bound["device_sequence"] = progress
            self.native_observation = observed
            self.sample_mono = time.monotonic()
            self.topic_mono.update({k: self.sample_mono for k in ("identity", "native_health", "native_status")})
            self.recorder.put({"kind": "native_telemetry", "body": observed})
        except Exception as exc:
            self.errors.append(str(exc))
            self.recorder.put({"kind": "native_poll_error", "error": str(exc)})
            # Re-identification requires a fresh attach, never a legacy fallback.
            self.stop = True
            self.control_plane.on_reconnect({})

    def transact(self, op: int, payload: bytes = b"", timeout: float = 2.0) -> dict:
        if self.transport is None:
            raise RuntimeError("no transport")
        if 0x50524200 <= op <= 0x50524207:
            if not self.control_enabled:
                raise RestrictedControlError("observe_only", "control service off")
            admitted, reason = hardware_writes_admitted(self.evidence, self._control_bound(self.bound))
            if not admitted:
                raise RestrictedControlError("ADMISSION_HOLD", reason)
            with self._sched:
                return self.transport.native_exchange(op, payload, timeout=timeout)
        with self._sched:
            if op == 25 and payload and len(payload) >= 4:
                if proto._u32(payload, 0) in (proto.CFG_PATCH, 9, 10, 11, 12, donor_wire.PATCH_SUB,
                                            donor_wire.EXTENDED_PATCH_SUB, donor_wire.INHOUSE_PATCH_SUB,
                                            donor_wire.WAVEFORMS_PATCH_SUB):
                    if self.quiesced():
                        raise RestrictedControlError("paused", "quiesce.flag")
                    if not self.control_enabled:
                        raise RestrictedControlError("observe_only", "control service off")
                    admitted, reason = hardware_writes_admitted(
                        self.evidence, self._control_bound(self.bound)
                    )
                    if not admitted:
                        raise RestrictedControlError("ADMISSION_HOLD", reason)
            return self.transport.transact(op, payload, timeout=timeout)

    def _control_bound(self, bound: dict | None) -> dict:
        bound = bound or {}
        return {
            "uid": bound.get("uid"),
            "build": bound.get("build"),
            "schema": bound.get("schema") or bound.get("schema_sha256"),
            "schema_sha256": bound.get("schema_sha256") or bound.get("schema"),
            "runtime_kind": bound.get("runtime_kind"),
            "contract": bound.get("contract"),
            "protocol": bound.get("protocol"),
            "controls_persistent": bound.get("controls_persistent"),
            "durable_commit": bound.get("durable_commit"),
        }

    def _bind_control_plane(self, bound: dict) -> None:
        if self.control_server is not None:
            self.control_server.close()
            self.control_server = None
        identity = self._control_bound(bound)
        self.control_plane = RestrictedControlPlane(
            transact=self.transact,
            evidence=self.evidence,
            bound=identity,
            isolated_execute=self._isolated_capture,
            fixture_status_read=self._fixture_status_read,
        )
        sock_path = self.evidence / "titan-control.sock"
        self.control_server = ControlServer(self.control_plane, sock_path)
        self.control_server.start()

    def _fixture_status_read(self, boot, epoch):
        """Maintenance-only single STATUS read; never a caller-selected opcode."""
        if self.transport is None or not self._sched.acquire(timeout=3.):
            raise TimeoutError('Fixture status transport unavailable')
        try:
            got = self.transport._isolated_fixture_exchange(
                0x50524602, struct.pack('<II', boot, epoch), timeout=3.)
            if got.get('ok') is not True:
                raise ValueError('Fixture STATUS rejected: ' + str(got.get('status')))
            return got['body']
        finally:
            self._sched.release()

    def _isolated_capture(self, request, admitted, current):
        # Import the existing collector only for a separately granted operation.
        import sys
        ael_root = '/Users/spectrasynq/ai-embedded-lab'
        if ael_root not in sys.path:
            sys.path.insert(0, ael_root)
        from ael.prism_native import validate_broker_capture, run_session, require_current_owner
        try:
            manifest, out = validate_broker_capture(request, admitted, current)
        except BaseException as exc:
            exc.collector_quiesced = True  # No transport operation has started.
            raise
        deadline = request['deadline']
        active = False
        allowed = {0x50524200, 0x50524201, 0x50524202, 0x50524205,
                   0x50524601, 0x50524602, 0x50524603, 0x50524604}
        def exchange(opcode, payload=b'', *, absolute_deadline=None, cancel_event=None):
            nonlocal active
            cutoff = min(deadline, absolute_deadline) if absolute_deadline is not None else deadline
            remaining = cutoff - time.monotonic()
            if opcode not in allowed or remaining <= 0 or self.transport is None:
                raise ValueError('Isolated collector opcode/deadline/transport refused')
            if cancel_event is not None and cancel_event.is_set():
                raise InterruptedError('Isolated capture cancelled')
            if opcode != 0x50524604 and time.time() >= admitted['isolated_capture_grant']['expires_at']:
                raise ValueError('Isolated grant expired during capture')
            if opcode != 0x50524604:
                require_current_owner(manifest)
            if not self._sched.acquire(timeout=remaining):
                raise TimeoutError('Isolated scheduler deadline exhausted')
            try:
                remaining = cutoff - time.monotonic()
                if remaining <= 0:
                    raise TimeoutError('Isolated transport deadline exhausted before write')
                if opcode == 0x50524601:
                    active = True  # Set before write: an interrupted reply is ambiguous.
                exchange_wire = (self.transport._isolated_fixture_exchange
                                 if 0x50524601 <= opcode <= 0x50524604
                                 else self.transport.native_exchange)
                got = exchange_wire(opcode, payload, timeout=min(3., remaining))
            finally:
                self._sched.release()
            if not got.get('ok'):
                raise ValueError('Isolated packet refused; no retry: ' + str(got.get('status')))
            if opcode == 0x50524604:
                active = False
            return got['body']
        try:
            result = run_session(manifest, out, 80, request['operation']['path'], deadline=deadline,
                               operation_sha256=request['operation']['sha256'], exchange_override=exchange)
            if active:
                raise ValueError('Collector cancellation not confirmed')
            return result
        except BaseException as exc:
            exc.collector_quiesced = not active
            raise

    def release_hardware(self) -> None:
        if self.transport and self.port is not None:
            close_serial(self.port, self.lock_handle)
        self.port = None
        self.transport = None
        cdc_lock.release(self.lock_handle)
        self.lock_handle = None

    def _progress(self):
        pdm = (self.last_metrics or {}).get("pdm_target") if isinstance(self.last_metrics, dict) else None
        if isinstance(pdm, dict) and pdm.get("ap_hops") is not None:
            return int(pdm["ap_hops"])
        if self.bound:
            return self.bound.get("device_sequence")
        return None

    def quiesced(self) -> bool:
        return (self.evidence / "quiesce.flag").exists()

    def write_live_stats(self) -> None:
        payload = {
            "identity": self.bound,
            "topic_mono": dict(self.topic_mono),
            "host_mono": time.monotonic(),
            "hw_writes": self.hw_writes,
            "serial_opens": self.serial_opens,
            "epoch": self.epoch,
            "quiesced": self.quiesced(),
            "pid": os.getpid(),
            "pdm": (self.last_metrics or {}).get("pdm_target") if isinstance(self.last_metrics, dict) else None,
            "palette": self.last_palette,
            "native": getattr(self, "native_observation", None),
        }
        temp = self.evidence / "live-stats.tmp"
        temp.write_text(json.dumps(payload) + "\n")
        temp.replace(self.evidence / "live-stats.json")

    def poll_once(self) -> None:
        if self.replay_only or self.transport is None or self.quiesced():
            return
        if (self.bound or {}).get("protocol") == "titan-native":
            self._poll_native()
            return
        ops = [op for op in (1, 6, 17) if op in self.transport.allowed]
        if not ops:
            return
        op = ops[self.poll_i % len(ops)]
        self.poll_i += 1
        started = time.monotonic()
        try:
            got = self.transact(op, b"", timeout=2.0)
        except Exception as exc:
            self.errors.append(str(exc))
            self.recorder.put({"kind": "poll_error", "op": op, "error": str(exc)})
            # A vanished USB node never recovers on this handle. Exit so the
            # supervisor restarts a fresh attach once Titan re-enumerates.
            if self.device_path and not os.path.exists(self.device_path):
                self.device_lost = {"device": self.device_path, "error": str(exc)}
                self.recorder.put({"kind": "device_lost", **self.device_lost})
                self.stop = True
            return
        self.rtt.append(time.monotonic() - started)
        if not got["ok"]:
            self.errors.append(f"op {op} status {got['status']}")
            return
        try:
            body = json.loads(got["body"])
        except (json.JSONDecodeError, UnicodeDecodeError, TypeError, ValueError) as exc:
            self.errors.append(f"op {op} malformed: {exc}")
            self.recorder.put({"kind": "poll_error", "op": op, "error": "malformed_body"})
            if op == 1 and self.control_plane is not None:
                self.control_plane.on_reconnect(self._control_bound(self.bound or {}))
            return
        self.sample_mono = time.monotonic()
        self.recorder.put({"kind": "telemetry", "op": op,
                           "request_started": started,
                           "response_parsed": self.sample_mono, "body": body})
        self.topic_mono[{1: "identity", 6: "pdm", 17: "palette"}[op]] = self.sample_mono
        if op == 1:
            checkpoint = load_checkpoint(self.evidence)
            rebound = bind_identity(body, checkpoint=checkpoint)
            previous = self.bound or {}
            changed = identity_tuple_changed(previous, rebound)
            try:
                new_seq = int(body.get("sequence"))
                prev_seq = int(self.last_info_sequence) if self.last_info_sequence is not None else None
                if prev_seq is not None and (new_seq < prev_seq or (new_seq == 1 and prev_seq > 1)):
                    changed = True
                self.last_info_sequence = new_seq
            except (TypeError, ValueError):
                changed = True
            self.bound = rebound
            if changed:
                self.epoch += 1
                self.last_metrics = None
                self.last_palette = None
                self.topic_mono.pop("pdm", None)
                self.topic_mono.pop("palette", None)
                if self.control_plane is not None:
                    self.control_plane.on_reconnect(self._control_bound(rebound))
                self.recorder.put({
                    "kind": "identity_change",
                    "epoch": self.epoch,
                    "bound": rebound,
                })
        elif op == 6:
            self.last_metrics = body
        elif op == 17:
            self.last_palette = body
        progress = self._progress()
        if progress is not None and progress == self.last_progress:
            self.same_progress += 1
        else:
            self.same_progress = 0
            self.last_progress = progress

    def current_row(self, origin: str = ORIGIN_LIVE) -> dict:
        age = 0.0 if self.sample_mono is None else (time.monotonic() - self.sample_mono) * 1000.0
        frozen = 1 if self.same_progress >= 8 and self.last_progress is not None else 0
        self.display_seq += 1
        return snapshot(
            origin=origin,
            bound=self.bound or {"ok": False},
            epoch=self.epoch,
            display_seq=self.display_seq,
            device_age_ms=age,
            device_progress=self._progress(),
            frozen=frozen,
            metrics=self.last_metrics,
            palette=self.last_palette,
            native=getattr(self, "native_observation", None),
        )

    def publish_row(self, origin: str = ORIGIN_LIVE) -> str:
        row = self.current_row(origin)
        line = encode(row)
        if not line:
            self.publisher.skips += 1
            self.recorder.put({"kind": "encode_fail", "row": row})
            return ""
        self.last_row = row
        self.publisher.accept()
        self.publisher.publish(line)
        self.recorder.put({"kind": "snapshot", "csv": line, "row": row})
        if self.recorder.failed:
            self.stop = True
        return line

    def _palette_flags(self, pal: dict, enable: bool) -> int:
        flags = 1
        if pal.get("automatic_cycle"):
            flags |= 2
        if enable:
            flags |= 4
        if pal.get("direction") == "edges_in":
            flags |= 8
        if pal.get("showcase") in (True, "true"):
            flags |= 16
        return flags

    def apply_emit(self, enable: bool) -> dict:
        """Arm or disarm physical output via opcode 16 flags bit 2. Preserve the current brightness."""
        if self.transport is None:
            raise RuntimeError("no transport")
        if 16 not in self.transport.allowed:
            raise ObserveDenied("opcode 16 is not admitted")
        if enable:
            if (self.evidence / "DIAGNOSTIC_FAULT").exists():
                raise RuntimeError("diagnostic fault latched")
            admitted, reason = hardware_writes_admitted(self.evidence, self._control_bound(self.bound))
            if not admitted:
                raise RestrictedControlError("ADMISSION_HOLD", reason)
            if not self.bound or not self.bound.get("accepted"):
                raise RuntimeError("identity not accepted")
            if any(time.monotonic() - self.topic_mono.get(k, -1e20) > 5.0
                   for k in ("identity", "pdm", "palette")):
                raise RuntimeError("stale pre-arm telemetry")
            if (self.last_palette or {}).get("mode_a") != 8:
                raise RuntimeError("mode drift")
            if any((self.last_palette or {}).get(k) != 0 for k in
                   ("pair_fault", "lane_a0_fault", "lane_a1_fault")):
                raise RuntimeError("fault or unknown fault state")
            if (self.last_palette or {}).get("emit_enabled") is not False:
                raise RuntimeError("already armed or unknown emit state")
            if (self.last_metrics or {}).get("pdm_target", {}).get("rate_locked") not in (1, True):
                raise RuntimeError("audio not locked")
        pal = self.last_palette
        # Mode selection may have happened since the observer's last poll.
        # Fault shutdown must use the current device configuration.
        if not pal or not enable:
            got = self.transact(17, b"", timeout=2.0)
            if not got.get("ok"):
                raise RuntimeError(f"palette status {got.get('status')}")
            pal = json.loads(got["body"])
            self.last_palette = pal
        flags = self._palette_flags(pal, enable)
        payload = struct.pack(
            "<10I",
            3,
            int(pal["palette_a"]),
            int(pal["palette_b"]),
            int(pal["mode_a"]),
            int(pal["mode_b"]),
            flags,
            int(pal["brightness"]),
            int(pal.get("output_channel") or 0),
            int(pal.get("transition_ms") or 1500),
            int(pal.get("travel_ms") or 4000),
        )
        if enable:
            self.emit_deadline = (None if getattr(self, "continuous_output", False)
                                  else time.monotonic() + getattr(self, "emit_window_seconds", 8.0))
        got = self.transact(16, payload, timeout=2.0)
        status = self.transact(17, b"", timeout=2.0)
        after = json.loads(status["body"]) if status.get("ok") else {}
        if status.get("ok"):
            self.last_palette = after
            self.topic_mono["palette"] = time.monotonic()
        if after.get("emit_enabled") is False:
            self.emit_deadline = None
        result = {
            "ok": bool(got.get("ok") and status.get("ok")),
            "enable": bool(enable),
            "continuous_output": getattr(self, "continuous_output", False),
            "configure_status": got.get("status"),
            "emit_enabled": after.get("emit_enabled"),
            "output_backend": after.get("output_backend"),
            "brightness": after.get("brightness"),
            "mode_a": after.get("mode_a"),
            "mode_b": after.get("mode_b"),
            "flags": flags,
        }
        name = "emit-arm.json" if enable else "emit-disarm.json"
        (self.evidence / name).write_text(json.dumps(result, indent=2) + "\n")
        self.recorder.put({"kind": "emit", **result})
        return result

    def _service_emit_requests(self) -> None:
        if self.transport is None or self.replay_only:
            return
        disarm = self.evidence / "EMIT_DISARM"
        arm = self.evidence / "EMIT_ARM"
        try:
            if self.emit_deadline is not None and time.monotonic() >= self.emit_deadline:
                disarm.write_text("watchdog\n")
            if disarm.exists():
                self.apply_emit(False)
                disarm.unlink(missing_ok=True)
                arm.unlink(missing_ok=True)
                return
            registers = self.evidence / "HWST_REQUEST"
            if registers.exists():
                if self.quiesced() or not self.control_enabled:
                    return
                if not self.bound or not self.bound.get("accepted"):
                    raise RuntimeError("register snapshot requires accepted identity")
                if (self.last_palette or {}).get("emit_enabled") is not False:
                    raise RuntimeError("register snapshot requires emission off")
                from titan_register_snapshot import decode
                got = self.transact(17, b"HWST")
                if not got.get("ok"):
                    raise RuntimeError("register snapshot rejected")
                raw = got["body"]
                decoded = decode(raw)
                (self.evidence / "registers.bin").write_bytes(raw)
                (self.evidence / "registers.json").write_text(json.dumps({
                    "identity": self.bound, "mono": time.monotonic(),
                    "registers": decoded, "scope": "sequential off-state MMIO reads; audio remains live"
                }, indent=2) + "\n")
                registers.unlink()
            bloom = self.evidence / "BLOOM_TRACE_REQUEST"
            if bloom.exists():
                if self.quiesced() or not self.control_enabled:
                    return
                admitted, reason = hardware_writes_admitted(self.evidence, self._control_bound(self.bound))
                if not admitted:
                    raise RuntimeError("Bloom trace requires accepted identity: " + reason)
                for board in (0, 1):
                    got = self.transact(17, b"BLM1" + board.to_bytes(4, "little"))
                    if not got.get("ok"):
                        raise RuntimeError("Bloom trace rejected")
                    (self.evidence / f"bloom-trace-{board}.bin").write_bytes(got["body"])
                (self.evidence / "bloom-trace.json").write_text(json.dumps({
                    "identity": self.bound, "mono": time.monotonic(),
                    "scope": "same-publication render feed and actual history writes; no acoustic classification"
                }))
                bloom.unlink()
            mir = self.evidence / "MIR_SNAPSHOT_REQUEST"
            if mir.exists():
                if self.quiesced() or not self.control_enabled:
                    return
                if not self.bound:
                    raise RuntimeError("MIR snapshot requires accepted identity")
                if not self.bound.get("accepted"):
                    admitted, reason = hardware_writes_admitted(self.evidence, self._control_bound(self.bound))
                    if not admitted:
                        raise RuntimeError("MIR snapshot requires accepted identity: " + reason)
                got = self.transact(21, b"")
                if not got.get("ok"):
                    raise RuntimeError("MIR snapshot rejected")
                body = json.loads(got["body"])
                if not isinstance(body, dict):
                    raise RuntimeError("MIR snapshot requires a JSON object")
                (self.evidence / "mir-snapshot.json").write_text(json.dumps({
                    "identity": self.bound, "mono": time.monotonic(), "body": body,
                    "scope": "bounded producer observation; not matched input or optical proof"
                }, indent=2) + "\n")
                mir.unlink()
            wire = self.evidence / "WIRE_SNAPSHOT_REQUEST"
            if wire.exists():
                if self.quiesced() or not self.control_enabled:
                    return
                if not self.bound:
                    raise RuntimeError("wire snapshot requires accepted identity")
                if not self.bound.get("accepted"):
                    # The live candidate admission already binds UID, build and
                    # schema. Reuse it for this read-only observation, too.
                    admitted, reason = hardware_writes_admitted(
                        self.evidence, self._control_bound(self.bound))
                    if not admitted:
                        raise RuntimeError("wire snapshot requires accepted identity: " + reason)
                emitting = (self.last_palette or {}).get("emit_enabled")
                # One immutable submission read is bounded independently of
                # normal music output. Continuous operation has no emit timer.
                marker = wire.read_text().strip()
                if marker not in ("", "bounded", "top", "bounded-top"):
                    raise RuntimeError("invalid wire snapshot board selector")
                board = 1 if marker in ("top", "bounded-top") else 0
                bounded = (marker in ("bounded", "bounded-top") and
                           (getattr(self, "continuous_output", False) or
                            (self.emit_deadline is not None and time.monotonic() < self.emit_deadline)))
                if emitting is not False and not (emitting is True and bounded):
                    raise RuntimeError("wire snapshot requires emission off or a bounded run")
                from palette_wire_snapshot import score_snapshot
                got = self.transact(19, board.to_bytes(4, "little") if board else b"")
                if not got.get("ok"):
                    raise RuntimeError("wire snapshot rejected")
                raw = got["body"]
                scored = score_snapshot(raw)
                if scored['output_channel'] != board:
                    raise RuntimeError("wire snapshot returned the wrong board")
                stem = "wire-snapshot-top" if board else "wire-snapshot"
                (self.evidence / (stem + ".bin")).write_bytes(raw)
                record = json.dumps({
                    "identity": self.bound, "mono": time.monotonic(),
                    "emission_active": emitting, "score": scored,
                    "scope": "retained top accepted submission; not a completion receipt" if board else "retained last submission attempt; not a completion receipt"
                }, indent=2) + "\n"
                (self.evidence / (stem + ".json")).write_text(record)
                archive = self.evidence / f"{stem}-{scored['sequence']:010d}"
                if not archive.with_suffix(".bin").exists():
                    archive.with_suffix(".bin").write_bytes(raw)
                    archive.with_suffix(".json").write_text(record)
                wire.unlink()
            request = self.evidence / "LOOPBACK_REQUEST"
            if request.exists():
                if self.quiesced() or not self.control_enabled:
                    return
                admitted, reason = hardware_writes_admitted(self.evidence, self._control_bound(self.bound))
                if not admitted or (self.last_palette or {}).get("emit_enabled") is not False:
                    raise RuntimeError("capture request denied: " + reason)
                got = self.transact(17, b"LBRQ")
                (self.evidence / "loopback-request.json").write_text(json.dumps({"ok": got.get("ok"), "status": got.get("status"), "bound": self.bound}) + "\n")
                request.unlink()
                if not got.get("ok"):
                    raise RuntimeError("capture request rejected")
            dump_request = self.evidence / "LOOPBACK_DUMP"
            if dump_request.exists():
                if (self.last_palette or {}).get("emit_enabled") is not False:
                    raise RuntimeError("capture readback requires emission off")
                raw = bytearray()
                total = 4096
                while len(raw) < total:
                    got = self.transact(17, b"LBPG" + struct.pack("<I", len(raw)))
                    if not got.get("ok") or not got.get("body"):
                        raise RuntimeError("capture page unavailable")
                    raw.extend(got["body"])
                    if len(raw) >= 88:
                        fields = struct.unpack_from("<22I", raw)
                        total = 88 + 480 + fields[7] * 2
                        if total > 88 + 480 + 40960 * 2:
                            raise RuntimeError("capture size invalid")
                if len(raw) != total:
                    raise RuntimeError("capture length mismatch")
                (self.evidence / "loopback-raw.bin").write_bytes(raw)
                names = ("generation payload_fnv1a truncated period_counts clock_hz valid armed samples_count "
                         "expected_cells rising falling first_rise last_fall extra_rising high_after_last "
                         "sample_period_counts open_ok start_level min_high max_high peek_head peek_tail").split()
                # Header is five metadata words followed by the original 17-word result.
                if len(names) != 22:
                    raise RuntimeError("capture header contract mismatch")
                meta = dict(zip(names, fields))
                meta.update(payload_hex=raw[88:568].hex(), samples=list(struct.unpack_from(
                    "<"+str(fields[7])+"H", raw, 568)), identity=self.bound,
                    uniform_sampling_qualified=False, optical="UNKNOWN", independent_din=False)
                (self.evidence / "loopback-capture.json").write_text(json.dumps(meta)+"\n")
                dump_request.unlink()
            if arm.exists():
                if self.quiesced():
                    return
                self.apply_emit(True)
                arm.unlink(missing_ok=True)
        except Exception as exc:  # noqa: BLE001
            arm.unlink(missing_ok=True)
            (self.evidence / "HWST_REQUEST").unlink(missing_ok=True)
            (self.evidence / "WIRE_SNAPSHOT_REQUEST").unlink(missing_ok=True)
            (self.evidence / "MIR_SNAPSHOT_REQUEST").unlink(missing_ok=True)
            (self.evidence / "BLOOM_TRACE_REQUEST").unlink(missing_ok=True)
            (self.evidence / "LOOPBACK_REQUEST").unlink(missing_ok=True)
            (self.evidence / "LOOPBACK_DUMP").unlink(missing_ok=True)
            (self.evidence / "DIAGNOSTIC_FAULT").write_text(str(exc) + "\n")
            if self.emit_deadline is not None:
                disarm.write_text("fault\n")
            self.errors.append(f"emit request: {exc}")
            (self.evidence / "emit-error.json").write_text(json.dumps({"error": str(exc)}) + "\n")

    def loop(self) -> None:
        poll_every = 1.0 / POLL_HZ
        pub_every = 1.0 / PUBLISH_HZ
        next_poll = time.monotonic()
        next_pub = time.monotonic()
        while not self.stop:
            now = time.monotonic()
            if (not self.replay_only and self.transport
                    and (self.bound or {}).get("protocol") != "titan-native"):
                self._service_emit_requests()
            if not self.replay_only and self.transport and now >= next_poll:
                self.poll_once()
                self.write_live_stats()
                next_poll = now + poll_every
            if now >= next_pub:
                origin = ORIGIN_REPLAY if self.replay_only else ORIGIN_LIVE
                self.publish_row(origin)
                next_pub = now + pub_every
            self.publisher.accept()
            time.sleep(0.01)

    def shutdown(self) -> None:
        self.stop = True
        if self.control_server is not None:
            self.control_server.close()
            self.control_server = None
        self.control_plane = None
        self.release_hardware()
        self.publisher.close()
        self.recorder.close()


def write_pid() -> None:
    PID_PATH.parent.mkdir(exist_ok=True)
    PID_PATH.write_text(str(os.getpid()))


def read_pid() -> int | None:
    if not PID_PATH.exists():
        return None
    text = PID_PATH.read_text().strip()
    return int(text) if text.isdigit() else None


def cmd_run(args) -> int:
    evidence = Path(args.evidence)
    evidence.mkdir(parents=True, exist_ok=True)
    write_pid()
    broker = Broker(
        evidence,
        replay_only=args.replay_only,
        control_enabled=bool(getattr(args, "live_control", False)),
        continuous_output=bool(getattr(args, "continuous_output", False)),
        emit_window_seconds=float(getattr(args, "emit_window_seconds", 8.0)),
    )
    signal.signal(signal.SIGTERM, lambda *_: setattr(broker, "stop", True))
    signal.signal(signal.SIGINT, lambda *_: setattr(broker, "stop", True))
    try:
        if not args.replay_only:
            usb = find_titan(inventory_lsof=False)
            (evidence / "pre-open-ownership.json").write_text(json.dumps(usb, indent=2) + "\n")
            broker.attach(usb)
        broker.loop()
        if broker.device_lost:
            (evidence / "device-lost.json").write_text(json.dumps(broker.device_lost) + "\n")
            return 75
        if broker.recorder.failed:
            (evidence / "recorder-failed.json").write_text(json.dumps({"dropped": broker.recorder.dropped}) + "\n")
            return 2
        return 0
    finally:
        stats = {
            "epoch": broker.epoch,
            "display_seq": broker.display_seq,
            "serial_opens": broker.serial_opens,
            "hw_writes": broker.hw_writes,
            "skips": broker.publisher.skips,
            "rejects": broker.publisher.rejects,
            "disconnects": broker.publisher.disconnects,
            "queue_high": broker.recorder.high,
            "dropped": broker.recorder.dropped,
            "errors": broker.errors[-20:],
            "rtt_ms": [round(x * 1000, 2) for x in broker.rtt[-20:]],
            "bound": broker.bound,
            "admitted": sorted(broker.transport.allowed) if broker.transport else [],
        }
        (evidence / "broker-stats.json").write_text(json.dumps(stats, indent=2) + "\n")
        broker.shutdown()
        PID_PATH.unlink(missing_ok=True)


def cmd_status(_args) -> int:
    pid = read_pid()
    print(json.dumps({"pid": pid, "pid_alive": pid_alive(pid) if pid else False, "lock_dir": str(HERE / ".locks")}))
    return 0


def pid_alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
        return True
    except OSError:
        return False


def cmd_stop(_args) -> int:
    pid = read_pid()
    if not pid:
        print(json.dumps({"stopped": False, "reason": "no pid file"}))
        return 0
    os.kill(pid, signal.SIGTERM)
    deadline = time.monotonic() + 8
    while time.monotonic() < deadline and pid_alive(pid):
        time.sleep(0.2)
    if pid_alive(pid):
        os.kill(pid, signal.SIGKILL)
        time.sleep(0.3)
    PID_PATH.unlink(missing_ok=True)
    print(json.dumps({"stopped": True, "pid": pid}))
    return 0


def cmd_info_once(args) -> int:
    evidence = Path(args.evidence)
    evidence.mkdir(parents=True, exist_ok=True)
    usb = find_titan(inventory_lsof=False)
    if usb["lsof"]:
        raise SystemExit(f"CDC owned: {usb['lsof']}")
    handle = cdc_lock.acquire(usb["device"], "ss03-info-once")
    port = None
    try:
        port = open_serial(usb["device"], handle)
        transport = Transport(port, allowed={1})
        info = transport.info(timeout=2.0)
        checkpoint = load_checkpoint(evidence)
        identified, identified_reason = identity_identified(info["info"])
        ok, reason = identity_ok(info["info"], checkpoint=checkpoint)
        payload = {
            "usb": usb,
            "identified": identified,
            "identified_reason": identified_reason,
            "ok": ok,
            "accepted": ok,
            "reason": reason,
            "checkpoint": checkpoint,
            "info": info["info"],
        }
        (evidence / "handoff-info.json").write_text(json.dumps(payload, indent=2) + "\n")
        print(json.dumps(payload))
        return 0 if identified else 1
    finally:
        close_serial(port, handle)
        cdc_lock.release(handle)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="cmd", required=True)
    run = sub.add_parser("run")
    run.add_argument("--evidence", required=True)
    run.add_argument("--replay-only", action="store_true")
    run.add_argument("--live-control", action="store_true",
                     help="explicitly enable opcode 23/25 control service; default is observe-only")
    run.add_argument("--continuous-output", action="store_true",
                     help="Captain authorised continuous product operation; no diagnostic emit timer")
    run.add_argument("--emit-window-seconds", type=float, default=8.0,
                     help="bounded diagnostic arm duration, 1..120 seconds; default 8")
    sub.add_parser("status")
    sub.add_parser("stop")
    info = sub.add_parser("info-once")
    info.add_argument("--evidence", required=True)
    args = parser.parse_args()
    if args.cmd == "run":
        return cmd_run(args)
    if args.cmd == "status":
        return cmd_status(args)
    if args.cmd == "stop":
        return cmd_stop(args)
    if args.cmd == "info-once":
        return cmd_info_once(args)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
