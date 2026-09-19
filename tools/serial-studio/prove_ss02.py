#!/usr/bin/env python3
"""SS-02 proof: freshness, new Historian session, field-for-field replay, fixture recovery."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import signal
import socket
import sqlite3
import struct
import subprocess
import sys
import time
from pathlib import Path

SS = Path("/Users/spectrasynq/Serial-Studio")
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(SS / "tests" / "utils"))
sys.path.insert(0, str(HERE))
from api_client import APIError, SerialStudioClient  # noqa: E402
from setup_titan_observability import (  # noqa: E402
    CONTROL,
    REPLAY,
    SAVE_PATH,
    SetupError,
    canonical_config,
    configure_process_io,
    connect_process,
    guard_session,
    install_parser,
    require,
)
from source_ownership import process_source_ok  # noqa: E402
from titan_fixture_replay import FREEZE_FLAG, IDENTITY, PAUSE_FLAG, SCENARIO_FLAG  # noqa: E402

PROOFS = HERE / "ss-02-proofs"
BANNER = "SIMULATED DATA — TITAN NOT CONNECTED"
STALE_LABEL = "STALE — NO VALID PACKET"
LIVE_ARGS = "--scenario live --hold 0 --period 0.2"
APP_NAME = "Serial Studio Pro"
API_HOST = "127.0.0.1"
API_PORT = 7777
CHECKPOINT_TITLES = (
    "SIMULATED / LIVE banner",
    "Operating state",
    "Test result",
    "UID",
    "Build",
    "Source pin",
    "Contract",
    "Lane A RMS",
    "LED health",
)
IDENTIFYING_EXPECT = {
    "SIMULATED / LIVE banner": BANNER,
    "Operating state": "IDENTIFYING",
    "Test result": "NONE",
    "UID": "UNIDENTIFIED",
    "Build": "UNIDENTIFIED",
    "Source pin": "UNIDENTIFIED",
    "Contract": "UNIDENTIFIED",
    "Lane A RMS": "UNAVAILABLE",
    "LED health": "UNAVAILABLE",
}
LIVE_EXPECT = {
    "SIMULATED / LIVE banner": BANNER,
    "Operating state": "LIVE",
    "Test result": "NONE",
    "UID": IDENTITY["uid"],
    "Build": IDENTITY["build"],
    "Source pin": IDENTITY["source"],
    "Contract": IDENTITY["contract"],
    "Lane A RMS": "-28",
    "LED health": "OK",
}


def clear_flags() -> None:
    for path in (PAUSE_FLAG, FREEZE_FLAG, SCENARIO_FLAG):
        path.unlink(missing_ok=True)


def write_flag(path: Path, text: str = "") -> None:
    path.write_text(text)


def values(c):
    data = require(c, "dashboard.getData")
    rows = {}
    for group in (data.get("frame") or {}).get("groups") or []:
        for ds in group.get("datasets") or []:
            rows[ds.get("title") or ds.get("alias")] = ds.get("value")
    return rows


def checkpoint_of(snap: dict) -> dict:
    return {title: snap.get(title) for title in CHECKPOINT_TITLES}


def field_equal(title: str, got, expect: str) -> bool:
    actual = str(got if got is not None else "")
    if title == "Lane A RMS" and expect == "-28":
        return actual in ("-28", "-28.0")
    return actual == expect


def checkpoint_matches(snap: dict, expect: dict) -> bool:
    return all(field_equal(title, snap.get(title), value) for title, value in expect.items())


def table_value(payload) -> float:
    if isinstance(payload, dict):
        return float(payload.get("value") or 0)
    return float(payload or 0)


def table_get(c, name: str):
    try:
        return require(c, "project.dataTable.getValue", {"table": "titan_watchdog", "name": name})
    except (APIError, SetupError):
        return require(c, "project.dataTable.getValue", {"table": "titan_watchdog", "register": name})


def wait_until(c, pred, seconds: float, what: str) -> dict:
    deadline = time.monotonic() + seconds
    snap = {}
    while time.monotonic() < deadline:
        snap = values(c)
        if pred(snap):
            return snap
        time.sleep(0.3)
    keys = CHECKPOINT_TITLES + ("Packet freshness", "Last valid (ms)", "Sequence", "Host stale")
    raise RuntimeError(f"{what}: { {k: snap.get(k) for k in keys} }")


def sequence_of(snap: dict) -> int:
    try:
        return int(float(str(snap.get("Sequence") or "0")))
    except ValueError:
        return 0


def live_sequence(c) -> int:
    snap = sequence_of(values(c))
    if snap > 0:
        return snap
    latest = require(c, "io.getLatestFrame").get("values") or []
    if len(latest) >= 24:
        try:
            return int(float(str(latest[23])))
        except ValueError:
            return 0
    return 0


def live_control_code(c) -> str:
    try:
        return require(c, "controlScript.get").get("code") or ""
    except (APIError, SetupError):
        return ""


def open_saved_project(c) -> None:
    status = require(c, "project.getStatus")
    path = str(status.get("filePath") or status.get("path") or "")
    if Path(path) != SAVE_PATH.resolve() or status.get("title") != "Titan Mini Observability":
        require(c, "project.open", {"filePath": str(SAVE_PATH)})
        time.sleep(0.8)
    if "validAge" not in live_control_code(c):
        require(c, "controlScript.setCode", {"code": CONTROL})


def connect_live_once(c) -> dict:
    io, project, srcs, proc = guard_session(c)
    require(c, "dashboard.setOperationMode", {"mode": 0})
    install_parser(c)
    if io.get("isConnected") and proc.get("arguments") == LIVE_ARGS:
        ok, reason = process_source_ok(io, srcs, REPLAY, proc)
        if not ok:
            raise SetupError(reason)
        return io
    if io.get("isConnected"):
        require(c, "io.disconnect")
        time.sleep(0.4)
    configure_process_io(c, LIVE_ARGS)
    install_parser(c)
    return connect_process(c)


def list_sessions(c, db_path: str) -> list:
    listed = ensure_database_open(c, db_path)
    return listed.get("sessions") or listed.get("items") or []


def ensure_database_open(c, db_path: str, seconds: float = 10.0) -> dict:
    deadline = time.monotonic() + seconds
    last_err = None
    opened = False
    while time.monotonic() < deadline:
        try:
            listed = require(c, "sessions.list")
            if listed.get("isOpen") or listed.get("sessions") is not None:
                return listed
        except (APIError, SetupError) as exc:
            last_err = exc
            if not opened:
                require(c, "sessions.openDatabase", {"filePath": db_path})
                opened = True
        time.sleep(0.25)
    raise RuntimeError(f"historian database did not open: {last_err}")


def wait_disconnected(c, seconds: float = 8.0) -> None:
    deadline = time.monotonic() + seconds
    last = {}
    while time.monotonic() < deadline:
        last = require(c, "io.getStatus")
        if not last.get("isConnected"):
            return
        time.sleep(0.2)
    raise RuntimeError(f"I/O still connected after disconnect: {last}")


def decode_text_blob(blob) -> list:
    if not blob:
        return []
    data = bytes(blob)
    out = []
    index = 0
    while index + 4 <= len(data):
        count = int.from_bytes(data[index : index + 4], "little")
        index += 4
        out.append(data[index : index + count].decode("utf-8", "replace"))
        index += count
    return out


def session_title_values(db_path: str, session_id: int, title: str) -> list:
    con = sqlite3.connect(f"file:{db_path}?mode=ro", uri=True)
    try:
        rows = con.execute(
            """
            SELECT b.frames, b.is_numeric, b.texts, b.values_blob
            FROM blocks b
            JOIN columns c
              ON c.session_id = b.session_id AND c.unique_id = b.unique_id
            WHERE b.session_id = ? AND c.title = ?
            ORDER BY b.t0_ns, b.block_number
            """,
            (session_id, title),
        ).fetchall()
    finally:
        con.close()
    out = []
    for frames, _is_numeric, texts, blob in rows:
        if texts:
            out.extend(decode_text_blob(texts))
            continue
        if not blob:
            continue
        count = int(frames or (len(blob) // 8))
        if count <= 0:
            continue
        try:
            out.extend(struct.unpack(f"<{count}d", bytes(blob)[: count * 8]))
        except struct.error:
            continue
    return [str(item) for item in out]


def sqlite_session_stats(db_path: str) -> list:
    con = sqlite3.connect(f"file:{db_path}?mode=ro", uri=True)
    try:
        rows = con.execute("SELECT session_id, started_at FROM sessions").fetchall()
        raw = dict(con.execute("SELECT session_id, COUNT(*) FROM raw_bytes GROUP BY session_id"))
    finally:
        con.close()
    return [
        {
            "session_id": int(session_id),
            "started_at": started,
            "raw_count": int(raw.get(session_id) or 0),
        }
        for session_id, started in rows
    ]


def session_raw_contains(db_path: str, session_id: int, needle: str) -> bool:
    con = sqlite3.connect(db_path)
    try:
        rows = con.execute("SELECT data FROM raw_bytes WHERE session_id=?", (session_id,)).fetchall()
        blob = b"".join(
            r[0] if isinstance(r[0], (bytes, bytearray)) else str(r[0]).encode() for r in rows
        )
        return needle.encode() in blob
    finally:
        con.close()


def api_up() -> bool:
    sock = socket.socket()
    sock.settimeout(1.0)
    try:
        sock.connect((API_HOST, API_PORT))
        return True
    except OSError:
        return False
    finally:
        sock.close()


def serial_studio_pids() -> list[int]:
    proc = subprocess.run(["pgrep", "-f", "Serial-Studio-Pro"], capture_output=True, text=True)
    return [int(item) for item in proc.stdout.split() if item.strip().isdigit()]


def pid_alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
        return True
    except OSError:
        return False


def cua_call(tool: str, payload: dict) -> dict:
    completed = subprocess.run(
        ["cua-driver", "call", tool, json.dumps(payload)],
        check=False,
        capture_output=True,
        text=True,
        timeout=20,
    )
    if completed.returncode != 0:
        raise RuntimeError((completed.stderr or completed.stdout or "cua-driver failed").strip())
    return json.loads(completed.stdout or "{}")


def titan_window_id(pid: int) -> int | None:
    windows = cua_call("list_windows", {"pid": pid}).get("windows") or []
    for window in windows:
        if window.get("is_on_screen") and "Titan Mini" in str(window.get("title") or ""):
            return int(window["window_id"])
    for window in windows:
        if window.get("is_on_screen"):
            return int(window["window_id"])
    return None


def session_player_windows(pid: int) -> list:
    windows = cua_call("list_windows", {"pid": pid}).get("windows") or []
    found = []
    for window in windows:
        title = str(window.get("title") or "")
        if "Session Player" in title or title == "Session Player":
            found.append(window)
    return found


def close_session_player() -> dict:
    notes: dict = {"closed": False, "titles": []}
    pids = serial_studio_pids()
    if not pids:
        notes["error"] = "Serial Studio Pro is not running"
        return notes
    pid = pids[0]
    notes["pid"] = pid
    try:
        found = session_player_windows(pid)
        notes["titles"] = [w.get("title") for w in found]
        if not found:
            return notes
        for window in found:
            cua_call(
                "press_key",
                {
                    "pid": pid,
                    "key": "w",
                    "modifiers": ["cmd"],
                    "window_id": int(window["window_id"]),
                    "delivery_mode": "foreground",
                },
            )
        time.sleep(0.6)
        remaining = session_player_windows(pid)
        notes["remaining"] = [w.get("title") for w in remaining]
        notes["closed"] = not remaining
    except (RuntimeError, subprocess.SubprocessError, json.JSONDecodeError, ValueError, KeyError) as exc:
        notes["error"] = str(exc)
    return notes


def step_replay_right() -> dict:
    notes: dict = {"pressed_right": False}
    pids = serial_studio_pids()
    if not pids:
        notes["error"] = "Serial Studio Pro is not running"
        return notes
    pid = pids[0]
    window_id = titan_window_id(pid)
    payload = {"pid": pid, "key": "right"}
    if window_id is not None:
        payload["window_id"] = window_id
        payload["delivery_mode"] = "foreground"
    cua_call("press_key", payload)
    notes["pressed_right"] = True
    notes["window_id"] = window_id
    return notes


def restart_serial_studio() -> dict:
    notes: dict = {"restarted": False}
    pids = serial_studio_pids()
    notes["old_pids"] = pids
    for pid in pids:
        os.kill(pid, signal.SIGTERM)
    deadline = time.monotonic() + 12
    while time.monotonic() < deadline and any(pid_alive(pid) for pid in pids):
        time.sleep(0.3)
    if any(pid_alive(pid) for pid in pids):
        for pid in pids:
            if pid_alive(pid):
                os.kill(pid, signal.SIGKILL)
        time.sleep(0.4)
    deadline = time.monotonic() + 8
    while api_up() and time.monotonic() < deadline:
        time.sleep(0.3)
    subprocess.check_call(["open", "-a", APP_NAME])
    deadline = time.monotonic() + 40
    last_err = "Serial Studio Pro did not accept API connections"
    while time.monotonic() < deadline:
        if api_up():
            client = SerialStudioClient(timeout=5)
            try:
                client.connect()
                client.command("project.getStatus")
                notes["restarted"] = True
                notes["new_pids"] = serial_studio_pids()
                return notes
            except (OSError, APIError, TimeoutError, ConnectionError) as exc:
                last_err = str(exc)
            finally:
                client.disconnect()
        time.sleep(0.5)
    raise RuntimeError(last_err)


def finalize_recording(c) -> None:
    try:
        require(c, "sessions.setExportEnabled", {"enabled": False})
    except (APIError, SetupError):
        pass
    try:
        require(c, "sessions.close")
    except (APIError, SetupError):
        pass


def restore_fixture_live(c, proof: dict) -> dict:
    notes: dict = {}
    finalize_recording(c)
    try:
        require(c, "csvPlayer.close")
    except (APIError, SetupError):
        pass
    try:
        require(c, "mdf4Player.close")
    except (APIError, SetupError):
        pass
    notes["session_player"] = close_session_player()
    if require(c, "io.getStatus").get("isConnected"):
        require(c, "io.disconnect")
        wait_disconnected(c)
    open_saved_project(c)
    io = connect_live_once(c)
    notes["connected"] = io.get("isConnected")
    notes["bus"] = io.get("busTypeSlug")
    deadline = time.monotonic() + 12
    running = False
    snap = {}
    seq1 = 0
    seq2 = 0
    while time.monotonic() < deadline:
        running = bool(require(c, "controlScript.getStatus").get("running"))
        snap = values(c)
        if running and str(snap.get("Packet freshness")) == "FRESH" and sequence_of(snap) > 0:
            seq1 = sequence_of(snap)
            break
        time.sleep(0.3)
    if not running:
        notes["session_player_retry"] = close_session_player()
        time.sleep(0.5)
        if "validAge" not in live_control_code(c):
            require(c, "controlScript.setCode", {"code": CONTROL})
        deadline = time.monotonic() + 8
        while time.monotonic() < deadline:
            running = bool(require(c, "controlScript.getStatus").get("running"))
            snap = values(c)
            if running and str(snap.get("Packet freshness")) == "FRESH":
                seq1 = sequence_of(snap)
                break
            time.sleep(0.3)
    if not running:
        try:
            c.disconnect()
        except OSError:
            pass
        notes["restart"] = restart_serial_studio()
        c = SerialStudioClient(timeout=60)
        c.connect()
        guard_session(c)
        open_saved_project(c)
        connect_live_once(c)
        deadline = time.monotonic() + 12
        while time.monotonic() < deadline:
            running = bool(require(c, "controlScript.getStatus").get("running"))
            snap = values(c)
            if running and str(snap.get("Packet freshness")) == "FRESH" and sequence_of(snap) > 0:
                seq1 = sequence_of(snap)
                break
            time.sleep(0.3)
    if running:
        wait = time.monotonic() + 5
        while time.monotonic() < wait:
            seq2 = live_sequence(c)
            if seq2 > seq1:
                break
            time.sleep(0.2)
    notes["control_running"] = running
    notes["freshness"] = snap.get("Packet freshness")
    notes["uid"] = snap.get("UID")
    notes["sequence_before"] = seq1
    notes["sequence_after"] = seq2
    notes["ok"] = bool(
        running
        and str(snap.get("Packet freshness")) == "FRESH"
        and str(snap.get("UID") or "") == IDENTITY["uid"]
        and seq2 > seq1
    )
    proof["fixture_restored"] = notes
    if not notes["ok"]:
        raise RuntimeError(f"fixture did not recover after replay: {notes}")
    return c


def recover_app(c, proof: dict) -> SerialStudioClient:
    notes: dict = {"restarted": False}
    guard_session(c)
    if require(c, "io.getStatus").get("isConnected"):
        require(c, "io.disconnect")
        wait_disconnected(c)
    notes["session_player"] = close_session_player()
    open_saved_project(c)
    io = connect_live_once(c)
    running = bool(require(c, "controlScript.getStatus").get("running"))
    notes["control_running_before"] = running
    if not running:
        notes["session_player_retry"] = close_session_player()
        time.sleep(0.4)
        running = bool(require(c, "controlScript.getStatus").get("running"))
    if not running:
        c.disconnect()
        notes["restart"] = restart_serial_studio()
        notes["restarted"] = True
        c = SerialStudioClient(timeout=60)
        c.connect()
        guard_session(c)
        open_saved_project(c)
        connect_live_once(c)
        running = bool(require(c, "controlScript.getStatus").get("running"))
    notes["control_running"] = running
    proof["recovery"] = notes
    if not running:
        raise RuntimeError(f"control script still stopped after recovery: {notes}")
    return c


def write_receipt(proof: dict, run_id: str, saved_hash: str) -> Path:
    PROOFS.mkdir(exist_ok=True)
    path = PROOFS / f"ss-02-proof-{run_id}-{saved_hash[:16]}.json"
    path.write_text(json.dumps(proof, indent=2) + "\n")
    proof["receipt_path"] = str(path)
    return path


def run_proof(run_id: str) -> int:
    c = SerialStudioClient(timeout=60)
    c.connect()
    proof: dict = {"run_id": run_id}
    saved_hash = hashlib.sha256(SAVE_PATH.read_bytes()).hexdigest()
    proof["saved_project_sha256"] = saved_hash
    clear_flags()
    try:
        c = recover_app(c, proof)
        saved_hash = hashlib.sha256(SAVE_PATH.read_bytes()).hexdigest()
        proof["saved_project_sha256"] = saved_hash
        saved = json.loads(SAVE_PATH.read_text())
        reopened = require(c, "project.exportJson").get("config") or {}
        try:
            reopened["controlScriptCode"] = require(c, "controlScript.get").get("code") or reopened.get(
                "controlScriptCode"
            )
        except (APIError, SetupError):
            pass
        live_export = require(c, "project.exportJson").get("config") or {}
        try:
            live_export["controlScriptCode"] = require(c, "controlScript.get").get("code") or live_export.get(
                "controlScriptCode"
            )
        except (APIError, SetupError):
            pass
        live_canon = canonical_config(live_export)
        saved_canon = canonical_config(saved)
        reopened_canon = canonical_config(reopened)
        proof["config_agreement"] = {
            "saved_sha256": saved_hash,
            "saved_matches_reopened": saved_canon == reopened_canon,
            "reopened_matches_active": reopened_canon == live_canon,
            "canonical": live_canon,
        }
        if saved_canon != reopened_canon or reopened_canon != live_canon:
            raise RuntimeError(f"project configs disagree: {proof['config_agreement']}")

        io = require(c, "io.getStatus")
        proof["io_at_start"] = {
            "connected": io.get("isConnected"),
            "bus": io.get("busTypeSlug"),
            "arguments": require(c, "io.process.getConfig").get("arguments"),
            "control_running": require(c, "controlScript.getStatus").get("running"),
        }
        if not proof["io_at_start"]["control_running"]:
            raise RuntimeError(f"control script is not running: {proof['io_at_start']}")
        ok, reason = process_source_ok(
            io, require(c, "project.source.list").get("sources") or [], REPLAY, require(c, "io.process.getConfig")
        )
        if not ok:
            raise RuntimeError(reason)

        healthy = wait_until(
            c,
            lambda s: checkpoint_matches(s, LIVE_EXPECT) and str(s.get("Packet freshness")) == "FRESH"
            and table_value({"value": s.get("Last valid (ms)")}) > 0,
            15,
            "healthy FRESH",
        )
        seq1 = sequence_of(healthy)
        seq2 = seq1
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            seq2 = live_sequence(c)
            if seq2 > seq1:
                break
            time.sleep(0.2)
        if seq2 <= seq1:
            raise RuntimeError(f"sequence not advancing: {seq1} -> {seq2}")
        proof["healthy"] = {
            **checkpoint_of(healthy),
            "freshness": healthy.get("Packet freshness"),
            "last_valid_ms": healthy.get("Last valid (ms)"),
            "sequence_before": seq1,
            "sequence_after": seq2,
        }

        connection_before_pause = require(c, "io.getStatus").get("isConnected")
        write_flag(PAUSE_FLAG)
        stale = wait_until(
            c,
            lambda s: str(s.get("Packet freshness")) == STALE_LABEL,
            8,
            "stale without reconnect",
        )
        io_paused = require(c, "io.getStatus")
        table_last = table_get(c, "last_valid_ms")
        observed = table_value(table_last)
        dash_ts = table_value({"value": stale.get("Last valid (ms)")})
        latest = require(c, "io.getLatestFrame")
        latest_frame = latest.get("result") if isinstance(latest.get("result"), dict) else latest
        proof["stale"] = {
            "freshness": stale.get("Packet freshness"),
            "host_stale": stale.get("Host stale"),
            "last_valid_ms": stale.get("Last valid (ms)"),
            "table_last_valid_ms": table_last,
            "latest_age_ms": latest_frame.get("ageMs"),
            "latest_has_data": latest_frame.get("hasData"),
            "still_connected": io_paused.get("isConnected"),
            "arguments_unchanged": require(c, "io.process.getConfig").get("arguments") == LIVE_ARGS,
        }
        if observed <= 0:
            raise RuntimeError(f"stale timestamp was zero: {proof['stale']}")
        if dash_ts <= 0:
            raise RuntimeError(f"dashboard last_valid was zero on stale: {proof['stale']}")
        if not connection_before_pause or not io_paused.get("isConnected"):
            raise RuntimeError("Process I/O dropped during pause")

        PAUSE_FLAG.unlink(missing_ok=True)
        recovered = wait_until(c, lambda s: str(s.get("Packet freshness")) == "FRESH", 12, "recovered FRESH")
        proof["recovered"] = {
            "freshness": recovered.get("Packet freshness"),
            "last_valid_ms": recovered.get("Last valid (ms)"),
            "sequence": recovered.get("Sequence"),
            "still_connected": require(c, "io.getStatus").get("isConnected"),
        }

        write_flag(FREEZE_FLAG)
        frozen = wait_until(
            c,
            lambda s: "FROZEN SEQUENCE" in str(s.get("SIMULATED / LIVE banner") or ""),
            8,
            "frozen sequence",
        )
        seq_a = live_sequence(c) or sequence_of(frozen)
        time.sleep(1.2)
        seq_b = live_sequence(c)
        proof["frozen"] = {
            "banner": frozen.get("SIMULATED / LIVE banner"),
            "sequence_held": seq_a,
            "sequence_later": seq_b,
            "freshness": values(c).get("Packet freshness"),
        }
        if seq_a <= 0:
            raise RuntimeError(f"frozen sequence was empty: {proof['frozen']}")
        if seq_b != seq_a:
            raise RuntimeError(f"frozen sequence advanced: {seq_a} -> {seq_b}")
        FREEZE_FLAG.unlink(missing_ok=True)
        resume_deadline = time.monotonic() + 8
        while time.monotonic() < resume_deadline:
            if live_sequence(c) > seq_b:
                break
            time.sleep(0.2)
        else:
            raise RuntimeError(f"sequence did not resume after freeze: held {seq_b}")

        write_flag(SCENARIO_FLAG, "identifying")
        identifying_live = wait_until(c, lambda s: checkpoint_matches(s, IDENTIFYING_EXPECT), 12, "identifying")
        identifying_checkpoint = checkpoint_of(identifying_live)

        db = require(c, "sessions.getDbPath", {"projectTitle": "Titan Mini Observability"})
        db_path = db["path"]
        proof["historian_path"] = db_path
        before_ids = {int(s["session_id"]) for s in sqlite_session_stats(db_path)}
        require(c, "sessions.setExportEnabled", {"enabled": True})
        rec_open = None
        for _ in range(40):
            rec_open = require(c, "sessions.getStatus")
            if rec_open.get("isOpen") and rec_open.get("exportEnabled"):
                break
            time.sleep(0.25)
        proof["recording_open"] = rec_open
        if not rec_open or not rec_open.get("isOpen"):
            raise RuntimeError(f"recording did not open: {rec_open}")

        identifying_rec = wait_until(
            c, lambda s: checkpoint_matches(s, IDENTIFYING_EXPECT), 10, "identifying while recording"
        )
        identifying_checkpoint = checkpoint_of(identifying_rec)
        time.sleep(2.0)
        write_flag(SCENARIO_FLAG, "live")
        live_rec = wait_until(c, lambda s: checkpoint_matches(s, LIVE_EXPECT), 12, "live measurements")
        live_checkpoint = checkpoint_of(live_rec)
        seq_recorded = sequence_of(live_rec) or live_sequence(c)
        deadline = time.monotonic() + 12
        while time.monotonic() < deadline:
            created = [s for s in sqlite_session_stats(db_path) if s["session_id"] not in before_ids]
            if created:
                best = max(created, key=lambda s: s["raw_count"])
                if (
                    best["raw_count"] >= 8
                    and session_raw_contains(db_path, best["session_id"], "UNAVAILABLE")
                    and session_raw_contains(db_path, best["session_id"], IDENTITY["uid"])
                ):
                    break
            time.sleep(0.5)
        finalize_recording(c)
        time.sleep(1.0)
        created_sql = [
            s
            for s in sqlite_session_stats(db_path)
            if s["session_id"] not in before_ids and s["raw_count"] > 0
        ]
        if not created_sql:
            raise RuntimeError(
                f"no new session with frames: before={sorted(before_ids)} sqlite={sqlite_session_stats(db_path)}"
            )
        best_sql = max(created_sql, key=lambda s: s["raw_count"])
        sid = int(best_sql["session_id"])
        meta = {}
        try:
            listed = list_sessions(c, db_path)
            meta = next((s for s in listed if int(s.get("session_id") or 0) == sid), {}) or {}
        except (APIError, SetupError, RuntimeError):
            meta = {}
        frame_count = int(meta.get("frame_count") or 0)
        if frame_count < best_sql["raw_count"]:
            frame_count = int(best_sql["raw_count"])
        stored = {title: session_title_values(db_path, sid, title) for title in CHECKPOINT_TITLES}
        proof["recorded_session"] = {
            "session_id": sid,
            "frame_count": frame_count,
            "started_at": meta.get("started_at_iso") or meta.get("started_at") or best_sql.get("started_at"),
            "project_sha256": saved_hash,
            "fixture_sequence": seq_recorded,
            "raw_count": best_sql["raw_count"],
            "raw_has_unavailable": session_raw_contains(db_path, sid, "UNAVAILABLE"),
            "raw_has_uid": session_raw_contains(db_path, sid, IDENTITY["uid"]),
            "raw_has_simulated": session_raw_contains(db_path, sid, "SIMULATED DATA"),
            "identifying": identifying_checkpoint,
            "live": live_checkpoint,
        }
        if int(proof["recorded_session"]["raw_count"] or 0) < 8:
            raise RuntimeError(f"new session too short: {proof['recorded_session']}")
        if not checkpoint_matches(identifying_checkpoint, IDENTIFYING_EXPECT):
            raise RuntimeError(f"recorded identifying checkpoint mismatch: {identifying_checkpoint}")
        if not checkpoint_matches(live_checkpoint, LIVE_EXPECT):
            raise RuntimeError(f"recorded live checkpoint mismatch: {live_checkpoint}")
        if "UNIDENTIFIED" not in (stored.get("UID") or []) or IDENTITY["uid"] not in (stored.get("UID") or []):
            raise RuntimeError(f"stored identity checkpoints missing: {stored.get('UID')}")
        if "UNAVAILABLE" not in (stored.get("Lane A RMS") or []) or not any(
            item in ("-28", "-28.0") for item in (stored.get("Lane A RMS") or [])
        ):
            raise RuntimeError(f"stored measurement checkpoints missing: {stored.get('Lane A RMS')}")

        require(c, "io.disconnect")
        wait_disconnected(c)
        ensure_database_open(c, db_path)
        replay = require(c, "sessions.replay", {"sessionId": sid})
        if not replay.get("replaying"):
            raise RuntimeError(f"sessions.replay did not start: {replay}")
        proof["replay"] = {"sessionId": replay.get("sessionId"), "replaying": replay.get("replaying")}
        time.sleep(1.2)
        identifying_replay = None
        live_replay = None
        last = {}
        for _ in range(40):
            last = checkpoint_of(values(c))
            if identifying_replay is None and checkpoint_matches(last, IDENTIFYING_EXPECT):
                identifying_replay = dict(last)
            if live_replay is None and checkpoint_matches(last, LIVE_EXPECT):
                live_replay = dict(last)
            if identifying_replay and live_replay:
                break
            step_replay_right()
            time.sleep(0.2)
        proof["replay_identifying"] = identifying_replay
        proof["replay_live"] = live_replay
        if identifying_replay is None:
            raise RuntimeError(f"replay never showed a complete identifying frame: {last}")
        if live_replay is None:
            raise RuntimeError(f"replay never showed a complete live frame: {last}")
        restored = require(c, "project.exportJson").get("config") or {}
        proof["replay_restored_title"] = restored.get("title")
        if restored.get("title") != "Titan Mini Observability":
            raise RuntimeError(f"replay restored unexpected project: {restored.get('title')}")

        c = restore_fixture_live(c, proof)
        write_receipt(proof, run_id, saved_hash)
        print(json.dumps({k: proof[k] for k in proof if k != "canonical"}, indent=2)[:5000])
        return 0
    except (APIError, RuntimeError, SetupError, OSError) as exc:
        proof["error"] = str(exc)
        try:
            c = restore_fixture_live(c, proof)
        except (APIError, RuntimeError, SetupError, OSError) as cleanup_exc:
            proof["cleanup_error"] = str(cleanup_exc)
        write_receipt(proof, run_id, saved_hash)
        print("PROOF_FAILED", exc, file=sys.stderr)
        print(json.dumps(proof, indent=2)[:4000])
        return 1
    finally:
        clear_flags()
        try:
            c.disconnect()
        except OSError:
            pass


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-id", default="run")
    parser.add_argument("--twice", action="store_true")
    args = parser.parse_args()
    if args.twice:
        first = run_proof("run1")
        second = run_proof("run2")
        print(json.dumps({"run1": first, "run2": second}))
        return 0 if first == 0 and second == 0 else 1
    return run_proof(args.run_id)


if __name__ == "__main__":
    raise SystemExit(main())
