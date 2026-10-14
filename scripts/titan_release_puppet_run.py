#!/usr/bin/env python3
"""One camera-recorded BOOM take on the release-puppet TEST IMAGE (host-008-puppet).

The host plays a BOOM clip and, unless --no-arm, sets board 0 speed to 0.501 at
(clip drop - 6 s + offset) through the mixer API, then back to 0.500. The device
lands the R01+R03 release 6 s after the arm. The camera server (Captain's Cursor
tab) records the plate with room audio so light can be timed against sound.

Playback is Bluetooth (variable 100-250 ms lag), so the first armed take is a
calibration: measure light minus sound at the drop in its video, then pass the
negative of that as --offset on the next take.

  python3.13 titan_release_puppet_run.py --label L --clip WAV --drop-s 15.766 [--offset S] [--no-arm]
"""
from __future__ import annotations

import argparse
import json
import subprocess
import time
import urllib.request
from pathlib import Path

API = "http://127.0.0.1:8776"
CAMERA = Path(__file__).resolve().parent / "titan_camera_record.py"
LEAD_S = 6.0          # v9_release_puppet.h: the drop lands hold_lead_s after the arm
CONTROL_S = 0.024     # P6 control latency p50 (docs/evidence/P6/control-latency)
GAIN = 0.25           # titan_cost_capture.py default


def get(path: str) -> dict:
    return json.load(urllib.request.urlopen(API + path, timeout=2))


def set_speed(value: float, token: str) -> dict:
    t0 = time.monotonic()
    req = urllib.request.Request(API + "/api/controls", method="POST",
                                 data=json.dumps({"fields": {"speed": value}, "board": 0}).encode(),
                                 headers={"Content-Type": "application/json", "X-SMC-Token": token})
    urllib.request.urlopen(req, timeout=2).read()
    while time.monotonic() - t0 < 2.0:
        s = (get("/api/status").get("board_states") or {}).get("0") or {}
        if abs(float(s.get("speed", -1)) - value) < 1e-4:
            return {"value": value, "sent": t0, "confirmed": time.monotonic()}
        time.sleep(0.005)
    raise SystemExit(f"REFUSED: speed {value} not confirmed on board 0")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--label", required=True)
    ap.add_argument("--clip", type=Path, required=True)
    ap.add_argument("--drop-s", type=float, required=True, help="measured drop onset in the clip")
    ap.add_argument("--offset", type=float, default=0.0, help="added to the arm time (calibration)")
    ap.add_argument("--no-arm", action="store_true")
    ap.add_argument("--audio", default="MacBook Pro Microphone")
    ap.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()

    token = get("/api/session")["token"]
    states = get("/api/status").get("board_states") or {}
    speeds = [float((states.get(k) or {}).get("speed", -1)) for k in ("0", "1")]
    if any(abs(v - 0.5) > 1e-4 for v in speeds):
        raise SystemExit(f"REFUSED: start speeds {speeds}, expected 0.5/0.5")
    dur = float(subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0",
                                str(a.clip)], capture_output=True, text=True, check=True).stdout)
    seconds = round(dur + 6.0, 1)
    req = subprocess.run(["python3", str(CAMERA), "request", "--label", a.label, "--seconds", str(seconds),
                          "--no-wait", "--audio", a.audio], capture_output=True, text=True)
    if req.returncode != 0:
        raise SystemExit(f"REFUSED: camera request failed: {req.stdout.strip()} {req.stderr.strip()}")
    time.sleep(3.0)  # camera open
    log = {"label": a.label, "clip": str(a.clip), "drop_s": a.drop_s, "offset_s": a.offset,
           "armed": not a.no_arm, "camera_seconds": seconds, "gain": GAIN}
    log["play_spawn"] = time.monotonic()
    player = subprocess.Popen(["afplay", "-v", str(GAIN), str(a.clip)])
    arm_at = log["play_spawn"] + a.drop_s - LEAD_S - CONTROL_S + a.offset
    log["arm_target"] = arm_at
    if not a.no_arm:
        while time.monotonic() < arm_at - 0.002:
            time.sleep(0.001)
        log["arm"] = set_speed(0.501, token)
        time.sleep(0.3)
        log["disarm"] = set_speed(0.5, token)
    player.wait(timeout=dur + 30)
    log["play_end"] = time.monotonic()
    result = Path("/Volumes/Disk Storage/SpectraSynq-Scratch/titan-lagfree-20261014/camera/queue") / f"{a.label}.result.json"
    deadline = time.monotonic() + seconds + 60
    while time.monotonic() < deadline and not result.exists():
        time.sleep(0.5)
    log["camera"] = json.loads(result.read_text()) if result.exists() else {"ok": False, "error": "no result"}
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / f"{a.label}.run.json").write_text(json.dumps(log, indent=2) + "\n")
    print(json.dumps({k: log[k] for k in ("label", "armed", "offset_s")}), "camera ok:", log["camera"].get("ok"),
          "streams:", log["camera"].get("streams"))
    return 0 if log["camera"].get("ok") else 1


if __name__ == "__main__":
    raise SystemExit(main())
