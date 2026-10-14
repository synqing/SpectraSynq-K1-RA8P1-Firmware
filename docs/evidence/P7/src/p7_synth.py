#!/usr/bin/env python3
"""Lane P7 step 2(a): deterministic synthetic tempo suite with truth by construction.

Every case is generated from its id alone (seed = first 8 bytes of sha256(id)), so a case is
bit-reproducible on the same numpy build. The truth is the list of beat times the generator itself
placed, per segment, plus segments that carry no tempo (lead-in, silence gaps, drumless pad).

Annotated level (Captain default, P7): the kick/backbeat level. Four-on-the-floor reports the kick;
half-time drums report the grid tempo whose eighths the hats play (DnB 174, not 87); swung eighths
report the quarter (kick 1/3, snare 2/4); hats-only reports the accented quarter.

Signal chain context: 40 kHz int16 mono, i.e. the PCM that enters k1_poly24 on the host (the device's
PDM decimation runs at 41.4 kHz). Levels are dBFS peak of that PCM. A -72 dBFS RMS seeded white noise
floor is present everywhere (no exact digital zero); silence = noise floor only.

  p7_synth.py list                       # case ids and truth summary (no audio)
  p7_synth.py generate OUT_DIR [ids...]  # writes <id>.s16, <id>.truth.json, manifest.json
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

import numpy as np

FS = 40000
LEAD_IN_S = 1.0
DEFAULT_LEVEL_DBFS = -6.0
NOISE_FLOOR_DBFS_RMS = -72.0
SUITE = "p7-synthetic-tempo-v1"


# ------------------------------------------------------------------------------------------------ case list

def case_list() -> list[dict]:
    cases: list[dict] = []
    for bpm in range(55, 201):
        cases.append({"id": f"fotf_{bpm}", "family": "four_on_the_floor", "bpm": float(bpm)})
    for bpm in range(55, 201):
        cases.append({"id": f"fotf332_{bpm}", "family": "four_on_the_floor_332", "bpm": float(bpm)})
    for bpm in (100, 120, 140, 150, 174):
        cases.append({"id": f"halftime_{bpm}", "family": "half_time", "bpm": float(bpm)})
    for bpm in (90, 110, 128, 160):
        cases.append({"id": f"swing_{bpm}", "family": "swung_eighths", "bpm": float(bpm)})
    for bpm in (100, 128, 174):
        cases.append({"id": f"hats_{bpm}", "family": "hats_only", "bpm": float(bpm)})
    for k in (1, 2, 3):
        cases.append({"id": f"pad_{k}", "family": "drumless_pad"})
    for a, b, label in ((120.0, 129.6, "p8"), (120.0, 110.4, "m8"), (140.0, 151.2, "p8"), (140.0, 128.8, "m8"),
                        (80.0, 160.0, "x2"), (160.0, 80.0, "xhalf"), (90.0, 180.0, "x2"), (180.0, 90.0, "xhalf")):
        cases.append({"id": f"step_{label}_{a:g}_{b:g}", "family": "tempo_step", "bpm": a, "bpm_after": b})
    for bpm in (90.0, 120.0, 140.0):
        cases.append({"id": f"rubato_{bpm:g}", "family": "rubato", "bpm": bpm, "depth": 0.05})
    for gap in (2.0, 5.0, 10.0):
        cases.append({"id": f"gap_{gap:g}s", "family": "silence_gap", "bpm": 120.0, "gap_s": gap})
    for level in (-40.0, -30.0, -20.0, -10.0, 0.0):
        cases.append({"id": f"level_{level:g}dBFS", "family": "level", "bpm": 120.0, "level_dbfs": level})
    return cases


# ------------------------------------------------------------------------------------------------ instruments

def _seed(case_id: str) -> int:
    return int.from_bytes(hashlib.sha256(case_id.encode()).digest()[:8], "little")


def _kick(rng):
    t = np.arange(int(0.30 * FS)) / FS
    f = 50.0 + 90.0 * np.exp(-t / 0.025)
    body = np.sin(2 * np.pi * np.cumsum(f) / FS) * np.exp(-t / 0.15)
    body = np.tanh(1.5 * body) / np.tanh(1.5)
    click = rng.standard_normal(len(t)) * np.exp(-t / 0.002) * 0.25
    return body + click


def _snare(rng):
    t = np.arange(int(0.25 * FS)) / FS
    n = np.diff(rng.standard_normal(len(t) + 1))
    return 0.35 * n * np.exp(-t / 0.07) + 0.5 * np.sin(2 * np.pi * 190.0 * t) * np.exp(-t / 0.05)


def _hat(rng):
    t = np.arange(int(0.08 * FS)) / FS
    n = np.diff(rng.standard_normal(len(t) + 2), n=2) / 2.0
    return n * np.exp(-t / 0.015)


def _stab():
    t = np.arange(int(0.30 * FS)) / FS
    env = np.minimum(t / 0.002, 1.0) * np.exp(-t / 0.12)
    tone = sum(np.sin(2 * np.pi * f * t) + 0.3 * np.sin(4 * np.pi * f * t) for f in (220.0, 277.18, 329.63, 440.0))
    return 0.25 * tone * env


def _place(buf, hit, at_s, amp):
    i = int(round(at_s * FS))
    if i >= len(buf):
        return
    n = min(len(hit), len(buf) - i)
    buf[i:i + n] += amp * hit[:n]


def _grid(start, end, bpm):
    p = 60.0 / bpm
    return [start + k * p for k in range(int(np.floor((end - start) / p - 1e-9)) + 1) if start + k * p < end]


# ------------------------------------------------------------------------------------------------ rendering

def render(case: dict) -> tuple[np.ndarray, dict]:
    """Return (float PCM at FS, truth). Truth segments: {kind: tempo|none, start_s, end_s, beats_s, grace}."""
    rng = np.random.default_rng(_seed(case["id"]))
    fam = case["family"]
    kick, snare, hat, stab = _kick(rng), _snare(rng), _hat(rng), _stab()
    segs: list[dict] = []
    a = LEAD_IN_S

    def drums_fotf(buf, beats, bpm, accent332=False):
        p = 60.0 / bpm
        for b in beats:
            _place(buf, kick, b, 1.0)
            _place(buf, hat, b + p / 2, 0.35)
        if accent332:
            for bar0 in beats[::4]:
                for e in (0, 3, 6):
                    _place(buf, stab, bar0 + e * p / 2, 0.9)

    if fam in ("four_on_the_floor", "four_on_the_floor_332", "level"):
        dur = a + 30.0
        buf = np.zeros(int((dur + 0.5) * FS))
        beats = _grid(a, dur, case["bpm"])
        drums_fotf(buf, beats, case["bpm"], fam == "four_on_the_floor_332")
        segs = [{"kind": "none", "start_s": 0.0, "end_s": a, "grace": "none_from_start"},
                {"kind": "tempo", "start_s": a, "end_s": dur + 0.5, "beats_s": beats, "grace": "acquire"}]
    elif fam == "half_time":
        dur = a + 30.0
        buf = np.zeros(int((dur + 0.5) * FS))
        beats = _grid(a, dur, case["bpm"])
        p = 60.0 / case["bpm"]
        for i, b in enumerate(beats):
            if i % 4 == 0:
                _place(buf, kick, b, 1.0)
            if i % 4 == 2:
                _place(buf, snare, b, 0.9)
            _place(buf, hat, b, 0.35)
            _place(buf, hat, b + p / 2, 0.35)
        segs = [{"kind": "none", "start_s": 0.0, "end_s": a, "grace": "none_from_start"},
                {"kind": "tempo", "start_s": a, "end_s": dur + 0.5, "beats_s": beats, "grace": "acquire"}]
    elif fam == "swung_eighths":
        dur = a + 30.0
        buf = np.zeros(int((dur + 0.5) * FS))
        beats = _grid(a, dur, case["bpm"])
        p = 60.0 / case["bpm"]
        for i, b in enumerate(beats):
            _place(buf, kick if i % 2 == 0 else snare, b, 1.0 if i % 2 == 0 else 0.9)
            _place(buf, hat, b, 0.35)
            _place(buf, hat, b + p * 2.0 / 3.0, 0.35)          # 2:1 swing
        segs = [{"kind": "none", "start_s": 0.0, "end_s": a, "grace": "none_from_start"},
                {"kind": "tempo", "start_s": a, "end_s": dur + 0.5, "beats_s": beats, "grace": "acquire"}]
    elif fam == "hats_only":
        dur = a + 30.0
        buf = np.zeros(int((dur + 0.5) * FS))
        beats = _grid(a, dur, case["bpm"])
        p = 60.0 / case["bpm"]
        for b in beats:
            _place(buf, hat, b, 1.0)
            _place(buf, hat, b + p / 2, 0.45)
        segs = [{"kind": "none", "start_s": 0.0, "end_s": a, "grace": "none_from_start"},
                {"kind": "tempo", "start_s": a, "end_s": dur + 0.5, "beats_s": beats, "grace": "acquire"}]
    elif fam == "drumless_pad":
        dur = a + 30.0
        buf = np.zeros(int((dur + 0.5) * FS))
        roots = {"C": 130.81, "D": 146.83, "E": 164.81, "F": 174.61, "G": 196.00, "A": 220.00}
        chords = [("C", (0, 4, 7, 12)), ("A", (0, 3, 7, 12)), ("F", (0, 4, 7, 12)), ("G", (0, 4, 7, 12)),
                  ("E", (0, 3, 7, 12)), ("D", (0, 3, 7, 12))]
        t_changes, t = [], a
        while t < dur - 1.5:
            t_changes.append(t)
            t += float(rng.uniform(1.5, 3.5))
        fade = int(0.6 * FS)
        tt = np.arange(len(buf)) / FS
        for k, tc in enumerate(t_changes):
            name, iv = chords[int(rng.integers(len(chords)))]
            f0 = roots[name]
            t_end = t_changes[k + 1] if k + 1 < len(t_changes) else dur
            i0, i1 = int(tc * FS), min(int(t_end * FS) + fade, len(buf))
            env = np.ones(i1 - i0)
            env[:fade] = 0.5 - 0.5 * np.cos(np.pi * np.arange(fade) / fade)
            env[-fade:] *= 0.5 + 0.5 * np.cos(np.pi * np.arange(fade) / fade)
            seg = sum(np.sin(2 * np.pi * f0 * 2 ** (s / 12) * tt[i0:i1] + float(rng.uniform(0, 2 * np.pi)))
                      for s in iv)
            buf[i0:i1] += 0.25 * seg * env
        segs = [{"kind": "none", "start_s": 0.0, "end_s": dur + 0.5, "grace": "none_from_start",
                 "chord_changes_s": t_changes}]
    elif fam == "tempo_step":
        a_end_nominal = a + 20.0
        beats_a = _grid(a, a_end_nominal, case["bpm"])
        t_step = beats_a[-1] + 60.0 / case["bpm"]
        dur = t_step + 20.0
        beats_b = _grid(t_step, dur, case["bpm_after"])
        buf = np.zeros(int((dur + 0.5) * FS))
        drums_fotf(buf, beats_a, case["bpm"])
        drums_fotf(buf, beats_b, case["bpm_after"])
        segs = [{"kind": "none", "start_s": 0.0, "end_s": a, "grace": "none_from_start"},
                {"kind": "tempo", "start_s": a, "end_s": t_step, "beats_s": beats_a, "grace": "acquire"},
                {"kind": "tempo", "start_s": t_step, "end_s": dur + 0.5, "beats_s": beats_b, "grace": "step"}]
    elif fam == "rubato":
        dur = a + 40.0
        t0, d = case["bpm"], case["depth"]
        knots_t = np.array([0.0, 10.0, 30.0, 40.0]) + a
        knots_v = np.array([t0, t0 * (1 + d), t0 * (1 - d), t0])
        fine = np.arange(a, dur, 1e-4)
        phase = np.concatenate([[0.0], np.cumsum(np.interp(fine[:-1], knots_t, knots_v) / 60.0 * 1e-4)])
        beats = [float(np.interp(k, phase, fine)) for k in range(int(phase[-1]) + 1)]
        buf = np.zeros(int((dur + 0.5) * FS))
        for i, b in enumerate(beats):
            _place(buf, kick, b, 1.0)
            nxt = beats[i + 1] if i + 1 < len(beats) else b + 60.0 / t0
            _place(buf, hat, (b + nxt) / 2, 0.35)
        segs = [{"kind": "none", "start_s": 0.0, "end_s": a, "grace": "none_from_start"},
                {"kind": "tempo", "start_s": a, "end_s": dur + 0.5, "beats_s": beats, "grace": "acquire"}]
    elif fam == "silence_gap":
        bpm = case["bpm"]
        beats1 = _grid(a, a + 20.0, bpm)
        g0 = beats1[-1] + 60.0 / bpm
        g1 = g0 + case["gap_s"]
        dur = g1 + 20.0
        beats2 = _grid(g1, dur, bpm)
        buf = np.zeros(int((dur + 0.5) * FS))
        drums_fotf(buf, beats1, bpm)
        drums_fotf(buf, beats2, bpm)
        segs = [{"kind": "none", "start_s": 0.0, "end_s": a, "grace": "none_from_start"},
                {"kind": "tempo", "start_s": a, "end_s": g0, "beats_s": beats1, "grace": "acquire"},
                {"kind": "none", "start_s": g0, "end_s": g1, "grace": "release", "release_bpm": bpm},
                {"kind": "tempo", "start_s": g1, "end_s": dur + 0.5, "beats_s": beats2, "grace": "acquire"}]
    else:
        raise ValueError(fam)

    level = case.get("level_dbfs", DEFAULT_LEVEL_DBFS)
    peak = np.max(np.abs(buf))
    if peak > 0:
        buf *= 10 ** (level / 20.0) / peak
    noise_rms = 10 ** (NOISE_FLOOR_DBFS_RMS / 20.0)
    buf += noise_rms * rng.standard_normal(len(buf))
    truth = {"suite": SUITE, "id": case["id"], "case": case, "fs": FS, "duration_s": len(buf) / FS,
             "level_dbfs_peak": level, "segments": segs}
    return buf, truth


def to_s16(x: np.ndarray) -> np.ndarray:
    return np.clip(np.round(x * 32768.0), -32768, 32767).astype("<i2")


def generate(out: Path, ids: list[str] | None = None) -> dict:
    out.mkdir(parents=True, exist_ok=True)
    manifest = {"suite": SUITE, "fs": FS, "numpy": np.__version__, "cases": []}
    for case in case_list():
        if ids and case["id"] not in ids:
            continue
        x, truth = render(case)
        s16 = to_s16(x)
        (out / f"{case['id']}.s16").write_bytes(s16.tobytes())
        (out / f"{case['id']}.truth.json").write_text(json.dumps(truth) + "\n")
        manifest["cases"].append({"id": case["id"], "pcm_sha256": hashlib.sha256(s16.tobytes()).hexdigest(),
                                  "truth_sha256": hashlib.sha256((out / f"{case['id']}.truth.json").read_bytes()).hexdigest(),
                                  "samples": int(len(s16))})
    (out / "manifest.json").write_text(json.dumps(manifest, indent=1) + "\n")
    return manifest


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("list")
    g = sub.add_parser("generate")
    g.add_argument("out", type=Path)
    g.add_argument("ids", nargs="*")
    a = ap.parse_args(argv)
    if a.cmd == "list":
        for c in case_list():
            print(json.dumps(c))
        return 0
    m = generate(a.out, a.ids or None)
    print(json.dumps({"cases": len(m["cases"])}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
