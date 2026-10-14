#!/usr/bin/env python3
"""Per-hop cost capture over BOOM 0-75 s on the installed Titan image (lane P2).

Sole serial owner for the run: the broker is booted out first and restored in
``finally``. Reads every LiveTimingRecord (opcode 26, v2 per-hop cost witness)
continuously, checks write_seq continuity, and computes exact (uncapped)
distributions per stage from every hop. Never writes controls, never disarms,
never flashes. Musical verdict: agent cannot listen.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "tools/serial-studio"), str(ROOT / "scripts")]
import cdc_lock, live_lease, live_protocol  # noqa: E402
from titan_broker import find_titan, open_serial, close_serial  # noqa: E402
from titan_transport import Transport  # noqa: E402
import titan_host_independence as hi  # noqa: E402

UID = "545433931bd25436593630352d068363"
STAGES = ("poll_cycles_max", "poll_cycles_sum", "asrc_cycles_sum", "ap_cycles", "donor_cycles",
          "inhouse_cycles", "vp_cycles", "emit_cycles_last", "queue_filled_at_consume", "poll_calls")


def percentile(sorted_values, p):
    if not sorted_values:
        return None
    k = (len(sorted_values) - 1) * p / 100.0
    lo, hi_ = math.floor(k), math.ceil(k)
    if lo == hi_:
        return sorted_values[int(k)]
    return sorted_values[lo] + (sorted_values[hi_] - sorted_values[lo]) * (k - lo)


def distribution(values):
    v = sorted(values)
    if not v:
        return {"count": 0}
    return {"count": len(v), "min": v[0], "p50": percentile(v, 50), "p95": percentile(v, 95),
            "p99": percentile(v, 99), "max": v[-1], "mean": sum(v) / len(v)}


def summarise(records, window=None):
    rows = [r for r in records if "poll_cycles_sum" in r]
    if window is not None:
        a, b = window
        rows = [r for r in rows if a <= r["_music_s"] <= b]
    return {stage: distribution([r[stage] for r in rows]) for stage in STAGES}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--expected-build", required=True)
    ap.add_argument("--gain", type=float, default=0.25)
    ap.add_argument("--preroll", type=float, default=3.0)
    a = ap.parse_args()
    out = a.out
    out.mkdir(parents=True, exist_ok=False)
    run = hi.Run(out)
    fx = hi.fixture("boom")
    run.event("start", expected_build=a.expected_build, wav=fx["wav"], wav_sha256=fx["wav_sha256"])
    broker_stopped = False
    port = handle = lease = player = None
    scratch = tempfile.TemporaryDirectory(prefix="k1cost-")
    records, counters = [], {}
    error = None
    try:
        hi.bootout(run)
        broker_stopped = True
        usb = find_titan(inventory_lsof=False)
        handle = cdc_lock.acquire(usb["device"], "lane-p2-cost-capture")
        port = open_serial(usb["device"], handle)
        t = Transport(port, {1}, campaign=True)
        info = t.info()["info"]
        assert info["uid"] == UID, "wrong target UID"
        assert info["build"] == a.expected_build, f"wrong target build {info['build']}"
        t.allow({1, 6, 17, 21, 23, 26})
        lease = live_lease.acquire(Path(scratch.name), "lane-p2-cost-capture")
        run.write("info.json", info)

        def raw(op, payload=b""):
            q = t.transact(op, payload)
            assert q["ok"], (op, q["status"])
            return q["body"]

        def js(op):
            return json.loads(raw(op))

        counters["before"] = {"pdm": js(6), "palette": js(17)}
        newest = live_protocol.unpack_snapshot(raw(23))["timing_write_seq"]
        cursor = newest
        gaps, reads = [], 0
        start = time.monotonic()
        last_renew = start
        player_start = None
        total = a.preroll + 75.0 + 3.0
        while time.monotonic() - start < total:
            now = time.monotonic()
            if player is None and now - start >= a.preroll:
                player = subprocess.Popen(["afplay", "-v", str(a.gain), fx["wav"]])
                player_start = time.monotonic()
                run.event("audio_process_started", pid=player.pid, gain=a.gain)
            if now - last_renew > 4:
                live_lease.renew(Path(scratch.name), lease["token"])
                last_renew = now
            h = live_protocol.unpack_history(raw(26, struct.pack("<Q", cursor)),
                                             live_protocol.TIMING_BYTES, live_protocol.unpack_timing)
            reads += 1
            if h.get("gap") or h.get("expired_cursor"):
                gaps.append({"after": cursor, "oldest": h["oldest_seq"], "newest": h["newest_seq"]})
            got = h.get("records", [])
            host_t = time.monotonic()
            for r in got:
                r["_host_read_s"] = host_t - start
                r["_music_s"] = (host_t - player_start) if player_start else -1.0
                records.append(r)
            if got:
                cursor = max(r["write_seq"] for r in got)
            if not got or len(got) < 10:
                time.sleep(0.02)
        if player is not None:
            rc = player.wait(timeout=10)
            run.event("audio_process_finished", returncode=rc)
        counters["after"] = {"pdm": js(6), "palette": js(17)}
        # continuity: write_seq must be consecutive
        seqs = [r["write_seq"] for r in records]
        missing = sum(max(0, b - a - 1) for a, b in zip(seqs, seqs[1:]))
        record_bytes = 88 if records and "poll_cycles_sum" in records[0] else 56
        with (out / "timing.jsonl").open("w") as f:
            for r in records:
                f.write(json.dumps(r) + "\n")
        pb, pa = counters["before"]["pdm"]["pdm_target"], counters["after"]["pdm"]["pdm_target"]
        lb, la = counters["before"]["palette"], counters["after"]["palette"]
        report = {
            "build": info["build"], "records": len(records), "record_bytes": record_bytes,
            "missing_write_seq": missing, "ring_gaps": gaps, "reads": reads,
            "cycles_per_us": 1000.0,
            "whole_run": summarise(records),
            "scored_28_75s": summarise(records, (28.0, 75.0)),
            "deltas": {
                "skipped_releases": la["skipped_releases"] - lb["skipped_releases"],
                "emit_errors": la["emit_errors"] - lb["emit_errors"],
                "emitted": la["emitted"] - lb["emitted"],
                "push_rejected": pa["push_rejected"] - pb["push_rejected"],
                "asrc_discarded": pa["asrc_discarded"] - pb["asrc_discarded"],
                "stream_epoch": pa["stream_epoch"] - pb["stream_epoch"],
                "ap_hops": pa["ap_hops"] - pb["ap_hops"],
            },
            "faults_after": {k: la.get(k) for k in ("pair_fault", "lane_a0_fault", "lane_a1_fault")},
            "secondary_fault_after": (la.get("secondary_output") or {}).get("fault"),
            "scope": "on-device DWT cycles at 1 GHz, every hop; host-read timestamps are host monotonic",
            "musical_verdict": "UNASSESSED: agent cannot listen",
        }
        run.write("counters.json", counters)
        run.write("report.json", report)
        run.event("completed", records=len(records), missing=missing)
        return 0
    except Exception as exc:
        error = str(exc)
        run.event("failed", error=error)
        raise
    finally:
        if player and player.poll() is None:
            player.terminate()
        if lease:
            try:
                live_lease.release(Path(scratch.name), lease["token"])
            except Exception as exc:  # recorded, never hidden
                run.event("lease_release_problem", error=str(exc))
        if port:
            close_serial(port, handle)
        if handle:
            cdc_lock.release(handle)
        scratch.cleanup()
        if broker_stopped:
            try:
                hi.bootstrap(run, a.expected_build)
            except Exception as exc:  # recorded, not hidden
                run.event("broker_restore_problem", error=str(exc))
        run.write("cleanup.json", {"broker_stopped": broker_stopped, "error": error,
                                   "never_emit_disarm": True, "no_control_writes": True})


if __name__ == "__main__":
    raise SystemExit(main())
