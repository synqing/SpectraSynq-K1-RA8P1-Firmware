#!/usr/bin/env python3
"""Observe the current composition using existing Surface/broker/camera owners.

No preset, device control, service or firmware writes. /api/board only selects
the Surface context and performs a fresh device GET; original context is restored.
Reuse: _scratch/continuous-bloom-r4/{return_match,observe2}.py.

Separate camera sessions reacquire exposure/WB/focus. This is a bounded before-
state observation, NOT automatically a matched before/after comparison. Final
comparison needs one existing bounded100/112-second session or verified
explicit camera settings restored before each capture. The current camera helper
cannot read back numeric exposure, so separate-session matching is not asserted.
Root may supply an existing100/112-second camera PID/event log to share one locked
session across passages. Complete deployment before that bounded recording. This helper never opens or stops that
external camera. Captured frames do not fulfil full-speed audiovisual inspection;
this helper makes no visual acceptance claim.
"""
from __future__ import annotations

import argparse
from datetime import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import queue
import shlex
import subprocess
import sys
import threading
import tempfile
import time
import urllib.request
from capture_av_validation import (validate_fixture, require_reserve,
                                   validate_player_completion, validate_recording)

F = Path(__file__).resolve().parents[4]
BASE = "http://127.0.0.1:8767"
EVID = Path("/tmp/titan-emotiscope-control")
CAMERA = F / "_scratch/visible-dual-20261011/product_camera"
AUDIO = F / "docs/emotiscope-port-001/evidence/audio-windows-001/sonic-samba-20-44s.wav"
UID = "545433931bd25436593630352d068363"
CAMERA_UID = "EB6E6241-CD5C-4917-91B3-7E6500000001"
PHYSICAL_FIELDS = (
    "hardware_output_brightness", "output_brightness", "emission_enabled", "output_precision",
    "primary_intensity", "secondary_intensity", "secondary_output_enabled",
    "output_intensity_a", "output_intensity_b", "flags",
)


def read_json(path):
    return json.loads(Path(path).read_text())


def http_get(path):
    with urllib.request.urlopen(BASE + path, timeout=3) as response:
        return json.load(response)


def select_board(board):
    # Only permitted POST; no controls/preset/authoring mutation is supported.
    token = http_get("/api/session")["token"]
    request = urllib.request.Request(
        BASE + "/api/board", data=json.dumps({"board": board}).encode(),
        headers={"Content-Type": "application/json", "X-SMC-Token": token,
                 "Origin": BASE},
    )
    with urllib.request.urlopen(request, timeout=3) as response:
        result = json.load(response)
    if result.get("state") != "pending":
        raise RuntimeError(f"board selection refused: {result}")
    end = time.monotonic() + 5
    while time.monotonic() < end:
        state = http_get("/status")
        if state.get("ready") and state.get("selected_board") == board:
            actual = (state.get("board_states") or {}).get(str(board))
            if actual and actual.get("selected_board") == board:
                return state
        time.sleep(0.08)
    raise RuntimeError(f"fresh board {board} readback timed out")


def fresh_boards(original):
    # Each observed transition can only occur after engine._get succeeded.
    result = {}
    for board in (1 - original, original):
        state = select_board(board)
        result[str(board)] = state["board_states"][str(board)]
    return result


def identity(expected_build):
    stats = read_json(EVID / "live-stats.json")
    bound = stats.get("identity") or {}
    if time.monotonic() - stats["host_mono"] > 3:
        raise RuntimeError("broker live status stale")
    if bound.get("uid") != UID or bound.get("build") != expected_build:
        raise RuntimeError(f"identity mismatch: {bound}")
    if stats.get("quiesced"):
        raise RuntimeError("broker quiesced by another owner")
    return stats


