#!/usr/bin/env python3
"""Drop-cue feasibility, step 1: where does each labelled drop actually land in the audio?

Labels are hand-marked to 0.1 s, which is as coarse as the whole [0, +50 ms] hit
window. Before any cue is judged, this measures the drop instant in each source
file, by band, at 2 ms resolution. The reference is zero-phase (non-causal) on
purpose: it is a ruler, not a detector.

  p6_drop_cue_probe.py reference --out DIR
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import soundfile as sf
from scipy import signal

W = Path(__file__).resolve().parents[1]
MI = W.parent / "titan-music-intelligence-001" / "tests/fixtures/tempo_field/v1/trance"
SOURCES = {
    "boom": W / "_scratch/waveform-migration-001/audio-boom-001/boom-0-75s-pcm16.wav",
    "sonic": MI / "sonic-samba.wav",
    "carte": MI / "carte-blanche.wav",
}
DROPS = [("boom", 55.7, "BOOM 55.7"), ("sonic", 27.7, "Sonic 27.7"),
         ("sonic", 54.7, "Sonic 54.7"), ("carte", 27.4, "Carte 27.4 (seen)")]
FS = 24000
BANDS = {"sub 30-110": (30, 110), "kick 40-160": (40, 160), "low-mid 150-500": (150, 500),
         "high 2k-8k": (2000, 8000)}
STEP_S = 0.002
WIN_S = 0.010


def load(path: Path) -> np.ndarray:
    x, fs = sf.read(str(path), always_2d=True)
    x = x.mean(axis=1)
    if fs != FS:
        g = np.gcd(fs, FS)
        x = signal.resample_poly(x, FS // g, fs // g)
    return x


def band_env_db(x: np.ndarray, lo: float, hi: float) -> tuple[np.ndarray, np.ndarray]:
    sos = signal.butter(4, [lo, hi], btype="band", fs=FS, output="sos")
    y = signal.sosfiltfilt(sos, x)
    hop, win = int(STEP_S * FS), int(WIN_S * FS)
    p = np.convolve(y * y, np.ones(win) / win, mode="same")[::hop]
    t = np.arange(len(p)) * STEP_S
    return t, 10 * np.log10(p + 1e-12)


SIDE_S = 0.150
SEARCH_S = 0.250


def onset(t: np.ndarray, db: np.ndarray, label: float) -> dict:
    """The sustained step nearest the label, and its half-rise instant.

    Step(t) = median level over the next 150 ms minus the median over the previous
    150 ms. A drop is a step that holds; a build pulse or a one-frame click is not.
    The step peak is taken in label ±250 ms. The onset is the first frame in the
    30 ms before that peak whose level reaches halfway from the pre to the post median.
    """
    pre_pre = (t >= label - 1.2) & (t < label - 0.3)
    side = int(round(SIDE_S / STEP_S))
    look = np.where((t >= label - SEARCH_S) & (t <= label + SEARCH_S))[0]
    steps = np.array([np.median(db[i:i + side]) - np.median(db[i - side:i]) for i in look])
    k = int(look[int(np.argmax(steps))])
    pre_m, post_m = float(np.median(db[k - side:k])), float(np.median(db[k:k + side]))
    half = pre_m + 0.5 * (post_m - pre_m)
    lo = max(0, k - int(round(0.030 / STEP_S)))
    seg = np.where(db[lo:k + side] >= half)[0]
    i0 = lo + int(seg[0]) if len(seg) else k
    return {"t": float(t[i0]), "rel_ms": 1000 * (float(t[i0]) - label),
            "step_db": float(steps.max()), "pre_db": pre_m, "post_db": post_m,
            "pre_median_db": float(np.median(db[pre_pre]))}


def reference(out: Path) -> dict:
    out.mkdir(parents=True, exist_ok=True)
    audio = {k: load(p) for k, p in SOURCES.items()}
    res = {"step_s": STEP_S, "win_s": WIN_S, "bands": BANDS, "drops": []}
    fig, axes = plt.subplots(len(DROPS), 1, figsize=(11, 3.0 * len(DROPS)))
    for ax, (src, label, name) in zip(axes, DROPS):
        row = {"name": name, "label_s": label, "bands": {}}
        for bname, (lo, hi) in BANDS.items():
            t, db = band_env_db(audio[src], lo, hi)
            row["bands"][bname] = onset(t, db, label)
            m = (t >= label - 1.0) & (t <= label + 0.4)
            ax.plot(1000 * (t[m] - label), db[m] - row["bands"][bname]["pre_median_db"],
                    lw=1, label=bname)
        ax.axvline(0, color="k", ls="--", lw=0.8)
        ax.axvspan(0, 50, color="gold", alpha=0.25)
        ax.set_title(f"{name}: band level over its pre-drop median (dB); gold = [0, +50 ms] after the label")
        ax.set_xlabel("ms from label")
        ax.set_ylim(-25, 40)
        ax.legend(fontsize=7, loc="upper left")
        res["drops"].append(row)
    fig.tight_layout()
    fig.savefig(out / "reference-onsets.png", dpi=110)
    (out / "reference-onsets.json").write_text(json.dumps(res, indent=2) + "\n")
    return res


def main() -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("reference")
    r.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    res = reference(a.out)
    for row in res["drops"]:
        print(row["name"], {b: (round(v["rel_ms"]), round(v["step_db"], 1)) for b, v in row["bands"].items()})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
