#!/usr/bin/env python3
"""Titan host-independence and no-host boot checks on the installed image.

Replaces the 14 Oct inline host-disconnect attempt, which failed on
``queue.get(timeout=3)``: three seconds without camera stdout were treated as a
camera failure. Here the camera start is polled in short slices until a bounded
deadline and fails only on deadline expiry or camera process exit.

Modes (all reuse the existing broker, Surface readback, camera and AV helpers):
  readback  fresh two-board readback compared with the expected boot profile.
  disconnect  approved Sonic 6-30 s; the sole broker is booted out mid-music and
              restored; light must continue (camera) and settings must not change.
  noboot    broker booted out, no serial owner, MCU reset by one E2 Lite
            signature-and-run (no write, probe power OFF, 1 MHz), BOOM 0-75 s
            recorded with the iPhone; broker restored only afterwards.
Never sends emit-disarm, never writes controls, never flashes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import plistlib
import queue
import re
import subprocess
import sys
import threading
import time
from pathlib import Path

F = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(F / "docs/emotiscope-port-001/evidence/waveform-migration-001"))
import capture_existing as c  # noqa: E402
from capture_av_validation import (validate_fixture, validate_player_completion,  # noqa: E402
                                   validate_recording)

UID = "545433931bd25436593630352d068363"
IPHONE_MIC = "EB6E6241-CD5C-4917-91B3-7E6500000003"
PORTS = ["/dev/cu.usbmodem00000000000011", "/dev/tty.usbmodem00000000000011"]
PLIST = Path.home() / "Library/LaunchAgents/com.spectrasynq.titan-broker.plist"
SERVICE = f"gui/{os.getuid()}/com.spectrasynq.titan-broker"
DOMAIN = f"gui/{os.getuid()}"
RFP = "/Users/spectrasynq/Applications/Renesas/RFP_CLI_V3.24.00/rfp-cli"
RFP_SIG_RUN = [RFP, "-d", "RA", "-t", "e2l:5AS079228B", "-if", "swd", "-s", "1000000",
               "-noquery", "-run", "-sig"]
OUTPUT_KEYS = ("emission_enabled", "hardware_output_brightness", "output_intensity_a",
               "output_intensity_b", "output_precision", "primary_intensity",
               "secondary_intensity", "secondary_output_enabled")
CAMERA_START_LIMIT_S = 60.0


class Run:
    def __init__(self, out: Path):
        self.out = out
        self.events: list[dict] = []

    def event(self, kind: str, **values) -> dict:
        row = {"event": kind, "mono": time.monotonic(), "wall": time.time(), **values}
        self.events.append(row)
        (self.out / "events.json").write_text(json.dumps(self.events, indent=2) + "\n")
        print(json.dumps(row), flush=True)
        return row

    def write(self, name: str, value) -> None:
        (self.out / name).write_text(json.dumps(value, indent=2) + "\n")


def fixture(stem_or_track: str) -> dict:
    manifest = json.loads((F / "docs/emotiscope-port-001/evidence/audio-windows-001/manifest.json").read_text())
    for row in manifest:
        if Path(row["wav"]).stem == stem_or_track or (row.get("track") == stem_or_track and row.get("duration_s") == 75 and row.get("suite_role") == "development"):
            validate_fixture(row)
            return row
    raise RuntimeError(f"no approved fixture {stem_or_track}")


def load_profile(path: Path) -> dict:
    """Boot preset files nest the generated settings under ``composition``."""
    raw = json.loads(Path(path).read_text())
    return raw.get("composition", raw)


def profile_diff(boards: dict, profile: dict) -> dict:
    """Every profile field the readback exposes must equal the expected profile."""
    out = {}
    for b, want in profile["boards"].items():
        row = boards[b].get("values", boards[b])
        delta = {}
        for k, v in want.items():
            rk = "mode" if k == "current_mode" else k
            if rk not in row:
                continue
            got = row[rk]
            same = (abs(float(got) - float(v)) <= 1e-4) if isinstance(v, float) and not isinstance(v, bool) else (got == v or (isinstance(v, bool) and got == int(v)))
            if not same:
                delta[k] = [got, v]
        for k in OUTPUT_KEYS:
            if k not in row or k not in profile["output"]:
                continue
            v, got = profile["output"][k], row[k]
            same = (abs(float(got) - float(v)) <= 1e-4) if isinstance(v, float) else (got == v or (isinstance(v, bool) and got == int(v)))
            if not same:
                delta["output." + k] = [got, v]
        if delta:
            out[b] = delta
    return out


def broker_running() -> bool:
    return subprocess.run(["pgrep", "-f", "titan_broker.py"], capture_output=True).returncode == 0


def serial_owners() -> str:
    r = subprocess.run(["lsof", "-nP", *PORTS], capture_output=True, text=True)
    return r.stdout.strip()


def bootout(run: Run) -> None:
    subprocess.run(["launchctl", "bootout", SERVICE], check=True)
    end = time.monotonic() + 10
    while broker_running() and time.monotonic() < end:
        time.sleep(0.2)
    if broker_running():
        raise RuntimeError("broker still running after bootout")
    owners = serial_owners()
    (run.out / "disconnected-port-ownership.txt").write_text(owners + "\n")
    if owners:
        raise RuntimeError("another serial owner present")
    run.event("sole_broker_stopped_no_serial_owner")


def bootstrap(run: Run, expected_build: str) -> dict:
    subprocess.run(["launchctl", "bootstrap", DOMAIN, str(PLIST)], check=True)
    run.event("broker_bootstrap_requested")
    end = time.monotonic() + 45
    last = None
    while time.monotonic() < end:
        try:
            stats = c.identity(expected_build)
            run.event("broker_identity_fresh", pid=stats.get("pid"))
            return stats
        except Exception as exc:  # stale/absent while the broker attaches
            last = str(exc)
        time.sleep(0.25)
    raise RuntimeError(f"broker identity not fresh after restore: {last}")


def start_camera(run: Run, seconds: int) -> tuple[subprocess.Popen, queue.Queue, float]:
    movie = run.out / "original.mov"
    command = [str(c.CAMERA), "capture", c.CAMERA_UID, str(seconds), str(movie),
               "operator-supplied-framing-authorised", "--audio-device", IPHONE_MIC]
    camera = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
    lines: queue.Queue = queue.Queue()
    threading.Thread(target=lambda: [lines.put(x) for x in camera.stdout], daemon=True).start()
    run.event("camera_process_started", command=command, pid=camera.pid)
    deadline = time.monotonic() + CAMERA_START_LIMIT_S
    log = (run.out / "camera-events.jsonl").open("a")
    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise RuntimeError(f"camera did not report recording_started within {CAMERA_START_LIMIT_S}s")
        try:
            line = lines.get(timeout=min(0.2, remaining))
        except queue.Empty:
            if camera.poll() is not None:
                raise RuntimeError(f"camera exited rc={camera.returncode} before recording_started")
            continue  # quiet stdout is not a failure
        log.write(line); log.flush()
        try:
            row = json.loads(line)
        except json.JSONDecodeError:
            run.event("camera_line", line=line.rstrip()); continue
        run.event("camera", record=row)
        if row.get("event") == "recording_started":
            if row.get("audio_connection") is not True:
                raise RuntimeError("camera started without its microphone connection")
            return camera, lines, time.monotonic()


def finish_camera(run: Run, camera, lines, seconds_left: float) -> None:
    camera.wait(timeout=max(1.0, seconds_left))
    with (run.out / "camera-events.jsonl").open("a") as log:
        while not lines.empty():
            line = lines.get(); log.write(line)
            try:
                run.event("camera", record=json.loads(line))
            except json.JSONDecodeError:
                run.event("camera_line", line=line.rstrip())
    if camera.returncode != 0:
        raise RuntimeError(f"camera rc={camera.returncode}")


def rfp_reset(run: Run) -> dict:
    started = time.monotonic()
    r = subprocess.run(RFP_SIG_RUN, capture_output=True, text=True, timeout=30)
    text = r.stdout

    def only(pattern, expected):
        return [v.strip() for v in re.findall(pattern, text, re.MULTILINE)] == [expected]
    valid = (r.returncode == 0 and only(r"^Tool: ([^\r\n]*)$", "E2 emulator Lite (5AS079228B)")
             and only(r"^Emulator power supply: ([^\r\n]*)$", "OFF")
             and only(r"^Speed: ([^\r\n]*)$", "1,000,000 Hz")
             and only(r"^Connected to ([^\r\n]*)$", "R7KA8P1KFLCAC")
             and only(r"^[ \t]+Device Unique ID: ([^\r\n]*)$", UID.upper())
             and len(re.findall(r"^Operation successful\s*$", text, re.MULTILINE)) == 1)
    receipt = {"command": RFP_SIG_RUN, "returncode": r.returncode, "stdout": r.stdout,
               "stderr": r.stderr, "identified": valid, "elapsed_s": time.monotonic() - started,
               "reset_end_mono": time.monotonic(), "reset_end_wall": time.time(),
               "scope": "E2 Lite signature read then -run: hardware reset boot, no write, probe power OFF"}
    run.write("rfp-reset.json", receipt)
    if not valid:
        raise RuntimeError("E2 Lite signature-and-run did not match every identity predicate")
    run.event("mcu_reset_by_debugger", elapsed_s=receipt["elapsed_s"])
    return receipt


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=("readback", "disconnect", "noboot"))
    ap.add_argument("--out", required=True)
    ap.add_argument("--expected-build", required=True)
    ap.add_argument("--profile", required=True, help="expected boot profile JSON (v8)")
    ap.add_argument("--gain", type=float, default=0.25)
    ap.add_argument("--noboot-fixture", choices=("boom", "sonic"), default="boom")
    a = ap.parse_args()
    out = Path(a.out); out.mkdir(parents=True, exist_ok=False)
    run = Run(out)
    profile = load_profile(Path(a.profile))
    run.event("start", mode=a.mode, profile_sha256=hashlib.sha256(Path(a.profile).read_bytes()).hexdigest(),
              expected_build=a.expected_build)
    launch = plistlib.loads(PLIST.read_bytes())
    admission = Path(launch["EnvironmentVariables"]["TITAN_ADMISSION_SOURCE"])
    if hashlib.sha256(admission.read_bytes()).hexdigest() != launch["EnvironmentVariables"]["TITAN_ADMISSION_SHA256"]:
        raise RuntimeError("broker admission hash mismatch before test")
    broker_stopped = restored = False
    camera = player = None
    error = None
    try:
        before = {"broker": c.identity(a.expected_build), "boards": c.fresh_boards(0)}
        run.write("before.json", before)
        if a.mode == "readback":
            diff = profile_diff(before["boards"], profile)
            run.write("profile-diff.json", diff)
            run.event("completed", profile_differences=diff)
            return 0 if not diff else 3

        if a.mode == "disconnect":
            fx = fixture("sonic-samba-6-30s"); seconds, play_s, score = 42, 24, (0, 24)
        else:
            if a.noboot_fixture == "boom":
                fx = fixture("boom"); seconds, play_s, score = 90, 75, (28, 75)
            else:
                fx = fixture("sonic-samba-6-30s"); seconds, play_s, score = 42, 24, (0, 24)
            bootout(run); broker_stopped = True
            reset = rfp_reset(run)

        camera, lines, started = start_camera(run, seconds)
        cam_deadline = started + seconds
        time.sleep(2)
        player_start = time.monotonic()
        player = subprocess.Popen(["afplay", "-v", str(a.gain), fx["wav"]])
        run.event("audio_process_started", pid=player.pid, wav=fx["wav"], wav_sha256=fx["wav_sha256"], gain=a.gain)
        if a.mode == "disconnect":
            time.sleep(max(0, player_start + 6 - time.monotonic()))
            bootout(run); broker_stopped = True
            time.sleep(max(0, player_start + 18 - time.monotonic()))
        rc = player.wait(timeout=play_s + 15)
        player_end = time.monotonic()
        validate_player_completion(rc, player_start, player_end, play_s)
        run.event("audio_process_finished", returncode=rc, seconds=player_end - player_start)
        if a.mode == "disconnect":
            restore_stats = bootstrap(run, a.expected_build); restored = True
        finish_camera(run, camera, lines, cam_deadline - time.monotonic() + 10)
        if a.mode == "noboot":
            restore_stats = bootstrap(run, a.expected_build); restored = True
            stats_mono = time.monotonic()  # frames below were read at (about) this instant
        after = {"broker": restore_stats, "boards": c.fresh_boards(0)}
        run.write("after.json", after)
        av = validate_recording(out / "original.mov", Path(fx["wav"]), player_start - started, *score)
        run.write("av-validation.json", av)
        pal_b, pal_a = before["broker"]["palette"], after["broker"]["palette"]
        report = {
            "mode": a.mode,
            "profile_differences_after": profile_diff(after["boards"], profile),
            "boot_id_before_after": [before["broker"]["identity"].get("boot_id"), after["broker"]["identity"].get("boot_id")],
            "frames_before_after": [pal_b.get("frames"), pal_a.get("frames")],
            "secondary_completions_before_after": [pal_b["secondary_output"]["completions"], pal_a["secondary_output"]["completions"]],
            "led_faults_after": {k: pal_a.get(k) for k in ("emit_errors", "pair_fault", "lane_a0_fault", "lane_a1_fault")},
            "secondary_fault_after": pal_a["secondary_output"].get("fault"),
            "pdm_after": {k: after["broker"]["pdm"].get(k) for k in ("push_rejected", "asrc_discarded", "stream_epoch")},
            "av_passed": av["passed"],
            "musical_verdict": "UNASSESSED: agent cannot listen",
        }
        if a.mode == "noboot":
            # Use the instant the broker stats (frame counter) were read, not report time.
            elapsed = stats_mono - reset["reset_end_mono"]
            report["seconds_since_reset_at_frame_read"] = elapsed
            report["frames_per_second_since_reset"] = pal_a.get("frames", 0) / elapsed if elapsed > 0 else None
        run.write("report.json", report)
        run.event("completed", report=report)
        return 0
    except Exception as exc:
        error = str(exc)
        run.event("failed", error=error)
        raise
    finally:
        if player and player.poll() is None:
            player.terminate()
        if camera and camera.poll() is None:
            camera.terminate()
        if broker_stopped and not restored:
            r = subprocess.run(["launchctl", "bootstrap", DOMAIN, str(PLIST)], capture_output=True, text=True)
            run.event("finally_broker_restore", returncode=r.returncode, stderr=r.stderr)
        run.write("cleanup.json", {"broker_stopped": broker_stopped, "broker_restored_in_body": restored,
                                   "error": error, "never_emit_disarm": True, "no_control_writes": True})


if __name__ == "__main__":
    raise SystemExit(main())
