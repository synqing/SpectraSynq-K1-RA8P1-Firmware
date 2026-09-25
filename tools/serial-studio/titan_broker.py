#!/usr/bin/env python3
"""Titan observe/broker: one CDC owner, TCP snapshots on 127.0.0.1:7778."""
from __future__ import annotations

import argparse
import json
import os
import queue
import signal
import socket
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


class Broker:
    def __init__(self, evidence: Path, replay_only: bool = False, backend=None):
        self.evidence = evidence
        self.replay_only = replay_only
        self.backend = backend
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
        self.last_row = None
        self.last_metrics = None
        self.last_palette = None
        self.poll_i = 0
        self.stop = False
        self.serial_opens = 0
        self.hw_writes = 0
        self.rtt = []
        self.errors = []
        self.poll_ops = []
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
        if self.backend is None:
            self.port = open_serial(usb["device"], self.lock_handle)
            self.serial_opens += 1
            backend = self.port
        else:
            backend = self.backend
            self.serial_opens += 1
        self.transport = Transport(backend, allowed={1}, log=self.log_tx)
        info = self.transport.info(timeout=2.0)
        raw_path = self.evidence / f"info-epoch-{self.epoch + 1}.json"
        checkpoint = load_checkpoint(self.evidence)
        raw_path.write_text(json.dumps({"usb": usb, "info": info["info"], "header_sequence": info["sequence"], "checkpoint": checkpoint}, indent=2) + "\n")
        bound = bind_identity(info["info"], checkpoint=checkpoint)
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
                got = self.transport.transact(op, b"", timeout=2.0)
            except (ObserveDenied, json.JSONDecodeError, RuntimeError) as exc:
                self.errors.append(f"opcode {op} probe: {exc}")
                continue
            if got["ok"]:
                admitted.add(op)
                body = json.loads(got["body"])
                if op == 6:
                    self.last_metrics = body
                else:
                    self.last_palette = body
                self.poll_ops.append(op)
        self.transport.allow(admitted)
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
            "hw_writes": self.hw_writes,
            "serial_opens": self.serial_opens,
            "epoch": self.epoch,
            "quiesced": self.quiesced(),
            "pid": os.getpid(),
            "pdm": (self.last_metrics or {}).get("pdm_target") if isinstance(self.last_metrics, dict) else None,
            "palette": self.last_palette,
        }
        (self.evidence / "live-stats.json").write_text(json.dumps(payload) + "\n")

    def poll_once(self) -> None:
        if self.replay_only or self.transport is None or self.quiesced():
            return
        ops = [op for op in (1, 6, 17) if op in self.transport.allowed]
        if not ops:
            return
        op = ops[self.poll_i % len(ops)]
        self.poll_i += 1
        started = time.monotonic()
        try:
            got = self.transport.transact(op, b"", timeout=2.0)
        except Exception as exc:
            self.errors.append(str(exc))
            self.recorder.put({"kind": "poll_error", "op": op, "error": str(exc)})
            return
        self.rtt.append(time.monotonic() - started)
        if not got["ok"]:
            self.errors.append(f"op {op} status {got['status']}")
            return
        body = json.loads(got["body"])
        self.sample_mono = time.monotonic()
        if op == 1:
            checkpoint = load_checkpoint(self.evidence)
            rebound = bind_identity(body, checkpoint=checkpoint)
            previous = self.bound or {}
            changed = (
                rebound.get("build") != previous.get("build")
                or rebound.get("accepted") != previous.get("accepted")
                or rebound.get("identified") != previous.get("identified")
            )
            self.bound = rebound
            if changed:
                self.epoch += 1
                self.last_metrics = None
                self.last_palette = None
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

    def loop(self) -> None:
        poll_every = 1.0 / POLL_HZ
        pub_every = 1.0 / PUBLISH_HZ
        next_poll = time.monotonic()
        next_pub = time.monotonic()
        while not self.stop:
            now = time.monotonic()
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
    broker = Broker(evidence, replay_only=args.replay_only)
    signal.signal(signal.SIGTERM, lambda *_: setattr(broker, "stop", True))
    signal.signal(signal.SIGINT, lambda *_: setattr(broker, "stop", True))
    try:
        if not args.replay_only:
            usb = find_titan()
            (evidence / "pre-open-ownership.json").write_text(json.dumps(usb, indent=2) + "\n")
            broker.attach(usb)
        broker.loop()
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
