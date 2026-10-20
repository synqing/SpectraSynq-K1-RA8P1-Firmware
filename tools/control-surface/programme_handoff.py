#!/usr/bin/env python3
"""Exclusive CDC ownership for programming. Server must not touch USB while claimed."""
from __future__ import annotations

import json
import os
import time
import urllib.error
import urllib.request
from pathlib import Path

LOCK = Path("/tmp/titan-cs-programme.lock")
QUIESCE_URL = "http://127.0.0.1:8765/api/programme/quiesce"
RESUME_URL = "http://127.0.0.1:8765/api/programme/resume"


def claimed() -> bool:
    return LOCK.exists()


def claim(owner: str) -> None:
    LOCK.write_text(json.dumps({
        "owner": owner,
        "pid": os.getpid(),
        "at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }) + "\n")


def release() -> None:
    try:
        LOCK.unlink()
    except FileNotFoundError:
        pass


def _post(url: str, timeout: float = 2.0) -> dict | None:
    try:
        req = urllib.request.Request(url, data=b"{}", method="POST",
                                     headers={"Content-Type": "application/json"})
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return json.loads(r.read())
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError, OSError):
        return None


def quiesce_server() -> dict:
    """Ask the control surface to stop USB. Best-effort: lock file is the gate."""
    got = _post(QUIESCE_URL)
    return got if isinstance(got, dict) else {"ok": False, "server": "absent"}


def resume_server() -> dict:
    got = _post(RESUME_URL)
    return got if isinstance(got, dict) else {"ok": False, "server": "absent"}


def wait_cdc_idle(seconds: float = 8.0) -> None:
    """Wait until no process holds a CDC flock for the Titan app/boot ports."""
    import sys
    root = Path(__file__).resolve().parents[1] / "serial-studio"
    sys.path.insert(0, str(root))
    import cdc_lock  # noqa: WPS433
    from serial.tools import list_ports
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        busy = False
        for p in list_ports.comports():
            if "usbmodem" not in (p.device or ""):
                continue
            try:
                h = cdc_lock.acquire(p.device, "programme-idle-check")
            except RuntimeError:
                busy = True
                break
            else:
                cdc_lock.release(h)
        if not busy:
            return
        time.sleep(0.15)
    raise RuntimeError("CDC still held after quiesce wait")
