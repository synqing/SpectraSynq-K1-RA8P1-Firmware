#!/usr/bin/env python3
"""Freeze an acceptance/tolerance file before any result exists, mechanically.

Why (Titan lag-free lane, 14 Oct 2026): hand-typed `frozen_utc` fields were wrong twice (P2 "07:55Z"
against an mtime of 07:35:45Z; P3 "10:20Z" against 10:11:01Z), and "never regenerated" rested on
memory. This tool stamps the time from the clock, records the SHA-256, makes the file read-only and
appends one STATUS line, so a later edit fails loudly and `verify` shows any change or any result
written before the freeze.

  freeze_tolerance.py freeze <tolerance.json> [--status STATUS.md] [--lane NAME]
  freeze_tolerance.py verify <tolerance.json> [result ...]

`freeze` refuses a file that already carries `frozen_utc`/`frozen_sha256`, or a JSON that is invalid.
`verify` exits 1 if the content no longer matches the recorded hash, the file is writable, or any
named result file is older than the freeze. Offline; touches only the named files.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import stat
import sys
from datetime import datetime, timezone
from pathlib import Path

STAMP_KEYS = ("frozen_utc", "frozen_sha256")


def body_sha256(data: dict) -> str:
    """Hash of the criterion itself, excluding the stamp fields, in canonical JSON."""
    core = {k: v for k, v in data.items() if k not in STAMP_KEYS}
    return hashlib.sha256(json.dumps(core, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def freeze(path: Path, status: Path | None, lane: str, now: datetime | None = None) -> dict:
    data = json.loads(path.read_text())
    if not isinstance(data, dict):
        raise SystemExit("REFUSED: tolerance must be a JSON object")
    if any(k in data for k in STAMP_KEYS):
        raise SystemExit("REFUSED: already frozen (a frozen file is never re-frozen or regenerated)")
    when = (now or datetime.now(timezone.utc)).strftime("%Y-%m-%dT%H:%M:%SZ")
    data["frozen_utc"] = when
    data["frozen_sha256"] = body_sha256(data)
    path.write_text(json.dumps(data, indent=2) + "\n")
    path.chmod(stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
    file_sha = hashlib.sha256(path.read_bytes()).hexdigest()
    if status is not None:
        with status.open("a") as out:
            out.write(f"{when} | {lane} | freeze | {path} frozen before results; body sha256 "
                      f"{data['frozen_sha256'][:16]}, file sha256 {file_sha[:16]}, mode 0444 | run\n")
    return {"frozen_utc": when, "body_sha256": data["frozen_sha256"], "file_sha256": file_sha}


def verify(path: Path, results: list[Path]) -> list[str]:
    problems = []
    data = json.loads(path.read_text())
    if not all(k in data for k in STAMP_KEYS):
        return ["not frozen with this tool (no frozen_utc / frozen_sha256)"]
    if body_sha256(data) != data["frozen_sha256"]:
        problems.append("criterion changed after the freeze (body hash mismatch)")
    if os.access(path, os.W_OK):
        problems.append("file is writable (expected mode 0444)")
    frozen = datetime.strptime(data["frozen_utc"], "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc).timestamp()
    for r in results:
        if r.stat().st_mtime < frozen:
            problems.append(f"result {r} was written before the freeze")
    return problems


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    f = sub.add_parser("freeze")
    f.add_argument("tolerance", type=Path)
    f.add_argument("--status", type=Path)
    f.add_argument("--lane", default="lane")
    v = sub.add_parser("verify")
    v.add_argument("tolerance", type=Path)
    v.add_argument("results", type=Path, nargs="*")
    a = ap.parse_args(argv)
    if a.cmd == "freeze":
        print(json.dumps(freeze(a.tolerance, a.status, a.lane)))
        return 0
    problems = verify(a.tolerance, a.results)
    print(json.dumps({"ok": not problems, "problems": problems}))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
