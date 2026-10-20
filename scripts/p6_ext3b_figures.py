#!/usr/bin/env python3
"""EXT-v3b figures. Renderer pixels. One plate, two edge fields."""
from __future__ import annotations

import json
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import p6_ext  # noqa: E402

BASE = Path("/tmp/p6-ext3b")
INPUTS = Path("/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p6/inputs6/inputs.json")


def song_clock(tag: str, t_file: np.ndarray) -> np.ndarray:
    spec = next(s for s in json.loads(INPUTS.read_text())["inputs"].values() if tag in s["files"])
    return (t_file - spec["offset_s"]) / spec["scale"]


def load(tag: str, folder: str):
    pic, t_file = p6_ext.load_frames(BASE / folder / f"{tag}.frames.bin")
    return pic, song_clock(tag, t_file)


def strip(out: Path) -> None:
    runs = [("switch off", "off"), ("R01 hold", "r01"), ("R01+R03 hold", "r01r03")]
    fig, axes = plt.subplots(1, 3, figsize=(13.2, 8.4))
    loaded = []
    for label, folder in runs:
        pic, t = load("boom_source", folder)
        m = (t >= 55.45) & (t <= 57.05)
        img = np.concatenate([pic[m, 0], np.zeros((int(m.sum()), 6, 3)), pic[m, 1]], axis=1)
        loaded.append((label, img, t[m]))
    peak = max(float(np.percentile(img.ravel(), 99.5)) for _, img, _ in loaded)
    gain = 1.0 / max(peak, 1e-6)
    for ax, (label, img, t) in zip(axes, loaded):
        ax.imshow(np.clip(img * gain, 0, 1) ** 0.45, aspect="auto", interpolation="nearest",
                  extent=[0, img.shape[1], t[-1], t[0]])
        ax.axhline(55.7, color="#ffe14a", lw=1.0, ls="--")
        ax.set_title(label, fontsize=13)
        ax.set_xlabel("field A    gap    field B\nedges at the outsides, centre at 79/80", fontsize=9)
        ax.set_ylabel("song time (s)")
    fig.suptitle("BOOM file, 55.45–57.05 s. Yellow line is the drop. The hold should stay wide after it.", fontsize=13)
    fig.tight_layout()
    fig.savefig(out / "boom-drop-hold-strip.png", dpi=140)
    plt.close(fig)


def traces(out: Path) -> None:
    drops = [("boom_source", 55.7, "BOOM file 55.7 s"),
             ("boom_room", 55.7, "BOOM room 55.7 s"),
             ("sonic_source", 27.7, "Sonic 27.7 s"),
             ("sonic_source", 54.7, "Sonic 54.7 s")]
    runs = [("switch off", "off", "#5c6168", "--"),
            ("R01 hold", "r01", "#c8361f", "-"),
            ("R01+R03 hold", "r01r03", "#1d4e89", "-"),
            ("planted recede", "recede", "#6b4c9a", ":")]
    fig, axes = plt.subplots(4, 2, figsize=(12.2, 12.6))
    for row, (tag, td, title) in enumerate(drops):
        for col in (0, 1):
            ax = axes[row, col]
            for label, folder, colour, ls in runs:
                pic, t = load(tag, folder)
                y = p6_ext.display_luma(pic)[:, col]
                extent, _, _ = p6_ext.half_trace(y)
                m = (t >= td - 0.2) & (t <= td + 1.6)
                ax.plot(t[m] - td, extent[m], color=colour, lw=1.8, ls=ls, label=label)
            ax.axvline(0.0, color="black", lw=0.8, ls="--")
            ax.axhline(0.90, color="#c8361f", lw=0.8, ls=":")
            ax.axvspan(0.0, 1.0, color="#c8361f", alpha=0.06)
            ax.set_ylim(-0.02, 1.05)
            ax.set_xlim(-0.2, 1.6)
            ax.grid(alpha=0.3)
            ax.set_title(f"{title}, edge field {'A' if col == 0 else 'B'}", fontsize=11)
            if row == 3:
                ax.set_xlabel("seconds after the drop")
            if col == 0:
                ax.set_ylabel("extent (1 = lit to the centre)")
            if row == 0 and col == 1:
                ax.legend(fontsize=8, loc="lower right")
    fig.suptitle("Extent after the drop. The tinted second is the 1000 ms hold. The dotted line is 0.90.", fontsize=13)
    fig.tight_layout()
    fig.savefig(out / "extent-hold-traces.png", dpi=140)
    plt.close(fig)


def main() -> None:
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else (
        ROOT / "_scratch/waveform-migration-001/runs/P6-20261014T124826Z/permitted/figures/ext3b")
    out.mkdir(parents=True, exist_ok=True)
    strip(out)
    traces(out)
    print(out)


if __name__ == "__main__":
    main()
