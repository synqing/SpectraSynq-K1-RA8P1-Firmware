#!/usr/bin/env python3
"""Lane P7 step 2(a): gate scorer for tempo-tracker traces against truth by construction.

A trace is the tracker's own update stream: one row per update with t_s (update instant, seconds of
input time), bpm, confidence, locked, beat_tick. Definitions (all frozen in GATE-SYNTHETIC-v1.json):

  locked(u)    TempoTrackerEvent.locked at update u.
  asserting(u) locked(u) or confidence(u) >= ASSERT_CONFIDENCE (0.30: the renderer's tempo gate floor,
               inhouse_waveform.cpp:347, clamp01((c-0.30)/0.30); above it the tempo reaches the light).
  correct(u)   |bpm(u) - truth(u)| <= tol * truth(u); truth(u) = 60 / (annotated beat interval holding u).
  P            annotated beat period at the start of the segment being judged.
  grace G      acquisition segments (case start, music after a silence gap): 4 beats;
               step segments (first segment after a tempo step): 2 beats; none segments: 0, except
               after music stops (silence gap): 2 beats of the preceding tempo (release).

Criteria, each per case; a case passes only if every applicable criterion passes:
  LOCK     every update in [a + G*P, b) of every tempo segment is locked and correct
           (= locked within 4 beats / re-acquired within 2 beats AND held to the segment end).
  BPM      every asserting update in [a + G*P, b) of every tempo segment is correct.
  FLIP     zero locked updates, anywhere in a tempo segment, at a metrical relative of the truth
           (x1/2, x2, x2/3, x3/2, x1/3, x3, x3/4, x4/3, each within tol) and not correct; the only
           exemption is a step segment's grace window while the bpm still equals the previous
           segment's truth within tol (hold-over, not a flip).
  LOWCONF  no asserting update in [a + G, b) of any none segment (pad, lead-in, silence gap).
Reported, not gating: acquisition time in beats, phase error of beat ticks against annotated beats.

  p7_gate.py score GATE.json TRUTH_DIR TRACE_DIR OUT.json    exit 0 only if every case passes
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

import numpy as np

RELATIVES = {"x1/2": 0.5, "x2": 2.0, "x2/3": 2.0 / 3.0, "x3/2": 1.5, "x1/3": 1.0 / 3.0, "x3": 3.0,
             "x3/4": 0.75, "x4/3": 4.0 / 3.0}
ASSERT_CONFIDENCE = 0.30
GRACE_BEATS = {"acquire": 4.0, "step": 2.0, "release": 2.0, "none_from_start": 0.0}


def truth_bpm(beats: np.ndarray, t: np.ndarray) -> np.ndarray:
    """60 / interval of the annotated beats holding t (first/last interval extended outward)."""
    iv = np.diff(beats)
    idx = np.clip(np.searchsorted(beats, t, side="right") - 1, 0, len(iv) - 1)
    return 60.0 / iv[idx]


def score_case(trace: dict, truth: dict, tol: float) -> dict:
    t = np.asarray(trace["t_s"], dtype=float)
    bpm = np.asarray(trace["bpm"], dtype=float)
    conf = np.asarray(trace["confidence"], dtype=float)
    locked = np.asarray(trace["locked"], dtype=bool)
    tick = np.asarray(trace.get("beat_tick", np.zeros(len(t))), dtype=bool)
    asserting = locked | (conf >= ASSERT_CONFIDENCE)
    res = {"id": truth["id"], "criteria": {}, "metrics": {}, "segments": []}
    lock_ok = bpm_ok = lowconf_ok = True
    flips: dict[str, int] = {}
    prev_tempo_seg = None
    has_tempo = has_none = False
    phase_err = []
    for seg in truth["segments"]:
        a, b = seg["start_s"], seg["end_s"]
        inseg = (t >= a) & (t < b)
        info = {"kind": seg["kind"], "start_s": a, "end_s": b, "grace": seg["grace"]}
        if seg["kind"] == "none":
            has_none = True
            g = GRACE_BEATS[seg["grace"]] * (60.0 / seg["release_bpm"] if seg["grace"] == "release" else 0.0)
            judged = inseg & (t >= a + g)
            bad = judged & asserting
            info.update({"grace_s": g, "asserting_updates": int(bad.sum()), "judged_updates": int(judged.sum())})
            if bad.any():
                lowconf_ok = False
                info["first_asserting_s"] = float(t[bad][0])
        else:
            has_tempo = True
            beats = np.asarray(seg["beats_s"], dtype=float)
            p = float(beats[1] - beats[0])
            g = GRACE_BEATS[seg["grace"]] * p
            tr = truth_bpm(beats, t)
            correct = np.abs(bpm - tr) <= tol * tr
            judged = inseg & (t >= a + g)
            lock_fail = judged & ~(locked & correct)
            bpm_fail = judged & asserting & ~correct
            # acquisition time: start of the final run of locked & correct updates lasting to segment end
            idx = np.flatnonzero(inseg)
            good = (locked & correct)[idx]
            acq = None
            if len(idx) and good[-1]:
                k = len(good) - 1
                while k > 0 and good[k - 1]:
                    k -= 1
                acq = float((t[idx[k]] - a) / p)
            # flips
            for name, r in RELATIVES.items():
                rel = np.abs(bpm - r * tr) <= tol * r * tr
                f = inseg & locked & rel & ~correct
                if seg["grace"] == "step" and prev_tempo_seg is not None:
                    prev_b = np.asarray(prev_tempo_seg["beats_s"], dtype=float)
                    prev_truth = 60.0 / float(prev_b[-1] - prev_b[-2])
                    hold = (t < a + g) & (np.abs(bpm - prev_truth) <= tol * prev_truth)
                    f &= ~hold
                if f.any():
                    flips[name] = flips.get(name, 0) + int(f.sum())
            # phase (reported only): ticks while locked & correct after grace
            for tt in t[judged & tick & locked & correct]:
                j = np.argmin(np.abs(beats - tt))
                phase_err.append(float(tt - beats[j]))
            info.update({"grace_s": g, "judged_updates": int(judged.sum()), "lock_fail_updates": int(lock_fail.sum()),
                         "bpm_fail_updates": int(bpm_fail.sum()), "acquired_beats": acq,
                         "modal_bpm": float(np.round(np.median(bpm[judged]))) if judged.any() else None})
            if lock_fail.any() or not judged.any():
                lock_ok = False
                if lock_fail.any():
                    info["first_lock_fail_s"] = float(t[lock_fail][0])
            if bpm_fail.any():
                bpm_ok = False
            prev_tempo_seg = seg
        res["segments"].append(info)
    if has_tempo:
        res["criteria"]["LOCK"] = lock_ok
        res["criteria"]["BPM"] = bpm_ok
        res["criteria"]["FLIP"] = not flips
    if has_none:
        res["criteria"]["LOWCONF"] = lowconf_ok
    res["metrics"]["flips"] = flips
    res["metrics"]["phase_ms_median_abs"] = float(np.median(np.abs(phase_err)) * 1e3) if phase_err else None
    res["metrics"]["phase_ms_p95_abs"] = float(np.percentile(np.abs(phase_err), 95) * 1e3) if phase_err else None
    res["pass"] = all(res["criteria"].values())
    return res


def load_trace_csv(path: Path, variant: str = "baseline") -> dict:
    import pandas as pd
    d = pd.read_csv(path)
    d = d[d.variant == variant]
    return {k: d[k].to_numpy() for k in ("t_s", "bpm", "confidence", "locked", "beat_tick")}


def file_sha256(path: Path) -> str:
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def score_suite(gate: dict, truth_dir: Path, traces: dict[str, dict]) -> dict:
    tol = gate["tolerance"]["bpm_relative"]
    results = []
    for case in gate["cases"]:
        truth = json.loads((truth_dir / f"{case['id']}.truth.json").read_text())
        if case.get("truth_sha256") and file_sha256(truth_dir / f"{case['id']}.truth.json") != case["truth_sha256"]:
            raise SystemExit(f"REFUSED: truth for {case['id']} does not match the frozen hash")
        tr = traces.get(case["id"])
        if tr is None:
            results.append({"id": case["id"], "pass": False, "criteria": {"MISSING_TRACE": False}})
            continue
        results.append(score_case(tr, truth, tol))
    failed = [r["id"] for r in results if not r["pass"]]
    by_criterion = {}
    for r in results:
        for c, ok in r["criteria"].items():
            by_criterion.setdefault(c, [0, 0])
            by_criterion[c][0 if ok else 1] += 1
    return {"cases": len(results), "passed": len(results) - len(failed), "failed": len(failed),
            "pass": not failed, "by_criterion_pass_fail": by_criterion, "failed_ids": failed, "results": results}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("score")
    s.add_argument("gate", type=Path)
    s.add_argument("truth_dir", type=Path)
    s.add_argument("trace_dir", type=Path)
    s.add_argument("out", type=Path)
    s.add_argument("--variant", default="baseline")
    a = ap.parse_args(argv)
    gate = json.loads(a.gate.read_text())
    if "frozen_sha256" not in gate:
        raise SystemExit("REFUSED: gate is not frozen")
    for role, rec in gate["tools"].items():
        if file_sha256(Path(__file__).parent / rec["file"]) != rec["sha256"]:
            raise SystemExit(f"REFUSED: {rec['file']} changed after the freeze")
    traces = {}
    for case in gate["cases"]:
        p = a.trace_dir / f"{case['id']}.csv"
        if p.exists():
            traces[case["id"]] = load_trace_csv(p, a.variant)
    out = score_suite(gate, a.truth_dir, traces)
    out["gate_frozen_sha256"] = gate["frozen_sha256"]
    out["variant"] = a.variant
    a.out.write_text(json.dumps(out, indent=1) + "\n")
    print(json.dumps({k: out[k] for k in ("cases", "passed", "failed", "pass", "by_criterion_pass_fail")}))
    return 0 if out["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
