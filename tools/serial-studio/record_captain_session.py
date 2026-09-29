#!/usr/bin/env python3
"""Read-only Captain-session recorder. Never opens the CDC port or MIDI; reads only
the controller status endpoint (127.0.0.1:8767) and the broker log.

Writes one JSONL row whenever raw MIDI, write IDs, mode/output/emission/precision,
blockers, qualification or pickup state change, plus a 1 Hz Titan counter heartbeat.
usage: record_captain_session.py OUTPUT_DIR [SECONDS]
"""
import json, os, sys, time, urllib.request

STATUS = "http://127.0.0.1:8767/status"
BROKER_LOG = "/tmp/titan-emotiscope-control/broker.jsonl"


def status():
    try:
        req = urllib.request.Request(STATUS, headers={"Host": "127.0.0.1:8767"})
        return json.load(urllib.request.urlopen(req, timeout=2))
    except Exception as exc:  # noqa: BLE001
        return {"error": repr(exc)}


def target():
    try:
        with open(BROKER_LOG, "rb") as fp:
            fp.seek(max(0, os.path.getsize(BROKER_LOG) - 200_000))
            for line in reversed(fp.read().splitlines()):
                try:
                    row = json.loads(line)
                except ValueError:
                    continue
                if row.get("kind") == "snapshot":
                    r = row["row"]
                    return {k: r.get(k) for k in ("build", "latched_frames", "device_progress", "led_faults")}
    except OSError as exc:
        return {"error": repr(exc)}
    return {}


def main():
    out = sys.argv[1]
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 1800.0
    os.makedirs(out, exist_ok=True)
    path = os.path.join(out, "session.jsonl")
    keys = ("mode_name", "output_brightness", "emission_enabled", "output_precision", "accepted_request_id",
            "rendered_request_id", "emitted_request_id", "write_state", "blockers", "faders_armed", "last_error")
    last, last_beat, end = None, 0.0, time.time() + seconds
    with open(path, "a") as fp:
        while time.time() < end:
            s = status()
            view = {k: s.get(k) for k in keys}
            view["recent"] = (s.get("recent") or [])[-8:]
            view["qualification"] = {k: (s.get("qualification") or {}).get(k)
                                     for k in ("active", "label", "qualified", "reloaded", "last_result")}
            view["pickup"] = {r["control"]: r.get("state") for r in s.get("controls") or []}
            now = time.time()
            if view != last or now - last_beat >= 1.0:
                fp.write(json.dumps({"t": now, "changed": view != last, **view, "target": target()}) + "\n")
                fp.flush()
                last, last_beat = view, now
            time.sleep(0.1)
    print(json.dumps({"session": path, "seconds": seconds}))


if __name__ == "__main__":
    main()
