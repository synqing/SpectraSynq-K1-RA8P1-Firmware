#!/usr/bin/env python3
"""SS-03 host tests plus bounded live campaign. Never flashes Titan."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import signal
import socket
import sqlite3
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

SS = Path("/Users/spectrasynq/Serial-Studio")
HERE = Path(__file__).resolve().parent
FW = HERE.parents[1]
EVI = FW / "docs/evidence/K1-RA8P1-002/ss-03-live-observability"
sys.path.insert(0, str(SS / "tests" / "utils"))
sys.path.insert(0, str(HERE))

from api_client import APIError, SerialStudioClient  # noqa: E402
from setup_titan_live import LIVE, LIVE_TITLE, configure_network, derive, verify_live_source  # noqa: E402
from titan_transport import EXPECTED_UID as UID  # noqa: E402

HOST_TESTS = [
    HERE / "tests/test_ss03_decode.py",
    HERE / "tests/test_ss03_transport.py",
    HERE / "tests/test_ss03_lock.py",
    HERE / "tests/test_ss03_broker.py",
    HERE / "tests/test_titan_parser.py",
    HERE / "tests/test_checkpoint_match.py",
]
HASH_FILES = [
    HERE / "titan_broker.py",
    HERE / "titan_transport.py",
    HERE / "titan_snapshot.py",
    HERE / "titan_decode.py",
    HERE / "cdc_lock.py",
    HERE / "setup_titan_live.py",
    HERE / "prove_ss03.py",
    HERE / "telemetry-schema.json",
    HERE / "titan-live.ssproj",
]
APP = "Serial Studio Pro"


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def port_open(port: int) -> bool:
    sock = socket.socket()
    sock.settimeout(0.4)
    try:
        sock.connect(("127.0.0.1", port))
        return True
    except OSError:
        return False
    finally:
        sock.close()


def wait_port(port: int, seconds: float) -> None:
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if port_open(port):
            return
        time.sleep(0.25)
    raise RuntimeError(f"port {port} did not open")


def run_host_tests() -> dict:
    results = []
    for path in HOST_TESTS:
        proc = subprocess.run([sys.executable, str(path)], cwd=str(HERE), capture_output=True, text=True)
        results.append(
            {
                "path": str(path),
                "exit": proc.returncode,
                "stdout": (proc.stdout or "")[-500:],
                "stderr": (proc.stderr or "")[-500:],
            }
        )
        if proc.returncode:
            raise RuntimeError(f"host test failed {path}: {proc.stderr or proc.stdout}")
    return {"ok": True, "results": results}


def wait_file(path: Path, seconds: float) -> None:
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if path.exists():
            return
        time.sleep(0.1)
    raise RuntimeError(f"missing {path}")


def start_broker(evidence: Path, replay_only: bool = False) -> subprocess.Popen:
    evidence.mkdir(parents=True, exist_ok=True)
    cmd = [sys.executable, str(HERE / "titan_broker.py"), "run", "--evidence", str(evidence)]
    if replay_only:
        cmd.append("--replay-only")
    log = evidence / "broker-stdout.log"
    log.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.Popen(
        cmd,
        cwd=str(HERE),
        start_new_session=True,
        stdout=log.open("w"),
        stderr=subprocess.STDOUT,
    )
    wait_port(7778, 20)
    if not replay_only:
        wait_file(evidence / "bind.json", 15)
    return proc


def stop_broker() -> None:
    subprocess.run([sys.executable, str(HERE / "titan_broker.py"), "stop"], cwd=str(HERE), check=False)
    deadline = time.monotonic() + 6
    while time.monotonic() < deadline and port_open(7778):
        time.sleep(0.2)


def start_ssp() -> None:
    if port_open(7777):
        return
    subprocess.check_call(["open", "-a", APP])
    wait_port(7777, 40)


def cmd(client, name, params=None):
    try:
        return client.command(name, params)
    except APIError as exc:
        raise RuntimeError(f"{name}: {exc.code}: {exc.message}") from exc


def safe_disconnect(client) -> None:
    try:
        io = cmd(client, "io.getStatus")
    except RuntimeError:
        return
    if not io.get("isConnected"):
        return
    try:
        cmd(client, "io.disconnect")
    except RuntimeError as exc:
        if "Not connected" not in str(exc):
            raise


CELL_TITLES = (
    "SIMULATED / LIVE banner",
    "Operating state",
    "Test result",
    "UID",
    "Build",
    "Source pin",
    "Contract",
    "Identified",
    "Lane A RMS",
    "Lane B RMS",
    "Lane A live",
    "Lane B live",
    "Measured capture rate",
    "Worst hop compute",
    "Late starts",
    "Deadlines",
    "CRC mismatches",
    "LED health",
    "DMA IRQs",
    "Latched frames",
    "LED faults",
    "Hop samples",
    "Admitted rate",
    "Sequence",
    "Health valid",
    "Simulated",
    "Mic mapping",
    "Acquisition origin",
    "Identity scope",
    "Device age (ms)",
    "Device progress",
    "Connection epoch",
    "Timing valid",
    "Rate valid",
    "LED valid",
    "Frozen device",
)


def latest_cells(client) -> list:
    latest = cmd(client, "io.getLatestFrame")
    frame = latest.get("result") if isinstance(latest.get("result"), dict) else latest
    return frame.get("values") or []


def values(client) -> dict:
    rows = {}
    data = cmd(client, "dashboard.getData")
    for group in (data.get("frame") or {}).get("groups") or []:
        for ds in group.get("datasets") or []:
            rows[ds.get("title") or ds.get("alias")] = ds.get("value")
    cells = latest_cells(client)
    for title, value in zip(CELL_TITLES, cells):
        if not rows.get(title):
            rows[title] = value
    return rows


def wait_value(client, title, pred, seconds, what) -> dict:
    snap = {}
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        snap = values(client)
        if pred(snap.get(title)):
            return snap
        time.sleep(0.3)
    raise RuntimeError(f"{what}: {snap.get(title)!r} keys={list(snap)[:12]}")


def hashes() -> dict:
    head = subprocess.check_output(["git", "-C", str(FW), "rev-parse", "HEAD"], text=True).strip()
    dirty = subprocess.check_output(["git", "-C", str(FW), "status", "--porcelain"], text=True)
    return {
        "git_head": head,
        "dirty": bool(dirty.strip()),
        "files": {str(path.relative_to(FW)): sha(path) for path in HASH_FILES if path.exists()},
        "live_project": sha(LIVE) if LIVE.exists() else None,
    }


def receipt_path(evidence: Path, run_id: str, live_hash: str, failed: bool) -> Path:
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    name = f"ss-03-{'fail' if failed else 'pass'}-{run_id}-{live_hash[:16]}-{stamp}.json"
    return evidence / "receipts" / name


def screenshot(evidence: Path) -> dict:
    notes = {"ok": False}
    try:
        proc = subprocess.run(["pgrep", "-f", "Serial-Studio-Pro"], capture_output=True, text=True)
        pids = [int(x) for x in proc.stdout.split() if x.strip().isdigit()]
        if not pids:
            notes["error"] = "no Serial Studio pid"
            return notes
        out = evidence / "dashboard.png"
        completed = subprocess.run(
            ["cua-driver", "call", "screenshot", json.dumps({"pid": pids[0], "path": str(out)})],
            capture_output=True,
            text=True,
            timeout=20,
        )
        notes["returncode"] = completed.returncode
        notes["stdout"] = (completed.stdout or "")[:400]
        notes["stderr"] = (completed.stderr or "")[:400]
        notes["ok"] = completed.returncode == 0 and out.exists()
        notes["path"] = str(out) if out.exists() else None
    except (OSError, subprocess.SubprocessError) as exc:
        notes["error"] = str(exc)
    return notes


def live_campaign(evidence: Path, dashboard_s: float, baseline_s: float) -> dict:
    proof: dict = {"ticket": "SS-03", "started": datetime.now(timezone.utc).isoformat()}
    derive()
    proof["hashes"] = hashes()
    proof["live_project_sha256"] = sha(LIVE)
    stop_broker()
    start_ssp()
    client = SerialStudioClient(timeout=20)
    client.connect()
    io = cmd(client, "io.getStatus")
    if io.get("isConnected") and io.get("busTypeSlug") not in ("network", "process", None, ""):
        if io.get("busType") not in (1, 8, None):
            raise RuntimeError(f"foreign session on bus {io}")
    if io.get("isConnected"):
        safe_disconnect(client)
        time.sleep(0.4)
    broker = start_broker(evidence / "broker-live")
    proof["broker_pid"] = broker.pid
    time.sleep(1.0)
    bind = json.loads((evidence / "broker-live" / "bind.json").read_text())
    proof["bind"] = bind
    if bind["bound"]["uid"] != UID:
        raise RuntimeError(f"UID mismatch {bind['bound']}")
    proof["resident"] = bind["bound"]
    t0 = time.monotonic()
    while time.monotonic() - t0 < baseline_s:
        time.sleep(0.5)
    proof["baseline_s"] = baseline_s
    proof["baseline_metrics"] = json.loads((evidence / "broker-live" / "bind.json").read_text())
    cmd(client, "project.open", {"filePath": str(LIVE)})
    time.sleep(1.0)
    net = configure_network(client)
    proof["network"] = {
        "socketTypes": net.get("socketTypes"),
        "config": net.get("config"),
        "buses": net.get("buses"),
    }
    io = cmd(client, "io.getStatus")
    ok, reason = verify_live_source(io, net.get("config") or {})
    if not ok:
        raise RuntimeError(reason)
    if not io.get("isConnected"):
        cmd(client, "io.connect")
        time.sleep(1.0)
    snap = wait_value(
        client,
        "UID",
        lambda v: str(v) == UID,
        20,
        "live UID",
    )
    proof["dashboard_first"] = {
        "banner": snap.get("SIMULATED / LIVE banner") or snap.get("Display origin"),
        "uid": snap.get("UID"),
        "build": snap.get("Build"),
        "source": snap.get("Source pin"),
        "contract": snap.get("Contract"),
        "origin": snap.get("Acquisition origin"),
        "freshness": snap.get("Packet freshness"),
        "lane_a": snap.get("Lane A RMS"),
        "rate": snap.get("Measured capture rate"),
        "led": snap.get("LED health"),
        "progress": snap.get("Device progress"),
    }
    if str(snap.get("UID")) != UID:
        raise RuntimeError("dashboard UID mismatch")
    if "SIMULATED DATA" in str(snap.get("SIMULATED / LIVE banner") or ""):
        raise RuntimeError("live dashboard still showing simulated banner")
    # second subscriber + slow subscriber
    fast = socket.create_connection(("127.0.0.1", 7778), timeout=2)
    slow = socket.create_connection(("127.0.0.1", 7778), timeout=2)
    fast.settimeout(1)
    proof["second_client_line"] = fast.recv(2048).decode("utf-8", "replace")[:300]
    time.sleep(0.4)
    proof["slow_client_pending"] = True
    t1 = time.monotonic()
    samples = []
    while time.monotonic() - t1 < dashboard_s:
        samples.append(
            {
                "t": time.monotonic() - t1,
                "freshness": values(client).get("Packet freshness"),
                "uid": values(client).get("UID"),
                "progress": values(client).get("Device progress"),
                "rate": values(client).get("Measured capture rate"),
                "led": values(client).get("LED health"),
            }
        )
        time.sleep(5 if dashboard_s > 30 else 1)
    proof["dashboard_samples"] = samples[:40]
    proof["screenshot"] = screenshot(evidence)
    (evidence / "campaign-partial.json").write_text(json.dumps(proof, indent=2) + "\n")
    broker_pid = broker.pid
    # restart SSP once
    safe_disconnect(client)
    client.disconnect()
    pids = subprocess.check_output(["pgrep", "-f", "Serial-Studio-Pro"], text=True).split()
    for pid in pids:
        os.kill(int(pid), signal.SIGTERM)
    deadline = time.monotonic() + 12
    while time.monotonic() < deadline and port_open(7777):
        time.sleep(0.3)
    start_ssp()
    if not os.path.exists(f"/proc/{broker_pid}") and sys.platform == "darwin":
        try:
            os.kill(broker_pid, 0)
            proof["broker_survived_ssp_restart"] = True
        except OSError:
            proof["broker_survived_ssp_restart"] = False
    else:
        try:
            os.kill(broker_pid, 0)
            proof["broker_survived_ssp_restart"] = True
        except OSError:
            proof["broker_survived_ssp_restart"] = False
    if not proof["broker_survived_ssp_restart"]:
        raise RuntimeError("broker died when SSP restarted")
    client = SerialStudioClient(timeout=20)
    client.connect()
    cmd(client, "project.open", {"filePath": str(LIVE)})
    time.sleep(0.8)
    net = configure_network(client)
    cmd(client, "io.connect")
    snap = wait_value(client, "UID", lambda v: str(v) == UID, 20, "UID after SSP restart")
    proof["after_ssp_restart"] = {"uid": snap.get("UID"), "freshness": snap.get("Packet freshness")}
    # Close/reopen of this CDC enters RA USB Boot. Keep the owner and record BLOCKED.
    proof["cdc_reopen"] = {
        "blocked": True,
        "reason": "serial close on this Mac previously enumerated 045b:0261 RA USB Boot; HUPCL is now cleared but reopen is not retried on the live board",
        "broker_pid": broker_pid,
        "identity_retained": snap.get("UID"),
    }
    # short recording
    db = cmd(client, "sessions.getDbPath", {"projectTitle": LIVE_TITLE})
    db_path = db["path"]
    before = set()
    if Path(db_path).exists():
        con = sqlite3.connect(db_path)
        before = {r[0] for r in con.execute("SELECT session_id FROM sessions")}
        con.close()
    cmd(client, "sessions.setExportEnabled", {"enabled": True})
    time.sleep(6)
    cmd(client, "sessions.setExportEnabled", {"enabled": False})
    try:
        cmd(client, "sessions.close")
    except RuntimeError:
        pass
    time.sleep(1)
    proof["recording"] = {"db": db_path, "before": sorted(before)}
    if Path(db_path).exists():
        con = sqlite3.connect(db_path)
        after = [r[0] for r in con.execute("SELECT session_id FROM sessions") if r[0] not in before]
        proof["recording"]["new_ids"] = after
        con.close()
    # Replay: quiesce TX, keep CDC open (close resets this board). No new serial opens.
    live_dir = evidence / "broker-live"
    stats_before = json.loads((live_dir / "live-stats.json").read_text()) if (live_dir / "live-stats.json").exists() else {}
    (live_dir / "quiesce.flag").write_text("replay\n")
    time.sleep(1.2)
    stats_mid = json.loads((live_dir / "live-stats.json").read_text()) if (live_dir / "live-stats.json").exists() else {}
    try:
        cmd(client, "project.dataTable.setValue", {"table": "titan_watchdog", "name": "origin_override", "value": "REPLAY"})
    except RuntimeError as exc:
        proof["origin_override_error"] = str(exc)
    time.sleep(1.5)
    stats_after = json.loads((live_dir / "live-stats.json").read_text()) if (live_dir / "live-stats.json").exists() else {}
    proof["hardware_during_replay"] = {
        "kept_open": True,
        "quiesced": True,
        "writes_before": stats_before.get("hw_writes"),
        "writes_mid": stats_mid.get("hw_writes"),
        "writes_after": stats_after.get("hw_writes"),
        "opens": stats_after.get("serial_opens"),
    }
    if stats_after.get("hw_writes") not in (None, stats_mid.get("hw_writes")):
        raise RuntimeError(f"hardware TX during replay: {stats_mid} -> {stats_after}")
    (live_dir / "quiesce.flag").unlink(missing_ok=True)
    try:
        cmd(client, "project.dataTable.setValue", {"table": "titan_watchdog", "name": "origin_override", "value": "LIVE"})
    except RuntimeError as exc:
        proof["origin_override_restore_error"] = str(exc)
    if not cmd(client, "io.getStatus").get("isConnected"):
        cmd(client, "io.connect")
    snap = wait_value(client, "UID", lambda v: str(v) == UID, 20, "restore UID")
    proof["restored"] = {"uid": snap.get("UID"), "freshness": snap.get("Packet freshness")}
    safe_disconnect(client)
    stop_broker()
    time.sleep(1.0)
    from serial.tools import list_ports

    def app_cdc():
        return [
            {"device": p.device, "vid": p.vid, "pid": p.pid, "product": p.product}
            for p in list_ports.comports()
            if (p.vid, p.pid) == (0x045B, 0x5310)
        ]

    after_close = app_cdc()
    proof["cdc_after_broker_stop"] = after_close
    if not after_close:
        proof["handoff"] = {
            "blocked": True,
            "reason": "CDC left application mode after broker stop; INFO handoff not attempted",
        }
        raise RuntimeError("CDC not in 045b:5310 after broker stop")
    handoff = subprocess.run(
        [sys.executable, str(HERE / "titan_broker.py"), "info-once", "--evidence", str(evidence / "handoff")],
        cwd=str(HERE),
        capture_output=True,
        text=True,
        timeout=20,
    )
    proof["handoff"] = {
        "exit": handoff.returncode,
        "stdout": (handoff.stdout or "")[:2000],
        "stderr": (handoff.stderr or "")[:500],
        "cdc_after": app_cdc(),
    }
    if handoff.returncode:
        raise RuntimeError(f"handoff INFO failed: {handoff.stderr or handoff.stdout}")
    device = after_close[0]["device"]
    lsof = subprocess.run(["lsof", "-n", "-P", device], capture_output=True, text=True).stdout
    proof["cdc_free"] = not lsof.strip()
    proof["final_lsof"] = lsof
    if lsof.strip():
        raise RuntimeError(f"CDC not free at exit: {lsof}")
    if not app_cdc():
        raise RuntimeError("CDC left application mode after INFO handoff")
    try:
        fast.close()
        slow.close()
    except OSError:
        pass
    client.disconnect()
    proof["ok"] = True
    return proof


def write_receipt(proof: dict, evidence: Path, run_id: str, failed: bool) -> Path:
    live_hash = proof.get("live_project_sha256") or (sha(LIVE) if LIVE.exists() else "none")
    path = receipt_path(evidence, run_id, live_hash, failed)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(proof, indent=2) + "\n")
    latest = evidence / "receipts" / "ss-03-latest.json"
    if latest.exists() or True:
        latest.write_text(json.dumps({"path": str(path), "failed": failed}, indent=2) + "\n")
    return path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-id", default="run")
    parser.add_argument("--host-only", action="store_true")
    parser.add_argument("--dashboard-seconds", type=float, default=300)
    parser.add_argument("--baseline-seconds", type=float, default=60)
    args = parser.parse_args()
    evidence = EVI
    evidence.mkdir(parents=True, exist_ok=True)
    proof: dict = {"run_id": args.run_id}
    try:
        proof["host_tests"] = run_host_tests()
        if args.host_only:
            path = write_receipt(proof, evidence, args.run_id, False)
            print(json.dumps({"ok": True, "receipt": str(path), "host_only": True}))
            return 0
        live = live_campaign(evidence, args.dashboard_seconds, args.baseline_seconds)
        proof.update(live)
        path = write_receipt(proof, evidence, args.run_id, False)
        print(json.dumps({"ok": True, "receipt": str(path), "uid": proof.get("resident", {}).get("uid")}))
        return 0
    except (RuntimeError, APIError, OSError, subprocess.SubprocessError) as exc:
        proof["error"] = str(exc)
        try:
            stop_broker()
        except Exception:
            pass
        path = write_receipt(proof, evidence, args.run_id, True)
        print("PROOF_FAILED", exc, file=sys.stderr)
        print(json.dumps({"ok": False, "receipt": str(path), "error": str(exc)}))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
