#!/usr/bin/env python3
"""Lane P7 step 2(b) support: independent tempo estimates for LOCAL candidate tracks (no download).

For each candidate: decode 75 s mono at 22.05 kHz (ffmpeg), then
  librosa beat_track tempo; librosa tempogram's three strongest tempi (60-200 BPM);
  kick-band estimate: autocorrelation of the rectified log-energy rise of a <150 Hz band, strongest lag in 60-200.
These are PROPOSAL inputs for the Captain, not truth: the agent cannot listen.

  p7_corpus_scan.py OUT.json
"""
from __future__ import annotations

import hashlib
import json
import subprocess
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[6]
FIX = ROOT.parent / "titan-music-intelligence-001/tests/fixtures/tempo_field/v1"
MUSDB = Path("/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/datasets/musdb18")
SR = 22050

CANDIDATES = [  # (id, slot, path, offset_s, stream)
    ("boom", "required (BOOM)", ROOT / "_scratch/waveform-migration-001/audio-boom-001/boom-0-75s-pcm16.wav", 0, None),
    ("sonic-samba", "required (Sonic)", FIX / "trance/sonic-samba.wav", 0, None),
    ("dopex", "required (Dopex)", FIX / "trance/dopex.wav", 0, None),
    ("carte-blanche", "extra trance", FIX / "trance/carte-blanche.wav", 0, None),
]
MUSDB_SLOTS = {
    "Rap": "hip-hop 85-100", "Reggae": "reggae (NOT reggaeton/dembow)", "Electronic": "house 120-128 candidate",
    "Singer/Songwriter": "ballad < 80 candidate",
}


def musdb_candidates():
    rows = []
    for line in (MUSDB / "README.md").read_text().splitlines():
        if not line.startswith("- "):
            continue
        parts = line[2:].split(",")
        if len(parts) < 4 or parts[-3] not in MUSDB_SLOTS:
            continue
        name, genre = ",".join(parts[:-3]), parts[-3]
        for split in ("train", "test"):
            p = MUSDB / split / f"{name}.stem.mp4"
            if p.exists():
                rows.append((f"musdb:{name}", f"{MUSDB_SLOTS[genre]} (MUSDB genre {genre}, {split})", p, 30, "0:0"))
    return rows


def decode(path: Path, offset: float, stream: str | None) -> np.ndarray:
    cmd = ["ffmpeg", "-v", "error", "-ss", str(offset), "-t", "75", "-i", str(path)]
    if stream:
        cmd += ["-map", stream]
    cmd += ["-vn", "-ac", "1", "-ar", str(SR), "-f", "f32le", "-"]
    return np.frombuffer(subprocess.run(cmd, capture_output=True, check=True).stdout, dtype="<f4").astype(np.float64)


def kick_band_tempo(x: np.ndarray) -> dict:
    from scipy.signal import butter, sosfilt
    sos = butter(4, 150.0, "low", fs=SR, output="sos")
    y = sosfilt(sos, x)
    hop = 256
    n = len(y) // hop
    e = np.log10(np.mean(y[: n * hop].reshape(n, hop) ** 2, axis=1) + 1e-10)
    rise = np.maximum(np.diff(e), 0.0)
    rise -= rise.mean()
    ac = np.correlate(rise, rise, "full")[len(rise) - 1:]
    fps = SR / hop
    bpms = np.arange(60.0, 200.5, 0.5)
    vals = np.interp(fps * 60.0 / bpms, np.arange(len(ac)), ac) / ac[0]
    order = np.argsort(vals)[::-1]
    peaks = []
    for i in order:
        if all(abs(bpms[i] - p) > 4 for p, _ in peaks):
            peaks.append((float(bpms[i]), round(float(vals[i]), 3)))
        if len(peaks) == 3:
            break
    return {"top3_bpm_acf": peaks}


def main(out: Path) -> int:
    import librosa
    res = []
    for cid, slot, path, off, stream in CANDIDATES + musdb_candidates():
        x = decode(path, off, stream)
        tempo, _ = librosa.beat.beat_track(y=x.astype(np.float32), sr=SR)
        oenv = librosa.onset.onset_strength(y=x.astype(np.float32), sr=SR)
        tg = librosa.feature.tempogram(onset_envelope=oenv, sr=SR)
        freqs = librosa.tempo_frequencies(tg.shape[0], sr=SR)
        prof = tg.mean(axis=1)
        m = (freqs >= 60) & (freqs <= 200)
        idx = np.flatnonzero(m)[np.argsort(prof[m])[::-1]]
        top = []
        for i in idx:
            if all(abs(freqs[i] - t) > 4 for t, _ in top):
                top.append((round(float(freqs[i]), 1), round(float(prof[i]), 3)))
            if len(top) == 3:
                break
        rec = {"id": cid, "slot": slot, "path": str(path), "offset_s": off, "seconds": round(len(x) / SR, 1),
               "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
               "librosa_beat_track_bpm": round(float(np.atleast_1d(tempo)[0]), 1),
               "librosa_tempogram_top3": top, "kick_band": kick_band_tempo(x),
               "rms_dbfs": round(float(20 * np.log10(np.sqrt(np.mean(x ** 2)) + 1e-12)), 1)}
        res.append(rec)
        print(json.dumps({k: rec[k] for k in ("id", "librosa_beat_track_bpm", "librosa_tempogram_top3")}), flush=True)
    out.write_text(json.dumps({"librosa": librosa.__version__, "tracks": res}, indent=1) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(Path(sys.argv[1])))
