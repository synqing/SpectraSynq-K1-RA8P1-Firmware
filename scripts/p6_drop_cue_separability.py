#!/usr/bin/env python3
"""Drop-cue feasibility, step 2: does any causal feature put every drop above everything else?

Reads CUE-SEPARABILITY-FROZEN.json and the frozen v4 anchors. Does not widen either.
Windows come from runs/P6-DROP-DETECTOR/v4/detector.py (24 kHz, 7.5 ms hops).

  p6_drop_cue_separability.py --run DIR [--plant anchor_shift_minus_100ms|shuffled_anchor]
"""
from __future__ import annotations

import argparse
import importlib.util
import json
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from scipy import signal

W = Path(__file__).resolve().parents[1]
V4 = W / "_scratch/waveform-migration-001/runs/P6-DROP-DETECTOR/v4/detector.py"
FS, HOP = 24000, 180
LAG_HOPS = int(round(0.050 / (HOP / FS)))  # trailing statistics end 50 ms before the hop
WARM_S = 2.0
NEAR_S = 0.5
MARGIN_DB = 3.0


def load_v4():
    spec = importlib.util.spec_from_file_location("drop_v4", V4)
    mod = importlib.util.module_from_spec(spec)
    sys.modules["drop_v4"] = mod
    spec.loader.exec_module(mod)
    return mod


def level_db(x: np.ndarray, band: tuple[float, float] | None) -> np.ndarray:
    """Causal 10 ms mean-square level at the end of each hop."""
    if band is not None:
        sos = signal.butter(2, band, btype="band", fs=FS, output="sos")
        x = signal.sosfilt(sos, x)
    win = int(0.010 * FS)
    c = np.concatenate([[0.0], np.cumsum(x * x)])
    n = len(x) // HOP
    ends = (np.arange(n) + 1) * HOP
    starts = np.maximum(ends - win, 0)
    p = (c[ends] - c[starts]) / np.maximum(ends - starts, 1)
    return 10 * np.log10(p + 1e-12)


def trailing(lv: np.ndarray, span_s: float, stat: str) -> np.ndarray:
    span = int(round(span_s / (HOP / FS)))
    out = np.full(len(lv), np.nan)
    for i in range(LAG_HOPS + span, len(lv)):
        seg = lv[i - LAG_HOPS - span:i - LAG_HOPS]
        out[i] = np.median(seg) if stat == "median" else (seg.max() if stat == "max" else np.percentile(seg, 90))
    return out


FEATURES = {
    "F1_sub_vs_median": ((30, 110), 1.0, "median"),
    "F2_sub_vs_max": ((30, 110), 1.0, "max"),
    "F3_kick_vs_max": ((40, 160), 1.0, "max"),
    "F4_broad_vs_max": (None, 1.0, "max"),
    "F5_sub_vs_p90_2s": ((30, 110), 2.0, "p90"),
}


def run(out: Path, plant: str | None) -> dict:
    v4 = load_v4()
    windows = v4.load_scored_windows()
    res = {"plant": plant, "features": {}}
    traces = {}
    for fname, (band, span_s, stat) in FEATURES.items():
        drops, worst = [], None
        for w in windows:
            lv = level_db(np.asarray(w.pcm, dtype=np.float64), band)
            f = lv - trailing(lv, span_s, stat)
            t = np.array([w.time_of(i) for i in range(len(lv))])
            anchors = list(w.drops)
            if plant == "anchor_shift_minus_100ms":
                anchors = [d - 0.100 for d in anchors]
            elif plant == "shuffled_anchor":
                anchors = [d - 10.0 for d in anchors]
            scored = (t >= w.scored_lo + WARM_S) & (t < w.scored_hi) & np.isfinite(f)
            for d in anchors:
                m = scored & (t >= d - 1e-9) & (t <= d + 0.050 + 1e-9)
                v = float(f[m].max()) if m.any() else float("nan")
                at = float(t[m][np.argmax(f[m])]) if m.any() else None
                drops.append({"window": w.id, "anchor_s": round(d, 3), "value_db": v,
                              "at_ms": None if at is None else round(1000 * (at - d), 1)})
            far = scored.copy()
            for d in w.drops:  # non-drop hops are always judged against the true anchors
                far &= np.abs(t - d) > NEAR_S
            if far.any():
                j = int(np.argmax(np.where(far, f, -1e9)))
                cand = {"window": w.id, "t_s": round(float(t[j]), 3), "value_db": float(f[j])}
                if worst is None or cand["value_db"] > worst["value_db"]:
                    worst = cand
            if fname == "F2_sub_vs_max" and plant is None:
                traces[w.id] = (t, f, w.drops)
        vals = [d["value_db"] for d in drops]
        low = min(vals) if all(np.isfinite(vals)) else float("nan")
        margin = low - worst["value_db"]
        res["features"][fname] = {"drops": drops, "worst_non_drop": worst,
                                  "margin_db": margin, "feasible": bool(margin >= MARGIN_DB)}
    best = max(res["features"], key=lambda k: np.nan_to_num(res["features"][k]["margin_db"], nan=-1e9))
    res["best"] = best
    res["any_feasible"] = any(v["feasible"] for v in res["features"].values())
    out.mkdir(parents=True, exist_ok=True)
    tag = plant or "measured"
    (out / f"separability-{tag}.json").write_text(json.dumps(res, indent=2) + "\n")
    if traces:
        fig, axes = plt.subplots(len(traces), 1, figsize=(12, 2.4 * len(traces)))
        for ax, (wid, (t, f, ds)) in zip(axes, traces.items()):
            ax.plot(t, f, lw=0.6, color="#1d4e89")
            for d in ds:
                ax.axvline(d, color="#c8361f", lw=1)
            ax.set_ylim(-40, 40)
            ax.set_title(f"{wid}: F2 (sub level minus its max over the previous second, dB); red = measured drop")
        axes[-1].set_xlabel("song time (s)")
        fig.tight_layout()
        fig.savefig(out / "separability-F2.png", dpi=100)
    return res


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", type=Path, required=True)
    ap.add_argument("--plant", choices=["anchor_shift_minus_100ms", "shuffled_anchor"])
    a = ap.parse_args()
    res = run(a.run, a.plant)
    for k, v in res["features"].items():
        dv = " ".join(f"{d['value_db']:.1f}@{d['at_ms']}" for d in v["drops"])
        wn = v["worst_non_drop"]
        print(f"{k:18s} margin {v['margin_db']:6.1f} dB {'FEASIBLE' if v['feasible'] else 'no'} | drops {dv} | "
              f"worst non-drop {wn['value_db']:.1f} at {wn['window']} {wn['t_s']}")
    print("best", res["best"], "any feasible", res["any_feasible"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
