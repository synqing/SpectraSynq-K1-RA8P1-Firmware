#!/usr/bin/env python3
"""Lane P3 figures for the Captain-facing page (reads analysis/p3-data.json + p3-answers.json)."""
from __future__ import annotations

import json
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

BG, FG, GRID = "#0f1115", "#e8e6e1", "#2a2e36"
GOLD, CRIMSON, TEAL, GREY = "#d9a441", "#c8324a", "#4fb3a9", "#8a8f98"
plt.rcParams.update({"figure.facecolor": BG, "axes.facecolor": BG, "axes.edgecolor": GRID, "axes.labelcolor": FG,
                     "xtick.color": FG, "ytick.color": FG, "text.color": FG, "grid.color": GRID, "font.size": 11,
                     "axes.titleweight": "bold", "axes.titlesize": 12, "legend.frameon": False})


def series(R, field, which=None):
    t, v = [], []
    for s in R["samples"]:
        val = s[field] if which is None else (s[field] or {}).get(which)
        tt = s["t_mir"] if which is None else s["t_w0" if field == "w0" else "t_w1"]
        if val is not None:
            t.append(tt); v.append(val)
    return np.array(t), np.array(v, dtype=float)


def smooth(t, v, s=0.25):
    dt = np.median(np.diff(t))
    k = max(1, int(s / dt))
    return np.convolve(v, np.ones(k) / k, mode="same")


