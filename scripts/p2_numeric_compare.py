#!/usr/bin/env python3
"""Lane P2 host tolerance gate: resident vs product float semantics on the v8 output path.

Builds tests/host/p2_numeric_probe.cpp twice from the same sources (resident:
-ffp-contract=off, no vectorisation; product: -ffp-contract=fast, vectorisation
on; -fno-fast-math and -O2 in both), runs both on the same frozen PCM and compares
against a FROZEN tolerance file (never regenerated here). --plant perturbs the
candidate stream to prove each comparison can go red. Host oracle only.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCES_CXX = ["tests/host/p2_numeric_probe.cpp", "platform/ra8p1/inhouse_audio.cpp",
               "src/k1/core/visual/inhouse_waveform.cpp", "src/k1/core/audio/gdft_goertzel.cpp",
               "src/k1/core/audio/tempo_tracker.cpp", "src/k1/core/audio/tempo_acf.cpp",
               "src/k1/core/visual/product_palette.cpp", "src/k1/core/visual/product_palette_data.cpp"]
SOURCE_C = "platform/ra8p1/k1_asrc_24k.c"
COMMON = ["-O2", "-fno-fast-math", "-DK1_EMOTISCOPE_PORT=1", "-Iplatform/ra8p1", "-Isrc", "-Isrc/k1"]
PROFILES = {"reference": ["-ffp-contract=off", "-fno-vectorize", "-fno-slp-vectorize"],
            "candidate": ["-ffp-contract=fast"]}
HOP_PCM = 64
FEATURES = 12 + 7


def build(name, flags, work):
    obj = work / f"asrc-{name}.o"
    subprocess.run(["cc", "-std=c11", *COMMON, *flags, "-c", SOURCE_C, "-o", str(obj)], cwd=ROOT, check=True)
    exe = work / f"probe-{name}"
    subprocess.run(["c++", "-std=c++17", *COMMON, *flags, *SOURCES_CXX, str(obj), "-o", str(exe)],
                   cwd=ROOT, check=True)
    return exe


def parse(blob):
    hops, frames, at = [], [], 0
    seq = 0
    while at < len(blob):
        pcm = struct.unpack_from(f"<{HOP_PCM}h", blob, at); at += HOP_PCM * 2
        (done,) = struct.unpack_from("<I", blob, at); at += 4
        hops.append((pcm, done)); seq += 1
        if seq % 2:
            continue
        boards = []
        for _ in range(2):
            (got,) = struct.unpack_from("<I", blob, at); at += 4
            feat = struct.unpack_from(f"<{FEATURES}f", blob, at); at += FEATURES * 4
            rgb = struct.unpack_from("<480H", blob, at); at += 960
            boards.append((got, feat, rgb))
        frames.append(boards)
    return hops, frames


def compare(ref, cand, tol):
    (rh, rf), (ch, cf) = ref, cand
    res = {"hops": [len(rh), len(ch)], "frames": [len(rf), len(cf)]}
    if len(rh) != len(ch) or len(rf) != len(cf):
        res["pass"] = False; res["reason"] = "length mismatch"; return res
    pcm_max = max(abs(a - b) for (pa, _), (pb, _) in zip(rh, ch) for a, b in zip(pa, pb))
    cat_mismatch = sum(da != db for (_, da), (_, db) in zip(rh, ch))
    feat_viol = feat_n = nonfinite = 0
    feat_max = 0.0
    sig = noise = 0.0
    rgb_max = 0
    for fr, fc in zip(rf, cf):
        for (gr, xr, pr), (gc, xc, pc) in zip(fr, fc):
            cat_mismatch += gr != gc
            for a, b in zip(xr, xc):
                feat_n += 1
                if not (math.isfinite(a) and math.isfinite(b)):
                    nonfinite += 1; continue
                d = abs(b - a)
                feat_max = max(feat_max, d)
                if d > tol["abs"] + tol["rel"] * abs(a):
                    feat_viol += 1
                sig += a * a; noise += d * d
            rgb_max = max(rgb_max, max(abs(a - b) for a, b in zip(pr, pc)))
    snr = float("inf") if noise == 0 else 10 * math.log10(sig / noise) if sig > 0 else float("-inf")
    completed = sum(d for _, d in rh)
    acquired = sum(g for fr in rf for (g, _, _) in fr)
    nonzero = sum(1 for fr in rf for (_, _, p) in fr for v in p if v)
    rgb_total = sum(len(p) for fr in rf for (_, _, p) in fr)
    res.update(analyses_completed=completed, frames_acquired=acquired, board_frames=2 * len(rf),
               nonzero_rgb_fraction=nonzero / rgb_total if rgb_total else 0.0)
    # A comparison of empty output proves nothing: require real analysis, real
    # acquisition and real light in the reference stream.
    res["non_vacuous"] = (completed >= len(rh) // 2 and acquired >= len(rf) and
                          res["nonzero_rgb_fraction"] >= 0.01)
    res.update(asrc_pcm_max_lsb=pcm_max, categorical_mismatches=cat_mismatch, feature_values=feat_n,
               feature_violations=feat_viol, feature_max_abs_diff=feat_max, nonfinite=nonfinite,
               feature_snr_db=snr, rgb16_max_lsb=rgb_max)
    res["pass"] = (res["non_vacuous"] and feat_viol == 0 and nonfinite == 0 and cat_mismatch == 0 and rgb_max <= tol["rgb"]
                   and snr >= tol["snr"] and pcm_max <= tol["rgb"])
    return res


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--input", type=Path, required=True)
    ap.add_argument("--tolerance", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=False)
    frozen = json.loads(a.tolerance.read_text())
    tol = {"abs": 2e-5, "rel": 2e-4, "rgb": int(frozen["rgb16_tolerance_lsb"]),
           "snr": float(frozen["aggregate_feature_snr_db_min"])}
    assert frozen["feature_tolerance"].startswith("abs(candidate - reference) <= 2e-5 + 2e-4"), "tolerance text drifted"
    result = {"input_sha256": hashlib.sha256(a.input.read_bytes()).hexdigest(),
              "tolerance_sha256": hashlib.sha256(a.tolerance.read_bytes()).hexdigest(),
              "sources_sha256": {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest()
                                 for p in [*SOURCES_CXX, SOURCE_C]},
              "profiles": PROFILES, "common": COMMON}
    streams = {}
    for name, flags in PROFILES.items():
        exe = build(name, flags, a.out)
        run = subprocess.run([str(exe), str(a.input), str(a.out / f"{name}.bin")], capture_output=True, text=True)
        result[f"{name}_run"] = {"rc": run.returncode, "stdout": run.stdout.strip(), "stderr": run.stderr[-400:]}
        assert run.returncode == 0, run
        streams[name] = parse((a.out / f"{name}.bin").read_bytes())
    result["comparison"] = compare(streams["reference"], streams["candidate"], tol)
    # Planted faults: each must turn the gate red.
    rh, rf = streams["reference"]
    plants = {}
    def with_frame(frames, idx, board, part, mutate):
        out = list(frames); fr = [list(x) for x in out[idx]]; b = list(fr[board]); v = list(b[part])
        mutate(v); b[part] = tuple(v); fr[board] = tuple(b); out[idx] = fr; return out
    plants["rgb_plus_9"] = compare((rh, rf), (rh, with_frame(rf, 100, 0, 2, lambda v: v.__setitem__(5, v[5] + 9))), tol)["pass"]
    plants["feature_rel_1e-3"] = compare((rh, rf), (rh, with_frame(rf, 100, 1, 1, lambda v: v.__setitem__(2, v[2] * 1.001 + 1e-3))), tol)["pass"]
    plants["vacuous_all_zero"] = compare((rh, [[(0, b[1], tuple(0 for _ in b[2])) for b in fr] for fr in rf]),
                                         (rh, [[(0, b[1], tuple(0 for _ in b[2])) for b in fr] for fr in rf]), tol)["pass"]
    plants["feature_nan"] = compare((rh, rf), (rh, with_frame(rf, 100, 0, 1, lambda v: v.__setitem__(0, float("nan")))), tol)["pass"]
    result["planted_faults_pass_value"] = plants
    result["planted_all_red"] = not any(plants.values())
    (a.out / "comparison.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"comparison": result["comparison"], "planted_all_red": result["planted_all_red"]}, indent=1))
    return 0 if result["comparison"]["pass"] and result["planted_all_red"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
