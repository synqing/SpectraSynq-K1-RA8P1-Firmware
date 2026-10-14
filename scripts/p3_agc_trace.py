#!/usr/bin/env python3
"""P3 follow-up (Captain, 14 Oct 2026): trace what sets Titan brightness.

Replays the BOOM recording through the v8 effects on the host with
tests/host/p3_tempo_probe.cpp (product ASRC -> InhouseAudio -> InhouseWaveformRenderer
-> renderInhouse display stage -> 0.25 output intensity) and writes per-frame CSVs.

Inputs, both calibrated by ONE scalar gain so the host's in-house raw RMS median over
BOOM 28-75 s equals the device's recorded median (inhouse_raw_rms in the snapshots):
  recording : the iPhone room audio of boom-admission-fix-candidate-001/original.mov,
              mapped to song time with the recording's own AV fit (offset, scale)
  source    : the BOOM song file used by the P2 gate (raw/boom-40k-mono.s16)
The device's own microphone PCM was not recorded, so neither input is the exact
device input. Offline, read-only; changes nothing on the device. Agent cannot listen.
"""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
REC = ROOT / "_scratch/waveform-migration-001/canonical-audio-v01/boom-admission-fix-candidate-001"
SRC = ROOT / "_scratch/waveform-migration-001/runs/P2-20261014T071112Z/permitted/raw/boom-40k-mono.s16"
P3 = ROOT / "_scratch/waveform-migration-001/runs/P3-20261014T080935Z/permitted/analysis/p3-data.json"
FS = 40000
SONG_S = 80.0


def build(work: Path) -> Path:
    work.mkdir(parents=True, exist_ok=True)
    flags = ["-O2", "-fno-fast-math", "-DK1_EMOTISCOPE_PORT=1", "-Iplatform/ra8p1", "-Isrc", "-Isrc/k1"]
    subprocess.run(["cc", "-std=c11", *flags, "-c", "platform/ra8p1/k1_asrc_24k.c", "-o", str(work / "asrc.o")],
                   cwd=ROOT, check=True)
    exe = work / "p3_tempo_probe"
    subprocess.run(["c++", "-std=c++17", *flags, "tests/host/p3_tempo_probe.cpp", "platform/ra8p1/inhouse_audio.cpp",
                    "src/k1/core/visual/inhouse_waveform.cpp", "src/k1/core/audio/gdft_goertzel.cpp",
                    "src/k1/core/audio/tempo_tracker.cpp", "src/k1/core/audio/tempo_acf.cpp",
                    "src/k1/core/visual/product_palette.cpp", "src/k1/core/visual/product_palette_data.cpp",
                    str(work / "asrc.o"), "-o", str(exe)], cwd=ROOT, check=True)
    return exe


def recording_pcm() -> np.ndarray:
    obs = json.loads((REC / "observation.json").read_text())
    av = obs["av_validation"]
    off, scale = av["source_to_movie_offset_s"], av["source_to_audio_scale"]
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(REC / "original.mov"), "-vn", "-ac", "1", "-ar",
                          str(FS), "-f", "f32le", "-"], capture_output=True, check=True).stdout
    x = np.frombuffer(raw, dtype="<f4").astype(np.float64)
    t_song = np.arange(int(SONG_S * FS)) / FS
    t_movie = t_song * scale + off  # same mapping as p3_expressiveness_audit: song = (movie - off) / scale
    return np.interp(t_movie * FS, np.arange(len(x)), x, left=0.0, right=0.0)


def source_pcm() -> np.ndarray:
    return np.fromfile(SRC, dtype="<i2").astype(np.float64) / 32768.0


def run(exe: Path, x: np.ndarray, gain: float, work: Path, name: str) -> Path:
    s16 = np.clip(np.round(x * gain * 32768.0), -32768, 32767).astype("<i2")
    p = work / f"{name}.s16"
    s16.tofile(p)
    out = work / f"{name}.csv"
    with out.open("w") as fh:
        subprocess.run([str(exe), str(p)], stdout=fh, check=True)
    p.unlink()
    return out


def host_raw_median(csv: Path) -> float:
    import pandas as pd
    d = pd.read_csv(csv)
    w = d[(d.board == 0) & (d.t_s >= 28) & (d.t_s <= 75)]
    return float(w.raw_rms.median())


def main(out_dir: Path):
    out_dir.mkdir(parents=True, exist_ok=True)
    work = Path("/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/agc-trace")
    exe = build(work)
    device = json.loads(P3.read_text())["boom"]
    target = float(np.median([s["inhouse_raw_rms"] for s in device["samples"]
                              if s.get("inhouse_raw_rms") is not None and 28 <= s["t_mir"] <= 75]))
    meta = {"device_raw_rms_median_28_75": target, "inputs": {}}
    for name, x in (("recording", recording_pcm()), ("source", source_pcm())):
        gain = 0.12 / max(np.sqrt(np.mean(x[28 * FS:75 * FS] ** 2)), 1e-9)  # first guess
        for _ in range(4):
            csv = run(exe, x, gain, work, name)
            got = host_raw_median(csv)
            gain *= target / got
        csv = run(exe, x, gain, work, name)
        final = host_raw_median(csv)
        dest = out_dir / f"agc-trace-boom-{name}.csv"
        dest.write_bytes(csv.read_bytes())
        clip = float(np.mean(np.abs(x * gain) >= 1.0))
        meta["inputs"][name] = {"gain": gain, "host_raw_rms_median_28_75": final, "clipped_fraction": clip,
                                "csv": dest.name}
        print(name, meta["inputs"][name], flush=True)
    (out_dir / "agc-trace-meta.json").write_text(json.dumps(meta, indent=2) + "\n")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
