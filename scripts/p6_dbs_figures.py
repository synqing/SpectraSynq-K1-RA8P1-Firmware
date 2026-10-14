#!/usr/bin/env python3
"""DBS pilot comparison figure: board-0 deposit A_k per arm with births and salient reference
onsets, and the final board-0 payload as a space-time strip, BOOM source 60-62 s (dense post-drop).
uv run --quiet --with pandas==2.2.3 --with numpy==2.1.3 --with matplotlib==3.9.2 python scripts/p6_dbs_figures.py"""
import json, sys
from pathlib import Path
import numpy as np, pandas as pd, matplotlib
matplotlib.use("Agg"); import matplotlib.pyplot as plt
sys.path.insert(0, str(Path(__file__).resolve().parent))
import p6_dbs_score as sc
RUN = sc.RUNDIR; OUT = sc.ROOT / "docs/evidence/P6/dbs/comparison"
T0, T1 = 60.0, 62.0
salient, ambiguous = sc.boom_reference()
missed = set(json.loads((RUN / "missed-set-boom_source.json").read_text())["missed_onset_times_s"])
arms = [("arm0", "baseline (off)"), ("arm1", "existing onset, s=0.25"), ("arm2", "full-bank raw, s=0.25"),
        ("arm3", "static 16 groups, s=0.25"), ("arm4", "adaptive 16 groups, s=0.25")]
fig, ax = plt.subplots(len(arms), 1, figsize=(15, 13), sharex=True)
for a, (tag, title) in zip(ax, arms):
    t = sc.board0(RUN, "boom_source", tag); tt = sc.song_time(t, "boom_source")
    b = sc.births(t, tt); m = (tt >= T0) & (tt < T1)
    a.step(tt[m], t.deposit_peak[m], where="post", color="#4b2a7a", lw=1.2, label="deposit amplitude A_k")
    bb = b[(b.t >= T0) & (b.t < T1)]
    a.scatter(bb.t, bb.amp + 0.04, marker="v", color="#2f6b3a", s=30, zorder=3, label="birth (prominence >= 1.20)")
    for o in salient.time_s[(salient.time_s >= T0) & (salient.time_s < T1)]:
        a.axvline(o, color="#c8361f" if o in missed else "#888", lw=1.0, ls="-" if o in missed else ":", alpha=.8)
    a.set_ylim(0, 0.7); a.set_ylabel("A_k"); a.set_title(title, loc="left", fontsize=11); a.grid(alpha=.25)
ax[0].plot([], [], color="#c8361f", label="salient onset in predeclared missed set"); ax[0].plot([], [], color="#888", ls=":", label="other salient onset")
ax[0].legend(ncol=4, fontsize=8, loc="upper right"); ax[-1].set_xlabel("BOOM song time (s)")
fig.suptitle("Lower Hybrid (board 0) deposit per frame, BOOM source 60-62 s — host replay, product v9 path", fontsize=13)
fig.tight_layout(); fig.savefig(OUT / "deposit-60-62s.png", dpi=110); plt.close(fig)
fig, ax = plt.subplots(1, 4, figsize=(17, 7), sharey=True)
lum = {}
for tag in ["arm0", "arm1", "arm4"]:
    f = np.fromfile(RUN / "arms/boom_source" / f"{tag}.frames.bin", dtype="<u2").reshape(-1, 2, 160, 3)[:, 0].astype(float) / 65535.0
    t = sc.board0(RUN, "boom_source", tag); tt = sc.song_time(t, "boom_source"); m = np.where((tt >= T0) & (tt < T1))[0]
    lum[tag] = (0.2126 * f[m][:, 80:, 0] + 0.7152 * f[m][:, 80:, 1] + 0.0722 * f[m][:, 80:, 2])
vmax = max(v.max() for v in lum.values())
for a, tag, title in zip(ax[:3], ["arm0", "arm1", "arm4"], ["baseline (off)", "existing onset", "adaptive 16 groups"]):
    im = a.imshow(lum[tag], aspect="auto", extent=[0, 80, T1, T0], interpolation="nearest", cmap="magma", vmin=0, vmax=vmax)
    a.set_title(title + " — payload luma"); a.set_xlabel("pixel from centre (right half)")
d = lum["arm4"] - lum["arm0"]; lim = np.abs(d).max()
ax[3].imshow(d, aspect="auto", extent=[0, 80, T1, T0], interpolation="nearest", cmap="RdBu_r", vmin=-lim, vmax=lim)
ax[3].set_title(f"adaptive minus baseline (max |diff| {lim:.3f})"); ax[3].set_xlabel("pixel from centre (right half)")
ax[0].set_ylabel("song time (s)")
fig.suptitle("Final board-0 renderer payload, Rec.709 luma, linear scale shared by the first three panels (no display gain)", fontsize=12)
fig.tight_layout(); fig.savefig(OUT / "payload-strip-60-62s.png", dpi=110); plt.close(fig)
print("payload luma max", round(float(vmax), 4), "mean baseline", round(float(lum["arm0"].mean()), 4), "adaptive", round(float(lum["arm4"].mean()), 4), "onset", round(float(lum["arm1"].mean()), 4))
print("ok")
