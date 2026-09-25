#!/usr/bin/env python3
"""Fail-closed Titan CDC inventory. Does not open the serial port or kill owners."""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
SS = REPO / "tools" / "serial-studio"
sys.path.insert(0, str(SS))

from cdc_lock import LOCK_DIR, VIDPID, alias_paths  # noqa: E402

WATCH_CMDS = ("titan_broker.py", "programme_scalar.py")
LSOF_TIMEOUT_S = 2.0


def timed_lsof(path: str) -> list[dict]:
    """Inventory holders. macOS lsof on a live CDC node can block; timeout is fail-closed."""
    try:
        proc = subprocess.run(
            ["lsof", "-n", "-P", path],
            capture_output=True,
            text=True,
            timeout=LSOF_TIMEOUT_S,
        )
    except subprocess.TimeoutExpired:
        return [{"command": "lsof", "pid": -1, "path": path, "raw": "TIMEOUT"}]
    rows = []
    for line in (proc.stdout or "").splitlines():
        if not line or line.startswith("COMMAND"):
            continue
        parts = line.split()
        if len(parts) >= 2 and parts[1].isdigit():
            rows.append({"command": parts[0], "pid": int(parts[1]), "path": path, "raw": line})
    return rows


def list_titan_ports() -> list[dict]:
    try:
        from serial.tools import list_ports
    except ImportError:
        return []
    rows = []
    for info in list_ports.comports():
        vid = getattr(info, "vid", None)
        pid = getattr(info, "pid", None)
        if vid == VIDPID[0] and pid == VIDPID[1]:
            rows.append(
                {
                    "device": info.device,
                    "vid": f"{vid:04x}",
                    "pid": f"{pid:04x}",
                    "serial_number": getattr(info, "serial_number", None),
                }
            )
    return rows


def ps_watch() -> list[dict]:
    proc = subprocess.run(["ps", "-ax", "-o", "pid=,command="], capture_output=True, text=True)
    hits = []
    for line in (proc.stdout or "").splitlines():
        text = line.strip()
        if not text:
            continue
        if any(name in text for name in WATCH_CMDS):
            pid_s, _, command = text.partition(" ")
            if pid_s.isdigit():
                hits.append({"pid": int(pid_s), "command": command.strip()})
    return hits


def lock_files() -> list[dict]:
    if not LOCK_DIR.is_dir():
        return []
    out = []
    for path in sorted(LOCK_DIR.glob("cdc-*.lock")):
        try:
            body = path.read_text(errors="replace")
        except OSError as exc:
            body = f"UNREADABLE:{exc}"
        holders = timed_lsof(str(path))
        out.append({"path": str(path), "body": body.strip(), "holders": holders})
    return out


def main() -> int:
    ports = list_titan_ports()
    owners = []
    lsof_timeouts = []
    for port in ports:
        seen = set()
        for alias in alias_paths(port["device"]):
            if alias in seen:
                continue
            seen.add(alias)
            for row in timed_lsof(alias):
                owners.append(row)
                if row.get("raw") == "TIMEOUT":
                    lsof_timeouts.append(alias)
    watch = ps_watch()
    locks = lock_files()
    live_locks = [row for row in locks if row["holders"]]
    summary = {
        "vid_pid": "045b:5310",
        "ports": ports,
        "lsof_owners": owners,
        "lsof_timeouts": lsof_timeouts,
        "watch_processes": watch,
        "lock_files": locks,
        "opened_serial": False,
        "killed": False,
    }
    if not ports:
        summary["status"] = "CDC_ABSENT"
        print(json.dumps(summary, indent=2))
        return 0
    if lsof_timeouts or owners or watch or live_locks:
        summary["status"] = "CDC_OWNED_OR_UNKNOWN"
        print(json.dumps(summary, indent=2))
        return 2
    summary["status"] = "CDC_FREE"
    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
