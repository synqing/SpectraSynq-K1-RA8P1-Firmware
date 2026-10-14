#!/usr/bin/env python3
"""Lane P3 iteration 4: which musical rates the light and the music carry (REVIEW-ITERATION-3.md F1, F2, F4, F7).

Criteria are frozen in analysis/TOLERANCE-FROZEN-iteration4.json before this ran; that file is read, not edited.
Writes analysis/iteration4.json (new; iteration-1/2/3 JSONs are not regenerated):
  A  rate shares at bar / half-time / accent (3+3+2) / beat, BOOM 55.5-75 s: room audio (linear RMS and the
     accent-weighted rise envelope the earlier Q3 test used), host-replay payload luma (both boards, both inputs),
     host input level (raw_rms), both camera bands. Primary: no pre-detrend; secondary: 1 s edge-padded detrend.
  B  trial-period comb scan, 0.300-0.655 s
  C  planted synthetic check (must go red: exit 1 if the criterion cannot tell beat from accent pattern)
  D  onset-alignment test re-run with the linear RMS envelope as the music reference, both directions
  E  SUPPLEMENTARY, post hoc (not in the frozen file): peak-to-neighbour ratio at each rate, against the same ratio
     at the 7 off-grid midpoints between bar harmonics (0.8625 ... 4.3125 Hz), which no musical rate occupies
Usage: p3_iteration4.py <analysis> [--red]   (--red swaps the with-beat synthetic for the without-beat one)
Offline, read-only. Agent cannot listen.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
import pandas as pd
from scipy.signal import detrend as lin_detrend, welch

sys.path.insert(0, str(Path(__file__).resolve().parent))
import p3_q3_alignment as q3  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
EVID = ROOT / "docs/evidence/P3"
WIN = (55.5, 75.0)
BEAT_S = 60.0 / 138.0


def rate_shares(x, fs, rates, pre_detrend):
    x = np.asarray(x, dtype=np.float64)
    if pre_detrend:
        x = q3.detrend(x, fs)
    nper = int(round(8 * fs))
    f, p = welch(x, fs=fs, window="hann", nperseg=nper, noverlap=nper // 2, nfft=8 * nper, detrend="linear")
    band = (f >= 0.3) & (f <= 5.0)
    tot = p[band].sum()
    out = {k: float(p[(f >= r - 0.06) & (f <= r + 0.06)].sum() / tot) for k, r in rates.items()}
    sel = (f >= 1.6) & (f <= 3.0)
    out["peak_hz_1p6_3p0"] = float(f[sel][np.argmax(p[sel])])
    return out


def comb_scan(x, fs, pre_detrend):
    x = np.asarray(x, dtype=np.float64)
    x = q3.detrend(x, fs) if pre_detrend else lin_detrend(x)
    r = q3.acf(x - x.mean())
    lags = np.arange(len(r)) / fs
    periods = np.round(np.arange(0.300, 0.6551, 0.005), 3)
    scores = np.array([np.mean([np.interp(k * P, lags, r) for k in range(1, 5)])
                       - np.mean([np.interp((k + 0.5) * P, lags, r) for k in range(1, 5)]) for P in periods])
    i_beat = int(np.argmin(np.abs(periods - BEAT_S)))
    return {"best_period_s": float(periods[np.argmax(scores)]), "score_at_beat": float(scores[i_beat]),
            "periods_scoring_above_beat": int((scores > scores[i_beat]).sum()), "n_periods": int(len(periods)),
            "scores": [float(s) for s in scores]}


MIDPOINTS = [round(0.575 * k + 0.2875, 4) for k in range(1, 8)]


def peak_to_neighbour(x, fs, freqs):
    nper = int(round(8 * fs))
    f, p = welch(np.asarray(x, dtype=np.float64), fs=fs, window="hann", nperseg=nper, noverlap=nper // 2, nfft=8 * nper, detrend="linear")
    out = {}
    for r in freqs:
        on = p[(f >= r - 0.06) & (f <= r + 0.06)].mean()
        fl = np.median(p[((f >= r - 0.25) & (f <= r - 0.10)) | ((f >= r + 0.10) & (f <= r + 0.25))])
        out[str(r)] = float(on / fl)
    return out


def assess(x, fs, rates, crit):
    sh = rate_shares(x, fs, rates, False)
    cb = comb_scan(x, fs, False)
    lo, hi = crit["best_period_window_s"]
    present = bool(lo <= cb["best_period_s"] <= hi and sh["beat"] >= crit["min_beat_share"])
    return {"rate_share_primary": sh, "rate_share_1s_detrend": rate_shares(x, fs, rates, True),
            "comb_primary": cb, "comb_1s_detrend": {k: v for k, v in comb_scan(x, fs, True).items() if k != "scores"},
            "beat_present": present,
            "E_peak_to_neighbour_posthoc": {"rates": peak_to_neighbour(x, fs, list(rates.values())),
                                            "offgrid_controls": peak_to_neighbour(x, fs, MIDPOINTS)}}


def synthetic(fs, with_beat, seed=404):
    rng = np.random.default_rng(seed)
    n = int(round((WIN[1] - WIN[0]) * fs)); t = np.arange(n) / fs
    eighth = BEAT_S / 2
    imp = np.zeros(n)
    for j in range(int(t[-1] / eighth) + 1):
        i = int(round(j * eighth * fs))
        if i < n:
            imp[i] += 1.0 if (j % 8) in (0, 3, 6) else 0.25
    kern = np.exp(-np.arange(int(0.5 * fs)) / fs / 0.08)
    acc = np.convolve(imp, kern)[:n]
    s = acc.std()
    x = acc + rng.normal(0, 0.5 * s, n) + 0.5 * s * np.sin(2 * np.pi * 0.05 * t)
    if with_beat:
        x = x + 0.6 * s * np.sin(2 * np.pi * 2.30 * t)
    return x


def main(out: Path, red: bool) -> int:
    tol = json.loads((out / "TOLERANCE-FROZEN-iteration4.json").read_text())
    rates = tol["rates_hz"]
    crit = {"best_period_window_s": (0.425, 0.445), "min_beat_share": 0.08}
    assert tol["beat_present_criterion"] == "comb-scan best period within 0.425-0.445 s AND primary beat-rate share >= 0.08"
    res = {"frozen": "TOLERANCE-FROZEN-iteration4.json", "window_s": WIN, "rates_hz": rates, "criterion": tol["beat_present_criterion"]}

    # C first: the criterion must tell a real beat from the accent pattern's own 4th harmonic
    planted = {}
    ok = True
    for fs in tol["planted_check"]["rates_checked_hz"]:
        w = assess(synthetic(fs, not red), fs, rates, crit)
        wo = assess(synthetic(fs, False), fs, rates, crit)
        planted[str(fs)] = {"with_beat": {"beat_present": w["beat_present"], "beat_share": w["rate_share_primary"]["beat"],
                                          "best_period_s": w["comb_primary"]["best_period_s"]},
                            "without_beat": {"beat_present": wo["beat_present"], "beat_share": wo["rate_share_primary"]["beat"],
                                             "best_period_s": wo["comb_primary"]["best_period_s"],
                                             "accent_share": wo["rate_share_primary"]["accent_3_3_2"]}}
        planted[str(fs)]["passed"] = bool(w["beat_present"] and not wo["beat_present"])
        ok &= planted[str(fs)]["passed"]
    res["C_planted_check"] = {"red_run": red, "passed": bool(ok), "cases": planted,
                              "note": "a series' beat_present is admissible only if the planted check passed at that series' own rate"}

    # A + B on the real series
    S = {}
    data = json.loads((out / "p3-data.json").read_text())
    B = data["boom"]
    at, adb = np.array(B["audio"]["t"]), np.array(B["audio"]["rms_dbfs"])
    ma = (at >= WIN[0]) & (at <= WIN[1])
    am = (at >= WIN[0] - 0.1) & (at <= WIN[1] + 0.1)
    lin = 10 ** (adb / 20.0)
    accent = np.maximum(0, np.diff(adb[am], prepend=adb[am][0]))
    S["room_audio_linear_rms_20hz"] = assess(lin[ma], 20.0, rates, crit)
    S["room_audio_accent_rise_envelope_20hz"] = assess(accent[(at[am] >= WIN[0]) & (at[am] <= WIN[1])], 20.0, rates, crit)
    for name, path in (("song_file", EVID / "agc-trace-boom-source.csv"), ("room_recording", EVID / "agc-trace-boom-recording.csv")):
        h = pd.read_csv(path)
        for b, bname in ((0, "lower_hybrid_k1"), (1, "upper_tempo")):
            w = h[(h.board == b) & (h.t_s >= WIN[0]) & (h.t_s <= WIN[1])]
            S[f"host_luma_{name}_{bname}"] = assess(w.luma.values, 100.0, rates, crit)
        w = h[(h.board == 0) & (h.t_s >= WIN[0]) & (h.t_s <= WIN[1])]
        S[f"host_input_raw_rms_{name}"] = assess(w.raw_rms.values, 100.0, rates, crit)
    vt = np.array(B["video"]["t"]); mv = (vt >= WIN[0]) & (vt <= WIN[1])
    for band in ("lower_band", "upper_band"):
        S[f"camera_{band.replace('_band', '_edge_band')}_30hz"] = assess(np.array(B["video"][band])[mv], 30.0, rates, crit)
    for k, v in S.items():
        fs_k = "20" if k.endswith("_20hz") else "30" if k.endswith("_30hz") else "100"
        v["beat_present_admissible"] = planted[fs_k]["passed"]
    res["A_B_series"] = S

    # D: alignment with the linear RMS reference (and the accent envelope, as before), both directions
    t30 = vt[mv]
    refs = {"linear_rms": q3.detrend(np.interp(t30, at[am], lin[am]), 30.0),
            "accent_rise_envelope": q3.detrend(np.interp(t30, at[am], accent), 30.0)}
    targets = {f"camera_{b.replace('_band', '_edge_band')}": q3.detrend(np.array(B["video"][b])[mv], 30.0) for b in ("lower_band", "upper_band")}
    for name, path in (("song_file", EVID / "agc-trace-boom-source.csv"), ("room_recording", EVID / "agc-trace-boom-recording.csv")):
        h = pd.read_csv(path)
        for b, bname in ((0, "lower"), (1, "upper")):
            w = h[(h.board == b) & (h.t_s >= WIN[0]) & (h.t_s <= WIN[1])]
            targets[f"host_luma_{name}_{bname}"] = q3.detrend(np.interp(t30, w.t_s.values, w.luma.values), 30.0)
    D = {}
    keep = ("max_r", "lag_s", "null_p95", "p_value", "detected")
    for rn, env in refs.items():
        D[rn] = {}
        for tn, y in targets.items():
            lagging = q3.test(env, y, np.random.default_rng(21))
            leading = q3.test(y, env, np.random.default_rng(22))
            D[rn][tn] = {"light_lags_music_0_500ms": {k: lagging[k] for k in keep},
                         "light_leads_music_0_500ms": {k: leading[k] for k in keep}}
    res["D_alignment"] = D
    res["D_note"] = ("Both directions use the iteration-3 test (1 s edge-padded detrend, 1,000 circular shifts). A periodic reference "
                     "makes lag and lead score alike, so a significant result is shared rhythm, not a measured response delay.")
    res["F7_note"] = "iteration-3 'percentile among lags' is a descriptive rank of one ACF value among lags of the same curve, not a significance test"

    name = "iteration4-red.json" if red else "iteration4.json"
    (out / name).write_text(json.dumps(res, indent=2) + "\n")
    print(json.dumps(res["C_planted_check"], indent=1))
    for k, v in S.items():
        sh = v["rate_share_primary"]
        print(f"{k:44s} bar {sh['bar']*100:5.1f} half {sh['half_time']*100:5.1f} acc {sh['accent_3_3_2']*100:5.1f} "
              f"beat {sh['beat']*100:5.1f} peak {sh['peak_hz_1p6_3p0']:.3f} comb {v['comb_primary']['best_period_s']:.3f} "
              f"({v['comb_primary']['periods_scoring_above_beat']} above) present={v['beat_present']} admissible={v['beat_present_admissible']}")
        e = v["E_peak_to_neighbour_posthoc"]
        print(f"{'':44s} peak/nbr rates {[round(a, 2) for a in e['rates'].values()]} controls max {max(e['offgrid_controls'].values()):.2f} "
              f"all {[round(a, 2) for a in e['offgrid_controls'].values()]}")
    for rn, dd in res["D_alignment"].items():
        for tn, v in dd.items():
            a, b = v["light_lags_music_0_500ms"], v["light_leads_music_0_500ms"]
            print(f"D {rn:22s} {tn:34s} lag r {a['max_r']:.3f} @{a['lag_s']*1000:.0f}ms p {a['p_value']:.3f} | lead r {b['max_r']:.3f} @{b['lag_s']*1000:.0f}ms p {b['p_value']:.3f}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main(Path(sys.argv[1]), "--red" in sys.argv[2:]))
