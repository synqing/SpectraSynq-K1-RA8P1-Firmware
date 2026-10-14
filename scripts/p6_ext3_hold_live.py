#!/usr/bin/env python3
"""Score a hold that is still music. Reads P6-EXT3-HOLD-LIVE-FROZEN.json. Does not widen it.

The reference is the switched-off replay over the second after each drop.
Exit status is the verdict.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import p6_ext  # noqa: E402


def song_time(tag: str, inputs: dict, t_file: np.ndarray) -> np.ndarray:
    spec = next(s for s in inputs["inputs"].values() if tag in s["files"])
    return (t_file - spec["offset_s"]) / spec["scale"]


def load_raw(path: Path) -> np.ndarray:
    raw = np.fromfile(path, dtype="<u2")
    if raw.size % (2 * 160 * 3) != 0:
        raise SystemExit(f"REFUSED: {path} is not a probe frames.bin")
    return raw.reshape(-1, 2, 160, 3)


def load_silence(csv: Path, board: int, n: int) -> np.ndarray:
    d = pd.read_csv(csv, usecols=["t_s", "board", "silence"])
    d = d[d.board == board]
    if len(d) != n:
        raise SystemExit(f"REFUSED: {csv} board {board} has {len(d)} rows, frames have {n}")
    return d.silence.to_numpy()


def hold_index(t: np.ndarray, extent: np.ndarray, td: float) -> int | None:
    hit = np.where((t >= td) & (t <= td + 0.15 + 1e-9) & (extent >= 0.90))[0]
    return int(hit[0]) if len(hit) else None


def window_at(t: np.ndarray, t0: float, sustain_s: float) -> np.ndarray:
    return np.where((t >= t0 - 1e-9) & (t <= t0 + sustain_s + 1e-9))[0]


def lit_half(frame_half: np.ndarray) -> np.ndarray:
    peak = int(frame_half.max())
    if peak < 1:
        return np.zeros(frame_half.shape[0], dtype=bool)
    return frame_half.max(axis=-1) >= int(0.10 * peak)


def spread_and_change(field: np.ndarray, idx: np.ndarray) -> tuple[float, float]:
    """field is one edge field, (n, 160, 3) uint16. idx selects the window."""
    spreads, changes = [], []
    prev = None
    prev_lit = None
    for i in idx:
        half = field[i, 80:].astype(np.int32)
        lit = lit_half(half)
        both = lit[:-1] & lit[1:]
        if both.any():
            spreads.append(float(np.abs(np.diff(half, axis=0)).sum(axis=1)[both].mean()))
        if prev is not None and prev_lit is not None and prev_lit.any():
            delta = np.abs(field[i].astype(np.int32) - prev)
            changes.append(float(delta[prev_lit].mean()))
        prev = field[i].astype(np.int32)
        prev_lit = np.zeros(160, dtype=bool)
        prev_lit[80:] = lit
    spread = float(np.mean(spreads)) if spreads else 0.0
    change = float(np.mean(changes)) if changes else 0.0
    return spread, change


def static_run(field: np.ndarray, idx: np.ndarray, silence: np.ndarray, n_frames: int, run: int) -> dict:
    """A lit run of `run` pixels within ±1 count across n_frames, music not silent."""
    worst = 0
    faults = []
    last = int(idx[-1]) if len(idx) else -1
    for i in idx:
        end = i + n_frames
        if end > field.shape[0] or end - 1 > last:
            continue
        if np.any(silence[i:end] != 0):
            continue
        span = field[i:end].astype(np.int32)
        base = span[0]
        stable = np.all(np.abs(span - base) <= 1, axis=(0, 2))
        bright = np.ones(160, dtype=bool)
        for k in range(n_frames):
            peak = int(span[k].max())
            floor = int(0.10 * peak) if peak >= 1 else 1
            bright &= span[k].max(axis=-1) >= floor
        mask = stable & bright
        cur = 0
        best = 0
        best_at = 0
        for p, v in enumerate(mask):
            if v:
                cur += 1
                if cur > best:
                    best = cur
                    best_at = p - cur + 1
            else:
                cur = 0
        worst = max(worst, best)
        if best >= run:
            faults.append({"t_index": int(i), "pixels": int(best), "at": int(best_at)})
            break
    return {"pass": len(faults) == 0, "longest": int(worst), "faults": faults[:4]}


def ratio_ok(value: float, ref: float, fraction: float, floor: float) -> dict:
    if ref < floor:
        return {"value": value, "reference": ref, "ratio": None, "pass": False, "refused": True}
    ratio = value / ref
    return {"value": value, "reference": ref, "ratio": ratio, "pass": bool(ratio + 1e-12 >= fraction),
            "refused": False}


def score_replay(replay: Path, off: Path, inputs: dict, frozen: dict) -> dict:
    cases = {"boom_source": [55.7], "boom_room": [55.7], "sonic_source": [27.7, 54.7]}
    rows = []
    ok = True
    spread_frac = float(frozen["palette_spread"]["fraction"])
    change_frac = float(frozen["frame_change"]["fraction"])
    n_frames = int(frozen["static_run"]["frames"].split()[0])
    run = int(frozen["static_run"]["min_pixels"])
    for tag, drops in cases.items():
        on_raw = load_raw(replay / f"{tag}.frames.bin")
        off_raw = load_raw(off / f"{tag}.frames.bin")
        _, t_file = p6_ext.load_frames(replay / f"{tag}.frames.bin")
        _, t_off_file = p6_ext.load_frames(off / f"{tag}.frames.bin")
        if on_raw.shape[0] != t_file.shape[0] or off_raw.shape[0] != t_off_file.shape[0]:
            raise SystemExit(f"REFUSED: {tag} frame count does not match the probe clock")
        t = song_time(tag, inputs, t_file)
        t_off = song_time(tag, inputs, t_off_file)
        on_pic, _ = p6_ext.load_frames(replay / f"{tag}.frames.bin")
        for board in (0, 1):
            extent, _, _ = p6_ext.half_trace(p6_ext.display_luma(on_pic)[:, board])
            sil = load_silence(replay / f"{tag}.csv", board, on_raw.shape[0])
            for td in drops:
                start = hold_index(t, extent, td)
                if start is None:
                    ok = False
                    rows.append({"case": tag, "drop_s": td, "edge_field": board, "reached": False,
                                 "pass": False})
                    continue
                hold = window_at(t, float(t[start]), 1.0)
                ref = window_at(t_off, td, 1.0)
                if len(hold) < n_frames or len(ref) < 3:
                    raise SystemExit(f"REFUSED: {tag} field {board} drop {td} window is short")
                h_spread, h_change = spread_and_change(on_raw[:, board], hold)
                r_spread, r_change = spread_and_change(off_raw[:, board], ref)
                spread = ratio_ok(h_spread, r_spread, spread_frac, 50.0)
                change = ratio_ok(h_change, r_change, change_frac, 50.0)
                still = static_run(on_raw[:, board], hold, sil, n_frames, run)
                passed = bool(spread["pass"] and change["pass"] and still["pass"])
                ok &= passed
                rows.append({"case": tag, "drop_s": td, "edge_field": board, "reached": True,
                             "t_full": float(t[start]), "palette_spread": spread,
                             "frame_change": change, "static_run": still, "pass": passed})
    return {"pass": bool(ok), "rows": rows}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--replay", type=Path, required=True)
    ap.add_argument("--off", type=Path, required=True)
    ap.add_argument("--inputs", type=Path, required=True)
    ap.add_argument("--criterion", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    a = ap.parse_args(argv)
    frozen = json.loads(a.criterion.read_text())
    inputs = json.loads(a.inputs.read_text())
    res = score_replay(a.replay, a.off, inputs, frozen)
    res.update({"criterion": a.criterion.name, "frozen_sha256": frozen.get("frozen_sha256"),
                "replay": str(a.replay), "off": str(a.off)})
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(res, indent=2) + "\n")
    for row in res["rows"]:
        if not row.get("reached"):
            print(f"{row['case']:16s} {row['drop_s']:5.1f} field {row['edge_field']} NO HOLD fail")
            continue
        sp, ch = row["palette_spread"], row["frame_change"]
        print(f"{row['case']:16s} {row['drop_s']:5.1f} field {row['edge_field']} "
              f"spread {sp['ratio'] if sp['ratio'] is not None else float('nan'):.3f} "
              f"{'PASS' if sp['pass'] else 'fail'} "
              f"change {ch['ratio'] if ch['ratio'] is not None else float('nan'):.3f} "
              f"{'PASS' if ch['pass'] else 'fail'} "
              f"run {row['static_run']['longest']:3d} "
              f"{'PASS' if row['static_run']['pass'] else 'fail'} "
              f"{'PASS' if row['pass'] else 'fail'}")
    print("overall", "PASS" if res["pass"] else "FAIL")
    return 0 if res["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
