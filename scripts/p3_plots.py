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
    B = data["boom"]
    # Q1
    fig, ax = plt.subplots(3, 1, figsize=(11, 7.2), sharex=True)
    at, adb = np.array(B["audio"]["t"]), np.array(B["audio"]["rms_dbfs"])
    m = (at > 44) & (at < 62)
    ax[0].plot(at[m], smooth(at[m], adb[m]), color=FG, lw=1.4)
    ax[0].set_ylabel("received audio\nRMS (dBFS)")
    ax[0].set_title("Q1  Quiet cut (50–55 s) → drop (55.5 s): the audio jumps, the light barely moves")
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
        a_.axvspan(50, 55, color=GREY, alpha=0.18); a_.axvspan(55.5, 60, color=CRIMSON, alpha=0.10); a_.grid(alpha=0.4)
    q1 = q["q1"]
    ax[0].text(50.2, ax[0].get_ylim()[1] - 3, f"audio +{q1['audio_db']:.1f} dB ({q1['audio_power_ratio_drop_over_quiet']:.1f}× power)",
               color=FG, fontsize=10, va="top")
    ax[2].text(55.7, ax[2].get_ylim()[1] * 0.95, f"light ×{q1['wire_lower_brightness']['ratio']:.2f} lower, ×{q1['wire_upper_brightness']['ratio']:.2f} upper",
               color=FG, fontsize=10, va="top")
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

    # Q3
    fig, ax = plt.subplots(figsize=(11, 4.4))
    q3 = q["q3"]
    for key, c, lab in (("audio_onset_envelope", FG, "received audio onset envelope"),
                        ("video_upper_band", CRIMSON, "upper board light (camera)"),
                        ("video_lower_band", GOLD, "lower board light (camera)")):
        r = q3[key]
        ax.plot(r["lags"], r["acf"], color=c, lw=1.8, label=f"{lab}  (at 138 BPM: {r['acf_at_138bpm']:+.2f})")
        ax.axhline(r["null_p95"], color=c, lw=0.8, ls=":")
    ax.axvline(60 / 138, color=TEAL, lw=1.4); ax.text(60 / 138 + 0.01, 0.8, "138 BPM", color=TEAL)
    ax.set_xlim(0, 1.2); ax.set_ylim(-0.4, 1.0); ax.grid(alpha=0.4); ax.legend(fontsize=9, loc="upper right")
    ax.set_xlabel("lag (s)"); ax.set_ylabel("autocorrelation")
    ax.set_title("Q3  BOOM 55.5–75 s: the music pulses at ~138 BPM; the light shows no matching pulse (dotted = chance level)")
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
    ax.set_title("Where the dynamics go: AGC compresses section-level loudness; the camera compresses everything")
    fig.tight_layout(); fig.savefig(out / "fig-stages.png", dpi=110); plt.close(fig)
    print("figures written")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
