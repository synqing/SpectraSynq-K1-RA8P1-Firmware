#!/usr/bin/env python3
"""Lane P3 Q3 correction: does the light follow the music's onsets in BOOM 55.5-75 s?

The first Q3 answer compared each autocorrelation with phase-randomised surrogates.
Those keep the power spectrum, hence (almost exactly) the autocorrelation itself, so
that "chance level" tested nothing about periodicity. It is retired here, not
regenerated; p3-answers.json stays as it ran.

Replacement test (reads analysis/p3-data.json, writes analysis/q3-correction.json):
  * statistic = max Pearson r between the detrended received-audio onset envelope and
    the detrended camera band, light lagging audio by 0-500 ms;
  * null = the same statistic after circularly shifting the audio by 2 s .. T-2 s
    (keeps both signals' own rhythm, breaks only their alignment);
  * planted positive control = the camera band, circularly shifted by half its length
    (decorrelated from the music), plus the onset envelope delayed 100 ms at PLANT_SCALE
    x its std; the test must detect it on both bands, or it is blind (exit 1).
Also reports the onset-envelope autocorrelation at 3/8 and 5/8 of a 138 BPM bar and
the light's largest |autocorrelation| between 0.4 and 1.2 s, the Q1 build-vs-drop
wire brightness, and per song the Spearman rank correlation of each board's wire
brightness with the device's raw RMS and AGC output (peak_scaled) during music. Camera = iPhone with auto-exposure. Agent cannot listen.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
from scipy.stats import spearmanr

WIN = (55.5, 75.0)
FS = 30.0
MAX_LAG_S = 0.5
SHIFTS = 1000


def detrend(x, fs):
    """Remove a 1 s moving mean. Pads with edge values: np.convolve(mode="same") pads
    with zeros, which made spikes at both ends that dominated correlations and ACFs
    (P3 iteration-2 review N1). Results before iteration 3 used the zero-padded form."""
    k = max(1, int(fs))
    x = np.asarray(x, dtype=np.float64)
    pad = np.pad(x, (k // 2, k - 1 - k // 2), mode="edge")
    y = x - np.convolve(pad, np.ones(k) / k, mode="valid")
    return y - y.mean()


def xcorr_curve(a, v, max_lag):
    return [float(np.corrcoef(a if L == 0 else a[:-L], v if L == 0 else v[L:])[0, 1]) for L in range(max_lag + 1)]


def xcorr_max(a, v, max_lag):
    c = xcorr_curve(a, v, max_lag)
    i = int(np.argmax(c))
    return c[i], i


def acf(z):
    f = np.fft.rfft(z, n=2 * len(z))
    r = np.fft.irfft(f * np.conj(f))[: len(z)]
    return r / r[0]


def test(env, band, rng):
    max_lag = int(MAX_LAG_S * FS)
    r, lag = xcorr_max(env, band, max_lag)
    n = len(env)
    lo, hi = int(2 * FS), n - int(2 * FS)
    null = [xcorr_max(np.roll(env, int(rng.integers(lo, hi))), band, max_lag)[0] for _ in range(SHIFTS)]
    null = np.array(null)
    return {"max_r": r, "lag_s": lag / FS, "null_p95": float(np.percentile(null, 95)),
            "p_value": float((1 + np.sum(null >= r)) / (1 + len(null))), "detected": bool(r > np.percentile(null, 95)),
            "curve": xcorr_curve(env, band, max_lag)}


PLANT_SCALE = 0.5


def main(out: Path, out_name: str = "q3-correction.json"):
    data = json.loads((out / "p3-data.json").read_text())
    B = data["boom"]
    at, adb = np.array(B["audio"]["t"]), np.array(B["audio"]["rms_dbfs"])
    vt = np.array(B["video"]["t"])
    m = (vt >= WIN[0]) & (vt <= WIN[1])
    t = vt[m]
    am = (at >= WIN[0] - 0.1) & (at <= WIN[1] + 0.1)
    onset20 = np.maximum(0, np.diff(adb[am], prepend=adb[am][0]))
    env = detrend(np.interp(t, at[am], onset20), FS)
    rng = np.random.default_rng(7)
    res = {"window_s": WIN, "fs_hz": FS, "frames": int(m.sum()), "max_lag_s": MAX_LAG_S, "shifts": SHIFTS,
           "retired": "q3 null_p95 / exceeds_null_fraction in p3-answers.json (phase-randomised surrogates keep the autocorrelation)"}
    for name in ("upper_band", "lower_band"):
        band = detrend(np.array(B["video"][name])[m], FS)
        row = test(env, band, rng)
        # Positive control (iteration 3, review N2): plant into circularly SHIFTED light,
        # so the control does not inherit the real effect and can fail on either band.
        base = np.roll(band, len(band) // 2)
        planted = base + PLANT_SCALE * base.std() / env.std() * np.concatenate([np.zeros(3), env[:-3]])
        row["planted_100ms_half_std"] = test(env, planted, rng)
        row["shifted_light_alone"] = test(env, base, rng)
        ra = acf(band)
        lags = np.arange(len(ra)) / FS
        sel = (lags >= 0.4) & (lags <= 1.2)
        row["acf_abs_max_0p4_1p2"] = float(np.max(np.abs(ra[sel])))
        row["acf_0_1p2"] = ra[: int(1.2 * FS) + 1].tolist()
        res["light_" + name] = row
    a20 = detrend(onset20[(at[am] >= WIN[0]) & (at[am] <= WIN[1])], 20.0)
    ra = acf(a20)
    eighth = 60.0 / 138.0 / 2
    res["audio_onset_acf"] = {f"{k}_eighths_{k * eighth:.3f}s": float(ra[int(round(k * eighth * 20))]) for k in (2, 3, 5, 8)}
    res["audio_onset_acf_0_1p2"] = ra[: int(1.2 * 20) + 1].tolist()
    res["audio_onset_acf_abs_max_0p4_1p2"] = float(np.max(np.abs(ra[int(0.4 * 20):int(1.2 * 20) + 1])))
    q1 = {}
    for key, which in (("lower", "w0"), ("upper", "w1")):
        tt = np.array([s["t_w0" if which == "w0" else "t_w1"] for s in B["samples"] if s[which]])
        bb = np.array([s[which]["brightness"] for s in B["samples"] if s[which]])
        build = bb[(tt >= 45) & (tt <= 50)].mean(); drop = bb[(tt >= 55.5) & (tt <= 60)].mean()
        q1[key] = {"build_45_50_mean": float(build), "drop_55p5_60_mean": float(drop), "drop_over_build": float(drop / build)}
    apow = 10 ** (adb / 10)
    q1["audio_drop_over_build_db"] = float(10 * np.log10(apow[(at >= 55.5) & (at <= 60)].mean() / apow[(at >= 45) & (at <= 50)].mean()))
    for f in ("inhouse_raw_rms", "inhouse_peak_scaled"):
        tt = np.array([s["t_mir"] for s in B["samples"] if s.get(f) is not None])
        vv = np.array([s[f] for s in B["samples"] if s.get(f) is not None])
        build = vv[(tt >= 45) & (tt <= 50)].mean(); drop = vv[(tt >= 55.5) & (tt <= 60)].mean()
        q1[f] = {"build_45_50_mean": float(build), "drop_55p5_60_mean": float(drop), "drop_over_build": float(drop / build)}
    res["q1_build_vs_drop_wire"] = q1
    lt = {}
    for k, R in data.items():
        S = [s for s in R["samples"] if s["w0"] and s["w1"] and s.get("inhouse_raw_rms") is not None
             and 0 <= s["t_mir"] <= R["fixture_s"]]
        raw = np.array([s["inhouse_raw_rms"] for s in S]); agc = np.array([s["inhouse_peak_scaled"] for s in S])
        lt[k] = {"n": len(S)}
        for b, name in (("w0", "lower"), ("w1", "upper")):
            br = np.array([s[b]["brightness"] for s in S])
            lt[k][name] = {"rho_raw_rms": float(spearmanr(raw, br)[0]), "rho_agc_peak_scaled": float(spearmanr(agc, br)[0]),
                           "median_wire_minus_mir_s": float(np.median([abs(s["t_" + b] - s["t_mir"]) for s in S]))}
    res["level_tracking_spearman"] = lt
    res["controls_ok"] = all(res[f"light_{n}"]["planted_100ms_half_std"]["detected"] for n in ("upper_band", "lower_band"))
    res["plant_scale"] = PLANT_SCALE
    (out / out_name).write_text(json.dumps(res, indent=2) + "\n")
    print(json.dumps({k: v for k, v in res.items() if not k.startswith(("light_", "audio_onset_acf_0"))}, indent=1))
    for n in ("upper_band", "lower_band"):
        r = res["light_" + n]
        print(n, {k: r[k] for k in ("max_r", "lag_s", "null_p95", "p_value", "detected")}, "planted detected:", r["planted_100ms_half_std"]["detected"])
    return 0 if res["controls_ok"] else 1


if __name__ == "__main__":
    # optional: plant scale and output name, for recorded red runs (P3 iteration 2)
    if len(sys.argv) > 2:
        PLANT_SCALE = float(sys.argv[2])
    raise SystemExit(main(Path(sys.argv[1]), sys.argv[3] if len(sys.argv) > 3 else "q3-correction.json"))
