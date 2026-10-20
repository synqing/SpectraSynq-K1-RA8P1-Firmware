#!/usr/bin/env python3
"""Lane P3 iteration 2: the measurements REVIEW-ITERATION-1.md asked for.

Reads analysis/p3-data.json, analysis/host-tempo-boom.csv and the raw snapshots of
the three 14 Oct recordings; writes analysis/iteration2.json and
analysis/timeseries.json. Nothing from iteration 1 is regenerated.

  A camera-band crosstalk (the camera cannot separate the boards)
  B device level in each silence window (Sonic post-roll was not silent)
  C donor tempo / confidence / beat-lock as recorded per window
  D section means and the AGC in true silence
  E stage ranges with one dB convention per quantity
  F payload ceiling below full scale
  G Q3 gate: power curve, negative controls
  H host replay of the in-house tempo the Tempo board reads
Offline, read-only. Agent cannot listen.
"""
from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

import numpy as np
from scipy.stats import spearmanr

sys.path.insert(0, str(Path(__file__).resolve().parent))
import p3_q3_alignment as q3  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
CAP = ROOT / "_scratch/waveform-migration-001/canonical-audio-v01"
DIRS = {"boom": "boom-admission-fix-candidate-001", "sonic": "sonic-6-30-admission-fix-candidate-001",
        "dopex": "dopex-admission-fix-holdout-001"}
WINDOWS = {"boom": (28.0, 75.0), "sonic": (0.0, 24.0), "dopex": (0.0, 75.0)}  # brief windows, music-relative


def raw_snapshots(key):
    d = CAP / DIRS[key]
    ev = json.loads((d / "events.json").read_text())
    ap = next(e for e in ev if e["event"] == "audio_process_started")
    t0 = ap.get("started", ap["mono"])
    rows = []
    for sd in sorted(d.glob("sample-*")):
        try:
            m = json.loads((sd / "mir-snapshot.json").read_text())
        except (OSError, ValueError):
            continue
        b = m.get("body", {})
        row = {"t": m["mono"] - t0, "donor_bpm": b.get("donor_dominant_tempo_bpm"),
               "donor_conf": b.get("donor_tempo_confidence"), "beat_locked": b.get("beat_locked")}
        for name, stem in (("lower", "wire-snapshot"), ("upper", "wire-snapshot-top")):
            try:
                raw = (sd / f"{stem}.bin").read_bytes()
                v = struct.unpack(">480H", raw[544:1504]) if len(raw) >= 1504 else None
            except OSError:
                v = None
            row[f"{name}_max_code"] = max(v) if v else None
        rows.append(row)
    return rows


def series(R, field, which=None):
    t, v = [], []
    for s in R["samples"]:
        val = s[field] if which is None else (s[field] or {}).get(which)
        tt = s["t_mir"] if which is None else s["t_w0" if field == "w0" else "t_w1"]
        if val is not None:
            t.append(tt); v.append(val)
    return np.array(t), np.array(v, dtype=float)


def pct_range_db(v, power):
    v = np.asarray(v, dtype=float)
    v = v[np.isfinite(v) & (v > 0)]
    p10, p90 = np.percentile(v, [10, 90])
    return float((10 if power else 20) * np.log10(p90 / max(p10, 1e-12)))


