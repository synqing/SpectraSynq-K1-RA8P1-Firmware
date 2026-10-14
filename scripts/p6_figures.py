#!/usr/bin/env python3
"""Lane P6 figures and the post-hoc light/colour split (not a frozen criterion).

  p6_figures.py --inputs INPUTS --v8 V8DIR --v9 V9DIR --eval8 E8 --eval9 E9 --out DIR

fig-boom-build-drop.png  per board x BOOM input: Rec.709 luma L and light S against time, v8 vs v9
fig-song-order.png       per board: median L and S against input loudness, 3 songs x (-6, 0, +6 dB)
fig-v9-colour-split.png  v9 BOOM file: intensity field, palette coordinate and S through build/cut/drop
posthoc-intensity.json   C1/C2 arithmetic repeated on v9's intensity field (light before palette colour)
"""
from __future__ import annotations

import argparse
import itertools
import json
import math
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

V8C, V9C = "#8a8f98", "#c8361f"


def load(d: Path, tag: str, spec: dict) -> pd.DataFrame:
    x = pd.read_csv(d / f"{tag}.csv")
    x["t_song"] = (x.t_s - spec["offset_s"]) / spec["scale"]
    x["zeros_s"] = x.t_s - spec["file_seconds"]
    return x


def smooth(t, v, s=0.25):
    k = max(1, int(round(s / 0.01)))
    return np.convolve(v, np.ones(k) / k, mode="valid"), t[k // 2: k // 2 + len(v) - k + 1]


def spec_of(manifest, tag):
    return next(s for s in manifest["inputs"].values() if tag in s["files"])


def boom_figure(manifest, v8, v9, out):
    fig, axes = plt.subplots(2, 2, figsize=(13, 7.5), sharex=True)
    for col, tag in enumerate(("boom_source", "boom_room")):
        sp = spec_of(manifest, tag)
        a, b = load(v8, tag, sp), load(v9, tag, sp)
        for row, board in enumerate((0, 1)):
            ax = axes[row, col]
            for d, c, name in ((a, V8C, "v8"), (b, V9C, "v9")):
                x = d[(d.board == board) & (d.t_song >= 40) & (d.t_song <= 65)]
                for metric, ls in (("L", "-"), ("S", "--")):
                    v, t = smooth(x.t_song.to_numpy(), x[metric].to_numpy() * 1000)
                    ax.plot(t, v, ls, color=c, lw=1.4 if metric == "L" else 1.0, label=f"{name} {'luma L' if metric == 'L' else 'light S'}")
            ax.axvspan(45, 50, color="#f2c14e", alpha=0.18)
            ax.axvspan(55.5, 60, color="#3b6fb6", alpha=0.12)
            ax.set_title(f"{'Lower board, Hybrid K1 (mode 32)' if board == 0 else 'Upper board, Tempo (mode 18)'} — "
                         f"{'song file' if tag == 'boom_source' else 'room recording'}", fontsize=10)
            ax.set_ylabel("per-mille of full scale (0.25 s mean)")
            ax.grid(alpha=0.25)
    for ax in axes[1]:
        ax.set_xlabel("BOOM song time (s)   yellow = build 45–50 s, blue = drop 55.5–60 s")
    axes[0, 0].legend(fontsize=8, ncol=2, loc="upper left")
    fig.suptitle("P6 host replay, BOOM build to drop: v8 (grey) against v9 (red), at matched average light", fontsize=12)
    fig.tight_layout()
    fig.savefig(out / "fig-boom-build-drop.png", dpi=130)
    plt.close(fig)


def order_figure(e8, e9, out):
    fig, axes = plt.subplots(2, 2, figsize=(12, 7.5), sharex=True)
    colours = {"boom_room": "#c8361f", "sonic_room": "#2f8f5b", "dopex_room": "#3b6fb6"}
    for col, (name, ev) in enumerate((("v8", e8), ("v9", e9))):
        cond = ev["C2"]["conditions"]
        for row, board in enumerate((0, 1)):
            ax = axes[row, col]
            for song, c in colours.items():
                pts = sorted((v["loudness_db"], v["L"][board] * 1000, v["S"][board] * 1000) for k, v in cond.items() if v["song"] == song)
                xs, ls, ss = zip(*pts)
                ax.plot(xs, ls, "-o", color=c, label=f"{song.split('_')[0]} luma L")
                ax.plot(xs, ss, ":s", color=c, alpha=0.7, label=f"{song.split('_')[0]} light S")
            b = ev["C2"]["boards"][board]
            ax.set_title(f"{name} — {'lower board (mode 32)' if board == 0 else 'upper board (mode 18)'}: same-song "
                         f"{b['same_song_concordant']}/{b['same_song_pairs']}, cross-song {b['cross_song_concordant']}/{b['cross_song_pairs']} in order", fontsize=9.5)
            ax.set_ylabel("median per-mille (Q4 window)")
            ax.grid(alpha=0.25)
    for ax in axes[1]:
        ax.set_xlabel("input loudness: host in-house raw RMS median (dBFS); each song at -6, 0, +6 dB")
    axes[0, 0].legend(fontsize=7.5, ncol=3)
    fig.suptitle("P6 song against song: does more loudness give more light? (frozen test uses light S)", fontsize=12)
    fig.tight_layout()
    fig.savefig(out / "fig-song-order.png", dpi=130)
    plt.close(fig)


def split_figure(manifest, v9, out):
    sp = spec_of(manifest, "boom_source")
    d = load(v9, "boom_source", sp)
    fig, axes = plt.subplots(3, 1, figsize=(12, 8), sharex=True)
    for board, c in ((0, "#c8361f"), (1, "#3b6fb6")):
        x = d[(d.board == board) & (d.t_song >= 40) & (d.t_song <= 65)]
        for ax, col, lab, k in ((axes[0], "intensity_mean", "intensity field mean (light before colour)", 1),
                                (axes[1], "palette_coordinate", "palette coordinate (colour layer)", 1),
                                (axes[2], "S", "light S per-mille (after palette colour)", 1000)):
            v, t = smooth(x.t_song.to_numpy(), x[col].to_numpy() * k)
            ax.plot(t, v, color=c, label=f"{'lower (mode 32, palette 32)' if board == 0 else 'upper (mode 18, palette 39)'}")
            ax.set_ylabel(lab, fontsize=8.5)
    x = d[(d.board == 0) & (d.t_song >= 40) & (d.t_song <= 65)]
    v, t = smooth(x.t_song.to_numpy(), x.tonality_gate.to_numpy())
    axes[1].plot(t, v, color="#666", lw=0.8, label="tonality gate (0 closed, 1 open)")
    for ax in axes:
        ax.axvspan(45, 50, color="#f2c14e", alpha=0.18)
        ax.axvspan(50, 55, color="#999", alpha=0.10)
        ax.axvspan(55.5, 60, color="#3b6fb6", alpha=0.12)
        ax.grid(alpha=0.25)
        ax.legend(fontsize=8, loc="upper left")
    axes[2].set_xlabel("BOOM song time (s), song file   yellow = build, grey = quiet cut, blue = drop")
    fig.suptitle("v9 on the BOOM file: intensity field, colour layer (with the tonality gate) and light through build, quiet cut and drop", fontsize=11)
    fig.tight_layout()
    fig.savefig(out / "fig-v9-colour-split.png", dpi=130)
    plt.close(fig)


def posthoc(manifest, v9, out):
    res = {"label": "POST HOC, not a frozen criterion: C1/C2 arithmetic on v9's intensity field mean (light at one fixed palette colour)",
           "C1": [], "C2": []}
    for tag in ("boom_source", "boom_room"):
        d = load(v9, tag, spec_of(manifest, tag))
        for board in (0, 1):
            w = lambda lo, hi: d[(d.board == board) & (d.t_song >= lo) & (d.t_song <= hi) & (d.zeros_s < 0)]
            b, q = w(45, 50), w(55.5, 60)
            res["C1"].append({"input": tag, "board": board, "intensity_ratio": q.intensity_mean.median() / b.intensity_mean.median(),
                              "palette_coordinate_build": b.palette_coordinate.median(), "palette_coordinate_drop": q.palette_coordinate.median()})
    cond = {}
    for song in ("boom_room", "sonic_room", "dopex_room"):
        for suffix in ("-6dB", "", "+6dB"):
            tag = song + suffix
            sp = spec_of(manifest, tag)
            d = load(v9, tag, sp)
            lo, hi = sp["window_song_s"]
            x = d[(d.t_song >= lo) & (d.t_song <= hi) & (d.zeros_s < 0)]
            cond[tag] = {"song": song, "loudness_db": 20 * math.log10(x[x.board == 0].raw_rms.median()),
                         "I": [x[x.board == b].intensity_mean.median() for b in (0, 1)]}
    for board in (0, 1):
        same = same_ok = cross = cross_ok = 0
        for a, b in itertools.combinations(cond, 2):
            A, B = cond[a], cond[b]
            dl = B["loudness_db"] - A["loudness_db"]
            if abs(dl) < 3.0:
                continue
            good = int(np.sign(B["I"][board] - A["I"][board]) == np.sign(dl))
            if A["song"] == B["song"]:
                same += 1; same_ok += good
            else:
                cross += 1; cross_ok += good
        res["C2"].append({"board": board, "same_song": f"{same_ok}/{same}", "cross_song": f"{cross_ok}/{cross}"})
    res["conditions"] = cond
    (out / "posthoc-intensity.json").write_text(json.dumps(res, indent=2, default=float) + "\n")
    return res


def compare_figure(manifest, runs, out):
    """Iteration 4: BOOM build->drop, light S and luma L per board and input, one line per profile."""
    colours = {"v8": "#8a8f98", "v9 it3 (palettes as is)": "#c8361f", "v9 A lifted low end": "#e08a1e",
               "v9 B restricted range": "#3b6fb6"}
    fig, axes = plt.subplots(4, 2, figsize=(13, 12), sharex=True)
    for col, tag in enumerate(("boom_source", "boom_room")):
        sp = spec_of(manifest, tag)
        for name, d in runs.items():
            x = load(d, tag, sp)
            for k, (board, metric) in enumerate(((0, "S"), (1, "S"), (0, "L"), (1, "L"))):
                y = x[(x.board == board) & (x.t_song >= 40) & (x.t_song <= 65)]
                v, t = smooth(y.t_song.to_numpy(), y[metric].to_numpy() * 1000)
                axes[k, col].plot(t, v, color=colours.get(name, "k"), lw=1.2, label=name)
        for k, (board, metric) in enumerate(((0, "S"), (1, "S"), (0, "L"), (1, "L"))):
            ax = axes[k, col]
            ax.axvspan(45, 50, color="#f2c14e", alpha=0.18)
            ax.axvspan(55.5, 60, color="#3b6fb6", alpha=0.10)
            ax.set_title(f"{'lower (mode 32)' if board == 0 else 'upper (mode 18)'} — "
                         f"{'light S' if metric == 'S' else 'Rec.709 luma L'} — {'song file' if tag == 'boom_source' else 'room recording'}",
                         fontsize=9.5)
            ax.set_ylabel("per-mille", fontsize=8)
            ax.grid(alpha=0.25)
    axes[0, 0].legend(fontsize=8, loc="upper left")
    for ax in axes[3]:
        ax.set_xlabel("BOOM song time (s)   yellow = build 45–50 s, blue = drop 55.5–60 s")
    fig.suptitle("P6 iteration 4: BOOM build to drop at matched average light — v8, v9 it3, variant A, variant B", fontsize=12)
    fig.tight_layout()
    fig.savefig(out / "fig-boom-build-drop-4way.png", dpi=120)
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    for k in ("--inputs", "--v8", "--v9", "--eval8", "--eval9", "--out"):
        ap.add_argument(k, type=Path, required=True)
    ap.add_argument("--compare", nargs="*", default=[], help="NAME=DIR pairs for the iteration-4 4-way figure")
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((a.inputs / "inputs.json").read_text())
    if a.compare:
        compare_figure(manifest, {k: Path(v) for k, v in (c.split("=", 1) for c in a.compare)}, a.out)
    boom_figure(manifest, a.v8, a.v9, a.out)
    order_figure(json.loads(a.eval8.read_text()), json.loads(a.eval9.read_text()), a.out)
    split_figure(manifest, a.v9, a.out)
    print(json.dumps(posthoc(manifest, a.v9, a.out)["C1"], default=float))


if __name__ == "__main__":
    main()