def bounded_request(name, record_name, raw_names, destination, request_value="bounded", deadline=None):
    request = EVID / name
    start = time.monotonic()
    deadline = min(start + 3, deadline) if deadline is not None else start + 3
    require_reserve(deadline, start, .1)
    # Publish a complete selector atomically. An empty visible file can be
    # consumed by the broker before buffered write/close, losing "bounded".
    # link() is exclusive: an existing observer's request is never replaced.
    with tempfile.NamedTemporaryFile(mode="w", dir=EVID, prefix=".capture-request-", delete=False) as stream:
        temporary = Path(stream.name)
        inode = os.fstat(stream.fileno()).st_ino
        stream.write(request_value + "\n")
    try:
        os.link(temporary, request)
    finally:
        temporary.unlink()
    try:
        while request.exists() and time.monotonic() < deadline:
            time.sleep(0.01)
        if request.exists():
            raise RuntimeError(f"{name} timed out")
        record = read_json(EVID / record_name)
        if record.get("mono", -1) < start:
            raise RuntimeError(f"stale {record_name}")
        for name in raw_names:
            raw = (EVID / name).read_bytes()
            (destination / name).write_bytes(raw)
        (destination / record_name).write_text(json.dumps(record, indent=2) + "\n")
        return record
    finally:
        # Remove only our exact still-pending request, never another request.
        if request.exists() and request.stat().st_ino == inode:
            request.unlink()


def cached_telemetry():
    """Retain already-polled MIR/VP fields; never request extra device traffic.

    Broker op6 metrics and op17 palette records are retained verbatim with their
    original request/response times. Absence or age stays explicit. The bounded
    tail is not a continuous telemetry capture or a fresh-read guarantee.
    """
    path = EVID / "broker.jsonl"
    result = {"source": str(path), "read_mono": time.monotonic(), "records": {}}
    try:
        with path.open("rb") as stream:
            stream.seek(0, os.SEEK_END)
            size = stream.tell()
            start = max(0, size - 262144)
            stream.seek(start)
            if start:
                stream.readline()  # discard a partial line
            data = stream.read(262144)
        for line in data.splitlines():
            try:
                row = json.loads(line)
            except (ValueError, UnicodeDecodeError):
                continue
            if row.get("kind") == "telemetry" and row.get("op") in (6, 17):
                result["records"][str(row["op"])] = row
        for row in result["records"].values():
            row["cached_age_seconds"] = result["read_mono"] - row.get("response_parsed", 0)
    except OSError as exc:
        result["unavailable"] = str(exc)
    return result


def capture_wire(destination, both=False, deadline=None):
    bottom = bounded_request("WIRE_SNAPSHOT_REQUEST", "wire-snapshot.json",
                             ["wire-snapshot.bin"], destination, deadline=deadline)
    result = {"wire": bottom}
    if both:
        if deadline is not None and deadline - time.monotonic() < 3.1:
            result['wire_pair_scope'] = 'bottom only; insufficient remaining request budget for top'
            return result
        if bottom.get("score", {}).get("output_channel") != 0:
            raise RuntimeError("bottom wire snapshot has wrong output_channel")
        top = bounded_request("WIRE_SNAPSHOT_REQUEST", "wire-snapshot-top.json",
                              ["wire-snapshot-top.bin"], destination, "bounded-top", deadline=deadline)
        if top.get("score", {}).get("output_channel") != 1:
            raise RuntimeError("top wire snapshot has wrong output_channel")
        result["wire_top"] = top
        if deadline is not None and deadline - time.monotonic() < 3.1:
            result['wire_pair_scope'] = 'both edges captured; insufficient remaining request budget for MIR'
            return result
        result["mir"] = bounded_request("MIR_SNAPSHOT_REQUEST", "mir-snapshot.json",
                                        [], destination, deadline=deadline)
        result["wire_pair_scope"] = "sequential retained submissions, not simultaneous optical frames"
    return result


def physical(boards):
    return {b: {k: state[k] for k in PHYSICAL_FIELDS if k in state}
            for b, state in boards.items()}


def stop_child(child):
    if child is not None and child.poll() is None:
        child.terminate()
        try:
            child.wait(timeout=5)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait(timeout=2)


