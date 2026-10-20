#!/usr/bin/env python3
"""Offline hop-136 five-millisecond timing-mutation detector.

A scorer that misses the declared injection FAILS.
Does not open CDC. Does not flash. Does not run a profiler image.
Reconciled into the live host tree from swarm incoming/P (P3-host).
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path


def detect_hop136_injection(raw_stage_trace: Path, clock_hz: int) -> dict:
    if clock_hz <= 0:
        return {"ok": False, "code": "BAD_CLOCK", "detail": "clock_hz must be positive"}
    if not raw_stage_trace.is_file():
        return {"ok": False, "code": "MISSING_TRACE", "detail": str(raw_stage_trace)}
    injected = []
    hop136 = None
    n = 0
    with raw_stage_trace.open() as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            row = json.loads(line)
            n += 1
            if row.get("hop") == 136:
                hop136 = row
            if row.get("injected_delay_cycles") or row.get("timing_mutation"):
                injected.append(row)
    minimum = clock_hz // 200  # declared 5 ms negative
    if len(injected) != 1:
        return {
            "ok": False,
            "code": "INJECTION_COUNT",
            "detail": f"expected exactly one injected hop, found {len(injected)}",
            "records": n,
            "injected": injected,
            "hop136": hop136,
            "minimum_cycles": minimum,
        }
    row = injected[0]
    if row.get("hop") != 136:
        return {
            "ok": False,
            "code": "WRONG_HOP",
            "detail": f"injection at hop {row.get('hop')} not 136",
            "records": n,
            "injected": injected,
            "minimum_cycles": minimum,
        }
    cycles = int(row.get("injected_delay_cycles") or 0)
    if cycles < minimum:
        return {
            "ok": False,
            "code": "TOO_SHORT",
            "detail": f"injected_delay_cycles={cycles} < minimum {minimum} (5 ms at clock_hz)",
            "records": n,
            "injected": injected,
            "minimum_cycles": minimum,
        }
    if not row.get("timing_mutation"):
        return {
            "ok": False,
            "code": "FLAG_MISSING",
            "detail": "timing_mutation flag false/absent on injected hop",
            "records": n,
            "injected": injected,
            "minimum_cycles": minimum,
        }
    return {
        "ok": True,
        "code": "DETECTED",
        "detail": "hop-136 five-millisecond injection detected",
        "records": n,
        "injected": injected,
        "minimum_cycles": minimum,
        "injected_delay_cycles": cycles,
        "injected_delay_us": (cycles * 1_000_000) // clock_hz,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--raw-stage-trace", type=Path, required=True)
    parser.add_argument("--clock-hz", type=int, required=True)
    args = parser.parse_args()
    result = detect_hop136_injection(args.raw_stage_trace, args.clock_hz)
    print(json.dumps(result, indent=2))
    return 0 if result["ok"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
