#!/usr/bin/env python3
"""Lane P6 iteration P: palette-mapping metrics (TOLERANCE-FROZEN-P6-PAL.json), from probe CSVs.

  p6_pal.py evaluate --inputs INPUTS --replay DIR --v8 V8DIR --out RESULT.json [--require-pass]

(a) fidelity: offpal_px / lit_px <= 0.001; (b) spread: median pal_bins >= 4 and median pal_span >= 0.25
(right half); (c) motion: share of 1 s windows whose palette_coordinate circular range > 1/32 must be
>= v8's - 0.02, and must not fall from -6 to 0 to +6 dB on each room song (0.02 tolerance).
Music frames: fresh, not confirmed quiet, at least one lit pixel, before the appended zeros.
Exit status: 0 only if every check passes.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
import pandas as pd

ZERO_DB = ["boom_source", "boom_room", "sonic_room", "dopex_room"]
ROOMS = ["boom_room", "sonic_room", "dopex_room"]


def load(replay: Path, manifest: dict, tag: str) -> pd.DataFrame:
    spec = next(s for s in manifest["inputs"].values() if tag in s["files"])
    d = pd.read_csv(replay / f"{tag}.csv")
    return d[(d.fresh == 1) & (d.silence == 0) & (d.lit_px > 0) & (d.t_s < spec["file_seconds"])]


def circular_range(v: np.ndarray) -> float:
    if len(v) < 2:
        return 0.0
    v = np.sort(np.mod(v, 1.0))
    gaps = np.diff(np.concatenate([v, [v[0] + 1.0]]))
    return float(1.0 - gaps.max())


def motion(d: pd.DataFrame) -> float:
    w = np.floor(d.t_s.to_numpy())
    vals = [circular_range(d.palette_coordinate.to_numpy()[w == k]) for k in np.unique(w)
            if (w == k).sum() >= 50]
    return float(np.mean(np.array(vals) > 1.0 / 32.0)) if vals else 0.0


def evaluate(replay: Path, v8: Path, manifest: dict) -> dict:
    rows, ok = [], True
    for tag in ZERO_DB:
        a, r = load(replay, manifest, tag), load(v8, manifest, tag)
        for board in (0, 1):
            x, y = a[a.board == board], r[r.board == board]
            lit, off = int(x.lit_px.sum()), int(x.offpal_px.sum())
            fid = off / lit if lit else 0.0
            bins, span = float(x.pal_bins.median()), float(x.pal_span.median())
            m, m8 = motion(x), motion(y)
            checks = {"a_fidelity": fid <= 0.001, "b_spread": bins >= 4 and span >= 0.25, "c_motion_vs_v8": m >= m8 - 0.02}
            ok &= all(checks.values())
            rows.append({"input": tag, "board": board, "offpal_fraction": fid, "lit_px": lit, "median_bins": bins,
                         "median_span": span, "motion": m, "motion_v8": m8, "checks": checks})
    ladder = []
    for song in ROOMS:
        for board in (0, 1):
            ms = [motion((lambda d: d[d.board == board])(load(replay, manifest, song + sfx))) for sfx in ("-6dB", "", "+6dB")]
            mono = ms[1] >= ms[0] - 0.02 and ms[2] >= ms[1] - 0.02
            ok &= mono
            ladder.append({"song": song, "board": board, "motion_-6_0_+6": ms, "not_falling": bool(mono)})
    return {"criterion": "TOLERANCE-FROZEN-P6-PAL.json", "rows": rows, "ladder": ladder, "pass": bool(ok)}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    e = sub.add_parser("evaluate")
    for k in ("--inputs", "--replay", "--v8", "--out"):
        e.add_argument(k, type=Path, required=True)
    a = ap.parse_args(argv)
    manifest = json.loads((a.inputs / "inputs.json").read_text())
    res = evaluate(a.replay, a.v8, manifest)
    a.out.write_text(json.dumps(res, indent=2) + "\n")
    for r in res["rows"]:
        print(f"{r['input']:12s} b{r['board']} off {r['offpal_fraction']:.4f} bins {r['median_bins']:.0f} span {r['median_span']:.2f} "
              f"motion {r['motion']:.2f} (v8 {r['motion_v8']:.2f}) {'ok' if all(r['checks'].values()) else 'FAIL ' + ','.join(k for k, v in r['checks'].items() if not v)}")
    for l in res["ladder"]:
        print(f"ladder {l['song']:11s} b{l['board']} {[round(v, 2) for v in l['motion_-6_0_+6']]} {'ok' if l['not_falling'] else 'FAIL'}")
    print("overall", "PASS" if res["pass"] else "FAIL")
    return 0 if res["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
