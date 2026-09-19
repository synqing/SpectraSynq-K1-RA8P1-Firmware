#!/usr/bin/env python3
"""Single Titan transaction scheduler. Framing is run_led_smoke.packet/read_exact."""
from __future__ import annotations

import json
import struct
import sys
import time
import zlib
from pathlib import Path
from typing import Callable

SCRIPTS = Path(__file__).resolve().parents[2] / "scripts"
sys.path.insert(0, str(SCRIPTS))
from run_led_smoke import packet, read_exact  # noqa: E402

import cdc_lock  # noqa: E402

EXPECTED_UID = "545433931bd25436593630352d068363"
BAUD = 115200
WRITE_TIMEOUT = 2
READ_SLICE = 0.5
MAX_BODY = 19968
HEADER = 32
OBSERVE_OPS = {
    1: {"payload": b"", "name": "INFO"},
    6: {"payload": b"", "name": "platform_metrics"},
    17: {"payload": b"", "name": "palette_status"},
}
FORBIDDEN_OPS = {2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 19, 22}


class ObserveDenied(RuntimeError):
    pass


class FramingError(RuntimeError):
    pass


class FakeSerial:
    """In-memory serial for host tests. Reads may be 1-byte to exercise partial frames."""

    def __init__(self, chunk: int = 1, timeout: float = 0.5):
        self.timeout = timeout
        self.write_timeout = WRITE_TIMEOUT
        self.chunk = chunk
        self.rx = bytearray()
        self.tx = bytearray()
        self.closed = False
        self.fd = None
        self.exclusive = True

    def write(self, data: bytes) -> int:
        if self.closed:
            raise RuntimeError("write on closed fake serial")
        self.tx.extend(data)
        return len(data)

    def flush(self) -> None:
        return None

    def read(self, size: int) -> bytes:
        if self.closed:
            return b""
        if not self.rx:
            time.sleep(min(self.timeout, 0.01))
            return b""
        n = min(size, self.chunk, len(self.rx))
        out = bytes(self.rx[:n])
        del self.rx[:n]
        return out

    def close(self) -> None:
        self.closed = True

    def feed(self, data: bytes) -> None:
        self.rx.extend(data)

    def feed_response(
        self,
        status: int,
        req: int,
        sequence: int,
        body: bytes,
        cycles: int = 0,
        bad_header_crc: bool = False,
        bad_body_crc: bool = False,
    ) -> None:
        payload_crc = zlib.crc32(body) ^ (1 if bad_body_crc else 0)
        prefix = struct.pack("<4s6I", b"K1R1", status, req, sequence, len(body), cycles, payload_crc)
        hcrc = zlib.crc32(prefix) ^ (1 if bad_header_crc else 0)
        self.feed(prefix + struct.pack("<I", hcrc) + body)


