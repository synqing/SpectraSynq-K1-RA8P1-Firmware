#!/usr/bin/env python3
"""Lane P3 expressiveness audit on the three 14 Oct recordings (offline, read-only).

For BOOM 0-75, Sonic 6-30 and Dopex 0-75 recorded on the Titan with lower Waveform
Hybrid K1 32 / upper Waveform Tempo 18 (build 9ab90873, 14 Oct 2026), measures along
time: received-audio RMS (iPhone microphone track), the device's consumed level state
(in-house raw RMS, VU, peak_scaled AGC output, silence flag; donor level), the rendered
per-board output read from the actual wire snapshots (GRB48), and board-region light in
the video. Answers the brief's four questions with numbers and locates the stage that
flattens dynamics. Musical/aesthetic verdict: agent cannot listen.
"""
from __future__ import annotations

import argparse
import json
import math
import struct
import subprocess
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
CAP = ROOT / "_scratch/waveform-migration-001/canonical-audio-v01"
RECS = {
    "boom": {"dir": "boom-admission-fix-candidate-001", "song_offset": 0.0, "label": "BOOM (MR.BLACK remix) 0–75 s"},
    "sonic": {"dir": "sonic-6-30-admission-fix-candidate-001", "song_offset": 6.0, "label": "Sonic Samba 6–30 s"},
    "dopex": {"dir": "dopex-admission-fix-holdout-001", "song_offset": 0.0, "label": "Dopex 0–75 s (holdout)"},
}
LUMA = np.array([0.2126, 0.7152, 0.0722])


def ffmpeg_audio(mov: Path, rate=16000):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(mov), "-vn", "-ac", "1", "-ar", str(rate),
                          "-f", "f32le", "-"], capture_output=True, check=True).stdout
    return np.frombuffer(raw, dtype="<f4"), rate


def ffmpeg_gray(mov: Path, w=320, h=180):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(mov), "-vf", f"scale={w}:{h},format=gray",
                          "-f", "rawvideo", "-"], capture_output=True, check=True).stdout
    return np.frombuffer(raw, dtype=np.uint8).reshape(-1, h, w).astype(np.float32) / 255.0


def decode_wire(path: Path):
    b = path.read_bytes()
    if len(b) < 1504:
        return None
    vals = np.array(struct.unpack(">480H", b[544:1504]), dtype=np.float64).reshape(160, 3)  # G,R,B
    rgb = vals[:, [1, 0, 2]] / 65535.0
    lum = rgb @ LUMA
    mean_rgb = rgb.mean(axis=0)
    return {"brightness": float(lum.mean()), "peak": float(lum.max()),
            "lit_fraction": float((lum > 0.01).mean()), "mean_rgb": mean_rgb.tolist()}


def rms_series(x, rate, win_s=0.05):
    n = int(rate * win_s)
    k = len(x) // n
    seg = x[: k * n].reshape(k, n)
    rms = np.sqrt((seg.astype(np.float64) ** 2).mean(axis=1))
    t = (np.arange(k) + 0.5) * win_s
    return t, rms


def window_mean(t, v, a, b):
    m = (t >= a) & (t <= b)
    return float(np.mean(v[m])) if m.any() else float("nan"), int(m.sum())


def acf_peak(x, fs, lo=0.30, hi=0.60, target=60.0 / 138.0, surrogates=200, seed=7):
    x = np.asarray(x, dtype=np.float64)
    k = max(1, int(fs))  # 1 s moving average detrend
    trend = np.convolve(x, np.ones(k) / k, mode="same")
    y = x - trend
    y = y - y.mean()
    if np.allclose(y, 0):
        return {"valid": False}
    def acf(z):
        f = np.fft.rfft(z, n=2 * len(z))
        r = np.fft.irfft(f * np.conj(f))[: len(z)]
        return r / r[0]
    r = acf(y)
    lags = np.arange(len(r)) / fs
    band = (lags >= lo) & (lags <= hi)
    i = int(np.argmax(np.where(band, r, -np.inf)))
    rng = np.random.default_rng(seed)
    null = []
    spec = np.fft.rfft(y)
    for _ in range(surrogates):
        ph = np.exp(1j * rng.uniform(0, 2 * np.pi, len(spec)))
        ph[0] = 1
        s = np.fft.irfft(np.abs(spec) * ph, n=len(y))
        null.append(float(np.max(acf(s)[band])))
    ti = int(round(target * fs))
    return {"valid": True, "peak_lag_s": float(lags[i]), "peak_bpm": float(60.0 / lags[i]),
            "peak_acf": float(r[i]), "acf_at_138bpm": float(r[ti]) if ti < len(r) else None,
            "null_p95": float(np.percentile(null, 95)), "exceeds_null_fraction": float(np.mean(np.array(null) >= r[i])),
            "lags": lags[: int(1.2 * fs)].tolist(), "acf": r[: int(1.2 * fs)].tolist()}


