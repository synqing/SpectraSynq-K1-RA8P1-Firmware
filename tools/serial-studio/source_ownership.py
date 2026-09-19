#!/usr/bin/env python3
"""Decide whether Serial Studio's current session is the Titan Process I/O fixture."""
from __future__ import annotations

from pathlib import Path

TITAN_TITLE = "Titan Mini Observability"
PROCESS_BUS = 8
REPLAY_NAME = "titan_fixture_replay.py"


def process_source_ok(
    io_status: dict,
    sources: list,
    expected_replay: Path,
    process_config: dict | None = None,
) -> tuple[bool, str]:
    """Return (ok, reason). Never treats UART/TCP as the Titan fixture."""
    slug = str(io_status.get("busTypeSlug") or "")
    bus = io_status.get("busType")
    if bus not in (None, PROCESS_BUS) and slug not in ("", "process"):
        return False, f"active bus is {slug or bus}, not Process I/O"
    if not sources:
        return False, "no sources"
    src = sources[0] if isinstance(sources[0], dict) else {}
    if int(src.get("busType", -1)) != PROCESS_BUS and src.get("busTypeSlug") != "process":
        return False, f"source bus is {src.get('busTypeSlug') or src.get('busType')}, not Process I/O"
    conn = src.get("connection") or src.get("connectionSettings") or {}
    exe = str(conn.get("executable") or (process_config or {}).get("executable") or "")
    if Path(exe).name != REPLAY_NAME:
        return False, f"executable {exe!r} is not {REPLAY_NAME}"
    want = str(expected_replay.resolve())
    if exe and Path(exe).resolve() != Path(want):
        return False, f"executable path {exe!r} != {want}"
    return True, "process-io fixture"


def titan_project_ok(project_status: dict, min_groups: int = 8) -> tuple[bool, str]:
    title = str(project_status.get("title") or "")
    groups = int(project_status.get("groupCount") or 0)
    if title != TITAN_TITLE:
        return False, f"title {title!r} is not {TITAN_TITLE!r}"
    if groups < min_groups:
        return False, f"groupCount {groups} < {min_groups}"
    return True, "titan project"


def should_leave_session_alone(
    io_status: dict,
    project_status: dict,
    sources: list,
    expected_replay: Path,
    process_config: dict | None = None,
) -> str | None:
    """If a foreign session is live, return a reason to abort. None means we may proceed."""
    if not io_status.get("isConnected"):
        return None
    ok, reason = process_source_ok(io_status, sources, expected_replay, process_config)
    proj_ok, _ = titan_project_ok(project_status, min_groups=1)
    if ok and proj_ok:
        return None
    return f"connected session is not Titan Process I/O ({reason})"
