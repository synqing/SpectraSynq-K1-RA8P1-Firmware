#!/usr/bin/env python3
"""Figures and summary numbers for docs/evidence/P3/agc-trace.html (Captain, 14 Oct 2026).

Reads docs/evidence/P3/agc-trace-boom-{recording,source}.csv (scripts/p3_agc_trace.py)
and the device's own payload snapshots (P3 analysis/p3-data.json). Writes
fig-trace.png, fig-colour-split.png, fig-correlation.png, fig-source-vs-room.png and
agc-trace-summary.json beside them. Offline, read-only.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy.stats import spearmanr

ROOT = Path(__file__).resolve().parents[1]
P3 = ROOT / "_scratch/waveform-migration-001/runs/P3-20261014T080935Z/permitted/analysis/p3-data.json"
BG, FG, GRID = "#0f1115", "#e8e6e1", "#2a2e36"
GOLD, CRIMSON, TEAL, GREY, BLUE = "#d9a441", "#c8324a", "#4fb3a9", "#8a8f98", "#6f8fd8"
plt.rcParams.update({"figure.facecolor": BG, "axes.facecolor": BG, "axes.edgecolor": GRID, "axes.labelcolor": FG,
                     "xtick.color": FG, "ytick.color": FG, "text.color": FG, "grid.color": GRID, "font.size": 10.5,
                     "axes.titleweight": "bold", "axes.titlesize": 11, "legend.frameon": True,
                     "legend.facecolor": BG, "legend.edgecolor": GRID, "legend.framealpha": 0.88})
LUMA = np.array([0.2126, 0.7152, 0.0722])
INPUTS = ["peak_scaled", "observed_peak_scaled", "vu_level", "raw_rms", "silence", "spectral_energy", "novelty",
          "tempo_confidence", "beat_gate", "tempo_bpm", "chroma_energy", "chroma_pregate_max", "palette_coordinate",
          "held_coordinate", "peak_stage2", "position", "decay_rate", "dot_max"]


def load(out, name):
    d = pd.read_csv(out / f"agc-trace-boom-{name}.csv")
    ch = [f"chroma{i}" for i in range(12)]
    d["chroma_energy"] = d[ch].mean(axis=1)
    d["dot_max"] = d[["dot_r", "dot_g", "dot_b"]].max(axis=1)
    return d


def shade(ax):
    ax.axvspan(45, 50, color=TEAL, alpha=0.08); ax.axvspan(50, 55, color=GREY, alpha=0.16)
    ax.axvspan(55.5, 60, color=CRIMSON, alpha=0.10); ax.grid(alpha=0.35)


def main(out: Path):
    dev = json.loads(P3.read_text())["boom"]
    rec = load(out, "recording"); src = load(out, "source")
    summ = {"windows_s": {"build": [45, 50], "quiet_cut": [50, 55], "drop": [55.5, 60]}}

    # Device payload: colour split of the luma change (verified on the device's own data)
    split = {}
    for w, name in (("w0", "lower"), ("w1", "upper")):
        def sec(a, b):
            rows = [s[w] for s in dev["samples"] if s[w] and a <= s["t_" + w] < b]
            return np.array([r["mean_rgb"] for r in rows]).mean(0), float(np.mean([r["lit_fraction"] for r in rows])), len(rows)
        (bb, bl, nb), (dd, dl, nd) = sec(45, 50), sec(55.5, 60)
        split[name] = {"build_rgb_permille": (bb * 1000).round(3).tolist(), "drop_rgb_permille": (dd * 1000).round(3).tolist(),
                       "luma_ratio": float((dd @ LUMA) / (bb @ LUMA)), "channel_sum_ratio": float(dd.sum() / bb.sum()),
                       "hue_efficiency_ratio": float(((dd @ LUMA) / dd.sum()) / ((bb @ LUMA) / bb.sum())),
                       "lit_fraction": [bl, dl], "n": [nb, nd]}
    summ["device_colour_split"] = split

    # Host replay: build vs drop means and 45-60 s rank correlations (0.25 s smoothing)
    host = {}
    for name, d in (("recording", rec), ("source", src)):
        host[name] = {}
        for b, bname in ((0, "lower"), (1, "upper")):
            h = d[d.board == b]
            win = h[(h.t_s >= 45) & (h.t_s <= 60)]
            L = win.luma.rolling(25, center=True, min_periods=1).mean()
            cor = {}
            for c in INPUTS:
                x = win[c].rolling(25, center=True, min_periods=1).mean()
                if x.std() > 0:
                    cor[c] = float(spearmanr(x, L)[0])
            bw = h[(h.t_s >= 45) & (h.t_s < 50)]; dw = h[(h.t_s >= 55.5) & (h.t_s < 60)]
            means = {c: [float(bw[c].mean()), float(dw[c].mean())] for c in
                     ["luma", "peak_scaled", "vu_level", "chroma_energy", "palette_coordinate", "held_coordinate",
                      "decay_rate", "dot_r", "dot_g", "dot_b", "tempo_bpm", "beat_gate"]}
            w_ = "w0" if b == 0 else "w1"
            tt = np.array([s["t_" + w_] for s in dev["samples"] if s[w_] and 0 <= s["t_" + w_] <= 75])
            vv = np.array([s[w_]["brightness"] for s in dev["samples"] if s[w_] and 0 <= s["t_" + w_] <= 75])
            dv = pd.Series(vv).rolling(3, center=True, min_periods=1).mean().values
            hl = np.interp(tt, h.t_s, h.luma.rolling(100, center=True, min_periods=1).mean())
            host[name][bname] = {"rank_corr_luma_45_60": dict(sorted(cor.items(), key=lambda kv: -abs(kv[1]))),
                                 "build_vs_drop": means, "fidelity_rho_vs_device_1s": float(spearmanr(hl, dv)[0])}
    summ["host"] = host
    summ["meta"] = json.loads((out / "agc-trace-meta.json").read_text())

    # Figure 1: per-frame trace, room-recording replay, 40-65 s
    r0 = rec[(rec.board == 0) & (rec.t_s >= 40) & (rec.t_s <= 65)]; r1 = rec[(rec.board == 1) & (rec.t_s >= 40) & (rec.t_s <= 65)]
    sm = lambda s: s.rolling(25, center=True, min_periods=1).mean()
    fig, ax = plt.subplots(5, 1, figsize=(11, 11.5), sharex=True)
    ax[0].plot(r0.t_s, sm(r0.vu_level), color=TEAL, lw=1.4, label="level, not AGC'd (vu_level)")
    ax[0].plot(r0.t_s, sm(r0.peak_scaled), color=GOLD, lw=1.4, label="AGC output (peak_scaled)")
    ax[0].set_ylabel("level inputs"); ax[0].legend(fontsize=8.5, loc="upper left")
    ax[0].set_title("Inputs the effects read (host replay of the room recording, 0.25 s smoothing)")
    ax[1].plot(r0.t_s, sm(r0.chroma_energy), color=FG, lw=1.4, label="chroma energy after the tonality gate")
    ax[1].set_ylabel("tonality"); ax[1].legend(fontsize=8.5, loc="upper left")
    ax[2].plot(r1.t_s, sm(r1.palette_coordinate), color=CRIMSON, lw=1.4, label="upper: palette position (chroma centroid)")
    ax[2].plot(r0.t_s, sm(r0.held_coordinate), color=GOLD, lw=1.4, label="lower: held palette position")
    ax[2].set_ylabel("palette 0–1"); ax[2].legend(fontsize=8.5, loc="upper left")
    ax[3].plot(r0.t_s, sm(r0.decay_rate), color=GOLD, lw=1.4, label="lower: trail decay rate (/s)")
    ax[3].plot(r1.t_s, sm(r1.decay_rate), color=CRIMSON, lw=1.4, label="upper: fade rate (/s)")
    ax[3].set_ylabel("decay (/s)"); ax[3].legend(fontsize=8.5, loc="upper left")
    ax[4].plot(r0.t_s, sm(r0.luma) * 1000, color=GOLD, lw=1.3, label="lower: host luma")
    ax[4].plot(r1.t_s, sm(r1.luma) * 1000, color=CRIMSON, lw=1.3, label="upper: host luma")
    for w_, c, lab in (("w0", GOLD, "lower: device payload"), ("w1", CRIMSON, "upper: device payload")):
        tt = [s["t_" + w_] for s in dev["samples"] if s[w_] and 40 <= s["t_" + w_] <= 65]
        vv = [s[w_]["brightness"] * 1000 for s in dev["samples"] if s[w_] and 40 <= s["t_" + w_] <= 65]
        ax[4].plot(tt, vv, ls="none", marker="o", ms=4, color=c, mfc="none", label=lab)
    ax[4].set_ylabel("luma (‰ of full scale)"); ax[4].legend(fontsize=8, loc="upper right", ncol=2)
    ax[4].set_xlabel("BOOM song time (s) · teal = build · grey = quiet cut · red = drop")
    for a in ax:
        shade(a)
    fig.tight_layout(); fig.savefig(out / "fig-trace.png", dpi=105); plt.close(fig)

    # Figure 2: device colour split
    fig, ax = plt.subplots(1, 2, figsize=(11, 4.2))
    for i, name in enumerate(("lower", "upper")):
        s = split[name]
        x = np.arange(2)
        b_, d_ = np.array(s["build_rgb_permille"]), np.array(s["drop_rgb_permille"])
        for k, (c, lab) in enumerate(((CRIMSON, "red"), ("#3fa34d", "green"), (BLUE, "blue"))):
            ax[i].bar(x + (k - 1) * 0.26, [b_[k], d_[k]], width=0.24, color=c, label=lab)
        ax[i].set_xticks(x); ax[i].set_xticklabels(["build 45–50 s", "drop 55.5–60 s"])
        ax[i].set_ylabel("mean channel (‰ of full scale)"); ax[i].grid(alpha=0.35, axis="y")
        ax[i].set_title(f"{name} board: luma ×{s['luma_ratio']:.2f} = light ×{s['channel_sum_ratio']:.2f} × colour ×{s['hue_efficiency_ratio']:.2f}")
        ax[i].legend(fontsize=8.5, loc="upper right")
    fig.suptitle("Device payload: at the drop the green goes; red stays — much of the 'dim' is a colour change", fontweight="bold", fontsize=11)
    fig.tight_layout(); fig.savefig(out / "fig-colour-split.png", dpi=110); plt.close(fig)

    # Figure 3: rank correlations (room recording replay, 45-60 s)
    fig, ax = plt.subplots(1, 2, figsize=(11, 4.6))
    labels = {"peak_scaled": "AGC output", "observed_peak_scaled": "AGC peak (window)", "vu_level": "level (not AGC'd)",
              "raw_rms": "raw RMS", "spectral_energy": "spectral energy", "novelty": "novelty",
              "tempo_confidence": "tempo confidence", "beat_gate": "beat gate", "tempo_bpm": "tempo BPM",
              "chroma_energy": "chroma energy", "chroma_pregate_max": "chroma (pre-gate) max",
              "palette_coordinate": "palette position", "held_coordinate": "held palette position",
              "peak_stage2": "smoothed peak", "position": "deposit position", "decay_rate": "decay rate",
              "dot_max": "colour brightness", "silence": "silence flag"}
    for i, name in enumerate(("lower", "upper")):
        cor = host["recording"][name]["rank_corr_luma_45_60"]
        keys = list(cor)[:9][::-1]
        vals = [cor[k] for k in keys]
        ax[i].barh([labels.get(k, k) for k in keys], vals, color=[TEAL if v > 0 else CRIMSON for v in vals])
        ax[i].axvline(0, color=GREY, lw=0.8); ax[i].set_xlim(-1, 1); ax[i].grid(alpha=0.35, axis="x")
        fid = host["recording"][name]["fidelity_rho_vs_device_1s"]
        ax[i].set_title(f"{name} board (replay vs device ρ {fid:.2f})")
        ax[i].set_xlabel("rank correlation with luma, 45–60 s")
    fig.suptitle("What moves brightness in the build → drop window (host replay of the room recording)", fontweight="bold", fontsize=11)
    fig.tight_layout(); fig.savefig(out / "fig-correlation.png", dpi=110); plt.close(fig)

    # Figure 4: source file vs room recording, upper board palette position and luma
    fig, ax = plt.subplots(2, 1, figsize=(11, 5.4), sharex=True)
    for d, c, lab in ((rec, CRIMSON, "room recording"), (src, GREY, "song file")):
        u = d[(d.board == 1) & (d.t_s >= 40) & (d.t_s <= 65)]
        ax[0].plot(u.t_s, sm(u.palette_coordinate), color=c, lw=1.3, label=lab)
        ax[1].plot(u.t_s, sm(u.luma) * 1000, color=c, lw=1.3, label=lab)
    ax[0].set_ylabel("upper palette position"); ax[0].legend(fontsize=8.5, loc="upper left")
    ax[1].set_ylabel("upper luma (‰)"); ax[1].set_xlabel("BOOM song time (s)")
    for a in ax:
        shade(a)
    fig.suptitle("Same effect, two inputs: the room recording holds gold (≈0.45) through the build; both sit at ≈0.2 in the drop", fontweight="bold", fontsize=11)
    fig.tight_layout(); fig.savefig(out / "fig-source-vs-room.png", dpi=110); plt.close(fig)

    (out / "agc-trace-summary.json").write_text(json.dumps(summ, indent=2) + "\n")
    print(json.dumps({k: v for k, v in summ.items() if k != "host"}, indent=1))
    for name in ("recording", "source"):
        for b in ("lower", "upper"):
            h = host[name][b]
            print(name, b, "fid", round(h["fidelity_rho_vs_device_1s"], 2), list(h["rank_corr_luma_45_60"].items())[:5])


if __name__ == "__main__":
    main(Path(sys.argv[1]))
