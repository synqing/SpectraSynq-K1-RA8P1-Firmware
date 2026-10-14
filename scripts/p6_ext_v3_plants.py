#!/usr/bin/env python3
"""Geometric replay overlays for the EXT-v3 TRAVEL gate.

These are not firmware. They paint the stored path-P frames so the frozen
TRAVEL clause can be shown to fail the wrong shapes and pass one inward front.
"""
from __future__ import annotations

import json
import shutil
import sys
from pathlib import Path

import numpy as np
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import p6_ext  # noqa: E402

TRAVEL_S = 0.12


def fill(frame: np.ndarray, i_min: int) -> None:
    frame[:] = 0
    frame[i_min:160] = 65535
    frame[0:160 - i_min] = 65535


def paint(pic: np.ndarray, t_song: np.ndarray, drops: list[float], kind: str) -> np.ndarray:
    """pic is uint16 (n, 2, 160, 3)."""
    out = pic.copy()
    y0 = p6_ext.display_luma(pic.astype(np.float64) / 65535.0)
    base = {b: p6_ext.half_trace(y0[:, b])[0] for b in (0, 1)}
    for td in drops:
        delay = 0.2 if kind == "d" else 0.0
        start = td + delay
        for board in (0, 1):
            before = np.where(t_song < start - 1e-9)[0]
            e0 = float(base[board][before[-1]]) if len(before) else 0.2
            if kind == "b":
                hit = np.where(t_song >= td - 1e-9)[0]
                if len(hit):
                    fill(out[hit[0], board], 80)
                continue
            if kind == "a":
                continue
            sel = np.where((t_song >= start - 1e-9) & (t_song <= start + TRAVEL_S + 1e-9))[0]
            for k in sel:
                u = min(1.0, max(0.0, (t_song[k] - start) / TRAVEL_S))
                ease = 1.0 - (1.0 - u) ** 3
                extent = e0 + (1.0 - e0) * ease if kind in ("d", "e") else 1.0 + (0.05 - 1.0) * ease
                i_min = int(np.clip(round(160.0 - extent * 80.0), 80, 159))
                fill(out[k, board], i_min)
    return out


def rewrite_csv(csv_path: Path, pic: np.ndarray, t_file: np.ndarray, out_csv: Path) -> None:
    y = p6_ext.display_luma(pic.astype(np.float64) / 65535.0)
    table = pd.read_csv(csv_path)
    extent = {}
    for board in (0, 1):
        extent[board] = p6_ext.half_trace(y[:, board])[0]
    # Probe rows are board 0 then board 1 at each 10 ms tick.
    if len(table) != pic.shape[0] * 2:
        raise SystemExit(f"REFUSED: {csv_path} rows {len(table)} != {pic.shape[0] * 2}")
    got = table.t_s.to_numpy()
    expect = np.repeat(t_file, 2)
    if float(np.max(np.abs(got - expect))) > 1e-4:
        raise SystemExit(f"REFUSED: {csv_path} clock does not match frames.bin")
    new = table.extent_edge.to_numpy().copy()
    for board in (0, 1):
        new[table.board.to_numpy() == board] = extent[board]
    table["extent_edge"] = new
    table.to_csv(out_csv, index=False)


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: p6_ext_v3_plants.py BASE_REPLAY OUT_DIR INPUTS.json", file=sys.stderr)
        return 2
    base, out, inputs_path = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
    manifest = json.loads(inputs_path.read_text())
    jobs = {
        "boom_source": [55.7],
        "boom_room": [55.7],
        "sonic_source": [27.7, 54.7],
    }
    kinds = {"a": "a", "b": "b", "c": "c", "d": "d", "e": "e"}
    for kind in kinds:
        dest = out / kind
        dest.mkdir(parents=True, exist_ok=True)
        for tag, drops in jobs.items():
            spec = next(s for s in manifest["inputs"].values() if tag in s["files"])
            raw = np.fromfile(base / f"{tag}.frames.bin", dtype="<u2")
            pic = raw.reshape(-1, 2, 160, 3)
            t_file = (np.arange(pic.shape[0]) + 1) * 0.01
            t_song = (t_file - spec["offset_s"]) / spec["scale"]
            painted = paint(pic, t_song, drops, kind)
            painted.astype("<u2").tofile(dest / f"{tag}.frames.bin")
            rewrite_csv(base / f"{tag}.csv", painted, t_file, dest / f"{tag}.csv")
            print(kind, tag, "frames", pic.shape[0], flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
