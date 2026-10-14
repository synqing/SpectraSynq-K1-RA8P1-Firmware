#!/usr/bin/env python3
"""Lane P5: deterministic latency stimulus (EXECUTION-BRIEF §12 P5).

Layout (44.1 kHz stereo PCM16, same format as the BOOM fixture):
  2 s silence | SYNC (3 bursts, 120 ms apart) | 2 s | N clicks, gaps U(0.3, 1.2) s, seeded |
  2 s | BOOM 0-75 s | 2 s | SYNC | 2 s
The SYNC pattern replaces the brief's hand clap: the Titan's microphone and the iPhone's
audio track hear the same bursts, so device stamps and video align without a person
clapping. Each click is a 30 ms kick-like transient (decaying 150->55 Hz sweep plus a
3 ms noise edge) so it moves both the low band and the broadband level.

Writes <out>.wav and <out>.truth.json (every onset's sample index, by construction).
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
import soundfile as sf

ROOT = Path(__file__).resolve().parents[1]
BOOM = ROOT / "_scratch/waveform-migration-001/audio-boom-001/boom-0-75s-pcm16.wav"
SR = 44100
CLICK_S = 0.030
SYNC_BURST_S = 0.050
SYNC_GAP_S = 0.120


def click(rng: np.random.Generator, amp: float) -> np.ndarray:
    n = int(CLICK_S * SR)
    t = np.arange(n) / SR
    freq = 55.0 + 95.0 * np.exp(-t / 0.008)
    phase = 2 * np.pi * np.cumsum(freq) / SR
    body = np.sin(phase) * np.exp(-t / 0.012)
    edge = rng.standard_normal(n) * np.exp(-t / 0.001) * 0.6
    return amp * (body + edge) / np.max(np.abs(body + edge))


def sync(rng: np.random.Generator, amp: float) -> tuple[np.ndarray, list[int]]:
    burst_n = int(SYNC_BURST_S * SR)
    gap_n = int(SYNC_GAP_S * SR)
    t = np.arange(burst_n) / SR
    out, starts = [], []
    pos = 0
    for k in range(3):
        b = rng.standard_normal(burst_n) * np.exp(-t / 0.02)
        out.append(amp * b / np.max(np.abs(b)))
        starts.append(pos)
        pos += burst_n
        if k < 2:
            out.append(np.zeros(gap_n))
            pos += gap_n
    return np.concatenate(out), starts


def build(n_clicks: int, seed: int, amp: float) -> tuple[np.ndarray, dict]:
    rng = np.random.default_rng(seed)
    boom, boom_sr = sf.read(BOOM, dtype="float64", always_2d=True)
    assert boom_sr == SR, boom_sr
    boom_mono = boom.mean(axis=1)
    parts: list[np.ndarray] = []
    pos = 0
    truth = {"sync_start": [], "clicks": [], "boom_start": None, "sync_end": []}

    def add(x: np.ndarray) -> int:
        nonlocal pos
        start = pos
        parts.append(x)
        pos += len(x)
        return start

    add(np.zeros(2 * SR))
    s, starts = sync(rng, amp)
    base = add(s)
    truth["sync_start"] = [base + k for k in starts]
    add(np.zeros(2 * SR))
    gaps = rng.uniform(0.3, 1.2, size=n_clicks)
    for g in gaps:
        truth["clicks"].append(add(click(rng, amp)))
        add(np.zeros(int(round(g * SR)) - int(CLICK_S * SR)))
    add(np.zeros(2 * SR))
    truth["boom_start"] = add(boom_mono)
    add(np.zeros(2 * SR))
    s, starts = sync(rng, amp)
    base = add(s)
    truth["sync_end"] = [base + k for k in starts]
    add(np.zeros(2 * SR))
    mono = np.concatenate(parts)
    truth.update(sample_rate=SR, samples=len(mono), seconds=len(mono) / SR, seed=seed,
                 n_clicks=n_clicks, gap_range_s=[0.3, 1.2], click_s=CLICK_S, amplitude=amp,
                 boom_source=str(BOOM.relative_to(ROOT)),
                 boom_sha256=hashlib.sha256(BOOM.read_bytes()).hexdigest())
    return np.stack([mono, mono], axis=1), truth


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, required=True, help="output path without extension")
    ap.add_argument("--clicks", type=int, default=520)
    ap.add_argument("--seed", type=int, default=20261014)
    ap.add_argument("--amp", type=float, default=0.7)
    a = ap.parse_args()
    if a.clicks < 500:
        raise SystemExit("REFUSED: the brief requires at least 500 onsets")
    audio, truth = build(a.clicks, a.seed, a.amp)
    wav = a.out.with_suffix(".wav")
    sf.write(wav, audio, SR, subtype="PCM_16")
    truth["wav_sha256"] = hashlib.sha256(wav.read_bytes()).hexdigest()
    a.out.with_suffix(".truth.json").write_text(json.dumps(truth, indent=1) + "\n")
    print(json.dumps({k: truth[k] for k in ("seconds", "n_clicks", "wav_sha256")}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
