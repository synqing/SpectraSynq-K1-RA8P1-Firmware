#!/usr/bin/env python3
"""Lane P7 step 1, causal check of the 3:2 inference with truth by construction (diagnostic, NOT gate cases).

138 BPM four-on-the-floor kick plus a 3+3+2 stab (eighths 0, 3, 6 of each bar) at rising stab/kick ratios,
and a stab-only pattern. If the inference holds, stronger 3+3+2 accents move the product tracker to 92
(and the flat prior keeps 138). Writes diagnosis/accent-sweep.json.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
import pandas as pd

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import p7common as c  # noqa: E402
import p7_synth as s  # noqa: E402

LEVELS = [(1.0, 0.0), (1.0, 0.9), (1.0, 2.0), (1.0, 4.0), (0.5, 4.0), (0.0, 1.0), ("bounce", 0.0), ("bounce", 1.0)]


def render(kick_amp, stab_amp: float) -> np.ndarray:
    """kick_amp == "bounce": the KICK itself plays 3+3+2 (eighths 0, 3, 6) with a snare on beats 2 and 4,
    offbeat hats, and an optional 3+3+2 stab; truth stays 138 (backbeat level)."""
    rng = np.random.default_rng(1383320)
    kick, hat, stab, snare = s._kick(rng), s._hat(rng), s._stab(), s._snare(rng)
    if kick_amp == "bounce":
        a, dur, p = 1.0, 31.0, 60.0 / 138.0
        buf = np.zeros(int((dur + 0.5) * s.FS))
        beats = s._grid(a, dur, 138.0)
        for i, b in enumerate(beats):
            s._place(buf, hat, b + p / 2, 0.35)
            if i % 2 == 1:
                s._place(buf, snare, b, 0.9)
        for bar0 in beats[::4]:
            for e in (0, 3, 6):
                s._place(buf, kick, bar0 + e * p / 2, 1.0)
                if stab_amp:
                    s._place(buf, stab, bar0 + e * p / 2, stab_amp)
        buf *= 10 ** (-6 / 20) / np.max(np.abs(buf))
        return buf + 10 ** (-72 / 20) * rng.standard_normal(len(buf))
    a, dur, p = 1.0, 31.0, 60.0 / 138.0
    buf = np.zeros(int((dur + 0.5) * s.FS))
    beats = s._grid(a, dur, 138.0)
    for b in beats:
        s._place(buf, kick, b, kick_amp)
        s._place(buf, hat, b + p / 2, 0.35)
    for bar0 in beats[::4]:
        for e in (0, 3, 6):
            s._place(buf, stab, bar0 + e * p / 2, stab_amp)
    buf *= 10 ** (-6 / 20) / np.max(np.abs(buf))
    return buf + 10 ** (-72 / 20) * rng.standard_normal(len(buf))


def main() -> int:
    exe = c.build()
    out = {}
    for k, st in LEVELS:
        name = f"kick{k:g}_stab{st:g}" if k != "bounce" else f"bounce_kick332_snare24_stab{st:g}"
        p = c.write_s16(render(k, st), 1.0, c.WORK / f"acc_{name}.s16")
        c.run_probe(exe, p, c.FS, c.WORK / f"acc_{name}.csv", "all")
        d = pd.read_csv(c.WORK / f"acc_{name}.csv")
        out[name] = {}
        for v in ("baseline", "prior_flat"):
            w = d[(d.variant == v) & (d.t_s >= 13) & (d.t_s < 31)]
            out[name][v] = {kk: round(float(((w.bpm >= lo) & (w.bpm <= hi)).mean()), 3)
                            for kk, (lo, hi) in {"69": (68, 70), "92": (91, 93), "138": (137, 139)}.items()}
            out[name][v]["locked"] = round(float(w.locked.mean()), 3)
            out[name][v]["comb92_over_comb138_median"] = round(float((w.comb92 / w.comb138).median()), 3)
        print(name, json.dumps(out[name]), flush=True)
    (c.RUN / "diagnosis/accent-sweep.json").write_text(json.dumps(out, indent=1) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
