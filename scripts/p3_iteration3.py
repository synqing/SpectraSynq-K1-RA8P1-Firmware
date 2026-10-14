#!/usr/bin/env python3
"""Lane P3 iteration 3: the measurements REVIEW-ITERATION-2.md asked for.

Writes analysis/iteration3.json (new; iteration-1/2 JSONs are not regenerated):
  A  camera-band crosstalk with the edge-padded detrend (review N1)
  B  light self-similarity at beat-related lags, with the lag-distribution percentile
  C  Q3 power curve and false positives with the edge-padded detrend
  D  Q2 silence medians (median vs median) and Dopex post-roll medians
  E  donor BPM by window
  F  Tempo board's in-house tempo: gain sweep on the song file + room-recording replay,
     effective (post-smoothstep) beat gate
  G  host-replay payload-luma periodicity and onset alignment, 55.5-75 s (song file and room)
  H  display-stage full-scale code and how often the payload reaches it
Offline, read-only. Agent cannot listen.
"""
from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

import numpy as np
import pandas as pd
from scipy.stats import spearmanr

sys.path.insert(0, str(Path(__file__).resolve().parent))
import p3_q3_alignment as q3  # noqa: E402  (edge-padded detrend since iteration 3)
import p3_agc_trace as tr  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
CAP = ROOT / "_scratch/waveform-migration-001/canonical-audio-v01"
EVID = ROOT / "docs/evidence/P3"
DIRS = {"boom": "boom-admission-fix-candidate-001", "sonic": "sonic-6-30-admission-fix-candidate-001",
        "dopex": "dopex-admission-fix-holdout-001"}
BEAT = 60.0 / 138.0
LAGS = {"3/8 bar": 1.5 * BEAT, "one beat": BEAT, "2 beats": 2 * BEAT, "5/8 bar": 2.5 * BEAT, "one bar": 4 * BEAT}


def series(R, field, which=None):
    t, v = [], []
    for s in R["samples"]:
        val = s[field] if which is None else (s[field] or {}).get(which)
        tt = s["t_mir"] if which is None else s["t_w0" if field == "w0" else "t_w1"]
        if val is not None:
            t.append(tt); v.append(val)
    return np.array(t), np.array(v, dtype=float)


def acf_report(y, fs):
    r = q3.acf(q3.detrend(y, fs))
    lags = np.arange(len(r)) / fs
    ref = (lags >= 0.3) & (lags <= 2.2)
    out = {}
    for name, L in LAGS.items():
        i = int(round(L * fs))
        out[name] = {"acf": float(r[i]), "lag_s": float(lags[i]),
                     "percentile_among_lags_0p3_2p2": float((r[ref] < r[i]).mean() * 100)}
    sel = (lags >= 0.4) & (lags <= 1.2)
    out["max_abs_0p4_1p2"] = float(np.abs(r[sel]).max())
    return out


def tempo_sections(u):
    secs = {}
    for a, b in ((7, 33), (33, 43), (55.5, 57), (57, 67), (67, 75)):
        w = u[(u.t_s >= a) & (u.t_s < b)]
        vc = w.tempo_bpm.round().value_counts(normalize=True)
        secs[f"{a}-{b}"] = {"bpm_mode": float(vc.index[0]), "share": float(vc.iloc[0]),
                            "gate_effective_mean": float(w.beat_gate.mean())}
    d = u[(u.t_s >= 55.5) & (u.t_s < 75)]
    vc = d.tempo_bpm.round().value_counts(normalize=True)
    secs["drop_55p5_75"] = {"share_near_138": float(((d.tempo_bpm >= 130) & (d.tempo_bpm <= 146)).mean()),
                            "top_bpm_shares": {str(k): float(v) for k, v in vc.head(3).items()}}
    return secs


