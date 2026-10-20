#!/usr/bin/env python3
"""Lane P6: evaluate one host replay against TOLERANCE-FROZEN-P6.json (C1, C2, C3, C5).

  p6_evaluate.py evaluate --inputs INPUTS --replay DIR --v8 V8DIR --out result.json
  p6_evaluate.py energy   --inputs INPUTS --replay DIR          # calibration-corpus mean S per board

Exit status is the verdict: 0 only when every evaluated criterion passes (C4 is checked by
p6_identity.py). Offline, reads CSVs written by tests/host/p6_effect_probe.cpp.
"""
from __future__ import annotations

import argparse
import itertools
import json
import math
import sys
from pathlib import Path

import numpy as np
import pandas as pd

BUILD, DROP = (45.0, 50.0), (55.5, 60.0)
LADDER = ["boom_room", "sonic_room", "dopex_room"]
ZERO_DB = ["boom_source", "boom_room", "sonic_room", "dopex_room"]
CALIBRATION = ["boom_source", "boom_room", "sonic_room"]


def frames(replay: Path, manifest: dict, tag: str) -> pd.DataFrame:
    name = next(n for n, s in manifest["inputs"].items() if tag in s["files"])
    spec = manifest["inputs"][name]
    d = pd.read_csv(replay / f"{tag}.csv")
    d["t_song"] = (d.t_s - spec["offset_s"]) / spec["scale"]
    d["zeros_s"] = d.t_s - spec["file_seconds"]  # >= 0 inside the appended digital zeros
    d.attrs.update(spec=spec, name=name)
    return d


def window(d: pd.DataFrame, board: int, lo: float, hi: float) -> pd.DataFrame:
    return d[(d.board == board) & (d.t_song >= lo) & (d.t_song <= hi) & (d.zeros_s < 0)]


def c1(replay, manifest):
    cases, ok = [], True
    for tag in ("boom_source", "boom_room"):
        d = frames(replay, manifest, tag)
        for board in (0, 1):
            b, q = window(d, board, *BUILD), window(d, board, *DROP)
            loud = q.raw_rms.median() / b.raw_rms.median()
            s = q.S.median() / b.S.median() if b.S.median() > 0 else math.inf
            lr = q.L.median() / b.L.median() if b.L.median() > 0 else math.inf
            valid = loud > 1.0
            passed = bool(valid and s >= 1.122)
            ok &= passed
            cases.append({"input": tag, "board": board, "loudness_ratio": loud, "S_build": b.S.median(),
                          "S_drop": q.S.median(), "S_ratio": s, "L_ratio": lr, "valid": bool(valid), "pass": passed})
    return {"pass": bool(ok), "cases": cases}


def c2(replay, manifest):
    cond = {}
    for song in LADDER:
        for suffix, db in (("-6dB", -6.0), ("", 0.0), ("+6dB", 6.0)):
            tag = song + suffix
            d = frames(replay, manifest, tag)
            lo, hi = d.attrs["spec"]["window_song_s"]
            w0 = window(d, 0, lo, hi)
            cond[tag] = {"song": song, "ladder_db": db, "loudness_db": 20 * math.log10(w0.raw_rms.median()),
                         "S": [window(d, b, lo, hi).S.median() for b in (0, 1)],
                         "L": [window(d, b, lo, hi).L.median() for b in (0, 1)]}
    boards, ok = [], True
    for board in (0, 1):
        same = cross = same_ok = cross_ok = cross_L_ok = same_L_ok = 0
        slopes = []
        for a, b in itertools.combinations(cond, 2):
            A, B = cond[a], cond[b]
            dl = B["loudness_db"] - A["loudness_db"]
            if abs(dl) < 3.0:
                continue
            good = np.sign(B["S"][board] - A["S"][board]) == np.sign(dl)
            good_l = np.sign(B["L"][board] - A["L"][board]) == np.sign(dl)
            if A["song"] == B["song"]:
                same += 1; same_ok += int(good); same_L_ok += int(good_l)
                if A["S"][board] > 0 and B["S"][board] > 0:
                    slopes.append(math.log10(B["S"][board] / A["S"][board]) * 10 / dl)  # dB of light per dB
            else:
                cross += 1; cross_ok += int(good); cross_L_ok += int(good_l)
        frac = cross_ok / cross if cross else 1.0
        passed = same_ok == same == 9 and frac >= 0.80
        ok &= passed
        boards.append({"board": board, "same_song_pairs": same, "same_song_concordant": same_ok,
                       "cross_song_pairs": cross, "cross_song_concordant": cross_ok, "cross_fraction": frac,
                       "L_same_concordant": same_L_ok, "L_cross_concordant": cross_L_ok,
                       "median_same_song_slope_db_per_db": float(np.median(slopes)) if slopes else None,
                       "pass": bool(passed)})
    return {"pass": bool(ok), "boards": boards, "conditions": cond}


