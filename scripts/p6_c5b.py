#!/usr/bin/env python3
"""Lane P6 C5b (TOLERANCE-FROZEN-P6-C5b.json): build and run the discriminating hue probe.

  p6_c5b.py --work DIR --out result.json

Exit status is the verdict: 0 only if the unfaulted v9 renderer has zero off-palette lit
pixels on both boards AND both planted faults (per-channel max, RGB blend) are red on both.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--work", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--palette", choices=("asis", "lifted", "range"), default="asis")  # iteration 4
    a = ap.parse_args(argv)
    a.work.mkdir(parents=True, exist_ok=True)
    exe = a.work / "p6_c5b_hue_probe"
    subprocess.run(["c++", "-std=c++17", "-O2", "-fno-fast-math", "-DK1_INHOUSE_MAPPING_V9=1",
                    "-DK1_INHOUSE_V9_HOST_VARIANTS=1", "-Isrc/k1", "-Isrc", "tests/host/p6_c5b_hue_probe.cpp",
                    "src/k1/core/visual/inhouse_waveform.cpp", "src/k1/core/visual/product_palette.cpp",
                    "src/k1/core/visual/product_palette_data.cpp", "-o", str(exe)], cwd=ROOT, check=True)
    runs = {}
    for fault in ("none", "channel_max", "rgb_blend"):
        out = subprocess.run([str(exe), fault, a.palette], capture_output=True, text=True, check=True).stdout
        runs[fault] = [json.loads(line) for line in out.splitlines()]
    clean = all(r["off_px"] == 0 and r["lit_px"] > 0 for r in runs["none"])
    red = {f: all(r["off_px"] > 0 for r in runs[f]) for f in ("channel_max", "rgb_blend")}
    res = {"criterion": "TOLERANCE-FROZEN-P6-C5b.json", "palette": a.palette, "runs": runs, "v9_clean": clean,
           "planted_red": red, "discriminating": all(red.values()), "pass": clean and all(red.values())}
    a.out.write_text(json.dumps(res, indent=2) + "\n")
    print(json.dumps({k: res[k] for k in ("v9_clean", "planted_red", "pass")}))
    return 0 if res["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