def main(out: Path):
    data = json.loads((out / "p3-data.json").read_text())
    res = {"detrend": "edge-padded 1 s moving mean (p3_q3_alignment.detrend, iteration 3)"}
    B = data["boom"]
    vt = np.array(B["video"]["t"]); up = np.array(B["video"]["upper_band"]); lo = np.array(B["video"]["lower_band"])
    m = (vt >= 55.5) & (vt <= 75)
    res["A_crosstalk_55p5_75"] = {"bands_r_detrended": float(np.corrcoef(q3.detrend(up[m], 30), q3.detrend(lo[m], 30))[0, 1]),
                                  "bands_r_raw": float(np.corrcoef(up[m], lo[m])[0, 1])}
    res["B_camera_light_acf_55p5_75"] = {"upper_edge_band": acf_report(up[m], 30.0), "lower_edge_band": acf_report(lo[m], 30.0)}

    # C. power and false positives (planted into decorrelated light), edge-padded detrend
    at = np.array(B["audio"]["t"]); adb = np.array(B["audio"]["rms_dbfs"])
    tv = vt[m]; am = (at >= 55.4) & (at <= 75.1)
    onset = np.maximum(0, np.diff(adb[am], prepend=adb[am][0]))
    env = q3.detrend(np.interp(tv, at[am], onset), 30.0)
    band = q3.detrend(lo[m], 30.0)
    rng = np.random.default_rng(11)
    q3.SHIFTS = 300
    power = {}
    for scale in (0.1, 0.2, 0.3, 0.5):
        hits = 0
        for _ in range(20):
            base = np.roll(band, int(rng.integers(120, len(band) - 120)))
            lag = int(rng.integers(0, 11))
            hits += q3.test(env, base + scale * base.std() / env.std() * np.concatenate([np.zeros(lag), env[:len(env) - lag]]), rng)["detected"]
        power[str(scale)] = hits
    fp = sum(q3.test(env, np.roll(band, int(rng.integers(120, len(band) - 120))), rng)["detected"] for _ in range(40))
    q3.SHIFTS = 1000
    res["C_q3_power"] = {"detected_of_20": power, "false_positives_of_40": fp,
                         "camera_onset_test": json.loads((out / "q3-iteration3.json").read_text())["light_lower_band"]["p_value"]}

    # D. Q2 medians and Dopex post-roll medians
    q2 = {}
    for k, R in data.items():
        end = R["fixture_s"]; q2[k] = {}
        for w, name in (("w0", "lower"), ("w1", "upper")):
            t, v = series(R, w, "brightness")
            sil = (t < -0.3) | (t > end + 0.5); mus = (t >= 2) & (t <= end - 0.5)
            mm, sm = float(np.median(v[mus])), float(np.median(v[sil]))
            q2[k][name] = {"music_median_permille": mm * 1000, "silence_median_permille": sm * 1000,
                           "music_over_silence": (mm / sm) if sm > 0 else None, "n_music": int(mus.sum()), "n_silence": int(sil.sum())}
    D = data["dopex"]; td, rd = series(D, "inhouse_raw_rms"); _, pd_ = series(D, "inhouse_peak_scaled"); _, sf = series(D, "inhouse_silence")
    post = td > D["fixture_s"] + 0.5; mus = (td >= 0) & (td <= D["fixture_s"])
    t0, w0 = series(D, "w0", "brightness")
    res["D_q2_medians"] = q2
    res["D_dopex_post_roll"] = {"median_peak_scaled": float(np.median(pd_[post])), "median_raw_rms": float(np.median(rd[post])),
                                "music_median_peak_scaled": float(np.median(pd_[mus])), "music_median_raw_rms": float(np.median(rd[mus])),
                                "silence_flag_fraction": float(sf[post].mean()),
                                "first_dark_lower_payload_s": float(t0[(t0 > D["fixture_s"]) & (w0 == 0)].min()),
                                "first_silence_flag_s": float(td[post & (sf > 0)].min()), "n": int(post.sum()),
                                "darkness_gate": "[INFERENCE] the donor admission gate (palette_runtime.cpp:258-264; composed_scene_runtime.h:26-33, PCM peak^2 0.0048 open / 0.0034 close): consistent with donor PCM RMS 0.012-0.025 after 76 s, but pcm_peak_squared is not recorded in the snapshots"}

    # E. donor BPM by window
    d = CAP / DIRS["boom"]
    ev = json.loads((d / "events.json").read_text()); ap = next(e for e in ev if e["event"] == "audio_process_started")
    tz = ap.get("started", ap["mono"]); rows = []
    for sd in sorted(d.glob("sample-*")):
        mj = json.loads((sd / "mir-snapshot.json").read_text())
        rows.append((mj["mono"] - tz, mj["body"].get("donor_dominant_tempo_bpm"), mj["body"].get("donor_tempo_confidence")))
    rows = np.array([(a, b, c) for a, b, c in rows if b is not None], dtype=float)
    res["E_donor_bpm_boom"] = {w: {"bpm_median": float(np.median(rows[(rows[:, 0] >= a) & (rows[:, 0] < b), 1])),
                                   "conf_max": float(rows[(rows[:, 0] >= a) & (rows[:, 0] < b), 2].max())}
                               for w, (a, b) in (("28-55.5", (28, 55.5)), ("55.5-75", (55.5, 75)))}

    # F. tempo: gain sweep (song file) + room-recording replay (upper board, mode 18)
    work = Path("/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/iter3-tempo")
    exe = tr.build(work); x = tr.source_pcm(); sweep = {}
    for g in (0.70, 0.85, 1.00, 1.155, 1.30, 1.60):
        csv = tr.run(exe, x, g, work, f"g{g}")
        u = pd.read_csv(csv); sweep[str(g)] = tempo_sections(u[u.board == 1])
    room = pd.read_csv(EVID / "agc-trace-boom-recording.csv")
    res["F_tempo"] = {"song_file_gain_sweep": sweep, "room_recording_replay": tempo_sections(room[room.board == 1]),
                      "gate": "effective gate after smoothstep (inhouse_waveform.cpp:347-348)",
                      "drives": "mode 18 travel rate 24*(bpm/60)*(1+0.5*cos(2*pi*phase)) px/s (inhouse_waveform.cpp:349-352)"}

    # G. host payload-luma periodicity and onset alignment, 55.5-75 s
    g_ = {}
    for name, path in (("song_file", EVID / "agc-trace-boom-source.csv"), ("room_recording", EVID / "agc-trace-boom-recording.csv")):
        h = pd.read_csv(path); g_[name] = {}
        for b, bname in ((0, "lower"), (1, "upper")):
            w = h[(h.board == b) & (h.t_s >= 55.5) & (h.t_s <= 75)]
            luma = w.luma.values
            entry = acf_report(luma, 100.0)
            t30 = np.arange(55.5, 75.0, 1 / 30.0)
            l30 = q3.detrend(np.interp(t30, w.t_s.values, luma), 30.0)
            e30 = q3.detrend(np.interp(t30, at[am], onset), 30.0)
            al = q3.test(e30, l30, np.random.default_rng(5))
            entry["onset_alignment"] = {k: al[k] for k in ("max_r", "lag_s", "null_p95", "p_value", "detected")}
            g_[name][bname] = entry
    res["G_host_payload_luma_55p5_75"] = g_
    res["G_scope"] = ("host replay (tests/host/p3_tempo_probe.cpp, v8 controls, display stage, x0.25); device payload snapshots "
                      "are 0.3-0.5 s apart and cannot resolve a 0.435 s beat")

    # H. display-stage full-scale code
    fs_code = 65535 * 0.25 * (0.75 + 0.25 * np.tanh(1.0))
    hits = {}
    for k, dname in DIRS.items():
        hits[k] = {}
        for name, stem in (("lower", "wire-snapshot"), ("upper", "wire-snapshot-top")):
            n = c = 0
            for sd in sorted((CAP / dname).glob("sample-*")):
                raw = (sd / f"{stem}.bin").read_bytes()
                if len(raw) < 1504:
                    continue
                v = struct.unpack(">480H", raw[544:1504]); n += 1; c += max(v) >= 15407
            hits[k][name] = {"snapshots": n, "at_full_scale_code": c}
    res["H_display_full_scale"] = {"full_scale_code": float(fs_code), "formula": "65535 x 0.25 x (0.75 + 0.25 tanh(1))", "hits": hits}

    (out / "iteration3.json").write_text(json.dumps(res, indent=2) + "\n")
    print(json.dumps(res, indent=1))


if __name__ == "__main__":
    main(Path(sys.argv[1]))
