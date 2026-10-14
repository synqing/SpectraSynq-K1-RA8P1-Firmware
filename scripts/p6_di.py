#!/usr/bin/env python3
"""Lane P6 iteration 5: drop impact (DI), TOLERANCE-FROZEN-P6-DI.json.

  p6_di.py anchors  SONG.s16|SONG.wav [--fs N]          # audio-only event anchors (song time)
  p6_di.py evaluate --replay DIR --inputs INPUTS --anchors ANCHORS.json --out RESULT.json [--plant-accent]

Anchors come from the stimulus audio only (0.1 s broadband RMS dB and the <150 Hz band):
  drop onset t_d  first 0.1 s frame whose <150 Hz band is >= 15 dB above the median of the
                  preceding 5 s and stays >= 10 dB above it (median) for the next 2 s; or, where
                  that first bass rise is not the main drop (BOOM: the intro and the build carry
                  bass), a declared onset: the first frame after the cut at the build-climax level
  build climax B* the 1.5 s window of highest mean broadband dB within [t_d - 10 s, t_d - 0.5 s]
  cut C           the longest run of frames <= (mean dB of B*) - 10 dB between B* and t_d,
                  if it lasts >= 1 s (else none; release R is then not computed)
DI per board = p95 of P over [t_d, t_d + 1 s] / p95 of P over B*, where P(t) is the frame mean
over 160 pixels of CIE L* of Rec.709 luma (tests/host/p6_effect_probe.cpp, column Lstar).
Secondary, reported: release R, extent X, sustain D (definitions in the frozen file).
Exit status is the verdict for evaluate: 0 only if DI >= 1.25 on both boards for every input.
"""
from __future__ import annotations

import argparse
import json
import math
import sys
import wave
from pathlib import Path

import numpy as np
import pandas as pd

BAR = 1.25


def load_audio(path: Path, fs: int | None) -> tuple[np.ndarray, int]:
    if path.suffix == ".wav":
        w = wave.open(str(path))
        return np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").astype(float) / 32768.0, w.getframerate()
    return np.fromfile(path, dtype="<i2").astype(float) / 32768.0, int(fs or 40000)


def anchors(x: np.ndarray, fs: int, drop_s: float | None = None) -> dict:
    n = int(fs * 0.1)
    k = len(x) // n
    frames = x[: k * n].reshape(k, n)
    db = 20 * np.log10(np.maximum(np.sqrt((frames ** 2).mean(1)), 1e-7))
    spec = np.fft.rfft(frames, axis=1)
    f = np.fft.rfftfreq(n, 1 / fs)
    bass = 20 * np.log10(np.sqrt((np.abs(spec[:, f < 150]) ** 2).sum(1)) + 1e-9)
    td = None if drop_s is None else int(round(drop_s * 10))
    for i in (range(50, k - 20) if td is None else ()):
        ref = np.median(bass[i - 50:i])
        if bass[i] >= ref + 15 and np.median(bass[i:i + 20]) >= ref + 10:
            td = i
            break
    if td is None:
        return {"valid": False}
    lo, hi = max(0, td - 100), td - 5
    means = [(db[j:j + 15].mean(), j) for j in range(lo, hi - 15 + 1)]
    climax_db, b0 = max(means)
    thresh = climax_db - 10
    best, run, start, cut = 0, 0, None, None
    for j in range(b0 + 15, td):
        if db[j] <= thresh:
            run += 1
            if run == 1:
                start = j
            if run > best:
                best, cut = run, (start, j + 1)
        else:
            run = 0
    return {"valid": True, "drop_onset_s": td / 10, "build_climax_s": [b0 / 10, (b0 + 15) / 10],
            "build_climax_mean_db": float(climax_db),
            "cut_s": [cut[0] / 10, cut[1] / 10] if cut and best >= 10 else None,
            "frame_s": 0.1, "rule": "see module docstring"}


def p95(v):
    return float(np.percentile(v, 95)) if len(v) else float("nan")


def evaluate(replay: Path, inputs: dict, anc: dict, plant_accent: bool) -> dict:
    rows, ok = [], True
    for tag, a in anc["inputs"].items():
        spec = next(s for s in inputs["inputs"].values() if tag in s["files"])
        d = pd.read_csv(replay / f"{tag}.csv")
        d["t_song"] = (d.t_s - spec["offset_s"]) / spec["scale"]
        td, (b0, b1) = a["drop_onset_s"], a["build_climax_s"]
        for board in (0, 1):
            x = d[d.board == board]
            w = lambda lo, hi: x[(x.t_song >= lo) & (x.t_song < hi)]
            drop = w(td, td + 1.0)
            dropP = drop.Lstar_x3 if plant_accent else drop.Lstar
            build = w(b0, b1)
            di = p95(dropP) / p95(build.Lstar) if p95(build.Lstar) > 0 else math.inf
            row = {"input": tag, "board": board, "DI": di, "p95_drop": p95(dropP), "p95_build": p95(build.Lstar),
                   "X_extent": float(np.percentile(w(td, td + 4.0).extent, 90) / max(np.percentile(build.extent, 90), 1e-9)),
                   "D_sustain": float(w(td, td + 4.0).Lstar.median() / max(np.percentile(build.Lstar, 90), 1e-9)),
                   "pass": bool(di >= BAR)}
            if a.get("cut_s"):
                c0, c1 = a["cut_s"]
                row["R_release"] = float(w(td + 0.05, td + 0.35).Lstar.median() / max(w(c0, c1).Lstar.median(), 1e-9))
            ok &= row["pass"]
            rows.append(row)
    return {"bar": BAR, "bar_status": "PROVISIONAL pending a Captain ladder calibration",
            "plant_accent_x3": plant_accent, "rows": rows, "pass": bool(ok)}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    an = sub.add_parser("anchors"); an.add_argument("audio", type=Path); an.add_argument("--fs", type=int)
    an.add_argument("--drop", type=float, help="declared drop onset (s) when the first bass rise is not the main drop")
    ev = sub.add_parser("evaluate")
    for k in ("--replay", "--inputs", "--anchors", "--out"):
        ev.add_argument(k, type=Path, required=True)
    ev.add_argument("--plant-accent", action="store_true")
    a = ap.parse_args(argv)
    if a.cmd == "anchors":
        x, fs = load_audio(a.audio, a.fs)
        print(json.dumps(anchors(x, fs, a.drop)))
        return 0
    res = evaluate(a.replay, json.loads((a.inputs / "inputs.json").read_text()), json.loads(a.anchors.read_text()),
                   a.plant_accent)
    a.out.write_text(json.dumps(res, indent=2) + "\n")
    print(json.dumps([(r["input"], r["board"], round(r["DI"], 3)) for r in res["rows"]] + [res["pass"]]))
    return 0 if res["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