def analyse(key, spec, out):
    d = CAP / spec["dir"]
    obs = json.loads((d / "observation.json").read_text())
    av = obs["av_validation"]
    off, scale = av["source_to_movie_offset_s"], av["source_to_audio_scale"]
    ev = json.loads((d / "events.json").read_text())
    ap = next(e for e in ev if e["event"] == "audio_process_started")
    player_mono = ap.get("started", ap["mono"])
    res = {"label": spec["label"], "dir": str(d), "movie": str(d / "original.mov"), "build": None,
           "av_passed": av["passed"], "av_landmarks": av["landmarks"], "movie_offset_s": off, "scale": scale,
           "song_offset_s": spec["song_offset"], "fixture_s": obs["audio_window"]["duration_s"]}
    # received audio
    x, rate = ffmpeg_audio(d / "original.mov")
    tm, rms = rms_series(x, rate)
    ts = (tm - off) / scale
    res["audio"] = {"t": ts.tolist(), "rms_dbfs": (20 * np.log10(np.maximum(rms, 1e-7))).tolist()}
    # device samples
    samples = []
    for sd in sorted(d.glob("sample-*")):
        try:
            mir = json.loads((sd / "mir-snapshot.json").read_text())
            wj = json.loads((sd / "wire-snapshot.json").read_text())
            tj = json.loads((sd / "wire-snapshot-top.json").read_text())
        except (OSError, ValueError):
            continue
        body = mir.get("body", {})
        inh = body.get("inhouse", {})
        w0 = decode_wire(sd / "wire-snapshot.bin")
        w1 = decode_wire(sd / "wire-snapshot-top.bin")
        if res["build"] is None:
            res["build"] = mir.get("identity", {}).get("build")
        samples.append({
            "t_mir": mir["mono"] - player_mono, "t_w0": wj["mono"] - player_mono, "t_w1": tj["mono"] - player_mono,
            "inhouse_raw_rms": inh.get("raw_rms"), "inhouse_vu": inh.get("vu"), "inhouse_peak_scaled": inh.get("peak_scaled"),
            "inhouse_raw_peak_pcm": inh.get("raw_peak_pcm"), "inhouse_silence": inh.get("silence"), "inhouse_mood": inh.get("mood"),
            "donor_vu": body.get("vu"), "donor_peak": body.get("peak"), "donor_pcm_rms": body.get("pcm_rms"),
            "w0": w0, "w1": w1,
            "mode0": (wj.get("score") or {}).get("mode"), "mode1": (tj.get("score") or {}).get("mode")})
    res["samples"] = samples
    # video bands
    g = ffmpeg_gray(d / "original.mov")
    mean = g.mean(axis=0)
    # Board bands read from the mean frames of these three recordings (same framing,
    # 320x180 scale; analysis/mean-frames.png): plate interior x 10-300; upper edge
    # band just below the top rail, lower edge band just above the bottom rail.
    up, lo_, c0, c1 = (48, 57), (76, 85), 10, 300
    top = g[:, up[0]:up[1], c0:c1].mean(axis=(1, 2))
    bot = g[:, lo_[0]:lo_[1], c0:c1].mean(axis=(1, 2))
    vt = (np.arange(len(g)) / 30.0 - off) / scale
    res["video"] = {"upper_rows": list(up), "lower_rows": list(lo_), "cols": [c0, c1], "fps": 30,
                    "t": vt.tolist(), "upper_band": top.tolist(), "lower_band": bot.tolist(),
                    "clipped_fraction_upper": float((g[:, up[0]:up[1], c0:c1] >= 0.98).mean()),
                    "clipped_fraction_lower": float((g[:, lo_[0]:lo_[1], c0:c1] >= 0.98).mean())}
    np.save(out / f"{key}-gray-mean.npy", mean)
    return res