def main(out: Path):
    data = json.loads((out / "p3-data.json").read_text())
    res = {}

    # A. crosstalk
    B = data["boom"]
    vt = np.array(B["video"]["t"]); up = np.array(B["video"]["upper_band"]); lo = np.array(B["video"]["lower_band"])
    m = (vt >= 55.5) & (vt <= 75)
    a = q3.detrend(up[m], 30.0); b = q3.detrend(lo[m], 30.0)
    t0, w0 = series(B, "w0", "brightness"); t1, w1 = series(B, "w1", "brightness")
    sel0 = (t0 >= 0) & (t0 <= 75); sel1 = (t1 >= 0) & (t1 <= 75)
    res["A_crosstalk"] = {
        "bands_r_detrended_55p5_75": float(np.corrcoef(a, b)[0, 1]),
        "bands_r_raw_55p5_75": float(np.corrcoef(up[m], lo[m])[0, 1]),
        "lower_band_vs_upper_wire_rho": float(spearmanr(np.interp(t1[sel1], vt, lo), w1[sel1])[0]),
        "lower_band_vs_lower_wire_rho": float(spearmanr(np.interp(t0[sel0], vt, lo), w0[sel0])[0]),
        "upper_band_vs_upper_wire_rho": float(spearmanr(np.interp(t1[sel1], vt, up), w1[sel1])[0]),
        "upper_band_vs_lower_wire_rho": float(spearmanr(np.interp(t0[sel0], vt, up), w0[sel0])[0]),
        "wires_rho": float(spearmanr(np.interp(t1[sel1], t0[sel0], w0[sel0]), w1[sel1])[0]),
    }

    # B. device level in silence windows
    sil = {}
    for k, R in data.items():
        end = R["fixture_s"]
        t, raw = series(R, "inhouse_raw_rms"); _, ps = series(R, "inhouse_peak_scaled")
        mus = (t >= 0) & (t <= end); sw = (t < -0.3) | (t > end + 0.5)
        post = t > end + 0.5
        sil[k] = {"music_median_raw_rms": float(np.median(raw[mus])),
                  "silence_raw_rms_mean": float(raw[sw].mean()), "silence_raw_rms_max": float(raw[sw].max()),
                  "silence_raw_rms_max_t": float(t[sw][np.argmax(raw[sw])]),
                  "post_roll_samples_above_half_music_median": int((raw[post] > 0.5 * np.median(raw[mus])).sum()),
                  "post_roll_samples": int(post.sum()),
                  "post_roll_peak_scaled_range": [float(ps[post].min()), float(ps[post].max())],
                  "post_roll_raw_rms_range": [float(raw[post].min()), float(raw[post].max())]}
    res["B_silence_device_level"] = sil

    # C + F. donor features and payload ceiling, per brief window
    snaps = {k: raw_snapshots(k) for k in DIRS}
    feat, ceil = {}, {}
    for k, rows in snaps.items():
        a_, b_ = WINDOWS[k]
        w = [r for r in rows if a_ <= r["t"] <= b_]
        conf = np.array([r["donor_conf"] for r in w if r["donor_conf"] is not None])
        bpm = np.array([r["donor_bpm"] for r in w if r["donor_bpm"] is not None])
        feat[k] = {"window_s": [a_, b_], "n": len(w), "donor_conf_median": float(np.median(conf)),
                   "donor_conf_p90": float(np.percentile(conf, 90)), "donor_conf_max": float(conf.max()),
                   "donor_bpm_median": float(np.median(bpm)), "beat_locked_any": bool(any(r["beat_locked"] for r in w))}
        ceil[k] = {}
        for name in ("lower", "upper"):
            c = np.array([r[f"{name}_max_code"] for r in w if r[f"{name}_max_code"]])
            top = c.max()
            ceil[k][name] = {"max_code": int(top), "p50_max_code": float(np.median(c)),
                             "fraction_within_5pct_of_max": float((c >= 0.95 * top).mean()),
                             "full_scale_fraction_of_max": float(top / 65535)}
    res["C_donor_features"] = feat
    res["F_payload_ceiling"] = ceil

    # D. section means (BOOM) and the AGC in true silence (Dopex post-roll)
    t, raw = series(B, "inhouse_raw_rms"); _, ps = series(B, "inhouse_peak_scaled")
    def mean_in(tt, vv, a_, b_):
        s_ = (tt >= a_) & (tt <= b_)
        return float(vv[s_].mean()), int(s_.sum())
    res["D_section_means_boom"] = {name: {"raw_rms": mean_in(t, raw, *w_), "peak_scaled": mean_in(t, ps, *w_)}
                                   for name, w_ in (("build_45_50", (45, 50)), ("quiet_50_55", (50, 55)),
                                                    ("drop_55p5_60", (55.5, 60)))}
    D = data["dopex"]
    td, rd = series(D, "inhouse_raw_rms"); _, pd = series(D, "inhouse_peak_scaled")
    post = td > D["fixture_s"] + 0.5
    mus = (td >= 0) & (td <= D["fixture_s"])
    res["D_agc_in_silence_dopex"] = {"post_roll_peak_scaled": [float(pd[post].min()), float(pd[post].max())],
                                     "post_roll_raw_rms": [float(rd[post].min()), float(rd[post].max())],
                                     "music_median_peak_scaled": float(np.median(pd[mus])),
                                     "music_median_raw_rms": float(np.median(rd[mus])), "n_post": int(post.sum())}

    # E. stage ranges, one convention per quantity
    st = {}
    for k, R in data.items():
        a_, b_ = WINDOWS[k]
        at = np.array(R["audio"]["t"]); adb = np.array(R["audio"]["rms_dbfs"])
        sa = (at >= a_) & (at <= b_)
        e = {"received_audio_rms_amplitude_20log": pct_range_db(10 ** (adb[sa] / 20), False)}
        for f, lab in (("inhouse_raw_rms", "device_raw_rms_amplitude_20log"),
                       ("inhouse_peak_scaled", "agc_peak_scaled_amplitude_ratio_20log")):
            tt, vv = series(R, f); s_ = (tt >= a_) & (tt <= b_)
            e[lab] = pct_range_db(vv[s_], False)
        for which, lab in (("w0", "payload_lower_luma_intensity_10log"), ("w1", "payload_upper_luma_intensity_10log")):
            tt, vv = series(R, which, "brightness"); s_ = (tt >= a_) & (tt <= b_)
            e[lab] = pct_range_db(vv[s_], True)
        vt_ = np.array(R["video"]["t"]); s_ = (vt_ >= a_) & (vt_ <= b_)
        for band in ("upper_band", "lower_band"):
            g = np.array(R["video"][band])[s_]
            lin = np.where(g <= 0.04045, g / 12.92, ((g + 0.055) / 1.055) ** 2.4)  # sRGB decode (approximate)
            e[f"camera_{band}_srgb_decoded_10log"] = pct_range_db(lin, True)
        st[k] = e
    res["E_stage_ranges_db"] = st
    res["E_conventions"] = ("20*log10 for amplitudes (audio RMS, device RMS, AGC ratio); 10*log10 for linear light "
                            "(payload luma); camera grey levels are gamma-encoded and were sRGB-decoded "
                            "(approximate: iPhone tone curve unknown) before 10*log10")

    # G. Q3 power curve and negative controls (same construction as p3_q3_alignment)
    at = np.array(B["audio"]["t"]); adb = np.array(B["audio"]["rms_dbfs"])
    mv = (vt >= 55.5) & (vt <= 75.0); tv = vt[mv]
    am = (at >= 55.4) & (at <= 75.1)
    onset = np.maximum(0, np.diff(adb[am], prepend=adb[am][0]))
    env = q3.detrend(np.interp(tv, at[am], onset), 30.0)
    rng = np.random.default_rng(11)
    band = q3.detrend(lo[mv], 30.0)
    power = {}
    for scale in (0.1, 0.2, 0.3, 0.5):
        hits = 0
        for _ in range(20):
            base = np.roll(band, int(rng.integers(120, len(band) - 120)))  # decorrelated light
            lag = int(rng.integers(0, 11))
            plant = base + scale * base.std() / env.std() * np.concatenate([np.zeros(lag), env[:len(env) - lag]])
            q3.SHIFTS = 300
            hits += q3.test(env, plant, rng)["detected"]
        power[str(scale)] = hits
    false_pos = 0
    for _ in range(40):
        q3.SHIFTS = 300
        false_pos += q3.test(env, np.roll(band, int(rng.integers(120, len(band) - 120))), rng)["detected"]
    ma = (at >= 30.0) & (at <= 49.5 + 0.1)
    on2 = np.maximum(0, np.diff(adb[ma], prepend=adb[ma][0]))
    t_shift = np.linspace(30.0, 49.5, len(tv))
    env2 = q3.detrend(np.interp(t_shift, at[ma], on2), 30.0)
    q3.SHIFTS = 1000
    mismatch = q3.test(env2, band, rng)
    res["G_q3_gate"] = {"power_detected_of_20": power, "false_positive_shifted_light_of_40": false_pos,
                        "mismatched_section_audio30_49p5_vs_light55p5_75": {k: mismatch[k] for k in ("max_r", "p_value", "detected")},
                        "recorded_red_run": "analysis/q3-red-run-plant0.json, exit 1 (plant scale 0: upper-band control not detected)"}

    # H. host replay of the in-house tempo (mode 18 reads it; inhouse_audio.cpp:357-359)
    import pandas as pd
    h = pd.read_csv(out / "host-tempo-boom.csv"); u = h[h.board == 1]
    hh = {}
    for name, (a_, b_) in (("intro_0_28", (0, 28)), ("verse_28_50", (28, 50)), ("quiet_50_55p5", (50, 55.5)),
                           ("drop_55p5_75", (55.5, 75))):
        w = u[(u.t_s >= a_) & (u.t_s < b_)]
        hh[name] = {"n": int(len(w)), "valid_fraction": float(w.tempo_valid.mean()),
                    "bpm_median": float(w.tempo_bpm.median()),
                    "bpm_mode": float(w.tempo_bpm.round().mode().iloc[0]),
                    "confidence_median": float(w.tempo_confidence.median()),
                    "gate_mean": float(w.beat_gate.mean()), "gate_open_fraction": float((w.beat_gate > 0).mean())}
    res["H_host_inhouse_tempo_boom_source"] = hh
    fine = {}
    for a_, b_ in ((0, 7), (7, 33), (33, 43), (43, 50), (50, 55.5), (55.5, 57), (57, 67), (67, 75)):
        w = u[(u.t_s >= a_) & (u.t_s < b_)]
        vc = w.tempo_bpm.round().value_counts(normalize=True)
        fine[f"{a_}-{b_}"] = {"bpm_mode": float(vc.index[0]), "bpm_mode_share": float(vc.iloc[0]),
                              "gate_mean": float(w.beat_gate.mean())}
    res["H_sections_fine"] = fine
    res["H_scope"] = ("host replay of the BOOM source file (runs/P2-20261014T071112Z/permitted/raw/boom-40k-mono.s16) "
                      "through the product ASRC + InhouseAudio + InhouseWaveformRenderer (tests/host/p3_tempo_probe.cpp); "
                      "not the room microphone, not M85 execution; the device's in-house tempo is not recorded in the snapshots")

    # time series for the per-song figure row
    ts = {}
    for k, R in data.items():
        a_, b_ = WINDOWS[k]
        rows = [r for r in snaps[k] if a_ - 2 <= r["t"] <= b_ + 2]
        ts[k] = {"donor_t": [r["t"] for r in rows], "donor_conf": [r["donor_conf"] for r in rows],
                 "donor_bpm": [r["donor_bpm"] for r in rows], "beat_locked": [bool(r["beat_locked"]) for r in rows]}
    (out / "timeseries.json").write_text(json.dumps(ts) + "\n")
    (out / "iteration2.json").write_text(json.dumps(res, indent=2) + "\n")
    print(json.dumps(res, indent=1))


if __name__ == "__main__":
    main(Path(sys.argv[1]))
