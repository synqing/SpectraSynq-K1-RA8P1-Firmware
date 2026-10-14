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

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
BG, FG, GRID = "#0f1115", "#e8e6e1", "#2a2e36"
GOLD, CRIMSON, TEAL, GREY = "#d9a441", "#c8324a", "#4fb3a9", "#8a8f98"
plt.rcParams.update({"figure.facecolor": BG, "axes.facecolor": BG, "axes.edgecolor": GRID, "axes.labelcolor": FG,
                     "xtick.color": FG, "ytick.color": FG, "text.color": FG, "grid.color": GRID, "font.size": 11,
                     "axes.titleweight": "bold", "axes.titlesize": 12, "legend.frameon": True,
                     "legend.facecolor": BG, "legend.edgecolor": GRID, "legend.framealpha": 0.88})


def series(R, field, which=None):
    t, v = [], []
    for s in R["samples"]:
        val = s[field] if which is None else (s[field] or {}).get(which)
        tt = s["t_mir"] if which is None else s["t_w0" if field == "w0" else "t_w1"]
        if val is not None:
            t.append(tt); v.append(val)
    return np.array(t), np.array(v, dtype=float)


def smooth(t, v, s=0.25):
    """Moving average with edge-value padding (zero padding made false edge spikes)."""
    dt = np.median(np.diff(t))
    k = max(1, int(s / dt))
    pad = np.pad(np.asarray(v, dtype=float), (k // 2, k - 1 - k // 2), mode="edge")
    return np.convolve(pad, np.ones(k) / k, mode="valid")


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
    ax[0].set_title("Q1  Build → quiet cut → drop (55.5 s): the music comes back hard; the boards' output does not")
    t, raw = series(B, "inhouse_raw_rms"); _, ps = series(B, "inhouse_peak_scaled")
    mm = (t > 44) & (t < 62)
    ax[1].plot(t[mm], raw[mm] / np.nanmax(raw[mm]), color=TEAL, lw=1.6, label="device raw RMS (input)")
    ax[1].plot(t[mm], ps[mm] / np.nanmax(ps[mm]), color=GOLD, lw=1.6, label="in-house AGC output (peak_scaled, one renderer input)")
    ax[1].set_ylabel("level state\n(÷ window max)"); ax[1].legend(loc="lower left", bbox_to_anchor=(0, 1.0), ncol=2, fontsize=8.5, borderaxespad=0.2)
    t0, b0 = series(B, "w0", "brightness"); t1, b1 = series(B, "w1", "brightness")
    for tt, bb, c, lab in ((t0, b0, GOLD, "lower board (Hybrid K1)"), (t1, b1, CRIMSON, "upper board (Tempo, Crimson Gold)")):
        mm = (tt > 44) & (tt < 62)
        ax[2].plot(tt[mm], bb[mm] * 1000, color=c, lw=1.4, marker="o", ms=3, label=lab)
    ax[2].set_ylabel("submitted payload\nbrightness (‰ FS)")
    ax[2].legend(loc="lower left", bbox_to_anchor=(0, 1.0), ncol=2, fontsize=8.5, borderaxespad=0.2)
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
               f"drop vs quiet: luma ×{q1['wire_lower_brightness']['ratio']:.2f} lower, ×{q1['wire_upper_brightness']['ratio']:.2f} upper\n"
               f"drop vs build: luma ×{bd['lower']['drop_over_build']:.2f} lower, ×{bd['upper']['drop_over_build']:.2f} upper\n"
               f"(drop is {bd['audio_drop_over_build_db']:+.1f} dB louder than the build)", color=FG, fontsize=9, va="top")
    fig.tight_layout(); fig.savefig(out / "fig-q1.png", dpi=110); plt.close(fig)

    # Q2
    fig, ax = plt.subplots(3, 1, figsize=(11, 7.4), sharex=False)
    for i, (k, R) in enumerate(data.items()):
        end = R["fixture_s"]
        t0, b0 = series(R, "w0", "brightness"); t1, b1 = series(R, "w1", "brightness")
        ax[i].plot(t0, b0 * 1000, color=GOLD, lw=1.2, label="lower board (payload)")
        ax[i].plot(t1, b1 * 1000, color=CRIMSON, lw=1.2, label="upper board (payload)")
        tr, rr = series(R, "inhouse_raw_rms")
        ax[i].plot(tr, rr / np.nanmax(rr) * max(np.nanmax(b0), np.nanmax(b1)) * 1000, color=TEAL, lw=1.0, ls=":",
                   label="device mic level (raw RMS, scaled)")
        vt = np.array(R["video"]["t"]); vu = np.array(R["video"]["upper_band"])
        a2 = ax[i].twinx(); a2.plot(vt, vu, color=GREY, lw=0.8, ls="--", label="camera upper band (auto-exposure)")
        a2.set_ylabel("camera luma", color=GREY); a2.tick_params(colors=GREY)
        ax[i].axvspan(min(t0.min(), vt.min()), -0.3, color=TEAL, alpha=0.12)
        ax[i].axvspan(end + 0.5, max(t0.max(), vt.max()), color=TEAL, alpha=0.12)
        ax[i].set_title(f"{R['label']}: shaded = no music"); ax[i].set_ylabel("payload ‰ FS"); ax[i].grid(alpha=0.4)
        if i == 0:
            h1, l1 = ax[i].get_legend_handles_labels(); h2, l2 = a2.get_legend_handles_labels()
            fig.legend(h1 + h2, l1 + l2, loc="lower center", ncol=4, fontsize=8.5)
    ax[2].set_xlabel("time from music start (s)")
    fig.suptitle("Q2  Silence: the payload dims when the device hears quiet; the camera brightens at silence edges", fontweight="bold", fontsize=11)
    fig.tight_layout(rect=(0, 0.04, 1, 1)); fig.savefig(out / "fig-q2.png", dpi=110); plt.close(fig)

    # Q3 (iteration 4: rate spectra and loudness alignment, iteration4.json; REVIEW-ITERATION-3.md F1/F2/F4)
    import pandas as pd
    from scipy.signal import welch
    import p3_q3_alignment as q3m
    it4 = json.loads((out / "iteration4.json").read_text())
    B_ = data["boom"]
    vt3 = np.array(B_["video"]["t"]); m3 = (vt3 >= 55.5) & (vt3 <= 75)
    at3, adb3 = np.array(B_["audio"]["t"]), np.array(B_["audio"]["rms_dbfs"]); am3 = (at3 >= 55.4) & (at3 <= 75.1)
    ma3 = (at3 >= 55.5) & (at3 <= 75)
    lin3 = 10 ** (adb3 / 20.0)
    room = pd.read_csv(ROOT / "docs/evidence/P3/agc-trace-boom-recording.csv")
    rw = {b: room[(room.board == b) & (room.t_s >= 55.5) & (room.t_s <= 75)] for b in (0, 1)}
    rows = (("music loudness (room audio, linear RMS)", lin3[ma3], 20.0, FG, "room_audio_linear_rms_20hz"),
            ("host replay: upper board (Tempo)", rw[1].luma.values, 100.0, CRIMSON, "host_luma_room_recording_upper_tempo"),
            ("host replay: lower board (Hybrid K1)", rw[0].luma.values, 100.0, TEAL, "host_luma_room_recording_lower_hybrid_k1"),
            ("camera band near lower edge", np.array(B_["video"]["lower_band"])[m3], 30.0, GOLD, "camera_lower_edge_band_30hz"))
    fig = plt.figure(figsize=(11, 6.4))
    gs = fig.add_gridspec(4, 2, width_ratios=(1.25, 1), hspace=0.18, wspace=0.22)
    rates = (("bar", 0.575), ("half-time", 1.15), ("accent", 1.725), ("beat", 2.30))
    for i, (lab, x, fs, col, key) in enumerate(rows):
        a_ = fig.add_subplot(gs[i, 0])
        n = int(8 * fs)
        f, p = welch(x, fs=fs, window="hann", nperseg=n, noverlap=n // 2, nfft=8 * n, detrend="linear")
        sel = (f >= 0.3) & (f <= 3.2)
        p = p / p[(f >= 0.3) & (f <= 5.0)].sum()
        a_.fill_between(f[sel], p[sel], color=col, alpha=0.35, lw=0); a_.plot(f[sel], p[sel], color=col, lw=1.3)
        for nm, r in rates:
            a_.axvline(r, color=GREY, lw=0.9, ls="--")
            if i == 0:
                a_.text(r, 1.02, nm, transform=a_.get_xaxis_transform(), color=GREY, fontsize=8.5, ha="center", va="bottom")
        sh = it4["A_B_series"][key]["rate_share_primary"]
        a_.text(0.99, 0.92, f"{lab}\nbar {sh['bar']*100:.0f}% · beat {sh['beat']*100:.0f}% of rhythmic power",
                transform=a_.transAxes, ha="right", va="top", fontsize=8, color=FG,
                bbox=dict(facecolor=BG, edgecolor="none", alpha=0.75, pad=1.5))
        a_.set_xlim(0.3, 3.2); a_.set_yticks([]); a_.grid(alpha=0.3)
        if i < 3:
            a_.set_xticklabels([])
        else:
            a_.set_xlabel("rate (Hz)")
        if i == 0:
            a_.set_title("Which rates carry power?  BOOM drop, 55.5–75 s\n", fontsize=11)
    ax1 = fig.add_subplot(gs[:, 1])
    t30 = vt3[m3]
    band = q3m.detrend(np.array(B_["video"]["lower_band"])[m3], 30.0)
    accent3 = np.maximum(0, np.diff(adb3[am3], prepend=adb3[am3][0]))
    D = it4["D_alignment"]
    for rn, env, col, lab in (("linear_rms", q3m.detrend(np.interp(t30, at3[am3], lin3[am3]), 30.0), FG, "music loudness (linear RMS)"),
                              ("accent_rise_envelope", q3m.detrend(np.interp(t30, at3[am3], accent3), 30.0), GREY, "accent-weighted rise envelope")):
        lagc = q3m.xcorr_curve(env, band, 15); leadc = q3m.xcorr_curve(band, env, 15)
        x_ms = np.r_[-np.arange(15, 0, -1), np.arange(16)] / 30.0 * 1000
        ax1.plot(x_ms, np.r_[leadc[:0:-1], lagc], color=col, lw=1.8, label=lab)
        d = D[rn]["camera_lower_edge_band"]
        ax1.hlines(d["light_lags_music_0_500ms"]["null_p95"], 0, 500, color=col, lw=0.9, ls=":")
        ax1.hlines(d["light_leads_music_0_500ms"]["null_p95"], -500, 0, color=col, lw=0.9, ls=":")
    d = D["linear_rms"]["camera_lower_edge_band"]["light_lags_music_0_500ms"]
    ax1.annotate(f"r {d['max_r']:.2f} at {d['lag_s']*1000:.0f} ms, p {d['p_value']:.3f}", xy=(d["lag_s"] * 1000, d["max_r"]),
                 xytext=(120, 0.80), color=FG, fontsize=9, arrowprops=dict(arrowstyle="->", color=FG, lw=0.8))
    ax1.axvline(0, color=GRID, lw=1)
    ax1.set_xlim(-500, 500); ax1.set_ylim(-0.72, 0.92); ax1.grid(alpha=0.4)
    ax1.text(0.02, 0.025, "◀ light leads", transform=ax1.transAxes, color=GREY, fontsize=9)
    ax1.text(0.98, 0.025, "light lags ▶", transform=ax1.transAxes, color=GREY, fontsize=9, ha="right")
    ax1.set_xlabel("delay of the light against the music (ms)"); ax1.set_ylabel("correlation (1 s detrended)")
    ax1.legend(fontsize=8, loc="upper left")
    ax1.set_title("Does the camera band follow the music?\n(dotted = 95th pct of shifted controls)", fontsize=11)
    fig.suptitle("Q3  The host replay carries the beat; on camera the lower band follows loudness, but no beat is found", fontweight="bold", fontsize=11)
    fig.subplots_adjust(left=0.03, right=0.985, top=0.86, bottom=0.09)
    fig.savefig(out / "fig-q3.png", dpi=110); plt.close(fig)

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
    ax[1].set_title("in-house AGC output (one input)\nmedian, p10–p90")
    for j, (fld, c) in enumerate((("wire_lower", GOLD), ("wire_upper", CRIMSON))):
        w = [q4[k][fld] for k in keys]
        ax[2].errorbar(x + (j - 0.5) * 0.18, [p["median"] * 1000 for p in w],
                       yerr=[[(p["median"] - p["p10"]) * 1000 for p in w], [(p["p90"] - p["median"]) * 1000 for p in w]],
                       fmt="o", color=c, capsize=5, lw=2, label=("lower" if j == 0 else "upper") + " board")
    ax[2].set_title("payload brightness (‰ FS)\nmedian, p10–p90"); ax[2].legend(fontsize=9)
    for a_ in ax:
        a_.set_xticks(x); a_.set_xticklabels(names, fontsize=9); a_.grid(alpha=0.4)
    fig.suptitle("Q4  Different songs reach the in-house AGC at the same level; the boards differ, but not by loudness", fontweight="bold", fontsize=11)
    fig.tight_layout(); fig.savefig(out / "fig-q4.png", dpi=110); plt.close(fig)

    # Stage chart (iteration 2): grouped bars, one dB convention per quantity, parallel outputs not joined
    it2 = json.loads((out / "iteration2.json").read_text())
    E = it2["E_stage_ranges_db"]
    keys = [("received_audio_rms_amplitude_20log", "received\naudio"), ("device_raw_rms_amplitude_20log", "device\nraw RMS"),
            ("agc_peak_scaled_amplitude_ratio_20log", "in-house\nAGC output"),
            ("payload_lower_luma_intensity_10log", "payload\nlower (light)"),
            ("payload_upper_luma_intensity_10log", "payload\nupper (light)"),
            ("camera_lower_band_srgb_decoded_10log", "camera\n(decoded)")]
    fig, ax = plt.subplots(figsize=(11, 4.4))
    x = np.arange(len(keys))
    for j_, (k, c, n) in enumerate((("boom", CRIMSON, "BOOM 28–75"), ("sonic", TEAL, "Sonic 6–30"), ("dopex", GOLD, "Dopex 0–75"))):
        ax.bar(x + (j_ - 1) * 0.26, [E[k][kk] for kk, _ in keys], width=0.24, color=c, label=n)
    ax.set_xticks(x); ax.set_xticklabels([l for _, l in keys], fontsize=9)
    ax.axvline(2.5, color=GREY, lw=0.8, ls=":")
    ax.set_ylim(0, 23.5)
    ax.text(-0.4, 22.2, "amplitude: 20·log10", color=GREY, fontsize=9)
    ax.text(2.6, 22.2, "light intensity: 10·log10 (parallel outputs, not stages)", color=GREY, fontsize=9)
    ax.set_ylabel("p90/p10 range (dB)"); ax.grid(alpha=0.4, axis="y"); ax.legend(loc="center right", fontsize=9)
    ax.set_title("Where the dynamics go (supporting evidence; per-snapshot ranges include smoothing differences)")
    fig.tight_layout(); fig.savefig(out / "fig-stages.png", dpi=110); plt.close(fig)

    # Per-song time series (iteration 2, brief windows; iteration 3 adds the host in-house confidence)
    import pandas as pd
    ts = json.loads((out / "timeseries.json").read_text())
    for k, (a_, b_), lab in (("boom", (28, 75), "BOOM 28–75 s"), ("sonic", (0, 24), "Sonic Samba 6–30 s (music-relative)"),
                             ("dopex", (0, 75), "Dopex 0–75 s")):
        R = data[k]
        fig, ax = plt.subplots(4, 1, figsize=(11, 8.2), sharex=True)
        at, adb = np.array(R["audio"]["t"]), np.array(R["audio"]["rms_dbfs"])
        m_ = (at >= a_ - 2) & (at <= b_ + 2)
        ax[0].plot(at[m_], smooth(at[m_], adb[m_]), color=FG, lw=1.2); ax[0].set_ylabel("iPhone audio\n(dBFS)")
        t_, raw = series(R, "inhouse_raw_rms"); _, ps = series(R, "inhouse_peak_scaled"); td, dv = series(R, "donor_vu")
        mm = (t_ >= a_ - 2) & (t_ <= b_ + 2); md = (td >= a_ - 2) & (td <= b_ + 2)
        ax[1].plot(t_[mm], raw[mm], color=TEAL, lw=1.4, label="device raw RMS")
        ax[1].plot(td[md], dv[md], color=GREY, lw=1.2, label="donor VU")
        a1 = ax[1].twinx(); a1.plot(t_[mm], ps[mm], color=GOLD, lw=1.2, label="in-house AGC (right)")
        a1.set_ylim(0, max(1.2, float(np.nanmax(ps[mm])) * 1.08))
        a1.tick_params(colors=GOLD)
        h1, l1 = ax[1].get_legend_handles_labels(); h2, l2 = a1.get_legend_handles_labels()
        ax[1].legend(h1 + h2, l1 + l2, fontsize=8, loc="lower left", bbox_to_anchor=(0, 1.0), ncol=3, borderaxespad=0.2)
        ax[1].set_ylabel("device level"); ax[1].set_ylim(0, max(0.5, float(np.nanmax(np.r_[raw[mm], dv[md]])) * 1.08))
        dt_ = np.array(ts[k]["donor_t"]); dc = np.array([c if c is not None else np.nan for c in ts[k]["donor_conf"]])
        ax[2].axhspan(0.30, 0.60, color=CRIMSON, alpha=0.12)
        ax[2].plot(dt_, dc, color=GREY, lw=1.0, marker="o", ms=2.2, label="donor confidence (recorded; no v8 effect reads it)")
        if k == "boom":
            hr = pd.read_csv(ROOT / "docs/evidence/P3/agc-trace-boom-recording.csv")
            hr = hr[(hr.board == 1) & (hr.t_s >= a_ - 2) & (hr.t_s <= b_ + 2)]
            ax[2].plot(hr.t_s, hr.tempo_confidence.rolling(25, center=True, min_periods=1).mean(), color=CRIMSON, lw=1.3,
                       label="in-house confidence the Tempo board reads (HOST replay, room recording)")
            ax[2].axhspan(0.30, 0.60, color=CRIMSON, alpha=0.10)
        ax[2].set_ylim(0, 1.0); ax[2].set_ylabel("confidence")
        ax[2].legend(fontsize=7.5, loc="lower left", bbox_to_anchor=(0, 1.0), ncol=2, borderaxespad=0.2)
        t0_, b0_ = series(R, "w0", "brightness"); t1_, b1_ = series(R, "w1", "brightness")
        s0 = (t0_ >= a_ - 2) & (t0_ <= b_ + 2); s1 = (t1_ >= a_ - 2) & (t1_ <= b_ + 2)
        ax[3].plot(t0_[s0], b0_[s0] * 1000, color=GOLD, lw=1.2, label="lower board payload")
        ax[3].plot(t1_[s1], b1_[s1] * 1000, color=CRIMSON, lw=1.2, label="upper board payload")
        ax[3].set_ylabel("payload\nbrightness (‰ FS)"); ax[3].legend(fontsize=8, loc="lower left", bbox_to_anchor=(0, 1.0), ncol=2, borderaxespad=0.2)
        ax[3].set_xlabel("time from music start (s)")
        for a__ in ax:
            a__.grid(alpha=0.4); a__.set_xlim(a_ - 2, b_ + 2)
        fig.suptitle(f"{lab} — audio · device level · tempo confidence · payload", fontweight="bold", fontsize=11)
        fig.tight_layout(); fig.savefig(out / f"fig-ts-{k}.png", dpi=105); plt.close(fig)

    # Host replay of the Tempo board's own tempo (iteration 3: three inputs, effective gate)
    import pandas as pd
    fig, ax = plt.subplots(2, 1, figsize=(11, 5.8), sharex=True)
    srcs = ((out / "host-tempo-boom.csv", GREY, "song file, gain 1.0", True),
            (ROOT / "docs/evidence/P3/agc-trace-boom-source.csv", GOLD, "song file, gain 1.155 (device-calibrated)", False),
            (ROOT / "docs/evidence/P3/agc-trace-boom-recording.csv", CRIMSON, "room recording (device-calibrated)", False))
    for path, c, lab, pre in srcs:
        u = pd.read_csv(path); u = u[u.board == 1]
        ax[0].plot(u.t_s, u.tempo_bpm, color=c, lw=1.2, label=lab)
        g = u.beat_gate.values
        if pre:
            g = g * g * (3 - 2 * g)  # the iteration-2 CSV stored the gate before smoothstep
        ax[1].plot(u.t_s, pd.Series(g).rolling(25, center=True, min_periods=1).mean(), color=c, lw=1.1, label=lab)
    ax[0].axhline(138, color=TEAL, lw=1, ls="--"); ax[0].text(0.5, 141, "138 BPM (song)", color=TEAL, fontsize=9)
    ax[0].text(14, 104, "69 = half-time\n92 = 2/3 of 138", color=GREY, fontsize=9, va="center")
    ax[0].set_ylabel("BPM"); ax[0].set_ylim(50, 165)
    ax[0].legend(fontsize=7.5, loc="lower left", bbox_to_anchor=(0, 1.0), ncol=3, borderaxespad=0.2)
    ax[1].set_ylabel("effective beat gate"); ax[1].set_xlabel("BOOM source time (s)"); ax[1].set_ylim(-0.05, 1.1)
    for a__ in ax:
        a__.axvspan(50, 55.5, color=GREY, alpha=0.15); a__.axvspan(55.5, 75, color=CRIMSON, alpha=0.08); a__.grid(alpha=0.4)
    fig.suptitle("Tempo board's tempo, host replay: half-time in the verse, ~138 at 33–43 s; the drop is mostly 92 or 69", fontweight="bold", fontsize=11)
    fig.tight_layout(); fig.savefig(out / "fig-tempo.png", dpi=110); plt.close(fig)
    print("figures written")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