class Transport:
    def __init__(self, backend, allowed: set[int] | None = None, log: Callable | None = None):
        self.backend = backend
        self.allowed = set(allowed or {1})
        self.next_req = 0
        self.in_flight = None
        self.writes: list[bytes] = []
        self.log = log or (lambda **kwargs: None)
        self.rx_buf = bytearray()
        self.timeouts = 0
        self.quarantine = False

    def _write(self, data: bytes) -> None:
        if self.backend.write(data) != len(data):
            raise RuntimeError("short USB write")
        if hasattr(self.backend, "flush"):
            self.backend.flush()
        self.writes.append(data)
        self.log(dir="TX", nbytes=len(data), hex=data.hex())

    def _read_exact(self, size: int, timeout: float) -> bytes:
        if hasattr(self.backend, "read") and type(self.backend).__name__ != "FakeSerial":
            return read_exact(self.backend, size, timeout)
        data = bytearray()
        deadline = time.monotonic() + timeout
        while len(data) < size and time.monotonic() < deadline:
            chunk = self.backend.read(size - len(data))
            if chunk:
                data.extend(chunk)
            else:
                time.sleep(0.001)
        if len(data) != size:
            self.timeouts += 1
            raise FramingError(f"timeout {len(data)}/{size}")
        return bytes(data)

    def transact(self, op: int, payload: bytes = b"", timeout: float = 2.0) -> dict:
        if self.quarantine:
            raise FramingError("transport quarantined until re-identify")
        if op in FORBIDDEN_OPS:
            raise ObserveDenied(f"opcode {op} is not observe-only")
        if op not in OBSERVE_OPS or op not in self.allowed:
            raise ObserveDenied(f"opcode {op} is not admitted")
        if payload != OBSERVE_OPS[op]["payload"]:
            raise ObserveDenied(f"opcode {op} payload not admitted")
        if self.in_flight is not None:
            raise RuntimeError("one outstanding transaction only")
        self.next_req += 1
        req = self.next_req
        outgoing = packet(op, req, 0, payload)
        self.in_flight = req
        try:
            self._write(outgoing)
            header = self._read_exact(HEADER, timeout)
            magic, status, rid, sequence, size, cycles, crc, hcrc = struct.unpack("<4s7I", header)
            if magic != b"K1R1" or zlib.crc32(header[:28]) != hcrc or rid != req or size > MAX_BODY:
                self.quarantine = True
                raise FramingError(
                    f"header mismatch magic={magic!r} status={status} rid={rid} want={req} size={size}"
                )
            body = self._read_exact(size, timeout) if size else b""
            if zlib.crc32(body) != crc:
                self.quarantine = True
                raise FramingError("body crc mismatch")
            self.log(
                dir="RX",
                op=op,
                req=req,
                status=status,
                sequence=sequence,
                cycles=cycles,
                nbytes=len(body),
                decode="ok" if status == 0 else "rejected",
            )
            if status:
                return {
                    "ok": False,
                    "status": status,
                    "op": op,
                    "req": req,
                    "sequence": sequence,
                    "cycles": cycles,
                    "body": body,
                }
            return {
                "ok": True,
                "status": 0,
                "op": op,
                "req": req,
                "sequence": sequence,
                "cycles": cycles,
                "body": body,
            }
        finally:
            self.in_flight = None

    def info(self, timeout: float = 2.0) -> dict:
        got = self.transact(1, b"", timeout)
        if not got["ok"]:
            raise FramingError(f"INFO rejected status={got['status']}")
        info = json.loads(got["body"])
        got["info"] = info
        return got

    def allow(self, ops: set[int]) -> None:
        extra = set(ops) - set(OBSERVE_OPS)
        if extra:
            raise ObserveDenied(f"cannot admit {extra}")
        self.allowed = set(ops)


def _clear_hupcl(fd: int) -> dict:
    import termios

    notes = {"hupcl_cleared": False}
    try:
        attrs = termios.tcgetattr(fd)
        iflag, oflag, cflag, lflag, ispeed, ospeed, cc = attrs
        notes["cflag_before"] = int(cflag)
        cflag &= ~termios.HUPCL
        termios.tcsetattr(fd, termios.TCSANOW, [iflag, oflag, cflag, lflag, ispeed, ospeed, cc])
        after = termios.tcgetattr(fd)
        notes["cflag_after"] = int(after[2])
        notes["hupcl_cleared"] = (after[2] & termios.HUPCL) == 0
    except (termios.error, OSError) as exc:
        notes["error"] = str(exc)
    return notes


def open_serial(device: str, lock_handle: dict) -> object:
    import serial

    port = serial.Serial(
        device,
        BAUD,
        timeout=READ_SLICE,
        write_timeout=WRITE_TIMEOUT,
        exclusive=True,
        dsrdtr=False,
        rtscts=False,
        xonxoff=False,
    )
    lock_handle["tiocexcl"] = cdc_lock.apply_tiocexcl(port.fd)
    lock_handle["hupcl"] = _clear_hupcl(port.fd)
    try:
        port.dtr = True
    except OSError as exc:
        lock_handle["dtr_error"] = str(exc)
    return port


def close_serial(port, lock_handle: dict | None = None) -> None:
    if port is None:
        return
    fd = getattr(port, "fd", None)
    if fd is not None:
        cdc_lock.drop_tiocexcl(fd)
    try:
        port.close()
    except OSError:
        pass
