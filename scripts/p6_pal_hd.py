#!/usr/bin/env python3
"""Lane P6 PAL-HD (TOLERANCE-FROZEN-P6-PAL-HD.json). Does not change p6_pal.py.

  p6_pal_hd.py evaluate --criterion FILE --inputs INPUTS --replay DIR --v8 V8DIR --out RESULT.json

(a) every lit pixel of every 0 dB music frame, both boards, is within the frozen L-inf of the
probe paletteCurve. (c) the same palette_coordinate motion test as p6_pal.py, including the ladder.
Exit 0 only if both pass. Frames are <u2 (tick, board, 160, 3) in the same order as the CSV.
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

import numpy as np
import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parent))
import p6_pal  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
FRACTION = 0.001  # same lit-pixel fraction as PAL (a)


def stops(pid: int) -> np.ndarray:
    exe = Path("/tmp") / f"p6_palhd_stops_{pid}_{os.getpid()}"
    cpp = exe.with_suffix(".cpp")
    cpp.write_text(
        '#include "core/visual/product_palette.h"\n#include <cstdio>\n'
        f'int main(){{auto&p=k1::core::visual::productPalette({pid});'
        'for(unsigned i=0;i<p.stop_count;++i)'
        'std::printf("%u %u %u %u\\n",p.stops[i].position,p.stops[i].colour.red,'
        'p.stops[i].colour.green,p.stops[i].colour.blue);}\n')
    subprocess.run(["c++", "-std=c++17", "-O2", "-I", str(ROOT / "src/k1"), "-I", str(ROOT / "src"),
                    str(cpp), str(ROOT / "src/k1/core/visual/product_palette.cpp"),
                    str(ROOT / "src/k1/core/visual/product_palette_data.cpp"), "-o", str(exe)], check=True)
    text = subprocess.run([str(exe)], capture_output=True, text=True, check=True).stdout
    return np.array([[float(v) for v in line.split()] for line in text.splitlines()], dtype=np.float64)


def palette_curve(pid: int) -> np.ndarray:
    """tests/host/p6_effect_probe.cpp paletteCurve, channels only (the phase column is unused here)."""
    s = stops(pid)
    n = len(s)
    out = []
    for k in range(4097):
        x = k / 4096.0
        i = 0
        while i + 1 < n and s[i + 1, 0] / 255.0 < x:
            i += 1
        if i + 1 >= n or x <= s[0, 0] / 255.0:
            c = (s[0, 1:4] if x <= s[0, 0] / 255.0 else s[-1, 1:4]).copy()
        else:
            pos = s[i, 0] / 255.0
            span = s[i + 1, 0] / 255.0 - pos
            t = 0.0 if span <= 1e-6 else min(1.0, max(0.0, (x - pos) / span))
            c = s[i, 1:4] + (s[i + 1, 1:4] - s[i, 1:4]) * t
        m = float(c.max())
        if m > 0.5:
            out.append(c / m)
    return np.asarray(out, dtype=np.float32)


def nearest(pixels: np.ndarray, curve: np.ndarray) -> np.ndarray:
    err = np.empty(pixels.shape[0], dtype=np.float32)
    c = curve[None, :, :]
    for i in range(0, len(pixels), 1024):
        chunk = pixels[i:i + 1024, None, :]
        err[i:i + len(chunk)] = np.max(np.abs(chunk - c), axis=2).min(axis=1)
    return err


def segment_pieces(stops_arr: np.ndarray) -> list[tuple]:
    """Pieces of the stop interpolation on which one channel is the maximum.

    Colour is 0..1. A piece is kept only when that maximum exceeds 0.5/255,
    the same keep test as palette_curve (max of the 0..255 channels > 0.5).
    """
    cols = stops_arr[:, 1:4] / 255.0
    pieces = []
    for i in range(len(stops_arr) - 1):
        a = cols[i]
        d = cols[i + 1] - a
        cuts = [0.0, 1.0]
        for u, v in ((0, 1), (0, 2), (1, 2)):
            den = d[u] - d[v]
            if abs(den) > 1e-15:
                t = (a[v] - a[u]) / den
                if 0.0 < t < 1.0:
                    cuts.append(float(t))
        cuts = sorted(set(np.round(cuts, 12)))
        for t0, t1 in zip(cuts, cuts[1:]):
            if t1 <= t0:
                continue
            mid = a + d * (0.5 * (t0 + t1))
            k = int(np.argmax(mid))
            if mid[k] * 255.0 <= 0.5:
                continue
            pieces.append((a, d, float(t0), float(t1), k))
    return pieces


def _piece_error(q: np.ndarray, a: np.ndarray, d: np.ndarray, t0: float, t1: float, k: int) -> np.ndarray:
    """Min L-inf distance from each normalised pixel to n(t) = c(t)/c_k(t), t in [t0, t1].

    n_i is fractional-linear, so |q_i - n_i| has no interior stationary point unless the
    ratio is constant. The minimum of the upper envelope is at an endpoint, where one
    channel error is zero, or where two channel errors have equal magnitude.
    """
    n = len(q)
    best = np.full(n, np.inf)
    pk = float(a[k])
    dk = float(d[k])

    def consider(t: np.ndarray, mask: np.ndarray) -> None:
        nonlocal best
        if not np.any(mask):
            return
        tt = np.clip(t, t0, t1)
        colour = a + d * tt[:, None]
        ck = colour[:, k]
        # Dividing by a channel that is not the maximum is not a point on the curve.
        ok = mask & (ck > 1e-15) & (ck + 1e-9 >= np.max(colour, axis=1))
        if not np.any(ok):
            return
        norm = np.ones_like(colour)
        norm[ok] = colour[ok] / ck[ok, None]
        err = np.full(n, np.inf)
        err[ok] = np.max(np.abs(q[ok] - norm[ok]), axis=1)
        best = np.minimum(best, err)

    for end in (t0, t1):
        colour = a + d * end
        if colour[k] <= 1e-15:
            continue
        norm = colour / colour[k]
        best = np.minimum(best, np.max(np.abs(q - norm), axis=1))

    others = [i for i in range(3) if i != k]
    for i in others:
        den = d[i] - q[:, i] * dk
        num = q[:, i] * pk - a[i]
        good = np.abs(den) > 1e-18
        t = np.zeros(n)
        t[good] = num[good] / den[good]
        consider(t, good & (t >= t0 - 1e-12) & (t <= t1 + 1e-12))
    i, j = others
    for sign in (1.0, -1.0):
        left = q[:, i] - sign * q[:, j]
        den = left * dk - (d[i] - sign * d[j])
        num = (a[i] - sign * a[j]) - left * pk
        good = np.abs(den) > 1e-18
        t = np.zeros(n)
        t[good] = num[good] / den[good]
        consider(t, good & (t >= t0 - 1e-12) & (t <= t1 + 1e-12))
    return best


def nearest_exact(pixels: np.ndarray, stops_arr: np.ndarray) -> np.ndarray:
    """Min L-inf distance to the normalised image of the piecewise-linear stops."""
    err = np.full(len(pixels), np.inf, dtype=np.float64)
    q = np.asarray(pixels, dtype=np.float64)
    for a, d, t0, t1, k in segment_pieces(stops_arr):
        err = np.minimum(err, _piece_error(q, a, d, t0, t1, k))
    return err.astype(np.float32)


def fidelity(replay: Path, manifest: dict, model: dict[int, np.ndarray], tol: float,
             exact: bool = False) -> tuple[list, bool]:
    rows, ok = [], True
    for tag in p6_pal.ZERO_DB:
        spec = next(s for s in manifest["inputs"].values() if tag in s["files"])
        csv = pd.read_csv(replay / f"{tag}.csv")
        frames = np.fromfile(replay / f"{tag}.frames.bin", dtype="<u2").reshape(-1, 2, 160, 3)
        if len(csv) != frames.shape[0] * 2:
            raise SystemExit(f"{tag}: csv rows {len(csv)} != 2 * frames {frames.shape[0]}")
        music = (csv.fresh.to_numpy() == 1) & (csv.silence.to_numpy() == 0) & (csv.lit_px.to_numpy() > 0) & (
            csv.t_s.to_numpy() < spec["file_seconds"])
        board = csv.board.to_numpy()
        for b, pid in ((0, 31), (1, 38)):
            sel = np.flatnonzero(music & (board == b))
            px = frames[sel // 2, b].reshape(-1, 3).astype(np.float32) / 65535.0
            m = px.max(axis=1)
            lit = px[m >= 0.01]
            colour = lit / m[m >= 0.01, None]
            if not len(colour):
                err = np.empty(0, np.float32)
            elif exact:
                err = nearest_exact(colour, model[pid])
            else:
                err = nearest(colour, model[pid])
            off = int((err > tol).sum())
            frac = off / len(colour) if len(colour) else 0.0
            passed = frac <= FRACTION
            ok &= passed
            rows.append({"input": tag, "board": b, "palette_id": pid, "lit_px": int(len(colour)),
                         "off_px": off, "off_fraction": frac, "max_error": float(err.max()) if len(err) else 0.0,
                         "a_hd_fidelity": passed})
    return rows, ok


def motion_checks(replay: Path, v8: Path, manifest: dict) -> tuple[list, list, bool]:
    rows, ladder, ok = [], [], True
    for tag in p6_pal.ZERO_DB:
        a, r = p6_pal.load(replay, manifest, tag), p6_pal.load(v8, manifest, tag)
        for board in (0, 1):
            m, m8 = p6_pal.motion(a[a.board == board]), p6_pal.motion(r[r.board == board])
            passed = m >= m8 - 0.02
            ok &= passed
            rows.append({"input": tag, "board": board, "motion": m, "motion_v8": m8, "c_motion_vs_v8": passed})
    for song in p6_pal.ROOMS:
        for board in (0, 1):
            ms = []
            for sfx in ("-6dB", "", "+6dB"):
                d = p6_pal.load(replay, manifest, song + sfx)
                ms.append(p6_pal.motion(d[d.board == board]))
            mono = ms[1] >= ms[0] - 0.02 and ms[2] >= ms[1] - 0.02
            ok &= mono
            ladder.append({"song": song, "board": board, "motion_-6_0_+6": ms, "not_falling": bool(mono)})
    return rows, ladder, ok


def evaluate(criterion: Path, replay: Path, v8: Path, manifest: dict) -> dict:
    crit = json.loads(criterion.read_text())
    tol = float(crit["measurement"]["tolerance"])
    exact = crit["measurement"].get("distance") == "exact_piecewise_linear"
    model = {31: stops(31), 38: stops(38)} if exact else {31: palette_curve(31), 38: palette_curve(38)}
    rows, a_ok = fidelity(replay, manifest, model, tol, exact=exact)
    motion_rows, ladder, c_ok = motion_checks(replay, v8, manifest)
    return {"criterion": criterion.name, "tolerance": tol, "fraction_limit": FRACTION,
            "rows": rows, "motion": motion_rows, "ladder": ladder,
            "a_hd_fidelity": bool(a_ok), "c_motion": bool(c_ok), "pass": bool(a_ok and c_ok)}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    e = sub.add_parser("evaluate")
    for name in ("--criterion", "--inputs", "--replay", "--v8", "--out"):
        e.add_argument(name, type=Path, required=True)
    a = ap.parse_args(argv)
    manifest = json.loads((a.inputs / "inputs.json").read_text())
    res = evaluate(a.criterion, a.replay, a.v8, manifest)
    a.out.write_text(json.dumps(res, indent=2) + "\n")
    for r in res["rows"]:
        print(f"{r['input']:12s} b{r['board']} off {r['off_fraction']:.4f} ({r['off_px']}/{r['lit_px']}) "
              f"max {r['max_error']:.5f} {'ok' if r['a_hd_fidelity'] else 'FAIL'}")
    for r in res["motion"]:
        print(f"motion {r['input']:12s} b{r['board']} {r['motion']:.3f} (v8 {r['motion_v8']:.3f}) "
              f"{'ok' if r['c_motion_vs_v8'] else 'FAIL'}")
    for row in res["ladder"]:
        print(f"ladder {row['song']:11s} b{row['board']} {[round(v, 3) for v in row['motion_-6_0_+6']]} "
              f"{'ok' if row['not_falling'] else 'FAIL'}")
    print("overall", "PASS" if res["pass"] else "FAIL",
          f"a_hd_fidelity={res['a_hd_fidelity']}", f"c_motion={res['c_motion']}")
    return 0 if res["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