def silence_ratio(replay, manifest, tag, board):
    d = frames(replay, manifest, tag)
    fx = d.attrs["spec"]["fixture_s"]
    sil = d[(d.board == board) & (d.t_song > fx + 0.5) & (d.zeros_s < 0)]
    mus = window(d, board, 2.0, fx - 0.5)
    m = mus.S.median()
    return (sil.S.median() / m if m > 0 else math.inf), int(len(sil)), m


def c3(replay, v8, manifest):
    rows, ok_a = [], True
    for tag in ("boom_room", "sonic_room", "dopex_room"):
        for board in (0, 1):
            r9, n, m9 = silence_ratio(replay, manifest, tag, board)
            r8, _, m8 = silence_ratio(v8, manifest, tag, board)
            passed = r9 <= 0.1 if r8 <= 0.1 else r9 <= 1.25 * r8
            ok_a &= passed
            rows.append({"input": tag, "board": board, "R": r9, "R_v8": r8, "n_silence_frames": n,
                         "S_music_median": m9, "S_music_median_v8": m8, "pass": bool(passed)})
    black, ok_b = [], True
    for tag in ZERO_DB:
        d = frames(replay, manifest, tag)
        for board in (0, 1):
            z = d[(d.board == board) & (d.zeros_s >= 0)]
            late = z[z.zeros_s >= 5.0]
            passed = bool(len(late) > 0 and (late.max_channel == 0).all())
            lit = z[z.max_channel > 0]
            first_black = float(lit.zeros_s.max() + 0.01) if len(lit) else 0.0
            ok_b &= passed
            black.append({"input": tag, "board": board, "frames_checked": int(len(late)),
                          "first_all_black_after_zeros_s": first_black if first_black < z.zeros_s.max() else None,
                          "pass": passed})
    return {"pass": bool(ok_a and ok_b), "a_q2_preserved": {"pass": bool(ok_a), "rows": rows},
            "b_true_black": {"pass": bool(ok_b), "rows": black}}


def c5(replay, manifest):
    rows, ok = [], True
    for tag in ZERO_DB:
        d = frames(replay, manifest, tag)
        for board in (0, 1):
            x = d[d.board == board]
            lit, off = int(x.lit_px.sum()), int(x.offpal_px.sum())
            frac = off / lit if lit else 0.0
            passed = frac <= 0.001
            ok &= passed
            rows.append({"input": tag, "board": board, "lit_px": lit, "offpal_px": off, "fraction": frac, "pass": bool(passed)})
    return {"pass": bool(ok), "rows": rows}


def energy(replay, manifest):
    out = []
    for board in (0, 1):
        vals = []
        for tag in CALIBRATION:
            d = frames(replay, manifest, tag)
            lo, hi = d.attrs["spec"]["window_song_s"]
            vals.append(window(d, board, lo, hi).S.to_numpy())
        out.append(float(np.concatenate(vals).mean()))
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    e = sub.add_parser("evaluate")
    for k in ("--inputs", "--replay", "--v8", "--out"):
        e.add_argument(k, type=Path, required=True)
    g = sub.add_parser("energy")
    g.add_argument("--inputs", type=Path, required=True)
    g.add_argument("--replay", type=Path, required=True)
    a = ap.parse_args(argv)
    manifest = json.loads((a.inputs / "inputs.json").read_text())
    if a.cmd == "energy":
        print(json.dumps(energy(a.replay, manifest)))
        return 0
    res = {"replay": str(a.replay), "C1": c1(a.replay, manifest), "C2": c2(a.replay, manifest),
           "C3": c3(a.replay, a.v8, manifest), "C5": c5(a.replay, manifest),
           "mean_S_calibration_corpus": energy(a.replay, manifest)}
    res["pass"] = all(res[k]["pass"] for k in ("C1", "C2", "C3", "C5"))
    a.out.write_text(json.dumps(res, indent=2, default=float) + "\n")
    print(json.dumps({k: res[k]["pass"] for k in ("C1", "C2", "C3", "C5")} | {"pass": res["pass"]}))
    return 0 if res["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
