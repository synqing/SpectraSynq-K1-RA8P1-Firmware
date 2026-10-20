#!/usr/bin/env python3
"""Lane P4 step 4: compare two titan_cost_capture.py runs (BOOM 0-75 s), scored window 28-75 s.

Usage: p4_cost_compare.py <baseline capture dir> <candidate capture dir> <out.json>
Prints per-stage cycle percentiles, the queue-occupancy distribution at hop consumption, and the
fault / loss / skip counter deltas (after - before) for each run. Read-only; no device access.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

STAGES = ("poll_cycles_max", "poll_cycles_sum", "asrc_cycles_sum", "ap_cycles", "donor_cycles", "inhouse_cycles",
          "vp_cycles", "emit_cycles_last", "queue_filled_at_consume")
COUNTERS = ("skipped_releases", "emit_errors", "push_rejected", "asrc_discarded", "stream_epoch", "pair_epoch_drops",
            "pair_skew_drops", "asrc_starved", "led_faults")


def walk(d, prefix=""):
    for k, v in d.items():
        if isinstance(v, dict):
            yield from walk(v, prefix + k + ".")
        else:
            yield prefix + k, v


def deltas(counters: dict) -> dict:
    before = dict(walk(counters.get("before", {})))
    after = dict(walk(counters.get("after", {})))
    out = {}
    for key, v in after.items():
        if key.rsplit(".", 1)[-1] in COUNTERS and isinstance(v, (int, float)) and isinstance(before.get(key), (int, float)):
            out[key] = v - before[key]
    return out


def load(d: Path) -> dict:
    r = json.loads((d / "report.json").read_text())
    c = json.loads((d / "counters.json").read_text())
    return {"build": r["build"], "records": r["records"], "missing_write_seq": r["missing_write_seq"],
            "ring_gaps": len(r["ring_gaps"]), "scored": {s: r["scored_28_75s"].get(s) for s in STAGES},
            "counter_deltas": deltas(c)}


def main() -> int:
    base, cand = load(Path(sys.argv[1])), load(Path(sys.argv[2]))
    rows = {}
    for s in STAGES:
        b, c = base["scored"][s], cand["scored"][s]
        if not b or not c:
            continue
        rows[s] = {k: {"baseline": b[k], "candidate": c[k], "ratio": (c[k] / b[k]) if b[k] else None}
                   for k in ("p50", "p95", "p99", "max")}
    out = {"baseline": {k: base[k] for k in ("build", "records", "missing_write_seq", "ring_gaps", "counter_deltas")},
           "candidate": {k: cand[k] for k in ("build", "records", "missing_write_seq", "ring_gaps", "counter_deltas")},
           "scored_28_75s": rows}
    Path(sys.argv[3]).write_text(json.dumps(out, indent=2) + "\n")
    print(f"baseline {base['build'][:8]}  candidate {cand['build'][:8]}   (scored 28-75 s; cycles @ 1 GHz)")
    for s, v in rows.items():
        print(f"{s:24s} p50 {v['p50']['baseline']:>10.0f} -> {v['p50']['candidate']:>10.0f}   "
              f"p99 {v['p99']['baseline']:>10.0f} -> {v['p99']['candidate']:>10.0f}   "
              f"max {v['max']['baseline']:>10.0f} -> {v['max']['candidate']:>10.0f}")
    for name, run in (("baseline", base), ("candidate", cand)):
        nz = {k: v for k, v in run["counter_deltas"].items() if v}
        print(name, "records", run["records"], "missing", run["missing_write_seq"], "ring gaps", run["ring_gaps"],
              "non-zero counter deltas:", nz or "none")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