def main(out: Path):
    data = json.loads((out / "p3-data.json").read_text())
    q = json.loads((out / "p3-answers.json").read_text())
    qc = json.loads((out / "q3-correction.json").read_text())
    B = data["boom"]
    # Q1
    fig, ax = plt.subplots(3, 1, figsize=(11, 7.2), sharex=True)
    at, adb = np.array(B["audio"]["t"]), np.array(B["audio"]["rms_dbfs"])
    m = (at > 44) & (at < 62)
    ax[0].plot(at[m], smooth(at[m], adb[m]), color=FG, lw=1.4)
    ax[0].set_ylabel("received audio\nRMS (dBFS)")
    ax[0].set_title("Q1  Build → quiet cut → drop (55.5 s): the music comes back hard; the light does not")
    t, raw = series(B, "inhouse_raw_rms"); _, ps = series(B, "inhouse_peak_scaled")
    mm = (t > 44) & (t < 62)
    ax[1].plot(t[mm], raw[mm] / np.nanmax(raw[mm]), color=TEAL, lw=1.6, label="device raw RMS (input)")
    ax[1].plot(t[mm], ps[mm] / np.nanmax(ps[mm]), color=GOLD, lw=1.6, label="AGC output (peak_scaled)")
    ax[1].set_ylabel("level state\n(÷ window max)"); ax[1].legend(loc="upper left", fontsize=9)
    t0, b0 = series(B, "w0", "brightness"); t1, b1 = series(B, "w1", "brightness")
    for tt, bb, c, lab in ((t0, b0, GOLD, "lower board (Hybrid K1)"), (t1, b1, CRIMSON, "upper board (Tempo, Crimson Gold)")):
        mm = (tt > 44) & (tt < 62)
        ax[2].plot(tt[mm], bb[mm] * 1000, color=c, lw=1.4, marker="o", ms=3, label=lab)
    ax[2].set_ylabel("rendered wire\nbrightness (‰ FS)"); ax[2].legend(loc="upper left", fontsize=9)
    ax[2].set_xlabel("song time (s)")
    for a_ in ax:
        a_.axvspan(45, 50, color=TEAL, alpha=0.08)
        a_.axvspan(50, 55, color=GREY, alpha=0.18); a_.axvspan(55.5, 60, color=CRIMSON, alpha=0.10); a_.grid(alpha=0.4)
    for x_, lab in ((47.5, "build"), (52.5, "quiet cut"), (57.75, "drop")):
        ax[0].text(x_, -68.5, lab, color=GREY, fontsize=9, ha="center")
    q1 = q["q1"]
    ax[0].text(50.2, ax[0].get_ylim()[1] - 3, f"quiet → drop: audio +{q1['audio_db']:.1f} dB ({q1['audio_power_ratio_drop_over_quiet']:.1f}× power)",
               color=FG, fontsize=10, va="top")
    bd = qc["q1_build_vs_drop_wire"]
    ax[2].text(55.7, ax[2].get_ylim()[1] * 0.95,
               f"drop vs quiet: light ×{q1['wire_lower_brightness']['ratio']:.2f} lower, ×{q1['wire_upper_brightness']['ratio']:.2f} upper\n"
               f"drop vs build: light ×{bd['lower']['drop_over_build']:.2f} lower, ×{bd['upper']['drop_over_build']:.2f} upper\n"
               f"(drop is {bd['audio_drop_over_build_db']:+.1f} dB louder than the build)", color=FG, fontsize=9, va="top")
    fig.tight_layout(); fig.savefig(out / "fig-q1.png", dpi=110); plt.close(fig)

    # Q2
    fig, ax = plt.subplots(3, 1, figsize=(11, 7.4), sharex=False)
    for i, (k, R) in enumerate(data.items()):
        end = R["fixture_s"]
        t0, b0 = series(R, "w0", "brightness"); t1, b1 = series(R, "w1", "brightness")
        ax[i].plot(t0, b0 * 1000, color=GOLD, lw=1.2, label="lower board (wire)")
        ax[i].plot(t1, b1 * 1000, color=CRIMSON, lw=1.2, label="upper board (wire)")
        vt = np.array(R["video"]["t"]); vu = np.array(R["video"]["upper_band"])
        a2 = ax[i].twinx(); a2.plot(vt, vu, color=GREY, lw=0.8, ls="--", label="camera upper band (auto-exposure)")
        a2.set_ylabel("camera luma", color=GREY); a2.tick_params(colors=GREY)
        ax[i].axvspan(min(t0.min(), vt.min()), -0.3, color=TEAL, alpha=0.12)
        ax[i].axvspan(end + 0.5, max(t0.max(), vt.max()), color=TEAL, alpha=0.12)
        ax[i].set_title(f"{R['label']}: shaded = no music"); ax[i].set_ylabel("wire ‰ FS"); ax[i].grid(alpha=0.4)
        if i == 0:
            h1, l1 = ax[i].get_legend_handles_labels(); h2, l2 = a2.get_legend_handles_labels()
            ax[i].legend(h1 + h2, l1 + l2, loc="upper right", fontsize=8)
    ax[2].set_xlabel("time from music start (s)")
    fig.suptitle("Q2  Silence: the boards do dim on the wire — the camera shows the opposite (auto-exposure)", fontweight="bold")
    fig.tight_layout(); fig.savefig(out / "fig-q2.png", dpi=110); plt.close(fig)

    # Q3 (corrected: q3-correction.json; the phase-randomised "chance level" is retired)
    fig, ax = plt.subplots(1, 2, figsize=(11, 4.4))
    al = np.arange(len(qc["audio_onset_acf_0_1p2"])) / 20.0
    ax[0].plot(al, qc["audio_onset_acf_0_1p2"], color=FG, lw=1.8, label="music onsets (received audio)")
    for key, col, lab in (("light_upper_band", CRIMSON, "upper board light"), ("light_lower_band", GOLD, "lower board light")):
        r = qc[key]["acf_0_1p2"]
        ax[0].plot(np.arange(len(r)) / 30.0, r, color=col, lw=1.6, label=lab)
    e8 = 60 / 138 / 2
    for k, lab in ((3, "3/8"), (5, "5/8")):
        ax[0].axvline(k * e8, color=TEAL, lw=1, ls="--"); ax[0].text(k * e8 + 0.01, -0.36, lab + " bar", color=TEAL, fontsize=9)
    ax[0].set_xlim(0, 1.2); ax[0].set_ylim(-0.4, 1.45); ax[0].grid(alpha=0.4); ax[0].legend(fontsize=8, loc="upper right")
    ax[0].set_xlabel("lag (s)"); ax[0].set_ylabel("self-similarity (autocorrelation)")
    ax[0].set_title("Does it repeat? Music yes (138 BPM 3+3+2);\nthe light does not", fontsize=11)
    lags = np.arange(len(qc["light_upper_band"]["curve"])) / 30.0
    for key, col, lab in (("light_upper_band", CRIMSON, "upper board"), ("light_lower_band", GOLD, "lower board")):
        r = qc[key]
        ax[1].plot(lags * 1000, r["curve"], color=col, lw=1.8,
                   label=f"{lab}: best r {r['max_r']:.2f} at {r['lag_s']*1000:.0f} ms, p={r['p_value']:.2f}")
        ax[1].axhline(r["null_p95"], color=col, lw=0.9, ls=":")
    ax[1].set_xlabel("light delay after the music (ms)"); ax[1].set_ylabel("correlation with music onsets")
    ax[1].set_ylim(-0.15, 0.35); ax[1].grid(alpha=0.4); ax[1].legend(fontsize=8, loc="upper left")
    ax[1].set_title("Does it follow the onsets? Lower weakly, upper no\n(dotted = 95th pct of 1000 time-shifted controls)", fontsize=11)
    fig.suptitle("Q3  BOOM 55.5–75 s, light measured by camera", fontweight="bold")
    fig.tight_layout(); fig.savefig(out / "fig-q3.png", dpi=110); plt.close(fig)

    # Q4
    q4 = q["q4"]
    keys = ["boom", "sonic", "dopex"]; names = ["BOOM 28–75", "Sonic 6–30", "Dopex 28–75"]
    fig, ax = plt.subplots(1, 3, figsize=(11, 4.2))
    x = np.arange(3)
    med = [q4[k]["audio_rms_dbfs_median"] for k in keys]
    lo = [med[i] - q4[k]["audio_rms_dbfs_p10_p90"][0] for i, k in enumerate(keys)]
    hi = [q4[k]["audio_rms_dbfs_p10_p90"][1] - med[i] for i, k in enumerate(keys)]
    ax[0].errorbar(x, med, yerr=[lo, hi], fmt="o", color=FG, capsize=6, lw=2); ax[0].set_title("received audio (dBFS)\nmedian, p10–p90")
    ps = [q4[k]["inhouse_peak_scaled"] for k in keys]
    ax[1].errorbar(x, [p["median"] for p in ps], yerr=[[p["median"] - p["p10"] for p in ps], [p["p90"] - p["median"] for p in ps]],
                   fmt="o", color=GOLD, capsize=6, lw=2)
    ax[1].set_title("AGC output the effects read\nmedian, p10–p90")
    for j, (fld, c) in enumerate((("wire_lower", GOLD), ("wire_upper", CRIMSON))):
        w = [q4[k][fld] for k in keys]
        ax[2].errorbar(x + (j - 0.5) * 0.18, [p["median"] * 1000 for p in w],
                       yerr=[[(p["median"] - p["p10"]) * 1000 for p in w], [(p["p90"] - p["median"]) * 1000 for p in w]],
                       fmt="o", color=c, capsize=5, lw=2, label=("lower" if j == 0 else "upper") + " board")
    ax[2].set_title("rendered brightness (‰ FS)\nmedian, p10–p90"); ax[2].legend(fontsize=9)
    for a_ in ax:
        a_.set_xticks(x); a_.set_xticklabels(names, fontsize=9); a_.grid(alpha=0.4)
    fig.suptitle("Q4  Different songs reach the effects at the same level; the boards' output differs, but not by loudness", fontweight="bold")
    fig.tight_layout(); fig.savefig(out / "fig-q4.png", dpi=110); plt.close(fig)

    # Stage chart
    st = q["stage_dynamic_range_db_p90_over_p10"]
    stages = ["received_audio_rms", "device_raw_rms", "device_peak_scaled_agc", "wire_lower_brightness",
              "wire_upper_brightness", "video_lower_band"]
    labels = ["received\naudio", "device\nraw RMS", "AGC output\n(effects read)", "wire\nlower", "wire\nupper", "camera\n(lower band)"]
    fig, ax = plt.subplots(figsize=(11, 4.2))
    for k, c, n in (("boom", CRIMSON, "BOOM"), ("sonic", TEAL, "Sonic"), ("dopex", GOLD, "Dopex")):
        ax.plot(range(len(stages)), [st[k][s] for s in stages], marker="o", color=c, lw=2, label=n)
    ax.set_xticks(range(len(stages))); ax.set_xticklabels(labels, fontsize=9)
    ax.set_ylabel("dynamic range p90/p10 (dB)"); ax.grid(alpha=0.4); ax.legend()
    ax.axvspan(1.6, 2.4, color=GOLD, alpha=0.10); ax.text(1.65, ax.get_ylim()[1] * 0.92, "AGC halves the range", color=GOLD, fontsize=10)
    ax.set_title("Where the dynamics go: range of each stage, p90/p10 in dB")
    fig.tight_layout(); fig.savefig(out / "fig-stages.png", dpi=110); plt.close(fig)
    print("figures written")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
