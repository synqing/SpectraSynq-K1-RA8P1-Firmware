#!/usr/bin/env python3
"""Lane P4 step 3 gate: in-house analysis at 24 kHz / 180 against 12.8 kHz / 96.

Applies runs/P4-20261014T085434Z/permitted/TOLERANCE-FROZEN-analysis24.json (read, never edited)
to tests/host/p4_analysis24_probe.cpp on three inputs, then re-runs the planted faults, each of
which must fail its own gate. Writes analysis24-gate.json beside the frozen file. rc 0 only if
every gate passes on the real run AND every planted fault is caught. Offline. Agent cannot listen.
"""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

import numpy as np
import pandas as pd
from scipy.stats import spearmanr

sys.path.insert(0, str(Path(__file__).resolve().parent))
import p3_agc_trace as tr  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
RUN = ROOT / "_scratch/waveform-migration-001/runs/P4-20261014T085434Z/permitted"
FROZEN = RUN / "TOLERANCE-FROZEN-analysis24.json"
CANON = ROOT / "_scratch/waveform-migration-001/canonical-audio-v01"
P3DATA = ROOT / "_scratch/waveform-migration-001/runs/P3-20261014T080935Z/permitted/analysis/p3-data.json"
WORK = Path("/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p4-gate")
FS = 40000
FLAGS = ["-O2", "-fno-fast-math", "-DK1_EMOTISCOPE_PORT=1", "-Iplatform/ra8p1", "-Isrc", "-Isrc/k1"]


def build() -> tuple[Path, Path]:
    WORK.mkdir(parents=True, exist_ok=True)
    for c, o in (("platform/ra8p1/k1_asrc_24k.c", "asrc.o"), ("platform/ra8p1/k1_poly24.c", "poly.o")):
        subprocess.run(["cc", "-std=c11", *FLAGS, "-c", c, "-o", str(WORK / o)], cwd=ROOT, check=True)
    exe = WORK / "p4_analysis24_probe"
    subprocess.run(["c++", "-std=c++17", *FLAGS, "tests/host/p4_analysis24_probe.cpp", "platform/ra8p1/inhouse_audio.cpp",
                    "src/k1/core/visual/inhouse_waveform.cpp", "src/k1/core/audio/gdft_goertzel.cpp",
                    "src/k1/core/audio/tempo_tracker.cpp", "src/k1/core/audio/tempo_acf.cpp",
                    "src/k1/core/visual/product_palette.cpp", "src/k1/core/visual/product_palette_data.cpp",
                    str(WORK / "asrc.o"), str(WORK / "poly.o"), "-o", str(exe)], cwd=ROOT, check=True)
    src = WORK / "bins.cpp"
    src.write_text('#include "core/audio/gdft_goertzel.h"\n#include <cstdio>\nint main(){auto c=k1::core::audio::'
                   'makeGdftConfiguration(k1::core::audio::kProductionAudioRate);for(auto&b:c)std::printf("%.3f\\n",'
                   'b.target_frequency_hz);}\n')
    binexe = WORK / "bins"
    subprocess.run(["c++", "-std=c++17", "-Isrc", "-Isrc/k1", str(src), "src/k1/core/audio/gdft_goertzel.cpp", "-o",
                    str(binexe)], cwd=ROOT, check=True)
    return exe, binexe


def recording(dirname: str, seconds: float) -> np.ndarray:
    obs = json.loads((CANON / dirname / "observation.json").read_text())["av_validation"]
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(CANON / dirname / "original.mov"), "-vn", "-ac", "1",
                          "-ar", str(FS), "-f", "f32le", "-"], capture_output=True, check=True).stdout
    x = np.frombuffer(raw, dtype="<f4").astype(np.float64)
    t_movie = np.arange(int(seconds * FS)) / FS * obs["source_to_audio_scale"] + obs["source_to_movie_offset_s"]
    return np.interp(t_movie * FS, np.arange(len(x)), x, left=0.0, right=0.0)


def probe(exe: Path, x: np.ndarray, gain: float, name: str, fault: str = "none"):
    s16 = np.clip(np.round(x * gain * 32768.0), -32768, 32767).astype("<i2")
    p = WORK / f"{name}.s16"
    s16.tofile(p)
    a, b = WORK / f"{name}-{fault}-12800.csv", WORK / f"{name}-{fault}-24000.csv"
    r = subprocess.run([str(exe), str(p), str(a), str(b), fault], capture_output=True, text=True, check=True)
    return pd.read_csv(a), pd.read_csv(b), json.loads(r.stdout), s16


