#!/usr/bin/env python3
"""Lane P6 primary metric EXT (TOLERANCE-FROZEN-P6-EXT.json v1, -EXT-v2.json v2): waveform extent release.

  p6_ext.py evaluate --replay DIR --inputs INPUTS --set tuning|holdout --out RESULT.json
                     [--ref B5595C35DIR] [--plant]

extent(t) is the probe column extent_edge (right half, innermost pixel >= 10% of the half's peak,
(160 - i_min)/80). --ref supplies b5595c35's replay for the build_contraction_kept bar; --plant
replaces extent with 1.0 over [t_d, t_d + 1 s] (the frozen planted case). Exit status is the verdict.
"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np
import pandas as pd

RUN = Path(__file__).resolve().parents[1] / "_scratch/waveform-migration-001/runs/P6-20261014T124826Z/permitted"
CRITERIA = {"v1": RUN / "TOLERANCE-FROZEN-P6-EXT.json", "v2": RUN / "TOLERANCE-FROZEN-P6-EXT-v2.json"}


def frames(replay: Path, inputs: dict, key: str) -> pd.DataFrame:
    tag = key.split("@")[0]
    spec = next(s for s in inputs["inputs"].values() if tag in s["files"])
    d = pd.read_csv(replay / f"{tag}.csv")
    d["t_song"] = (d.t_s - spec["offset_s"]) / spec["scale"]
    return d


def stats(x: pd.DataFrame, c: dict, plant: bool) -> dict:
    t = x.t_song.to_numpy()
    e = x.extent_edge.to_numpy().copy()
    td = c["t_d"]
    if plant:
        e[(t >= td) & (t < td + 1.0)] = 1.0
    w = lambda lo, hi: e[(t >= lo) & (t < hi)]
    e_pre = float(np.median(w(td - 0.5, td)))
    e_climax = float(np.median(w(*c["climax"])))
    e_early = float(np.median(w(*c["early"])))
    m = (t >= td) & (t <= td + 0.3 + 1e-9)
    e_max = float(e[m].max())
    after = np.where((t >= td) & (t <= td + 1.0 + 1e-9) & (e >= 0.90))[0]
    t90 = float(t[after[0]] - td) if len(after) else math.inf
    thr = e_pre + 0.5 * (e_max - e_pre)
    on = np.where((t >= td - 0.2) & (t <= td + 0.3 + 1e-9) & (e >= thr))[0]
    lag = float(t[on[0]] - td) if len(on) and e_max - e_pre > 0 else math.inf
    speed = 0.0
    for i in np.where((t >= td - 0.05) & (t <= td + 0.3 + 1e-9))[0]:
        if i >= 3:
            speed = max(speed, (e[i] - e[i - 3]) / 0.03)
    jump = (e_max - e_climax) / max(1.0 - e_climax, 0.05)
    return {"E_pre": e_pre, "E_climax": e_climax, "E_early": e_early, "E_drop_max": e_max, "T90": t90,
            "lag": lag, "speed_per_s": float(speed), "jump": float(jump), "contraction": e_early - e_climax}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    ev = sub.add_parser("evaluate")
    for k in ("--replay", "--inputs", "--out"):
        ev.add_argument(k, type=Path, required=True)
    ev.add_argument("--set", choices=("tuning", "holdout"), required=True)
    ev.add_argument("--ref", type=Path)
    ev.add_argument("--plant", action="store_true")
    ev.add_argument("--criterion", choices=("v1", "v2"), default="v2")
    a = ap.parse_args(argv)
    FROZEN = CRITERIA[a.criterion]
    frozen = json.loads(FROZEN.read_text())
    cases = frozen["cases"]["tuning" if a.set == "tuning" else "holdout_once"]
    inputs = json.loads((a.inputs / "inputs.json").read_text())
    rows, ok = [], True
    for key, c in cases.items():
        d = frames(a.replay, inputs, key)
        r = frames(a.ref, inputs, key) if a.ref else None
        for board in (0, 1):
            s = stats(d[d.board == board], c, a.plant)
            ref_climax = stats(r[r.board == board], c, False)["E_climax"] if r is not None else None
            checks = {"release_speed": s["T90"] <= 0.15,
                      "congruence": (s["E_drop_max"] - s["E_pre"] >= (0.30 if a.criterion == "v1" else
                                     min(0.30, 0.8 * (1.0 - s["E_pre"])))) and (-0.05 <= s["lag"] <= 0.05),
                      "release_size": s["jump"] >= 0.80,
                      "build_contraction_kept": (ref_climax is None) or (s["E_climax"] <= ref_climax + 0.10)}
            passed = all(checks.values())
            ok &= passed
            rows.append({"case": key, "board": board, **s, "ref_E_climax": ref_climax,
                         "checks": checks, "pass": bool(passed)})
    res = {"criterion": str(FROZEN.name), "frozen_sha256": frozen["frozen_sha256"], "set": a.set,
           "plant": a.plant, "ref": str(a.ref) if a.ref else None, "rows": rows, "pass": bool(ok)}
    a.out.write_text(json.dumps(res, indent=2, default=lambda v: None if v is None else float(v)) + "\n")
    for row in rows:
        print(f"{row['case']:18s} b{row['board']} pre {row['E_pre']:.2f} climax {row['E_climax']:.2f} "
              f"early {row['E_early']:.2f} max {row['E_drop_max']:.2f} T90 {row['T90']:.2f} lag {row['lag']:.2f} "
              f"J {row['jump']:.2f} {'PASS' if row['pass'] else 'fail'}")
    print("overall", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
