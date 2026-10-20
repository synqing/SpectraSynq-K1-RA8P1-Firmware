#!/usr/bin/env python3
"""DBS pilot: noise-flux salience floor of the frozen onset reference, per analysis rate.

The reference detector (scripts/p6_dbs_reference_onsets.py, frozen delta) fires on the
attack-free sustain probe. Its strongest steady-span (3-11 s) detection, measured as
picked_up_to_delta, is the floor below which a reference onset cannot be told apart from
noise flux. BOOM (44.1/40 kHz) uses the value recorded by the reference tool; Sonic is
12.8 kHz, so the same probe is resampled to 12.8 kHz and measured with Detector(12800).
Descriptive engineering yardstick, not a musical-salience threshold.

  uv run --quiet --with numpy==2.1.3 --with scipy==1.14.1 --with matplotlib==3.9.2 \
      python scripts/p6_dbs_salience_floor.py
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
from scipy.ndimage import uniform_filter1d
from scipy.signal import resample_poly

sys.path.insert(0, str(Path(__file__).resolve().parent))
import p6_dbs_reference_onsets as ref  # noqa: E402


def floor_at(fs: int, delta: float) -> float:
    sustain = ref.load_s16(ref.PROBES / "sustain.s16")
    if fs != 40000:
        g = np.gcd(fs, 40000)
        sustain = resample_poly(sustain, fs // g, 40000 // g)
    det = ref.Detector(fs)
    r = det.compute(sustain)
    k = det.pick(r["odf"], delta)
    avg_n = 2 * int(round(ref.PARAMS["thr_avg_s"] / det.hop_s)) + 1
    thr = uniform_filter1d(r["odf"], size=avg_n, mode="constant") + delta
    steady = (r["t"][k] >= 3) & (r["t"][k] < 11)
    pu = r["odf"][k][steady] - (thr[k][steady] - delta)
    return float(pu.max())


def main() -> None:
    boom = json.loads((ref.OUT / "boom-onsets.json").read_text())
    delta = float(boom["params"]["delta"] if "delta" in boom["params"] else boom["params"]["frozen_delta"])
    out = {"delta": delta, "boom_40k_floor": round(floor_at(40000, delta), 3),
           "sonic_12k8_floor": round(floor_at(12800, delta), 3)}
    sonic = json.loads((ref.OUT / "sonic-6-30-onsets.json").read_text())
    out["boom_scored_above"] = sum(o["scored"] and o["picked_up_to_delta"] > out["boom_40k_floor"] for o in boom["onsets"])
    out["sonic_scored_above"] = sum(o["scored"] and o["picked_up_to_delta"] > out["sonic_12k8_floor"] for o in sonic["onsets"])
    print(json.dumps(out, indent=1))


if __name__ == "__main__":
    main()
