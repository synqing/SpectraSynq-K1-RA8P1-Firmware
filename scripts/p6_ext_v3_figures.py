#!/usr/bin/env python3
"""EXT-v3 figures. One plate, two edge fields. Renderer pixels, not a camera."""
from __future__ import annotations

import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import p6_ext  # noqa: E402

RUNS = [
    ("host-006", Path.home() / ".claude/jobs/c8f0d616/tmp/p6/replay-pfix/P"),
    ("R01", Path("/tmp/p6-ext3-replay/r01")),
    ("R01+R03", Path("/tmp/p6-ext3-replay/r01r03")),
]


def load(tag: str, directory: Path):
    path = directory / f"{tag}.frames.bin"
    if not path.is_file() and tag == "sonic_source":
        # Switch-off render from this same probe. BOOM file frames matched replay-pfix/P bit for bit.
        path = Path("/tmp/p6-ext3-base/sonic_source.frames.bin")
    pic, t = p6_ext.load_frames(path)
    return pic, t


def strip(out: Path) -> None:
    fig, axes = plt.subplots(1, 3, figsize=(12.6, 8.2), squeeze=False)
    loaded = []
    for label, directory in RUNS:
        pic, t = load("boom_source", directory)
        m = (t >= 50.0) & (t <= 60.0)
        # Edge field A, a gap, edge field B. Centre of each field is pixels 79/80.
        img = np.concatenate([pic[m, 0], np.zeros((int(m.sum()), 8, 3)), pic[m, 1]], axis=1)
        loaded.append((label, img, t[m]))
    peak = max(float(np.percentile(img.ravel(), 99.5)) for _, img, _ in loaded)
    gain = 1.0 / max(peak, 1e-6)
    for ax, (label, img, t) in zip(axes[0], loaded):
        ax.imshow(np.clip(img * gain, 0, 1) ** 0.45, aspect="auto", interpolation="nearest",
                  extent=[0, img.shape[1], t[-1], t[0]])
        ax.axhline(55.7, color="#ffe14a", lw=0.9, ls="--")
        ax.set_title(label, fontsize=12)
        ax.set_xlabel("edge field A  ·  gap  ·  edge field B\n0 and 159 are the edges, 79/80 the centre", fontsize=8)
        ax.set_ylabel("file time (s)")
    fig.suptitle("BOOM file, 50–60 s. Yellow line is the drop at 55.7 s. Shared gain.", fontsize=12)
    fig.tight_layout()
    fig.savefig(out, dpi=130)
    plt.close(fig)


def zoom(out: Path) -> None:
    fig, axes = plt.subplots(1, 3, figsize=(12.6, 6.4), squeeze=False)
    loaded = []
    for label, directory in RUNS:
        pic, t = load("boom_source", directory)
        m = (t >= 55.45) & (t <= 56.05)
        img = np.concatenate([pic[m, 0], np.zeros((int(m.sum()), 8, 3)), pic[m, 1]], axis=1)
        loaded.append((label, img, t[m]))
    peak = max(float(np.percentile(img.ravel(), 99.5)) for _, img, _ in loaded)
    gain = 1.0 / max(peak, 1e-6)
    for ax, (label, img, t) in zip(axes[0], loaded):
        ax.imshow(np.clip(img * gain, 0, 1) ** 0.45, aspect="auto", interpolation="nearest",
                  extent=[0, img.shape[1], t[-1], t[0]])
        ax.axhline(55.7, color="#ffe14a", lw=0.9, ls="--")
        ax.set_title(label, fontsize=12)
        ax.set_xlabel("edge field A  ·  edge field B", fontsize=8)
        ax.set_ylabel("file time (s)")
    fig.suptitle("Same strip, 55.45–56.05 s, so the release is readable.", fontsize=12)
    fig.tight_layout()
    fig.savefig(out, dpi=140)
    plt.close(fig)


def traces(out: Path) -> None:
    drops = [("boom_source", 55.7, "BOOM 55.7 s"),
             ("sonic_source", 27.7, "Sonic 27.7 s"),
             ("sonic_source", 54.7, "Sonic 54.7 s")]
    fig, axes = plt.subplots(3, 2, figsize=(11.2, 9.2), sharex=False)
    colours = {"host-006": "#5c6168", "R01": "#c8361f", "R01+R03": "#1d4e89"}
    styles = {"host-006": (1.3, "--"), "R01": (2.0, "-"), "R01+R03": (2.0, "-")}
    for row, (tag, td, title) in enumerate(drops):
        for col, field in enumerate((0, 1)):
            ax = axes[row, col]
            for label, directory in RUNS:
                pic, t = load(tag, directory)
                y = p6_ext.display_luma(pic)[:, field]
                _, i_right, _ = p6_ext.half_trace(y)
                m = (t >= td - 0.15) & (t <= td + 0.35)
                # Distance of the innermost lit pixel from the centre. 0 is the centre, 80 is the edge.
                lw, ls = styles[label]
                ax.plot(t[m] - td, (i_right[m] - 80), color=colours[label], lw=lw, ls=ls, label=label)
            ax.axvline(0.0, color="k", lw=0.8, ls="--")
            ax.axvspan(0.0, 0.15, color="#c8361f", alpha=0.08)
            ax.set_ylim(82, -2)
            ax.set_xlim(-0.15, 0.35)
            ax.grid(alpha=0.3)
            ax.set_title(f"{title} — edge field {'A' if field == 0 else 'B'}", fontsize=10)
            if row == 2:
                ax.set_xlabel("seconds after the drop")
            if col == 0:
                ax.set_ylabel("pixels from centre\n(0 = centre, 80 = edge)")
            if row == 0 and col == 1:
                ax.legend(fontsize=8, loc="lower right")
    fig.suptitle("Innermost lit pixel of the right half. Down is toward the centre. The red band is 150 ms.", fontsize=12)
    fig.tight_layout()
    fig.savefig(out, dpi=130)
    plt.close(fig)


def main() -> None:
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "_scratch/waveform-migration-001/runs/P6-20261014T124826Z/permitted/figures/ext3"
    out.mkdir(parents=True, exist_ok=True)
    strip(out / "boom-50-60s-strip.png")
    zoom(out / "boom-drop-zoom-strip.png")
    traces(out / "innermost-pixel-traces.png")
    print(out)


if __name__ == "__main__":
    main()
