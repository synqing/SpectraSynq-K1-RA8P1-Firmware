#!/usr/bin/env python3
"""OS-held CDC ownership lock shared by the broker and the read-only INFO path.

fcntl.flock on a dedicated lock file is the cooperative owner. The file is not
unlinked while locked. pyserial exclusive=True also flocks the serial fd
(LOCK_EX|LOCK_NB) on this macOS pyserial 3.5; it does not issue TIOCEXCL.
The broker applies TIOCEXCL after open where the kernel supports it.
"""
from __future__ import annotations

import fcntl
import os
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
LOCK_DIR = HERE / ".locks"
VIDPID = (0x045B, 0x5310)
TIOCEXCL = getattr(__import__("termios"), "TIOCEXCL", 0x2000740D)
TIOCNXCL = getattr(__import__("termios"), "TIOCNXCL", 0x2000740E)


def port_key(device: str) -> str:
    path = Path(device)
    try:
        path = path.resolve()
    except OSError:
        pass
    name = path.name.replace("cu.", "tty.")
    return "".join(ch if ch.isalnum() or ch in "._-" else "_" for ch in name)


def lock_path(device: str) -> Path:
    LOCK_DIR.mkdir(exist_ok=True)
    return LOCK_DIR / f"cdc-{port_key(device)}.lock"


def alias_paths(device: str) -> list[str]:
    path = str(Path(device))
    aliases = [path]
    if "/cu." in path:
        aliases.append(path.replace("/cu.", "/tty."))
    if "/tty." in path:
        aliases.append(path.replace("/tty.", "/cu."))
    out = []
    for item in aliases:
        if item not in out:
            out.append(item)
    return out


def lsof_owners(device: str) -> list[dict]:
    rows = []
    for path in alias_paths(device):
        proc = subprocess.run(["lsof", "-n", "-P", path], capture_output=True, text=True)
        lines = [line for line in (proc.stdout or "").splitlines() if line and not line.startswith("COMMAND")]
        for line in lines:
            parts = line.split()
            if len(parts) >= 2 and parts[1].isdigit():
                rows.append({"command": parts[0], "pid": int(parts[1]), "path": path, "raw": line})
    return rows


def acquire(device: str, owner: str) -> dict:
    """Acquire the process lock before opening CDC. Returns a handle dict."""
    path = lock_path(device)
    fd = os.open(str(path), os.O_RDWR | os.O_CREAT, 0o644)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except OSError as exc:
        os.close(fd)
        holders = lsof_owners(device)
        raise RuntimeError(f"CDC lock busy for {device}: {exc}; holders={holders}") from exc
    payload = f"owner={owner}\npid={os.getpid()}\ndevice={device}\n"
    os.lseek(fd, 0, os.SEEK_SET)
    os.ftruncate(fd, 0)
    os.write(fd, payload.encode())
    os.fsync(fd)
    return {"fd": fd, "path": str(path), "device": device, "owner": owner, "pid": os.getpid()}


def release(handle: dict | None) -> None:
    if not handle:
        return
    fd = handle.get("fd")
    if fd is None:
        return
    try:
        fcntl.flock(fd, fcntl.LOCK_UN)
    except OSError:
        pass
    os.close(fd)
    handle["fd"] = None


def apply_tiocexcl(fd: int) -> dict:
    notes = {"applied": False, "code": TIOCEXCL}
    try:
        fcntl.ioctl(fd, TIOCEXCL)
        notes["applied"] = True
    except OSError as exc:
        notes["error"] = str(exc)
    return notes


def drop_tiocexcl(fd: int) -> None:
    try:
        fcntl.ioctl(fd, TIOCNXCL)
    except OSError:
        pass
