#!/usr/bin/env python3
"""BOOM drop strip for the live hold. Same layout as the ext3b strip."""
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

INPUTS = Path("/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p6/inputs/inputs.json")


def song_clock(tag: str, t_file: np.ndarray) -> np.ndarray:
    spec = next(s for s in json.loads(INPUTS.read_text())["inputs"].values() if tag in s["files"])
    return (t_file - spec["offset_s"]) / spec["scale"]


def main() -> None:
    base = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("/tmp/p6-ext3c")
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else (
        ROOT / "_scratch/waveform-migration-001/runs/P6-20261014T124826Z/permitted/figures/ext3c")
    out.mkdir(parents=True, exist_ok=True)
    runs = [("switch off", base / "off"), ("R01 hold", base / "r01"), ("R01+R03 hold", base / "r01r03")]
    fig, axes = plt.subplots(1, 3, figsize=(13.2, 8.4))
    loaded = []
    for label, folder in runs:
        pic, t_file = p6_ext.load_frames(folder / "boom_source.frames.bin")
        t = song_clock("boom_source", t_file)
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
    print(out / "boom-drop-hold-strip.png")


if __name__ == "__main__":
    main()