def calibrate(exe: Path, x: np.ndarray, target: float, name: str) -> float:
    gain = 0.12 / max(np.sqrt(np.mean(x[28 * FS:75 * FS] ** 2)), 1e-9)
    for _ in range(4):
        a, _, _, _ = probe(exe, x, gain, name)
        w = a[(a.t_end_s >= 28) & (a.t_end_s <= 75)]
        gain *= target / float(w.raw_rms.median())
    return gain


def pair(a: pd.DataFrame, b: pd.DataFrame):
    """Pair generations by time, then a single lag (-3..+3) chosen on raw_rms and held."""
    idx = np.clip(np.searchsorted(b.t_end_s.values, a.t_end_s.values), 0, len(b) - 1)
    best = None
    for lag in range(-3, 4):
        j = idx + lag
        ok = (j >= 0) & (j < len(b))
        r = np.corrcoef(a.raw_rms.values[ok], b.raw_rms.values[j[ok]])[0, 1]
        if best is None or r > best[0]:
            best = (r, lag)
    j = idx + best[1]
    ok = (j >= 0) & (j < len(b))
    return a[ok].reset_index(drop=True), b.iloc[j[ok]].reset_index(drop=True), best[1]


def gate_i(A, B, freqs):
    below = [k for k, f in enumerate(freqs) if f < 5120.0]
    band = [k for k, f in enumerate(freqs) if 5120.0 <= f <= 6400.0]
    w = A.t_end_s <= 75
    r = {k: float(np.corrcoef(A[f"m{k}"][w], B[f"m{k}"][w])[0, 1]) for k in below + band}
    rb = [r[k] for k in below]
    return {"bins_tested": len(below), "min_r": float(np.min(rb)), "median_r": float(np.median(rb)),
            "worst_bin": int(below[int(np.argmin(rb))]), "transition_band_r": {str(k): r[k] for k in band},
            "pass": bool(np.min(rb) >= 0.90 and np.median(rb) >= 0.97)}


def hp_envelope(s16: np.ndarray, t_end: np.ndarray) -> np.ndarray:
    x = s16.astype(np.float64) / 32768.0
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / FS)
    X[(f < 6500) | (f > 11000)] = 0
    y = np.fft.irfft(X, n=len(x))
    c = np.concatenate([[0.0], np.cumsum(y * y)])
    n = int(round(0.0075 * FS))
    e = np.clip((t_end * FS).astype(int), n, len(x))
    return np.sqrt((c[e] - c[e - n]) / n)


def gate_ii(B, freqs, s16):
    above = [k for k, f in enumerate(freqs) if f > 6400.0]
    below = [k for k, f in enumerate(freqs) if f < 5120.0]
    w = (B.t_end_s >= 28) & (B.t_end_s <= 75)
    ref = float(np.mean([B[f"m{k}"][w].mean() for k in below]))
    means = {str(k): float(B[f"m{k}"][w].mean()) for k in above}
    nonzero = all(v >= 0.001 * ref for v in means.values())
    w2 = B.t_end_s <= 75
    hi = B.loc[w2, [f"m{k}" for k in above]].sum(axis=1).values
    env = hp_envelope(s16, B.t_end_s[w2].values)
    best = max((float(np.corrcoef(hi[max(0, L):len(hi) + min(0, L)], env[max(0, -L):len(env) - max(0, L)])[0, 1]), L)
               for L in range(-4, 5))
    return {"bins": above, "mean_magnitude": means, "reference_mean_below_5120": ref, "nonzero_pass": bool(nonzero),
            "hp_envelope_r": best[0], "hp_lag_generations": best[1], "real_pass": bool(best[0] >= 0.60),
            "pass": bool(nonzero and best[0] >= 0.60)}


def gate_iii(A, B, fixture_s=None):
    out = {}
    ok = True
    for s in ("raw_rms", "vu_level"):
        m = A[s] > 0
        ratio = float(np.median(B[s][m] / A[s][m]))
        rho = float(spearmanr(A[s], B[s])[0])
        out[s] = {"median_ratio_24k_over_12k8": ratio, "spearman": rho}
        ok &= 0.90 <= ratio <= 1.15 and rho >= 0.95
    rho = float(spearmanr(A.peak_scaled, B.peak_scaled)[0])
    mad = float(np.median(np.abs(A.peak_scaled - B.peak_scaled)))
    out["peak_scaled"] = {"spearman": rho, "median_abs_diff": mad}
    ok &= rho >= 0.90 and mad <= 0.10
    agree = float((A.silence == B.silence).mean())
    out["silence_agreement"] = agree
    ok &= agree >= 0.99
    if fixture_s is not None:
        fa = A[(A.t_end_s > fixture_s) & (A.silence == 1)].t_end_s
        fb = B[(B.t_end_s > fixture_s) & (B.silence == 1)].t_end_s
        if len(fa) and len(fb):
            out["post_roll_first_silence_s"] = {"12k8": float(fa.iloc[0]), "24k": float(fb.iloc[0])}
            ok &= abs(float(fa.iloc[0]) - float(fb.iloc[0])) <= 0.5
        else:
            out["post_roll_first_silence_s"] = {"12k8": None if not len(fa) else float(fa.iloc[0]),
                                                "24k": None if not len(fb) else float(fb.iloc[0])}
            ok = False
    out["pass"] = bool(ok)
    return out


