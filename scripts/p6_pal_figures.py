#!/usr/bin/env python3
"""Lane P6 iteration P: palette-fidelity scatter (TOLERANCE-FROZEN-P6-PAL.json (a)).

  p6_pal_figures.py --out PNG --palette-id 31 --board 0 --t0 40 --t1 65 LABEL=FRAMES.bin ...

Each lit renderer pixel (max channel >= 0.01) of the chosen board is plotted by its max-normalised
green and blue against the palette's max-normalised HD stop curve; off-palette pixels (L-inf > 0.03)
are drawn red. The curve is computed from the product palette stops (no renderer code).
"""
import argparse
import subprocess
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parents[1]


def palette_curve(pid: int) -> np.ndarray:
    src = ROOT / "src/k1/core/visual/product_palette_data.cpp"
    exe = Path("/tmp") / f"p6_palstops_{pid}"
    code = f'#include "core/visual/product_palette.h"\n#include <cstdio>\nint main(){{auto&p=k1::core::visual::productPalette({pid});for(unsigned i=0;i<p.stop_count;++i)std::printf("%u %u %u %u\\n",p.stops[i].position,p.stops[i].colour.red,p.stops[i].colour.green,p.stops[i].colour.blue);}}'
    cpp = exe.with_suffix(".cpp")
    cpp.write_text(code)
    subprocess.run(["c++", "-std=c++17", "-I", str(ROOT / "src/k1"), "-I", str(ROOT / "src"), str(cpp),
                    str(ROOT / "src/k1/core/visual/product_palette.cpp"), str(src), "-o", str(exe)], check=True)
    stops = np.array([[float(v) for v in l.split()] for l in subprocess.run([str(exe)], capture_output=True, text=True, check=True).stdout.splitlines()])
    x = np.linspace(0, 1, 4097)
    rgb = np.stack([np.interp(x, stops[:, 0] / 255.0, stops[:, k]) for k in (1, 2, 3)], axis=1)
    m = rgb.max(1)
    keep = m > 0.5
    return rgb[keep] / m[keep, None]


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, required=True); ap.add_argument("--palette-id", type=int, required=True)
    ap.add_argument("--board", type=int, default=0); ap.add_argument("--t0", type=float, default=40.0)
    ap.add_argument("--t1", type=float, default=65.0); ap.add_argument("runs", nargs="+")
    a = ap.parse_args()
    curve = palette_curve(a.palette_id)
    fig, axes = plt.subplots(1, len(a.runs), figsize=(5 * len(a.runs), 5), squeeze=False)
    for ax, run in zip(axes[0], a.runs):
        label, f = run.split("=", 1)
        fr = np.fromfile(f, dtype="<u2").reshape(-1, 2, 160, 3).astype(np.float64) / 65535.0
        t = (np.arange(fr.shape[0]) + 1) * 0.01
        px = fr[(t >= a.t0) & (t <= a.t1), a.board].reshape(-1, 3)
        m = px.max(1)
        px = px[m >= 0.01] / m[m >= 0.01, None]
        idx = np.random.default_rng(0).choice(len(px), size=min(len(px), 20000), replace=False)
        px = px[idx]
        dist = np.array([np.abs(curve - p).max(1).min() for p in px])
        ax.plot(curve[:, 1], curve[:, 2], color="k", lw=2, label="palette HD curve (G/max, B/max)")
        ok = dist <= 0.03
        ax.scatter(px[ok, 1], px[ok, 2], s=2, alpha=0.3, color="#3b6fb6", label=f"on palette ({ok.mean():.1%})")
        ax.scatter(px[~ok, 1], px[~ok, 2], s=3, alpha=0.6, color="#c8361f", label=f"off palette ({(~ok).mean():.1%})")
        ax.set_title(label, fontsize=10)
        ax.set_xlabel("green / max channel"); ax.set_ylabel("blue / max channel")
        ax.set_xlim(-0.05, 1.05); ax.set_ylim(-0.05, 1.05); ax.grid(alpha=0.25); ax.legend(fontsize=8, loc="upper right")
    fig.suptitle(f"Palette fidelity, board {a.board} (palette id {a.palette_id}), {a.t0:.0f}-{a.t1:.0f} s: lit pixels against the HD curve", fontsize=11)
    fig.tight_layout()
    fig.savefig(a.out, dpi=115)
    plt.close(fig)


if __name__ == "__main__":
    sys.exit(main())
