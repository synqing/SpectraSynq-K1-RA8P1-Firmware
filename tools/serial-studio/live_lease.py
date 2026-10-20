#!/usr/bin/env python3
"""Broker-side 128-bit campaign lease. TTL 30 s, renew every 5 s."""
from __future__ import annotations

import json
import os
import secrets
import socket
import time
from pathlib import Path

TTL_S = 30.0
RENEW_S = 5.0
_SOCKETS: dict[str, socket.socket] = {}


class LeaseError(RuntimeError):
    pass


def _path(run_dir: Path) -> Path:
    return Path(run_dir) / "campaign.lease.json"


def _sock_path(run_dir: Path) -> Path:
    return Path(run_dir) / "campaign.sock"


def _bind_socket(run_dir: Path) -> str:
    path = _sock_path(run_dir)
    if path.exists():
        path.unlink()
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.bind(str(path))
    sock.listen(1)
    sock.setblocking(False)
    _SOCKETS[str(Path(run_dir).resolve())] = sock
    return str(path)


def acquire(run_dir: Path, owner: str) -> dict:
    run_dir = Path(run_dir)
    run_dir.mkdir(parents=True, exist_ok=True)
    path = _path(run_dir)
    now = time.time()
    if path.is_file():
        current = json.loads(path.read_text())
        if now < float(current.get("expires_at", 0)) and pid_alive(current.get("pid")):
            raise LeaseError(f"lease held by {current.get('owner')} pid {current.get('pid')}")
        leftover = _SOCKETS.pop(str(run_dir.resolve()), None)
        if leftover is not None:
            leftover.close()
    token = secrets.token_hex(16)
    payload = {
        "token": token,
        "owner": owner,
        "pid": os.getpid(),
        "acquired_at": now,
        "expires_at": now + TTL_S,
        "renew_s": RENEW_S,
        "socket": _bind_socket(run_dir),
    }
    path.write_text(json.dumps(payload, indent=2) + "\n")
    return payload


def renew(run_dir: Path, token: str) -> dict:
    path = _path(run_dir)
    if not path.is_file():
        raise LeaseError("no lease")
    payload = json.loads(path.read_text())
    if payload.get("token") != token:
        raise LeaseError("token mismatch")
    payload["expires_at"] = time.time() + TTL_S
    path.write_text(json.dumps(payload, indent=2) + "\n")
    return payload


def require(run_dir: Path, token: str) -> dict:
    path = _path(run_dir)
    if not path.is_file():
        raise LeaseError("no lease")
    payload = json.loads(path.read_text())
    if payload.get("token") != token:
        raise LeaseError("token mismatch")
    if time.time() >= float(payload.get("expires_at", 0)):
        raise LeaseError("lease expired")
    return payload


def release(run_dir: Path, token: str) -> None:
    path = _path(run_dir)
    if not path.is_file():
        return
    payload = json.loads(path.read_text())
    if payload.get("token") != token:
        raise LeaseError("token mismatch")
    key = str(Path(run_dir).resolve())
    sock = _SOCKETS.pop(key, None)
    if sock is not None:
        sock.close()
    sock_path = _sock_path(run_dir)
    if sock_path.exists():
        sock_path.unlink()
    path.unlink()


def pid_alive(pid) -> bool:
    try:
        pid = int(pid)
    except (TypeError, ValueError):
        return False
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True
