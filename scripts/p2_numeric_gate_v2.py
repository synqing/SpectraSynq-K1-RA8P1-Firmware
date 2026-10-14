#!/usr/bin/env python3
"""Lane P2 iteration 2 host gate: noise-calibrated tolerance for the v8 output path.

Applies iteration-2/TOLERANCE-FROZEN-v2.json exactly (its sha256 is checked first):
limits are the maxima, over 20 calibration seeds, of the resident build's divergence
from itself when 66 input samples move by +-1 LSB; 5 held-out seeds and 5 planted
faults decide whether the gate is valid; then the product candidate (-ffp-contract=fast
-O3 -funroll-loops) is judged against the frozen limits once. Host oracle only.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import subprocess
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import p2_numeric_compare as v1  # noqa: E402  (build/parse are the v1 gate's, unchanged)

FEAT_POS = 14
CAL = range(101, 121)
HELD = range(121, 126)
K = 66


def arrays(stream):
    hops, frames = stream
    done = np.array([d for _, d in hops], dtype=np.int64)
    got = np.array([[b[0] for b in fr] for fr in frames], dtype=np.int64)
    feat = np.array([[b[1] for b in fr] for fr in frames], dtype=np.float64)
    rgb = np.array([[b[2] for b in fr] for fr in frames], dtype=np.int64)
    return {"done": done, "got": got, "feat": feat, "rgb": rgb}


def runs(mask):
    out, n = [], 0
    for m in mask:
        if m:
            n += 1
        elif n:
            out.append(n); n = 0
    if n:
        out.append(n)
    return out


def metrics(ref, oth):
    res = {"categorical_mismatches": int((ref["done"] != oth["done"]).sum() + (ref["got"] != oth["got"]).sum()),
           "nonfinite": int((~np.isfinite(oth["feat"])).sum())}
    hops = len(ref["done"])
    nz = float((ref["rgb"] != 0).mean())
    res["non_vacuous"] = bool(ref["done"].sum() >= hops // 2 and ref["got"].sum() >= ref["got"].shape[0]
                              and nz >= 0.01 and (oth["rgb"] != 0).mean() >= 0.01)
    fr, fo = ref["feat"], oth["feat"]
    ok = np.isfinite(fo)
    d = np.where(ok, np.abs(fo - fr), 0.0)
    sig, noise = float((fr[ok] ** 2).sum()), float((d ** 2).sum())
    res["feature_snr_db"] = math.inf if noise == 0 else 10 * math.log10(sig / noise)
    for b, name in ((0, "board0"), (1, "board1")):
        drgb = np.abs(oth["rgb"][:, b, :] - ref["rgb"][:, b, :]).max(axis=1)
        r = runs(drgb > 8)
        dpos = np.abs(fo[:, b, FEAT_POS] - fr[:, b, FEAT_POS])
        dpos = np.where(np.isfinite(dpos), dpos, np.inf)
        viol = (d[:, b, :] > 2e-5 + 2e-4 * np.abs(fr[:, b, :])) | ~ok[:, b, :]
        res[name] = {"events": len(r), "longest_event_frames": max(r) if r else 0, "peak_event_lsb": int(drgb.max()),
                     "position_flip_frames": int((dpos >= 0.5).sum()), "max_position_abs_px": float(dpos.max()),
                     "feature_violations": int(viol.sum())}
    return res


def judge(m, limits):
    bad = []
    if not m["non_vacuous"]:
        bad.append("non_vacuous")
    if m["categorical_mismatches"]:
        bad.append("categorical_mismatches")
    if m["nonfinite"]:
        bad.append("nonfinite")
    if not m["feature_snr_db"] >= 70.0:
        bad.append("feature_snr_db")
    for b in ("board0", "board1"):
        for k, lim in limits[b].items():
            if m[b][k] > lim:
                bad.append(f"{b}.{k}={m[b][k]}>{lim}")
    return bad


def run_probe(exe, inp, out):
    r = subprocess.run([str(exe), str(inp), str(out)], capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    s = arrays(v1.parse(out.read_bytes()))
    out.unlink()
    return s


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--input", type=Path, required=True)
    ap.add_argument("--tolerance", type=Path, required=True)
    ap.add_argument("--expected-tolerance-sha256", required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--scratch", type=Path, required=True, help="large temporary streams (deleted after use)")
    a = ap.parse_args()
    tol_sha = hashlib.sha256(a.tolerance.read_bytes()).hexdigest()
    assert tol_sha == a.expected_tolerance_sha256, f"tolerance changed: {tol_sha}"
    a.out.mkdir(parents=True, exist_ok=False)
    a.scratch.mkdir(parents=True, exist_ok=True)
    work = a.out / "build"; work.mkdir()
    ref_exe = v1.build("reference", v1.PROFILES["reference"], work)
    cand_flags = ["-ffp-contract=fast", "-O3", "-funroll-loops"]
    cand_exe = v1.build("candidate", cand_flags, work)
    pcm = np.fromfile(a.input, dtype="<i2")
    result = {"tolerance_sha256": tol_sha, "input_sha256": hashlib.sha256(a.input.read_bytes()).hexdigest(),
              "reference_flags": v1.COMMON + v1.PROFILES["reference"], "candidate_flags": v1.COMMON + cand_flags,
              "sources_sha256": {p: hashlib.sha256((v1.ROOT / p).read_bytes()).hexdigest()
                                 for p in [*v1.SOURCES_CXX, v1.SOURCE_C]}}
    ref = run_probe(ref_exe, a.input, a.scratch / "ref.bin")
    seeds = {}
    for seed in [*CAL, *HELD]:
        rng = np.random.default_rng(seed)
        x = pcm.astype(np.int32).copy()
        idx = rng.choice(len(x), K, replace=False)
        x[idx] = np.clip(x[idx] + rng.choice([-1, 1], K), -32768, 32767)
        p = a.scratch / f"in-{seed}.s16"
        x.astype("<i2").tofile(p)
        seeds[seed] = metrics(ref, run_probe(ref_exe, p, a.scratch / f"n-{seed}.bin"))
        p.unlink()
        print(seed, {b: seeds[seed][b] for b in ("board0", "board1")}, flush=True)
    keys = ["events", "longest_event_frames", "peak_event_lsb", "position_flip_frames", "max_position_abs_px",
            "feature_violations"]
    limits = {b: {k: max(seeds[s][b][k] for s in CAL) for k in keys} for b in ("board0", "board1")}
    for b in limits:
        limits[b]["max_position_abs_px"] = max(1.0, limits[b]["max_position_abs_px"])
    result["calibration"] = {str(s): seeds[s] for s in CAL}
    result["limits"] = limits
    held = {str(s): {"metrics": seeds[s], "fails": judge(seeds[s], limits)} for s in HELD}
    result["held_out"] = held
    held_pass = sum(1 for v in held.values() if not v["fails"])
    cand = run_probe(cand_exe, a.input, a.scratch / "cand.bin")
    cm = metrics(ref, cand)
    result["candidate"] = {"metrics": cm, "fails": judge(cm, limits)}

    def planted(name, mutate, both=False):
        c = {k: v.copy() for k, v in cand.items()}
        r = {k: v.copy() for k, v in ref.items()} if both else ref
        mutate(c, r)
        return judge(metrics(r, c), limits)

    def p1(c, r):
        c["feat"][3000:, 1, FEAT_POS] += 2
        v = c["rgb"][3000:, 1, :].reshape(-1, 160, 3)
        c["rgb"][3000:, 1, :] = np.roll(v, 2, axis=1).reshape(-1, 480)

    def p2(c, r):
        c["rgb"][3000:, 0, 80 * 3] = np.minimum(65535, c["rgb"][3000:, 0, 80 * 3] + 200)

    def p3(c, r):
        for i in range(int(limits["board0"]["events"]) + 1):
            s = 1000 + 200 * i
            c["rgb"][s:s + 3, 0, 10 * 3] = np.minimum(65535, c["rgb"][s:s + 3, 0, 10 * 3] + 100)

    def p4(c, r):
        for d in (c, r):
            d["rgb"][:] = 0
            d["got"][:] = 0

    def p5(c, r):
        c["feat"][100, 0, 0] = float("nan")

    plants = {"P1_position_shift_2px": planted("P1", p1), "P2_persistent_offset": planted("P2", p2),
              "P3_excess_events": planted("P3", p3), "P4_vacuous": planted("P4", p4, both=True),
              "P5_nan_feature": planted("P5", p5)}
    result["planted_faults_failures"] = plants
    plants_red = all(bool(v) for v in plants.values())
    valid = held_pass >= 4 and plants_red
    result["gate_validity"] = {"held_out_pass": held_pass, "held_out_total": len(HELD), "planted_all_red": plants_red,
                               "valid": valid}
    result["outcome"] = "PASS" if valid and not result["candidate"]["fails"] else "FAIL"
    (a.out / "gate-v2.json").write_text(json.dumps(result, indent=2, default=float) + "\n")
    print(json.dumps({"limits": limits, "held_out_pass": held_pass, "plants": plants,
                      "candidate": result["candidate"], "outcome": result["outcome"]}, indent=1, default=float))
    return 0 if result["outcome"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
