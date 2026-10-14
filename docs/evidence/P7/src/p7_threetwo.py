#!/usr/bin/env python3
"""Lane P7 step 1: test the inference "92 = 2/3 x 138 is the dotted-quarter of BOOM's 3+3+2 accent pattern".

Three independent looks, each with a control:
  1. Comb decomposition. A numpy replica of tempo_acf.cpp rebuilds the harmonic comb from the tracker's own
     novelty ring (hist_sample, novelty_scale logged by the probe) and is checked against the C++ comb values.
     It splits comb(92) and comb(138) into their four teeth (lags 1, 2, 3, 4 x the bin's beat period).
  2. Line test (rhythm_evidence.line_test, off-grid controls): does the tracker's novelty hold a spectral
     line at 1.533 Hz (92 BPM)? A 3+3+2 pattern repeats once per bar, so it predicts lines only at bar
     harmonics (k x 0.575 Hz); a genuine dotted-quarter pulse (polymeter control) predicts a 1.533 Hz line.
  3. Autocorrelation at 1, 1.25, 1.5, 1.75 and 2 beats of 138 (rhythm_evidence.acf).
Controls: synthetic four-on-the-floor 138 (no accent), the same with a 3+3+2 stab overlay (suite cases
fotf_138, fotf332_138), a polymetric dotted-quarter stab (dq_138, not a suite case), Dopex (file, room).

  p7_threetwo.py      (reads diagnosis/csv/*.csv.gz, writes diagnosis/threetwo.json)
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
import pandas as pd

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, "/Users/spectrasynq/Workspace_Management/Software/agent-skills/skills/spectrasynq-lightshow-loop/scripts")
import p7common as c  # noqa: E402
import p7_synth  # noqa: E402
import rhythm_evidence as rev  # noqa: E402

RATE = np.float32(24000.0) / np.float32(180.0) / np.float32(3.0)
HIST = 512
WEIGHTS = (1.0, 0.8, 0.7, 0.6)
BEAT_HZ = 138.0 / 60.0
OUT = c.RUN / "diagnosis"


def comb_replica(hist: np.ndarray, scale: np.ndarray, k: int):
    """Comb salience and per-tooth values (normalised by the max comb over 60..155) at update k."""
    lo = max(0, k - HIST + 1)
    s = hist[lo:k + 1]
    ages = np.arange(len(s))[::-1]                     # newest has age 0 (written after the decay)
    vals = s * (0.999 ** ages)
    work = np.zeros(HIST)
    work[HIST - len(vals):] = vals
    work = work * scale[k]
    work = work - work.mean()
    min_lag = int(np.floor(RATE * 60.0 / 160.0)) - 1
    max_lag = int(np.ceil(4 * RATE * 60.0 / 55.0)) + 1
    lag_count = min(max_lag - min_lag + 1, 200)
    full = np.correlate(work, work, "full")[HIST - 1:]
    acf = full[min_lag:min_lag + lag_count]

    def at(real):
        lf = int(np.floor(real))
        fr = real - lf
        ce = lf - min_lag
        if 1 <= ce < lag_count - 1:
            p, cu, n = acf[ce - 1], acf[ce], acf[ce + 1]
            return cu + 0.5 * fr * (n - p) + 0.5 * fr * fr * (n - 2 * cu + p)
        if 0 <= ce < lag_count:
            return acf[ce]
        return 0.0

    teeth = np.zeros((96, 4))
    for b in range(96):
        rl = float(RATE) * 60.0 / (60.0 + b)
        for t in range(4):
            teeth[b, t] = WEIGHTS[t] * max(0.0, at(rl * (t + 1)))
    comb = teeth.sum(axis=1)
    m = max(comb.max(), 1e-12)
    return comb / m, teeth / m


def decompose(d: pd.DataFrame, lo: float, hi: float) -> dict:
    d = d[d.variant == "baseline"].reset_index(drop=True)
    hist = d.hist_sample.to_numpy()
    scale = d.novelty_scale.to_numpy()
    idx = np.flatnonzero((d.t_s >= lo) & (d.t_s < hi))
    err, teeth92, teeth138, teeth69 = [], [], [], []
    for k in idx[::4]:
        comb, teeth = comb_replica(hist, scale, k)
        for bpm, col in ((69, "comb69"), (92, "comb92"), (138, "comb138")):
            err.append(abs(comb[bpm - 60] - d[col].iloc[k]))
        teeth92.append(teeth[32])
        teeth138.append(teeth[78])
        teeth69.append(teeth[9])
    t92, t138, t69 = (np.mean(x, axis=0) for x in (teeth92, teeth138, teeth69))
    return {"replica_max_abs_err_vs_cpp": float(max(err)), "replica_updates": len(err) // 3,
            "teeth92_lags_beats138": [1.5, 3.0, 4.5, 6.0], "teeth92_mean": t92.round(4).tolist(),
            "teeth138_lags_beats138": [1, 2, 3, 4], "teeth138_mean": t138.round(4).tolist(),
            "teeth69_lags_beats138": [2, 4, 6, 8], "teeth69_mean": t69.round(4).tolist(),
            "comb92_share_from_kick_grid_teeth_2_4": float((t92[1] + t92[3]) / max(t92.sum(), 1e-12)),
            "comb92_share_from_dotted_quarter_tooth_1": float(t92[0] / max(t92.sum(), 1e-12)),
            "comb92_over_comb138": float(t92.sum() / max(t138.sum(), 1e-12)),
            "comb69_over_comb138": float(t69.sum() / max(t138.sum(), 1e-12))}


def periodicity(d: pd.DataFrame, lo: float, hi: float) -> dict:
    d = d[(d.variant == "baseline") & (d.t_s >= lo) & (d.t_s < hi)]
    x = d.hist_sample.to_numpy()
    fs = float(RATE)
    rates = {"beat_138": BEAT_HZ, "dotted_quarter_92": 92.0 / 60.0, "half_69": 69.0 / 60.0, "bar_34.5": BEAT_HZ / 4}
    lt = rev.line_test(x, fs, list(rates.values()), rev.offgrid_controls(BEAT_HZ / 4))
    names = {str(v): k for k, v in rates.items()}
    a = rev.acf(x)
    lags = {f"{b}": float(np.interp(b * fs / BEAT_HZ, np.arange(len(a)), a)) for b in (1.0, 1.25, 1.5, 1.75, 2.0)}
    return {"line_ratio": {names[k]: round(v, 2) for k, v in lt["rates"].items()},
            "line": {names[k]: bool(v) for k, v in lt["line"].items()},
            "control_max": round(lt["control_max"], 2), "controls": {k: round(v, 2) for k, v in lt["controls"].items()},
            "acf_at_beats138": {k: round(v, 3) for k, v in lags.items()},
            "dotted_quarter_excess": round(lags["1.5"] - 0.5 * (lags["1.25"] + lags["1.75"]), 3),
            "seconds": round(len(x) / fs, 2)}


def dq_case() -> np.ndarray:
    """Polymetric control: kick + offbeat hat at 138 and a stab every 3 eighths, never reset at the bar."""
    rng = np.random.default_rng(138332)
    kick, hat, stab = p7_synth._kick(rng), p7_synth._hat(rng), p7_synth._stab()
    dur, a, p = 31.0, 1.0, 60.0 / 138.0
    buf = np.zeros(int((dur + 0.5) * p7_synth.FS))
    for b in p7_synth._grid(a, dur, 138.0):
        p7_synth._place(buf, kick, b, 1.0)
        p7_synth._place(buf, hat, b + p / 2, 0.35)
    for s in p7_synth._grid(a, dur, 138.0 / 1.5):
        p7_synth._place(buf, stab, s, 0.9)
    buf *= 10 ** (-6 / 20) / np.max(np.abs(buf))
    return buf


def main() -> int:
    exe = c.build()
    cases = {c_["id"]: c_ for c_ in p7_synth.case_list()}
    ctl = {}
    for cid in ("fotf_138", "fotf332_138"):
        x, _ = p7_synth.render(cases[cid])
        ctl[cid] = x
    ctl["dq_138"] = dq_case()
    frames = {}
    for name, x in ctl.items():
        s16 = c.write_s16(x, 1.0, c.WORK / f"ctl_{name}.s16")
        c.run_probe(exe, s16, c.FS, c.WORK / f"ctl_{name}.csv", "baseline")
        frames[f"synthetic_{name}"] = (pd.read_csv(c.WORK / f"ctl_{name}.csv"), 13.0, 31.0)
        d = frames[f"synthetic_{name}"][0]
        d.to_csv(OUT / "csv" / f"ctl_{name}.csv.gz", index=False)
    for name, lo, hi in (("boom_file", 55.5, 75.0), ("boom_room", 55.5, 75.0), ("dopex_file", 55.5, 75.0),
                         ("dopex_room", 55.5, 75.0)):
        frames[name] = (pd.read_csv(OUT / "csv" / f"{name}.csv.gz"), lo, hi)
    res = {"rhythm_evidence_selftest": rev.selftest()["all_red_as_expected"], "windows": {}}
    for name, (d, lo, hi) in frames.items():
        b = d[(d.variant == "baseline") & (d.t_s >= lo) & (d.t_s < hi)]
        res["windows"][name] = {
            "window_s": [lo, hi],
            "baseline_winner_share": {k: round(float(((b.bpm >= x) & (b.bpm <= y)).mean()), 3)
                                      for k, (x, y) in {"69": (68, 70), "92": (91, 93), "138": (137, 139)}.items()},
            "cpp_comb_median": {k: round(float(b[f"comb{k}"].median()), 3) for k in ("69", "92", "138")},
            "decomposition": decompose(d, lo, hi),
            "periodicity": periodicity(d, lo, hi),
        }
        print(name, json.dumps(res["windows"][name]["baseline_winner_share"]), flush=True)
    res["prior"] = {str(b): float(np.exp(-0.5 * (np.log2(b / 88.0) / 0.75) ** 2)) for b in (69, 92, 138)}
    res["win_thresholds"] = {"92_beats_138_if_comb92/comb138 >": res["prior"]["138"] / res["prior"]["92"],
                             "69_beats_138_if_comb69/comb138 >": res["prior"]["138"] / res["prior"]["69"]}
    (OUT / "threetwo.json").write_text(json.dumps(res, indent=1) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
