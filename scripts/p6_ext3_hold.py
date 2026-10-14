#!/usr/bin/env python3
"""Score the EXT-v3 extent hold and the drop-row black frame.

Reads P6-EXT3-HOLD-FROZEN.json. Does not widen it. Exit status is the verdict.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
RUN = ROOT / "_scratch/waveform-migration-001/runs/P6-20261014T124826Z/permitted"
sys.path.insert(0, str(ROOT / "scripts"))
import p6_ext  # noqa: E402


def song_time(tag: str, inputs: dict, t_file: np.ndarray) -> np.ndarray:
    spec = next(s for s in inputs["inputs"].values() if tag in s["files"])
    return (t_file - spec["offset_s"]) / spec["scale"]


def hold_ok(t: np.ndarray, extent: np.ndarray, td: float, line: float, sustain_s: float) -> dict:
    hit = np.where((t >= td) & (t <= td + 0.15 + 1e-9) & (extent >= line))[0]
    if len(hit) == 0:
        return {"reached": False, "sustain_s": 0.0, "pass": False}
    t0 = float(t[hit[0]])
    window = (t >= t0 - 1e-9) & (t <= t0 + sustain_s + 1e-9)
    kept = bool(np.all(extent[window] >= line - 1e-9))
    after = np.where(t >= t0 - 1e-9)[0]
    dip = after[extent[after] < line - 1e-9]
    above_s = float(t[dip[0]] - t0) if len(dip) else float(t[after[-1]] - t0)
    return {"reached": True, "t_full": t0, "lag_s": t0 - td,
            "sustain_s": float(t[window][-1] - t0) if window.any() else 0.0,
            "above_s": above_s, "frames": int(window.sum()), "pass": kept}


def black_ok(pic: np.ndarray, t: np.ndarray, silence: np.ndarray, td: float) -> dict:
    """pic is one edge field, (n, 160, 3) uint16. silence is aligned to t."""
    window = np.where((t >= td - 1e-9) & (t <= td + 0.05 + 1e-9))[0]
    bad = []
    for i in window:
        fully = bool(pic[i].max() == 0)
        interior = pic[i, 8:152].max() == 0
        prev_lit = i > 0 and pic[i - 1, 8:152].max() > 0
        edge_cut = bool(interior and prev_lit)
        quiet = bool(silence[i] == 1)
        if (fully or edge_cut) and not quiet:
            bad.append({"t": float(t[i]), "fully_black": fully, "edge_cut": edge_cut})
    return {"pass": len(bad) == 0, "faults": bad}


def load_silence(csv: Path, board: int, n: int) -> np.ndarray:
    d = pd.read_csv(csv, usecols=["t_s", "board", "silence"])
    d = d[d.board == board]
    if len(d) != n:
        raise SystemExit(f"REFUSED: {csv} board {board} has {len(d)} rows, frames have {n}")
    return d.silence.to_numpy()


def score(replay: Path, inputs: dict, frozen: dict, plant_black: bool) -> dict:
    line = float(frozen["hold"]["full_extension"])
    sustain_s = float(frozen["hold"]["sustain_ms"]) / 1000.0
    cases = {
        "boom_source": [55.7],
        "boom_room": [55.7],
        "sonic_source": [27.7, 54.7],
    }
    rows = []
    ok = True
    for tag, drops in cases.items():
        path = replay / f"{tag}.frames.bin"
        pic, t_file = p6_ext.load_frames(path)
        t = song_time(tag, inputs, t_file)
        y = p6_ext.display_luma(pic)
        if plant_black:
            pic = pic.copy()
            for td in drops:
                at = np.where(t >= td - 1e-9)[0]
                if len(at) == 0:
                    raise SystemExit(f"REFUSED: {tag} has no frame at {td}")
                pic[int(at[0])] = 0
        for board in (0, 1):
            extent, _, _ = p6_ext.half_trace(y[:, board] if not plant_black else p6_ext.display_luma(pic)[:, board])
            silence = load_silence(replay / f"{tag}.csv", board, pic.shape[0])
            field = pic[:, board]
            for td in drops:
                held = hold_ok(t, extent, td, line, sustain_s)
                dark = black_ok(field, t, silence, td)
                passed = (not plant_black and held["pass"] and dark["pass"]) or (
                    plant_black and not dark["pass"])
                # A black plant is a check of the black clause only. The hold
                # numbers on a zeroed frame are reported and are not the verdict.
                if plant_black:
                    passed = not dark["pass"]
                ok &= passed
                rows.append({"case": tag, "drop_s": td, "edge_field": board,
                             "hold": held, "black": dark, "pass": bool(passed)})
    return {"pass": bool(ok), "plant_black": plant_black, "rows": rows}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--replay", type=Path, required=True)
    ap.add_argument("--inputs", type=Path, required=True)
    ap.add_argument("--criterion", type=Path, default=RUN / "P6-EXT3-HOLD-FROZEN.json")
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--plant-black", action="store_true")
    ap.add_argument("--switch", default="")
    ap.add_argument("--argv-json", default="")
    a = ap.parse_args(argv)
    frozen = json.loads(a.criterion.read_text())
    inputs = json.loads(a.inputs.read_text())
    res = score(a.replay, inputs, frozen, a.plant_black)
    res.update({"criterion": str(a.criterion.name), "frozen_sha256": frozen.get("frozen_sha256"),
                "replay": str(a.replay), "switch": a.switch, "argv": a.argv_json})
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(res, indent=2) + "\n")
    for row in res["rows"]:
        h = row["hold"]
        print(f"{row['case']:16s} {row['drop_s']:5.1f} field {row['edge_field']} "
              f"hold {'PASS' if h['pass'] else 'fail'} above {h.get('above_s', 0):.3f}s "
              f"black {'PASS' if row['black']['pass'] else 'fail'} "
              f"{'PASS' if row['pass'] else 'fail'}")
    print("overall", "PASS" if res["pass"] else "FAIL")
    return 0 if res["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
