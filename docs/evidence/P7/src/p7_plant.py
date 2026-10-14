#!/usr/bin/env python3
"""Lane P7: oracle and planted-fault tracker traces, written in the probe's CSV format.

The oracle is a tracker that does what the gate asks: locks one beat after each tempo segment starts,
reports the annotated tempo, holds the previous tempo for one beat after a step, drops to zero
confidence the moment music stops, and never asserts on a pad. It must score green on every case.
Each planted fault breaks exactly one behaviour and must turn its criterion red:

  half_time       reports truth x1/2 while locked              -> FLIP, LOCK
  three_two       reports truth x2/3 while locked              -> FLIP, LOCK
  late_lock       first lock 6 beats after music starts        -> LOCK (BPM and FLIP stay green)
  pad_false_lock  pad: locked at 120 BPM, confidence 0.8, 10 s -> LOWCONF
  slow_reacquire  after a tempo step, re-locks after 3 beats   -> LOCK
  single_flip     one locked x3/2 update mid-segment           -> FLIP, LOCK
  silence_hold    stays locked through a silence gap           -> LOWCONF
  asserting_wrong unlocked but confidence 0.5 at x2/3          -> BPM (FLIP stays green: not locked)

  p7_plant.py write GATE.json TRUTH_DIR OUT_DIR FAULT|oracle
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np

UPDATE_HZ = 24000.0 / 180.0 / 3.0
FAULTS = ("oracle", "half_time", "three_two", "late_lock", "pad_false_lock", "slow_reacquire", "single_flip",
          "silence_hold", "asserting_wrong")


def oracle_trace(truth: dict, fault: str = "oracle") -> dict:
    t = np.arange(0.0, truth["duration_s"], 1.0 / UPDATE_HZ)
    bpm = np.full(len(t), 120.0)
    conf = np.zeros(len(t))
    locked = np.zeros(len(t), dtype=bool)
    tick = np.zeros(len(t), dtype=bool)
    prev_truth = None
    is_pad = truth["case"]["family"] == "drumless_pad"
    for seg in truth["segments"]:
        a, b = seg["start_s"], seg["end_s"]
        m = (t >= a) & (t < b)
        if seg["kind"] == "none":
            if fault == "silence_hold" and seg["grace"] == "release":
                locked[m] = True
                conf[m] = 0.9
                bpm[m] = prev_truth
            if fault == "pad_false_lock" and is_pad:
                mm = m & (t >= 10.0) & (t < 20.0)
                locked[mm] = True
                conf[mm] = 0.8
                bpm[mm] = 120.0
            continue
        beats = np.asarray(seg["beats_s"])
        p = beats[1] - beats[0]
        iv = np.diff(beats)
        idx = np.clip(np.searchsorted(beats, t, side="right") - 1, 0, len(iv) - 1)
        tr = 60.0 / iv[idx]
        lock_after = 1.0
        if fault == "late_lock" and seg["grace"] == "acquire":
            lock_after = 6.0
        if fault == "slow_reacquire" and seg["grace"] == "step":
            lock_after = 3.0
        on = m & (t >= a + lock_after * p)
        if seg["grace"] == "step" and prev_truth is not None and fault != "slow_reacquire":
            hold = m & ~on
            locked[hold] = True
            conf[hold] = 0.9
            bpm[hold] = prev_truth
        locked[on] = True
        conf[on] = 0.9
        bpm[on] = tr[on]
        if fault == "half_time":
            bpm[on] = tr[on] * 0.5
        if fault == "three_two":
            bpm[on] = tr[on] * 2.0 / 3.0
        if fault == "single_flip":
            k = np.flatnonzero(on)
            if len(k):
                j = k[len(k) // 2]
                bpm[j] = tr[j] * 1.5
        if fault == "asserting_wrong":
            locked[on] = False
            conf[on] = 0.5
            bpm[on] = tr[on] * 2.0 / 3.0
        for bt in beats:
            j = np.searchsorted(t, bt)
            if j < len(t) and on[j]:
                tick[j] = True
        prev_truth = 60.0 / iv[-1]
    return {"t_s": t, "bpm": bpm, "confidence": conf, "locked": locked.astype(int), "beat_tick": tick.astype(int)}


def write_csv(trace: dict, path: Path, variant: str = "baseline") -> None:
    rows = ["t_s,variant,bpm,confidence,locked,beat_tick"]
    for i in range(len(trace["t_s"])):
        rows.append(f"{trace['t_s'][i]:.6f},{variant},{trace['bpm'][i]:.4f},{trace['confidence'][i]:.4f},"
                    f"{int(trace['locked'][i])},{int(trace['beat_tick'][i])}")
    path.write_text("\n".join(rows) + "\n")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    w = sub.add_parser("write")
    w.add_argument("gate", type=Path)
    w.add_argument("truth_dir", type=Path)
    w.add_argument("out", type=Path)
    w.add_argument("fault", choices=FAULTS)
    a = ap.parse_args(argv)
    gate = json.loads(a.gate.read_text())
    a.out.mkdir(parents=True, exist_ok=True)
    for case in gate["cases"]:
        truth = json.loads((a.truth_dir / f"{case['id']}.truth.json").read_text())
        write_csv(oracle_trace(truth, a.fault), a.out / f"{case['id']}.csv")
    print(json.dumps({"fault": a.fault, "cases": len(gate["cases"])}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