def gate_iv(A, B):
    def med(D):
        w = D[(D.t_end_s >= 33) & (D.t_end_s <= 43) & (D.tempo_valid == 1)]
        return float(w.tempo_bpm.median()) if len(w) else float("nan")
    a, b = med(A), med(B)
    return {"median_12k8": a, "median_24k": b, "abs_diff": abs(a - b), "pass": bool(abs(a - b) <= 2.0)}


def main() -> int:
    frozen = json.loads(FROZEN.read_text())
    import hashlib
    exe, binexe = build()
    freqs = [float(v) for v in subprocess.run([str(binexe)], capture_output=True, text=True, check=True).stdout.split()]
    meta = json.loads((ROOT / "docs/evidence/P3/agc-trace-meta.json").read_text())["inputs"]
    dev = json.loads(P3DATA.read_text())
    dopex_target = float(np.median([s["inhouse_raw_rms"] for s in dev["dopex"]["samples"]
                                    if s.get("inhouse_raw_rms") is not None and 28 <= s["t_mir"] <= 75]))
    inputs = {"boom_song": (tr.source_pcm(), meta["source"]["gain"]),
              "boom_room": (tr.recording_pcm(), meta["recording"]["gain"])}
    dx = recording("dopex-admission-fix-holdout-001", 87.0)
    inputs["dopex_room"] = (dx, calibrate(exe, dx, dopex_target, "dopex-cal"))
    res = {"frozen_sha256": hashlib.sha256(FROZEN.read_bytes()).hexdigest(), "frozen_purpose": frozen["purpose"],
           "gains": {k: v[1] for k, v in inputs.items()}, "real": {}, "planted": {}}

    def evaluate(fault: str):
        r = {}
        for name, (x, g) in inputs.items():
            A0, B0, info, s16 = probe(exe, x, g, name, fault)
            A, B, lag = pair(A0, B0)
            row = {"probe": info, "lag_generations": lag,
                   "iii": gate_iii(A, B, dev["dopex"]["fixture_s"] if name == "dopex_room" else None)}
            if name in ("boom_song", "boom_room"):
                row["i"] = gate_i(A, B, freqs)
                row["iv"] = gate_iv(A, B)
            if name == "boom_song":
                row["ii"] = gate_ii(B, freqs, s16)
            r[name] = row
        return r

    real = evaluate("none")
    res["real"] = real
    res["real_pass"] = {
        "i": all(real[n]["i"]["pass"] for n in ("boom_song", "boom_room")),
        "ii": real["boom_song"]["ii"]["pass"],
        "iii": all(real[n]["iii"]["pass"] for n in inputs),
        "iv": all(real[n]["iv"]["pass"] for n in ("boom_song", "boom_room"))}
    for fault, gates in (("rate", ("i", "iv")), ("upsample", ("ii",)), ("gain", ("iii",))):
        r = evaluate(fault)
        caught = {}
        for gname in gates:
            if gname == "ii":
                caught[gname] = not r["boom_song"]["ii"]["pass"]
            elif gname == "iii":
                caught[gname] = not all(r[n]["iii"]["pass"] for n in inputs)
            else:
                caught[gname] = not all(r[n][gname]["pass"] for n in ("boom_song", "boom_room"))
        res["planted"][fault] = {"caught": caught, "detail": r}
    res["planted_all_caught"] = all(all(v["caught"].values()) for v in res["planted"].values())
    res["overall_pass"] = bool(all(res["real_pass"].values()) and res["planted_all_caught"])
    (RUN / "analysis24-gate.json").write_text(json.dumps(res, indent=2) + "\n")
    summary = {"real_pass": res["real_pass"], "planted": {k: v["caught"] for k, v in res["planted"].items()},
               "overall_pass": res["overall_pass"]}
    print(json.dumps(summary, indent=1))
    for n in inputs:
        print(n, json.dumps({g: {k: v for k, v in real[n][g].items() if k not in ("transition_band_r", "mean_magnitude")}
                             for g in real[n] if g not in ("probe",) and isinstance(real[n][g], dict)}))
    return 0 if res["overall_pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
