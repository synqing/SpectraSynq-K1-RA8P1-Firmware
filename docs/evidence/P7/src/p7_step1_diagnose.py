#!/usr/bin/env python3
"""Lane P7 step 1: reproduce the half-time / 3:2 lock on the host at 24 kHz and find which change moves it.

Runs p7_tempo_probe (product tracker + host-only variants) on the BOOM song file and room recording,
Sonic Samba and Dopex (song file and room recording), all at device-calibrated level; BOOM file also at
-6 dB and +6 dB. Writes per-update CSVs (gzip) and diagnosis/step1-summary.json.

  p7_step1_diagnose.py
"""
from __future__ import annotations

import gzip
import json
import shutil
import sys
from pathlib import Path

import numpy as np
import pandas as pd

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import p7common as c  # noqa: E402

OUT = c.RUN / "diagnosis"
VARIANTS = ["baseline", "prior_flat", "prior_120", "gdft_low", "pcm_kick", "flat_kick"]
FAMILY = {"69": (68, 70), "92": (91, 93), "138": (137, 139)}   # +-1 BPM bins (138 also covers 139)
SECTIONS = {
    "boom": {"intro 12-28": (12, 28), "build 28-45": (28, 45), "cut 45-55.5": (45, 55.5), "drop 55.5-75": (55.5, 75)},
    "sonic_file": {"12-75": (12, 75)}, "sonic_room": {"12-24 (slice time; source 18-30)": (12, 24)},
    "dopex": {"12-28": (12, 28), "28-75": (28, 75)},
}


def inputs() -> dict:
    meta = json.loads(c.P3_META.read_text())["inputs"]
    boom_file = np.fromfile(c.BOOM_FILE, dtype="<i2").astype(np.float64) / 32768.0
    boom_room = c.room_pcm("boom-admission-fix-candidate-001", 80.0)
    sonic_file = c.wav24_to_40k(c.FIXTURES / "trance/sonic-samba.wav")
    sonic_room = c.room_pcm("sonic-6-30-admission-fix-candidate-001", 26.0)
    dopex_file = c.wav24_to_40k(c.FIXTURES / "trance/dopex.wav")
    dopex_room = c.room_pcm("dopex-admission-fix-holdout-001", 80.0)
    # device medians of the in-house raw RMS (P3 p3-data.json): BOOM 28-75 0.1187, Sonic slice 0-24 0.1271,
    # Dopex 28-75 0.1181. BOOM gains are the committed P3 calibration; the others match the numpy hop-RMS median.
    tgt = {"sonic": 0.1271, "dopex": 0.1181}

    def cal(x, target, lo, hi):
        return target / c.hop_rms_median(x, 1.0, lo, hi)

    g = {
        "boom_file": meta["source"]["gain"], "boom_room": meta["recording"]["gain"],
        "sonic_file": cal(sonic_file, tgt["sonic"], 6, 30), "sonic_room": cal(sonic_room, tgt["sonic"], 0, 24),
        "dopex_file": cal(dopex_file, tgt["dopex"], 28, 75), "dopex_room": cal(dopex_room, tgt["dopex"], 28, 75),
    }
    out = {
        "boom_file": (boom_file, g["boom_file"]), "boom_file_m6dB": (boom_file, g["boom_file"] * 0.5),
        "boom_file_p6dB": (boom_file, g["boom_file"] * 2.0), "boom_room": (boom_room, g["boom_room"]),
        "sonic_file": (sonic_file, g["sonic_file"]), "sonic_room": (sonic_room, g["sonic_room"]),
        "dopex_file": (dopex_file, g["dopex_file"]), "dopex_room": (dopex_room, g["dopex_room"]),
    }
    return out


def section_key(name: str) -> str:
    return "boom" if name.startswith("boom") else name if name.startswith("sonic") else "dopex"


def summarise(d: pd.DataFrame, name: str) -> dict:
    res = {}
    for v in VARIANTS:
        g = d[d.variant == v]
        res[v] = {}
        for label, (lo, hi) in SECTIONS[section_key(name)].items():
            w = g[(g.t_s >= lo) & (g.t_s < hi)]
            if not len(w):
                continue
            fam = {k: float(((w.bpm >= a) & (w.bpm <= b)).mean()) for k, (a, b) in FAMILY.items()}
            fam["other"] = 1.0 - sum(fam.values())
            top = w.bpm.round().value_counts(normalize=True).head(3)
            res[v][label] = {
                "winner_share": {k: round(x, 3) for k, x in fam.items()},
                "top_bpm": {str(int(k)): round(float(x), 3) for k, x in top.items()},
                "locked_share": round(float(w.locked.mean()), 3),
                "confidence_median": round(float(w.confidence.median()), 3),
                "comb_median": {k: round(float(w[f"comb{k}"].median()), 3) for k in ("69", "92", "138")},
                "score_median": {k: round(float(w[f"score{k}"].median()), 3) for k in ("69", "92", "138")},
                "updates": int(len(w)),
            }
    return res


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "csv").mkdir(exist_ok=True)
    exe = c.build()
    summary = {"probe": "src/p7_tempo_probe.cpp", "probe_sha256": c.sha256(HERE / "p7_tempo_probe.cpp"),
               "inputs": {}, "results": {}}
    for name, (x, gain) in inputs().items():
        s16 = c.write_s16(x, gain, c.WORK / f"{name}.s16")
        csv = c.WORK / f"{name}.csv"
        info = c.run_probe(exe, s16, c.FS, csv, "all")
        clip = float(np.mean(np.abs(x * gain) >= 1.0))
        summary["inputs"][name] = {"gain": gain, "clipped_fraction": clip, "seconds": len(x) / c.FS,
                                   "hop_rms_median_28_75": c.hop_rms_median(x, gain, 28, 75) if len(x) > 75 * c.FS else None,
                                   "probe": info, "s16_sha256": c.sha256(s16)}
        d = pd.read_csv(csv)
        summary["results"][name] = summarise(d, name)
        with open(csv, "rb") as fi, gzip.open(OUT / "csv" / f"{name}.csv.gz", "wb", compresslevel=6) as fo:
            shutil.copyfileobj(fi, fo)
        s16.unlink()
        print(name, json.dumps(summary["inputs"][name]["probe"]), flush=True)
    (OUT / "step1-summary.json").write_text(json.dumps(summary, indent=1) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
