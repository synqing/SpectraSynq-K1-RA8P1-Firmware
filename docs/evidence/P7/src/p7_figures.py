#!/usr/bin/env python3
"""Lane P7 figures: (1) BOOM tracker output over time, product vs host-only variants; (2) baseline gate map
for the four-on-the-floor sweep (reported locked BPM against truth)."""
from __future__ import annotations

import json
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
import pandas as pd  # noqa: E402

RUN = Path(__file__).resolve().parents[1]


def fig_boom():
    fig, axes = plt.subplots(2, 1, figsize=(12, 7), sharex=True)
    for ax, name, title in ((axes[0], "boom_file", "BOOM song file (device-calibrated gain)"),
                            (axes[1], "boom_room", "BOOM room recording (iPhone, device-calibrated gain)")):
        d = pd.read_csv(RUN / "diagnosis/csv" / f"{name}.csv.gz")
        for v, col, lw in (("baseline", "#c0392b", 2.2), ("prior_flat", "#2c7fb8", 1.4), ("pcm_kick", "#7f7f7f", 1.0)):
            g = d[d.variant == v]
            label = {"baseline": "product (prior 88 BPM)", "prior_flat": "flat prior (host-only switch)",
                     "pcm_kick": "kick-band novelty, product prior (host-only switch)"}[v]
            ax.plot(g.t_s, g.bpm, color=col, lw=lw, label=label, alpha=0.9)
        for y, lab in ((138, "138"), (92, "92 = 2/3"), (69, "69 = 1/2")):
            ax.axhline(y, color="k", lw=0.6, ls=":")
            ax.text(0.5, y + 1.5, lab, fontsize=8)
        for x0, x1, lab in ((28, 45, "build"), (45, 55.5, "cut"), (55.5, 75, "drop")):
            ax.axvspan(x0, x1, color="#eeeeee" if lab != "drop" else "#fde0dd", zorder=0)
            ax.text((x0 + x1) / 2, 158, lab, ha="center", fontsize=8)
        ax.axvline(11.52, color="#555", lw=0.8, ls="--")
        ax.text(11.7, 52, "lock warm-up ends (512 updates)", fontsize=7, color="#555")
        ax.set_ylim(50, 165)
        ax.set_ylabel("reported BPM")
        ax.set_title(title, fontsize=10)
    axes[0].legend(loc="lower right", fontsize=8)
    axes[1].set_xlabel("song time (s)")
    fig.suptitle("P7 step 1 (host, 24 kHz): the 88 BPM prior moves the lock; the novelty change alone does not",
                 fontsize=11)
    fig.tight_layout()
    fig.savefig(RUN / "diagnosis/fig-boom-variants.png", dpi=110)


def fig_gate():
    g = json.loads((RUN / "gate/GATE-SYNTHETIC-v1.json").read_text())
    fig, axes = plt.subplots(1, 2, figsize=(12, 5), sharey=True)
    for ax, fam, title in ((axes[0], "four_on_the_floor", "four-on-the-floor"),
                           (axes[1], "four_on_the_floor_332", "four-on-the-floor + 3+3+2 stab")):
        xs, ys, cs = [], [], []
        for c in g["cases"]:
            if c["family"] != fam:
                continue
            d = pd.read_csv(RUN / "baseline/traces" / f"{c['id']}.csv")
            w = d[(d.t_s >= 13) & (d.t_s < 31)]
            lk = w[w.locked == 1]
            ys.append(lk.bpm.median() if len(lk) else w.bpm.median())
            xs.append(c["bpm"])
            cs.append("#2c7fb8" if len(lk) else "#bbbbbb")
        ax.scatter(xs, ys, c=cs, s=10)
        t = np.arange(55, 201)
        for r, lab in ((1, "truth"), (0.5, "x1/2"), (2, "x2"), (2 / 3, "x2/3")):
            ax.plot(t, r * t, lw=0.7, ls="-" if r == 1 else ":", color="k")
            ax.text(200, min(r * 200, 205), lab, fontsize=8, ha="right")
        ax.axhspan(60, 155, color="#f5f5f5", zorder=0)
        ax.set_title(f"{title}: median reported BPM 12-30 s after start", fontsize=10)
        ax.set_xlabel("annotated BPM (kick)")
        ax.set_ylim(40, 210)
    axes[0].set_ylabel("reported BPM (blue: locked; grey: never locked)")
    fig.suptitle("P7 synthetic gate, current tracker: 0 of 326 cases pass (shaded: tracker range 60-155)", fontsize=11)
    fig.tight_layout()
    fig.savefig(RUN / "baseline/fig-baseline-fotf.png", dpi=110)


if __name__ == "__main__":
    fig_boom()
    fig_gate()
    sys.exit(0)
