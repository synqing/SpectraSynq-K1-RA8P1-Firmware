#!/usr/bin/env python3
"""Measure render cadence and skipped_releases on the installed image.

Uses the control-surface HTTP API only (no direct CDC). Does not flash.
skipped_releases counts missed 8333 µs render slots on the device.
hops_rejected / asrc_starved are audio-path counters, kept separate.
publication_deadline_misses is 0 on the installed v1 image; the field is
reserved at snapshot offset 808 for the next Rearm image.
"""
from __future__ import annotations

import json
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/evidence/K1-RA8P1-002/cadence-20260921-01"


def get(path: str) -> dict:
    with urllib.request.urlopen("http://127.0.0.1:8765" + path, timeout=20) as r:
        return json.loads(r.read())


def snapshot_row(label: str) -> dict:
    state = get("/api/state")
    snap: dict = {}
    try:
        snap = get("/api/snapshot").get("snapshot") or {}
    except Exception as exc:
        snap = {"error": str(exc)}
    ps = state.get("palette_status") or {}
    return {
        "label": label,
        "t": time.time(),
        "frames": ps.get("frames"),
        "skipped_releases": ps.get("skipped_releases"),
        "period_us": ps.get("period_us"),
        "visual_path": ps.get("visual_path"),
        "musical": ps.get("musical"),
        "emit_errors": ps.get("emit_errors"),
        "hops_rejected": snap.get("hops_rejected"),
        "asrc_starved": snap.get("asrc_starved"),
        "publication_deadline_misses": snap.get("publication_deadline_misses"),
        "stale": snap.get("stale"),
        "freshness_us": snap.get("freshness_us"),
        "measured_hz": snap.get("measured_hz") or ps.get("measured_hz"),
        "deadline_us": snap.get("deadline_us"),
        "hop_dt_us": snap.get("hop_dt_us"),
    }


def delta(a: dict, b: dict, wall: float) -> dict:
    frames = (b.get("frames") or 0) - (a.get("frames") or 0)
    skipped = (b.get("skipped_releases") or 0) - (a.get("skipped_releases") or 0)
    period = a.get("period_us") or 8333
    expected = wall * 1e6 / period if period else 0
    hops_rej = None
    if a.get("hops_rejected") is not None and b.get("hops_rejected") is not None:
        hops_rej = b["hops_rejected"] - a["hops_rejected"]
    asrc = None
    if a.get("asrc_starved") is not None and b.get("asrc_starved") is not None:
        asrc = b["asrc_starved"] - a["asrc_starved"]
    deadline_miss = None
    if (a.get("publication_deadline_misses") is not None and
            b.get("publication_deadline_misses") is not None):
        deadline_miss = b["publication_deadline_misses"] - a["publication_deadline_misses"]
    return {
        "wall_s": wall,
        "frames_delta": frames,
        "skipped_delta": skipped,
        "expected_slots": expected,
        "render_hz": frames / wall if wall else 0,
        "skip_rate_per_s": skipped / wall if wall else 0,
        "hops_rejected_delta": hops_rej,
        "asrc_starved_delta": asrc,
        "publication_deadline_misses_delta": deadline_miss,
        "period_us": period,
        "promised_hz": 1e6 / period if period else 0,
    }


def frame_gaps(polls: list) -> dict:
    gaps_ms = []
    stall_polls = 0
    for prev, cur in zip(polls, polls[1:]):
        dt_ms = (cur["t"] - prev["t"]) * 1000.0
        df = (cur.get("frames") or 0) - (prev.get("frames") or 0)
        gaps_ms.append(dt_ms)
        if df == 0:
            stall_polls += 1
    return {
        "polls": len(polls),
        "max_poll_gap_ms": max(gaps_ms) if gaps_ms else None,
        "median_poll_gap_ms": sorted(gaps_ms)[len(gaps_ms) // 2] if gaps_ms else None,
        "zero_frame_advance_polls": stall_polls,
    }


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    receipt = {
        "measurement_complete": False,
        "scheduling_ok": False,
        "note": "installed-image measurement; not a flash; skipped_releases is render slots",
    }
    idle_a = snapshot_row("idle_start")
    time.sleep(8.0)
    idle_b = snapshot_row("idle_end")
    idle = delta(idle_a, idle_b, 8.0)
    busy_a = snapshot_row("busy_start")
    t0 = time.time()
    polls = []
    while time.time() - t0 < 8.0:
        polls.append(snapshot_row("busy_poll"))
        time.sleep(0.25)
    busy_b = polls[-1] if polls else snapshot_row("busy_end")
    busy = delta(busy_a, busy_b, time.time() - t0)
    gaps = frame_gaps(polls)
    contention = busy["skip_rate_per_s"] - idle["skip_rate_per_s"]
    promised = idle["promised_hz"]
    idle_hz_ok = abs(idle["render_hz"] - promised) <= promised * 0.15 if promised else False
    skip_quiet = idle["skip_rate_per_s"] < 5.0
    receipt.update({
        "idle": idle,
        "busy_api_poll": busy,
        "busy_polls": len(polls),
        "frame_gaps": gaps,
        "skip_rate_increase_during_api": contention,
        "usb_contention_demonstrated": contention > 5.0,
        "promised_period_us": idle["period_us"],
        "idle_start": idle_a,
        "idle_end": idle_b,
        "busy_start": busy_a,
        "busy_end": busy_b,
        "hops_rejected_idle": idle["hops_rejected_delta"],
        "hops_rejected_busy": busy["hops_rejected_delta"],
        "asrc_starved_idle": idle["asrc_starved_delta"],
        "asrc_starved_busy": busy["asrc_starved_delta"],
        "publication_deadline_misses_idle": idle["publication_deadline_misses_delta"],
        "publication_deadline_misses_busy": busy["publication_deadline_misses_delta"],
        "measurement_complete": True,
        "scheduling_ok": idle_hz_ok and skip_quiet and not (contention > 5.0),
        "cadence_verdict": (
            "idle render near 120 Hz and skipped_releases quiet"
            if idle_hz_ok and skip_quiet else
            "skipped_releases or render-rate off promised 8333 µs cadence"
        ),
    })
    (OUT / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({
        "idle_skip_per_s": idle["skip_rate_per_s"],
        "idle_render_hz": idle["render_hz"],
        "promised_hz": promised,
        "busy_skip_per_s": busy["skip_rate_per_s"],
        "busy_render_hz": busy["render_hz"],
        "usb_contention_demonstrated": receipt["usb_contention_demonstrated"],
        "scheduling_ok": receipt["scheduling_ok"],
        "hops_rejected_idle": idle["hops_rejected_delta"],
        "hops_rejected_busy": busy["hops_rejected_delta"],
        "asrc_starved_idle": idle["asrc_starved_delta"],
        "zero_frame_advance_polls": gaps["zero_frame_advance_polls"],
        "path": str(OUT / "receipt.json"),
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
