#!/usr/bin/env python3
"""Lane P6 extent figures (TOLERANCE-FROZEN-P6-EXT-v2.json).

  p6_ext_figures.py extent --inputs INPUTS --out PNG LABEL=REPLAYDIR=EXTRESULT ...
      extent(t) per board and tuning case across early build, climax and drop; audio drop onset marked
  p6_ext_figures.py strip  --out PNG --t-d 55.7 --offset 0 LABEL=FRAMES.bin ...
      frame strip (space x time) of both boards, drop -1 s .. +1 s, renderer output, shared gain
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

RUN = Path(__file__).resolve().parents[1] / "_scratch/waveform-migration-001/runs/P6-20261014T124826Z/permitted"
COLOURS = ["#8a8f98", "#3b6fb6", "#c8361f", "#e08a1e", "#6b2fa3"]


def extent_figure(inputs: Path, runs: list[tuple[str, Path, Path]], out: Path) -> None:
    frozen = json.loads((RUN / "TOLERANCE-FROZEN-P6-EXT-v2.json").read_text())
    cases = frozen["cases"]["tuning"]
    manifest = json.loads((inputs / "inputs.json").read_text())
    fig, axes = plt.subplots(2, len(cases), figsize=(5.2 * len(cases), 7), squeeze=False)
    for col, (key, c) in enumerate(cases.items()):
        tag = key.split("@")[0]
        spec = next(s for s in manifest["inputs"].values() if tag in s["files"])
        for k, (label, d, res) in enumerate(runs):
            x = pd.read_csv(d / f"{tag}.csv")
            x["t"] = (x.t_s - spec["offset_s"]) / spec["scale"]
            r = json.loads(res.read_text())
            for board in (0, 1):
                y = x[(x.board == board) & (x.t >= c["early"][0] - 0.5) & (x.t <= c["t_d"] + 2.5)]
                row = next(q for q in r["rows"] if q["case"] == key and q["board"] == board)
                t90 = "inf" if row["T90"] is None or row["T90"] == float("inf") else f"{row['T90']:.2f}s"
                axes[board, col].plot(y.t, y.extent_edge, color=COLOURS[k % len(COLOURS)], lw=1.0,
                                      label=f"{label}: T90 {t90}, J {row['jump']:.2f}, {'PASS' if row['pass'] else 'fail'}")
        for board in (0, 1):
            ax = axes[board, col]
            ax.axvspan(*c["early"], color="#2f8f5b", alpha=0.12)
            ax.axvspan(*c["climax"], color="#f2c14e", alpha=0.25)
            ax.axvline(c["t_d"], color="k", lw=1.2, ls="--")
            ax.axvspan(c["t_d"], c["t_d"] + 0.15, color="#c8361f", alpha=0.12)
            ax.set_ylim(-0.02, 1.05)
            ax.set_title(f"{'lower (mode 32)' if board == 0 else 'upper (mode 18)'} — {key}", fontsize=9.5)
            ax.set_ylabel("extent: edge to centre (1 = full)")
            ax.grid(alpha=0.25)
            ax.legend(fontsize=7, loc="lower left")
            ax.set_xlabel("song time (s); green early build, yellow climax, dashed audio drop onset, red 150 ms", fontsize=8)
    fig.suptitle("P6 EXT: waveform extent (edge to centre) across build, climax and drop", fontsize=12)
    fig.tight_layout()
    fig.savefig(out, dpi=115)
    plt.close(fig)


def strip_figure(runs: list[tuple[str, Path]], t_d: float, offset: float, out: Path, half: float = 1.0,
                 mark: float | None = None) -> None:
    fig, axes = plt.subplots(1, len(runs), figsize=(4.2 * len(runs), 7.5), squeeze=False)
    imgs = []
    for label, frames in runs:
        a = np.fromfile(frames, dtype="<u2").reshape(-1, 2, 160, 3).astype(np.float64) / 65535.0
        t = (np.arange(a.shape[0]) + 1) * 0.01 - offset  # probe tick k is at (k+1) x 10 ms
        m = (t >= t_d - half) & (t <= t_d + half)
        imgs.append((label, np.concatenate([a[m, 0], np.zeros((m.sum(), 6, 3)), a[m, 1]], axis=1), t[m]))
    gain = 1.0 / max(np.percentile(np.concatenate([i[1].ravel() for i in imgs]), 99.5), 1e-6)
    for ax, (label, img, t) in zip(axes[0], imgs):
        ax.imshow(np.clip(img * gain, 0, 1) ** 0.5, aspect="auto", interpolation="nearest",
                  extent=[0, img.shape[1], t[-1], t[0]])
        ax.axhline(t_d if mark is None else mark, color="w", lw=0.8, ls="--")
        ax.set_title(label, fontsize=10)
        ax.set_xlabel("pixels: lower 0-159 | upper 0-159\n(edges at 0/159, centre 79/80)", fontsize=8.5)
        ax.set_ylabel("song time (s); dashed = audio drop onset")
    fig.suptitle(f"Frame strips, {t_d - half:.1f}-{t_d + half:.1f} s (renderer output, one shared gain, gamma 0.5 for display)", fontsize=11)
    fig.tight_layout()
    fig.savefig(out, dpi=115)
    plt.close(fig)


def main() -> None:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    e = sub.add_parser("extent"); e.add_argument("--inputs", type=Path, required=True); e.add_argument("--out", type=Path, required=True)
    e.add_argument("runs", nargs="+")
    s = sub.add_parser("strip"); s.add_argument("--out", type=Path, required=True); s.add_argument("--t-d", type=float, required=True)
    s.add_argument("--offset", type=float, default=0.0); s.add_argument("--half", type=float, default=1.0)
    s.add_argument("--mark", type=float, help="time of the dashed drop-onset line (default: --t-d)")
    s.add_argument("runs", nargs="+")
    a = ap.parse_args()
    if a.cmd == "extent":
        extent_figure(a.inputs, [(l, Path(d), Path(r)) for l, d, r in (x.split("=", 2) for x in a.runs)], a.out)
    else:
        strip_figure([(l, Path(f)) for l, f in (x.split("=", 1) for x in a.runs)], a.t_d, a.offset, a.out, a.half, a.mark)


if __name__ == "__main__":
    main()