def series(res, field, which=None):
    t, v = [], []
    for s in res["samples"]:
        val = s[field] if which is None else (s[field] or {}).get(which)
        tt = s["t_mir"] if which is None else s["t_w0" if field == "w0" else "t_w1"]
        if val is not None:
            t.append(tt); v.append(val)
    return np.array(t), np.array(v, dtype=np.float64)


def db10(r):
    return 10 * math.log10(r) if r and r > 0 else float("nan")


def dynamic_range_db(v, power=False):
    v = np.asarray(v, dtype=np.float64)
    v = v[np.isfinite(v)]
    if len(v) < 5:
        return None
    p10, p90 = np.percentile(v, 10), np.percentile(v, 90)
    if p10 <= 0:
        p10 = max(p10, 1e-6)
    return (20 if not power else 10) * math.log10(p90 / p10) if p90 > 0 else None


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    data = {k: analyse(k, v, a.out) for k, v in RECS.items()}
    q = {}
    # Q1 BOOM quiet 50-55 vs drop 55.5-60
    B = data["boom"]
    at, adb = np.array(B["audio"]["t"]), np.array(B["audio"]["rms_dbfs"])
    apow = 10 ** (adb / 10)
    qa, _ = window_mean(at, apow, 50.0, 55.0); da, _ = window_mean(at, apow, 55.5, 60.0)
    q1 = {"audio_power_ratio_drop_over_quiet": da / qa, "audio_db": db10(da / qa)}
    for name, field, which in [("inhouse_raw_rms", "inhouse_raw_rms", None), ("inhouse_peak_scaled", "inhouse_peak_scaled", None),
                               ("donor_vu", "donor_vu", None), ("wire_lower_brightness", "w0", "brightness"),
                               ("wire_upper_brightness", "w1", "brightness")]:
        t, v = series(B, field, which)
        m0, n0 = window_mean(t, v, 50.0, 55.0); m1, n1 = window_mean(t, v, 55.5, 60.0)
        q1[name] = {"quiet_mean": m0, "drop_mean": m1, "ratio": m1 / m0 if m0 else None, "n": [n0, n1]}
    vt = np.array(B["video"]["t"])
    for name in ("upper_band", "lower_band"):
        v = np.array(B["video"][name])
        m0, _ = window_mean(vt, v, 50.0, 55.0); m1, _ = window_mean(vt, v, 55.5, 60.0)
        q1["video_" + name] = {"quiet_mean": m0, "drop_mean": m1, "ratio": m1 / m0 if m0 else None}
    q["q1"] = q1
    # Q2 silence: pre-roll (< -0.3 s) and post-roll (> fixture + 0.5 s)
    q2 = {}
    for k, R in data.items():
        end = R["fixture_s"]
        row = {}
        for name, field, which in [("wire_lower", "w0", "brightness"), ("wire_upper", "w1", "brightness"),
                                   ("inhouse_raw_rms", "inhouse_raw_rms", None)]:
            t, v = series(R, field, which)
            sil = (t < -0.3) | (t > end + 0.5)
            mus = (t >= 2) & (t <= end - 0.5)
            row[name] = {"silence_mean": float(v[sil].mean()) if sil.any() else None, "n_silence": int(sil.sum()),
                         "music_median": float(np.median(v[mus])) if mus.any() else None, "n_music": int(mus.sum())}
        t, v = series(R, "inhouse_silence")
        sil = (t < -0.3) | (t > end + 0.5)
        row["silence_flag_fraction_in_silence"] = float(np.mean(v[sil])) if sil.any() else None
        vt = np.array(R["video"]["t"])
        for name in ("upper_band", "lower_band"):
            v = np.array(R["video"][name]); sil = (vt < -0.3) | (vt > end + 0.5); mus = (vt >= 2) & (vt <= end - 0.5)
            row["video_" + name] = {"silence_mean": float(v[sil].mean()) if sil.any() else None,
                                    "music_median": float(np.median(v[mus]))}
        q2[k] = row
    q["q2"] = q2
    # Q3 periodicity 55.5-75 in BOOM, video bands (30 fps) + audio envelope reference
    q3 = {}
    vt = np.array(B["video"]["t"])  # BOOM timeline (Q2 loop rebinds vt)
    for name in ("upper_band", "lower_band"):
        v = np.array(B["video"][name]); m = (vt >= 55.5) & (vt <= 75.0)
        q3["video_" + name] = acf_peak(v[m], 30.0)
    m = (at >= 55.5) & (at <= 75.0)
    env = np.maximum(0, np.diff(adb[m], prepend=adb[m][0]))
    q3["audio_onset_envelope"] = acf_peak(env, 20.0)
    q["q3"] = q3
    # Q4 weak Sonic vs loud BOOM (and Dopex): distributions
    q4 = {}
    for k, R in data.items():
        lo, hi = (28.0, 75.0) if k != "sonic" else (0.5, 23.5)
        at_, adb_ = np.array(R["audio"]["t"]), np.array(R["audio"]["rms_dbfs"])
        ma = (at_ >= lo) & (at_ <= hi)
        row = {"window_s": [lo, hi], "audio_rms_dbfs_median": float(np.median(adb_[ma])),
               "audio_rms_dbfs_p10_p90": [float(np.percentile(adb_[ma], 10)), float(np.percentile(adb_[ma], 90))]}
        for name, field, which in [("wire_lower", "w0", "brightness"), ("wire_upper", "w1", "brightness"),
                                   ("inhouse_peak_scaled", "inhouse_peak_scaled", None), ("inhouse_raw_rms", "inhouse_raw_rms", None)]:
            t, v = series(R, field, which); mm = (t >= lo) & (t <= hi)
            row[name] = {"median": float(np.median(v[mm])), "p10": float(np.percentile(v[mm], 10)),
                         "p90": float(np.percentile(v[mm], 90)), "n": int(mm.sum())}
        q4[k] = row
    q["q4"] = q4
    # Stage dynamic range (p90/p10) over scored window per recording
    stages = {}
    for k, R in data.items():
        lo, hi = (28.0, 75.0) if k != "sonic" else (0.5, 23.5)
        at_, adb_ = np.array(R["audio"]["t"]), np.array(R["audio"]["rms_dbfs"])
        ma = (at_ >= lo) & (at_ <= hi)
        row = {"received_audio_rms": float(np.percentile(adb_[ma], 90) - np.percentile(adb_[ma], 10))}
        for name, field, which in [("device_raw_rms", "inhouse_raw_rms", None), ("device_peak_scaled_agc", "inhouse_peak_scaled", None),
                                   ("wire_lower_brightness", "w0", "brightness"), ("wire_upper_brightness", "w1", "brightness")]:
            t, v = series(R, field, which); mm = (t >= lo) & (t <= hi)
            row[name] = dynamic_range_db(v[mm])
        vt_ = np.array(R["video"]["t"]); mv = (vt_ >= lo) & (vt_ <= hi)
        row["video_upper_band"] = dynamic_range_db(np.array(R["video"]["upper_band"])[mv])
        row["video_lower_band"] = dynamic_range_db(np.array(R["video"]["lower_band"])[mv])
        stages[k] = row
    q["stage_dynamic_range_db_p90_over_p10"] = stages
    q["scope"] = ("Offline analysis of 14 Oct recordings on build 9ab90873 (P1/P2 images not yet recorded with music+AV). "
                  "Wire brightness = mean Rec.709 luma of the actual GRB48 wire payload (16-bit code values / 65535); "
                  "video bands = iPhone frames with auto-exposure (camera can compress); timing is host/movie time via the "
                  "AV map. Musical/aesthetic verdict: agent cannot listen.")
    (a.out / "p3-data.json").write_text(json.dumps(data))
    (a.out / "p3-answers.json").write_text(json.dumps(q, indent=1))
    print(json.dumps({k: v for k, v in q.items() if k != "q3"}, indent=1)[:6000])
    print(json.dumps({k: {kk: vv for kk, vv in v.items() if kk not in ('lags', 'acf')} for k, v in q["q3"].items()}, indent=1))


if __name__ == "__main__":
    main()