def external_camera(pid, log_path, reserve_seconds, audio_device):
    """Verify root's existing bounded 100/112-second recording; never open/stop that camera."""
    if pid <= 0:
        raise RuntimeError("external camera PID must be positive")
    process = subprocess.run(["/bin/ps", "-ww", "-p", str(pid), "-o", "command="],
                             capture_output=True, text=True, timeout=3, check=True)
    command = shlex.split(process.stdout.strip())
    if '--audio-device' not in command or command[-2:] != ['--audio-device', audio_device]:
        raise RuntimeError('external camera lacks the exact selected microphone argument')
    command = command[:-2]
    if (len(command) not in (6, 7) or Path(command[0]).resolve() != CAMERA.resolve()
            or command[1] != "capture" or command[2] != CAMERA_UID
            or float(command[3]) not in (100, 112)):
        raise RuntimeError("external PID is not the exact authorised 100/112-second camera command")
    records = []
    for line in log_path.read_text().splitlines():
        try:
            records.append(json.loads(line))
        except json.JSONDecodeError:
            continue
    starts = [r for r in records if r.get("event") == "recording_started"
              and r.get("persistent_id") == CAMERA_UID
              and r.get("audio_persistent_id") == audio_device
              and r.get("audio_connection") is True and r.get("video_connection") is True
              and r.get("path") == command[4]]
    if len(starts) != 1 or any(r.get("event") in ("recording_finished", "session_stopped") for r in records):
        raise RuntimeError("external log does not prove one active matching recording")
    start = datetime.fromisoformat(starts[0]["utc"].replace("Z", "+00:00")).timestamp()
    now = time.time()
    duration = float(command[3])
    if start > now or now - start >= duration:
        raise RuntimeError("external recording start is future or expired")
    # Existing product_camera reserves four seconds before its absolute deadline.
    deadline = min(start + duration, float(command[6]) - 4 if len(command) == 7 else start + duration)
    require_reserve(deadline, now, reserve_seconds)
    return {"pid": pid, "event_log": str(log_path), "recording_started": starts[0],
            "path": command[4], "audio_persistent_id": audio_device,
            "record_deadline_unix": deadline, "owned_by": "root; helper never stops this process"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    approved_audio = read_json(F / "docs/emotiscope-port-001/evidence/audio-windows-001/manifest.json")
    audio_windows = {Path(row['wav']).stem: row for row in approved_audio}
    parser.add_argument('--audio-window', choices=sorted(audio_windows), default=AUDIO.stem,
                        help='Approved source-bound window; saved observation records its exact cue and hash')
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--expected-build", required=True)
    parser.add_argument("--readback-only", action="store_true")
    parser.add_argument("--audio-device", help="Exact authorised microphone UID; no implicit default or fallback")
    parser.add_argument("--bloom-trace", action="store_true",
                        help="Optional BLM1, meaningful only for Bloom consumers")
    parser.add_argument("--both-wire", action="store_true",
                        help="Requires installed board-selective wire snapshot support; preserve both native payloads")
    parser.add_argument("--external-camera-pid", type=int)
    parser.add_argument("--external-camera-log", type=Path)
    parser.add_argument("--audio-gain", type=float, default=1.0,
                        help="Bounded afplay gain for matched weak-input checks; does not change system or LED gain")
    parser.add_argument("--pre-seconds", type=float, default=2)
    parser.add_argument("--post-seconds", type=float, default=12)
    parser.add_argument("--dry-run", action="store_true",
                        help="Validate local inputs only; no HTTP, camera or service access")
    args = parser.parse_args()
    audio_record = audio_windows[args.audio_window]
    audio_path = Path(audio_record['wav'])
    audio_format = validate_fixture(audio_record)
    duration = audio_format["duration_s"]
    if not args.readback_only and not args.audio_device:
        parser.error("performance capture requires explicit --audio-device UID")
    if not 0.0 < args.audio_gain <= 1.0:
        parser.error("audio gain must be greater than zero and at most one")
    if bool(args.external_camera_pid is not None) != bool(args.external_camera_log):
        parser.error("external camera PID and log must be supplied together")
    if not 0 <= args.pre_seconds <= (10 if args.external_camera_pid else 2) or not 1 <= args.post_seconds <= 12:
        parser.error("pre-seconds must be 0..2 (0..10 with external camera) and post-seconds 1..12")
    if len(args.expected_build) != 64 or any(c not in "0123456789abcdef" for c in args.expected_build):
        parser.error("expected build must be a complete lowercase SHA-256")
    record_seconds = math.ceil(args.pre_seconds + duration + args.post_seconds + 2)
    # Include initial camera acquisition, driver start and four-second close.
    require_reserve(120, 0, record_seconds + 12)
    if not CAMERA.is_file() or not os.access(CAMERA, os.X_OK):
        raise RuntimeError("existing product_camera executable unavailable")
    if args.output_dir.exists():
        raise RuntimeError("output directory already exists; preserve earlier evidence")
    if args.dry_run:
        print(json.dumps({"local_inputs_valid": True, "repo": str(F),
                          "audio_seconds": duration, "audio_format": audio_format,
                          "owned_record_seconds": record_seconds, "audio_device": args.audio_device,
                          "camera_uid": CAMERA_UID,
                          "both_wire_requested": args.both_wire,
                          "external_camera_requested": args.external_camera_pid is not None,
                          "live_operations_executed": False}))
        return 0

    args.output_dir.mkdir(parents=True)
    events = []
    camera_lines = queue.Queue()
    samples = []
    original = None
    camera = player = None
    failure = None
    before = after = None
    attached = None
    av_validation = None
    player_state = {}
    playback_cancel = threading.Event()
    playback_thread = None
    camera_start = None

    def log(event, **fields):
        row = dict(event=event, wall=time.time(), mono=time.monotonic(), **fields)
        events.append(row)
        print(json.dumps(row), flush=True)

    def camera_events():
        while not camera_lines.empty():
            line = camera_lines.get_nowait()
            try:
                row = json.loads(line)
            except json.JSONDecodeError:
                row = {"line": line}
            log("camera", record=row)

    try:
        stats = identity(args.expected_build)
        initial = http_get("/status")
        if not initial.get("ready") or initial.get("pending_actions") or initial.get("pending_fields"):
            raise RuntimeError("Surface is not ready or has pending operator changes")
        original = initial["selected_board"]
        before = fresh_boards(original)
        (args.output_dir / "before.json").write_text(json.dumps(
            {"broker": stats, "surface": initial, "boards": before}, indent=2) + "\n")
        if not args.readback_only:
            if args.external_camera_pid is not None:
                attached = external_camera(args.external_camera_pid, args.external_camera_log,
                                           args.pre_seconds + duration + args.post_seconds + 2,
                                           args.audio_device)
                camera_start = attached['recording_started']
                recording_path = Path(attached['path'])
                record_deadline_wall = attached['record_deadline_unix']
                log("external_camera_attached", **attached)
            else:
                operation_deadline = time.time() + 120
                recording_path = args.output_dir / "original.mov"
                camera = subprocess.Popen(
                    [str(CAMERA), "capture", CAMERA_UID, str(record_seconds), str(recording_path),
                     "operator-supplied-framing-authorised", str(operation_deadline),
                     "--audio-device", args.audio_device],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                )
                def reader():
                    for line in camera.stdout:
                        camera_lines.put(line.strip())
                threading.Thread(target=reader, daemon=True).start()
                ready_end = time.monotonic() + 10
                while True:
                    remaining = ready_end - time.monotonic()
                    if remaining <= 0:
                        raise RuntimeError("camera recording readiness deadline exceeded")
                    try:
                        line = camera_lines.get(timeout=min(0.2, remaining))
                    except queue.Empty:
                        if camera.poll() is not None:
                            raise RuntimeError("camera exited before recording readiness")
                        continue
                    row = json.loads(line)
                    log("camera", record=row)
                    if row.get("event") == "recording_started":
                        if (row.get('audio_persistent_id') != args.audio_device
                                or row.get('audio_connection') is not True
                                or row.get('video_connection') is not True):
                            raise RuntimeError('camera readiness lacks selected microphone and both connections')
                        camera_start = row
                        break
                start_wall = datetime.fromisoformat(camera_start['utc'].replace('Z', '+00:00')).timestamp()
                record_deadline_wall = min(start_wall + record_seconds, operation_deadline - 4)
            require_reserve(record_deadline_wall, time.time(),
                            args.pre_seconds + duration + args.post_seconds + 1)
            started = time.monotonic()
            observation_deadline = started + (record_deadline_wall - time.time())
            # Serial snapshots can block. A separate waiter owns playback start
            # and exit timestamps, so an early exit cannot masquerade as 75 s.
            def playback():
                nonlocal player
                if playback_cancel.wait(args.pre_seconds):
                    return
                try:
                    require_reserve(observation_deadline, time.monotonic(), duration + args.post_seconds)
                    command = ["/usr/bin/afplay", "-v", str(args.audio_gain), str(audio_path)]
                    request_mono, request_wall = time.monotonic(), time.time()
                    player = subprocess.Popen(command)
                    player_state.update(started=request_mono, start_wall=request_wall, pid=player.pid)
                    code = player.wait()
                    player_state.update(ended=time.monotonic(), returncode=code)
                except Exception as exc:
                    player_state['error'] = str(exc)
            playback_thread = threading.Thread(target=playback, daemon=True)
            playback_thread.start()
            completion_logged = start_logged = False
            while True:
                now = time.monotonic()
                camera_events()
                if player_state.get('error'):
                    raise RuntimeError(player_state['error'])
                if 'started' in player_state and not start_logged:
                    log('audio_process_started', **player_state, audio_gain=args.audio_gain)
                    start_logged = True
                if 'ended' in player_state:
                    validate_player_completion(player_state['returncode'], player_state['started'],
                                               player_state['ended'], duration)
                    if not completion_logged:
                        log('audio_process_finished', **player_state)
                        completion_logged = True
                    if now >= player_state['ended'] + args.post_seconds:
                        break
                if attached:
                    os.kill(attached["pid"], 0)
                    external_rows = args.external_camera_log.read_text()
                    if '"recording_finished"' in external_rows or '"session_stopped"' in external_rows:
                        raise RuntimeError("external camera recording ended during observation")
                elif camera.poll() is not None:
                    raise RuntimeError("camera ended before complete observation")
                if now >= observation_deadline:
                    raise RuntimeError("observation exceeded available recording deadline")
                # Stop starting requests near the post-roll boundary. The budget
                # includes worst-case per-request serial wait, not just duration.
                post_end = player_state.get('ended', started + args.pre_seconds + duration) + args.post_seconds
                sample_deadline = min(observation_deadline - .25, post_end)
                needed = 3.1
                if sample_deadline - now < needed:
                    samples.append({'mono': now, 'live_stats': identity(args.expected_build),
                                    'cached_telemetry': cached_telemetry(),
                                    'wire_not_requested': 'insufficient remaining serial request budget'})
                    time.sleep(.2)
                    continue
                folder = args.output_dir / f"sample-{len(samples):04d}"
                folder.mkdir()
                sample = {"mono": time.monotonic(), **capture_wire(folder, args.both_wire, sample_deadline),
                          "live_stats": identity(args.expected_build),
                          "cached_telemetry": cached_telemetry()}
                if args.bloom_trace and sample_deadline - time.monotonic() >= 3.1:
                    sample["bloom_trace"] = bounded_request(
                        "BLOOM_TRACE_REQUEST", "bloom-trace.json",
                        ["bloom-trace-0.bin", "bloom-trace-1.bin"], folder, deadline=sample_deadline)
                samples.append(sample)
                time.sleep(.2)
            stop_child(camera)
            camera_events()
            if camera is not None and camera.returncode != 0:
                raise RuntimeError(f"camera finalisation failed: {camera.returncode}")
            if attached:
                # Read-only wait for the external owner's normal finalisation.
                finish_deadline = attached['record_deadline_unix'] + 4
                while time.time() < finish_deadline:
                    rows = [json.loads(line) for line in args.external_camera_log.read_text().splitlines()
                            if line.startswith('{')]
                    finished = [r for r in rows if r.get('event') == 'recording_finished']
                    if finished:
                        if finished[-1].get('error'):
                            raise RuntimeError('external camera reported a recording error')
                        break
                    time.sleep(.1)
                else:
                    raise RuntimeError('external recording did not finalise within its bound')
            camera_start_wall = datetime.fromisoformat(camera_start['utc'].replace('Z', '+00:00')).timestamp()
            scored = audio_record.get('scored_window_s', [0, duration])
            av_validation = validate_recording(recording_path, audio_path,
                                               player_state['start_wall'] - camera_start_wall,
                                               scored[0], scored[1])
            (args.output_dir / 'av-validation.json').write_text(json.dumps(av_validation, indent=2) + '\n')
            if not av_validation['passed']:
                raise RuntimeError('received audio/video coverage or source correspondence failed; inspect av-validation.json')
        after = fresh_boards(original)
        stats = identity(args.expected_build)
        (args.output_dir / "after.json").write_text(json.dumps(
            {"broker": stats, "boards": after}, indent=2) + "\n")
        if physical(before) != physical(after):
            raise RuntimeError("physical operating controls changed during observation")
    except BaseException as exc:
        failure = f"{type(exc).__name__}: {exc}"
        log("observation_failed", error=failure)
    finally:
        playback_cancel.set()
        # Prevent a pre-roll worker launching after cleanup observed player=None.
        if playback_thread is not None and 'started' not in player_state:
            playback_thread.join(timeout=1)
        for child in (player, camera):
            try:
                stop_child(child)
            except Exception as exc:
                log("child_cleanup_failed", error=str(exc))
                failure = failure or f"child cleanup failed: {exc}"
        if playback_thread is not None:
            playback_thread.join(timeout=1)
        camera_events()
        if original is not None:
            try:
                # Force a fresh transition, then restore the operator context.
                final_boards = fresh_boards(original)
                if after is None:
                    after = final_boards
                    (args.output_dir / 'after.json').write_text(json.dumps(
                        {'broker': identity(args.expected_build), 'boards': after}, indent=2) + '\n')
                log("surface_selection_restored", board=original)
            except Exception as exc:
                log("surface_selection_restore_failed", error=str(exc))
                failure = failure or f"selection restore failed: {exc}"
        (args.output_dir / "events.json").write_text(json.dumps(events, indent=2) + "\n")
        (args.output_dir / "capture.json").write_text(json.dumps(samples, indent=2) + "\n")
        (args.output_dir / "observation.json").write_text(json.dumps({
            "expected_uid": UID, "expected_build": args.expected_build,
            "audio_gain": args.audio_gain, "audio": str(audio_path), "audio_sha256": audio_record['wav_sha256'],
            "audio_window": audio_record,
            "camera_uid": CAMERA_UID, "readback_only": args.readback_only,
            "camera_executable": str(CAMERA),
            "camera_executable_sha256": hashlib.sha256(CAMERA.read_bytes()).hexdigest(),
            "external_camera": attached, "audio_device": args.audio_device,
            "audio_format": audio_format, "owned_record_seconds": record_seconds,
            "player": player_state, "av_validation": av_validation,
            "pre_seconds_requested": args.pre_seconds, "post_seconds_requested": args.post_seconds,
            "both_wire_requested": args.both_wire,
            "error": failure, "samples": len(samples),
            "physical_controls_unchanged": before is not None and after is not None and physical(before) == physical(after),
            "physical_acceptance": "NOT_ASSESSED", "matched_comparison": False,
            "limitations": __doc__,
        }, indent=2) + "\n")
    return 1 if failure else 0


if __name__ == "__main__":
    sys.exit(main())
