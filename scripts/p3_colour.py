#!/usr/bin/env python3
"""Lane P3 colour: what each board actually rendered, pixel by pixel, over each song.

Reads the wire snapshots of the three 14 Oct recordings (GRB48 payload, bytes 544-1504,
160 payload pixels per board, big-endian G,R,B) and draws a space-time image per board per
song: x = time from music start, y = payload pixel index, colour = the rendered RGB with
ONE display gain and gamma shared by every song and board (so songs stay comparable).
Writes analysis/fig-colour.png and analysis/colour.json (circular mean hue, concentration,
lit fraction, mean RGB in thousandths of full scale). Offline, read-only.
"""
from __future__ import annotations

import colorsys
import json
import struct
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
CAP = ROOT / "_scratch/waveform-migration-001/canonical-audio-v01"
RECS = [("boom", "boom-admission-fix-candidate-001", "BOOM 0–75 s"),
        ("sonic", "sonic-6-30-admission-fix-candidate-001", "Sonic Samba 6–30 s"),
        ("dopex", "dopex-admission-fix-holdout-001", "Dopex 0–75 s (holdout)")]
BG, FG = "#0f1115", "#e8e6e1"
GAMMA = 0.5


def decode(path: Path):
    b = path.read_bytes()
    if len(b) < 1504:
        return None
    v = np.array(struct.unpack(">480H", b[544:1504]), dtype=np.float64).reshape(160, 3)
    return v[:, [1, 0, 2]] / 65535.0


def load(dirname):
    d = CAP / dirname
    ev = json.loads((d / "events.json").read_text())
    ap = next(e for e in ev if e["event"] == "audio_process_started")
    t0 = ap.get("started", ap["mono"])
    dur = json.loads((d / "observation.json").read_text())["audio_window"]["duration_s"]
    out = {"lower": [], "upper": [], "dur": dur}
    for sd in sorted(d.glob("sample-*")):
        for name, stem in (("lower", "wire-snapshot"), ("upper", "wire-snapshot-top")):
            try:
                t = json.loads((sd / f"{stem}.json").read_text())["mono"] - t0
                px = decode(sd / f"{stem}.bin")
            except (OSError, ValueError, KeyError):
                continue
            if px is not None:
                out[name].append((t, px))
    for name in ("lower", "upper"):
        out[name].sort(key=lambda x: x[0])
    return out


def stats(frames, dur):
    rgb = np.array([px.mean(axis=0) for t, px in frames if 0 <= t <= dur])
    hues = [colorsys.rgb_to_hsv(*(c / c.max()))[0] for c in rgb if c.max() > 0]
    z = np.exp(1j * 2 * np.pi * np.array(hues))
    lum = np.array([px @ [0.2126, 0.7152, 0.0722] for t, px in frames if 0 <= t <= dur])
    return {"frames": len(rgb), "mean_rgb_permille": (rgb.mean(axis=0) * 1000).round(3).tolist(),
            "hue_deg": float((np.angle(z.mean()) / (2 * np.pi) % 1) * 360), "hue_concentration": float(abs(z.mean())),
            "lit_fraction": float((lum > 0.01).mean())}


def main(out: Path):
    data = {k: load(d) for k, d, _ in RECS}
    allv = np.concatenate([px.ravel() for k in data for n in ("lower", "upper") for _, px in data[k][n]])
    gain = 1.0 / np.percentile(allv[allv > 0], 99.5)
    res = {"display_gain": float(gain), "gamma": GAMMA, "songs": {}}
    fig, axes = plt.subplots(3, 2, figsize=(11, 7.6), facecolor=BG)
    for i, (k, _, label) in enumerate(RECS):
        res["songs"][k] = {}
        for j, name in enumerate(("lower", "upper")):
            fr = data[k][name]
            ts = np.array([t for t, _ in fr])
            edges = np.concatenate([[ts[0] - 0.15], (ts[1:] + ts[:-1]) / 2, [ts[-1] + 0.15]])
            img = np.stack([px for _, px in fr], axis=1)  # 160 x N x 3
            img = np.clip(img * gain, 0, 1) ** GAMMA
            ax = axes[i, j]
            ax.set_facecolor(BG)
            for c in range(len(ts)):
                ax.imshow(img[:, c:c + 1, :], extent=(edges[c], edges[c + 1], 159.5, -0.5), aspect="auto",
                          interpolation="nearest")
            ax.set_xlim(edges[0], edges[-1]); ax.set_ylim(159.5, -0.5)
            dur = data[k]["dur"]
            ax.axvline(0, color=FG, lw=0.6, ls=":"); ax.axvline(dur, color=FG, lw=0.6, ls=":")
            st = stats(fr, dur)
            res["songs"][k][name] = st
            ax.set_title(f"{label} — {name} board · hue {st['hue_deg']:.0f}°, lit {st['lit_fraction']*100:.0f}%",
                         color=FG, fontsize=10)
            ax.tick_params(colors=FG, labelsize=8)
            for s in ax.spines.values():
                s.set_color("#2a2e36")
            if j == 0:
                ax.set_ylabel("payload pixel", color=FG, fontsize=9)
            if i == 2:
                ax.set_xlabel("time from music start (s) · dotted = music start / end", color=FG, fontsize=9)
    fig.suptitle("What the boards rendered — every pixel over time, from the wire (same display gain for all)",
                 color=FG, fontweight="bold")
    fig.tight_layout()
    fig.savefig(out / "fig-colour.png", dpi=110, facecolor=BG)
    (out / "colour.json").write_text(json.dumps(res, indent=2) + "\n")
    print(json.dumps(res, indent=1))


if __name__ == "__main__":
    main(Path(sys.argv[1]))
