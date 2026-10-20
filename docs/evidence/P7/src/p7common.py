"""Shared helpers for lane P7 (tempo half-time / 3:2 lock): build the host probe, prepare inputs, run it.

Host only. Reads product sources, never modifies them. Offline; no device, broker or serial access.
"""
from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path

import numpy as np

RUN = Path(__file__).resolve().parents[1]                     # .../P7-<utc>/permitted
ROOT = Path(__file__).resolve().parents[6]                    # product worktree
WORK = Path("/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p7")
CANON = ROOT / "_scratch/waveform-migration-001/canonical-audio-v01"
BOOM_FILE = ROOT / "_scratch/waveform-migration-001/runs/P2-20261014T071112Z/permitted/raw/boom-40k-mono.s16"
FIXTURES = ROOT.parent / "titan-music-intelligence-001/tests/fixtures/tempo_field/v1"
FLAGS = ["-O2", "-fno-fast-math", "-DK1_EMOTISCOPE_PORT=1", "-Iplatform/ra8p1", "-Isrc", "-Isrc/k1"]
PRODUCT_SOURCES = ["platform/ra8p1/inhouse_audio.cpp", "src/k1/core/visual/inhouse_waveform.cpp",
                   "src/k1/core/audio/gdft_goertzel.cpp", "src/k1/core/audio/tempo_tracker.cpp",
                   "src/k1/core/audio/tempo_acf.cpp", "src/k1/core/visual/product_palette.cpp",
                   "src/k1/core/visual/product_palette_data.cpp"]
FS = 40000
# Device-calibrated replay gains for BOOM, from the committed P3 AGC trace (docs/evidence/P3/agc-trace-meta.json):
# one scalar per input so the host in-house raw-RMS median over 28-75 s equals the device's recorded median.
P3_META = ROOT / "docs/evidence/P3/agc-trace-meta.json"


def sha256(path: Path) -> str:
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def build() -> Path:
    WORK.mkdir(parents=True, exist_ok=True)
    subprocess.run(["cc", "-std=c11", *FLAGS, "-c", "platform/ra8p1/k1_poly24.c", "-o", str(WORK / "poly.o")],
                   cwd=ROOT, check=True)
    exe = WORK / "p7_tempo_probe"
    subprocess.run(["c++", "-std=c++17", *FLAGS, "-Wall", "-Wextra", str(RUN / "src/p7_tempo_probe.cpp"),
                    *PRODUCT_SOURCES, str(WORK / "poly.o"), "-o", str(exe)], cwd=ROOT, check=True)
    return exe


def write_s16(x: np.ndarray, gain: float, path: Path) -> Path:
    s16 = np.clip(np.round(x * gain * 32768.0), -32768, 32767).astype("<i2")
    s16.tofile(path)
    return path


def run_probe(exe: Path, s16: Path, rate: int, csv: Path, variants: str = "all") -> dict:
    r = subprocess.run([str(exe), str(s16), str(rate), str(csv), variants], capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(f"probe exit {r.returncode}: {r.stdout} {r.stderr}")
    return json.loads(r.stdout)


def room_pcm(dirname: str, seconds: float) -> np.ndarray:
    """iPhone room audio of a canonical recording, mapped to song time with its own AV fit (as P3/P4)."""
    obs = json.loads((CANON / dirname / "observation.json").read_text())["av_validation"]
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(CANON / dirname / "original.mov"), "-vn", "-ac", "1",
                          "-ar", str(FS), "-f", "f32le", "-"], capture_output=True, check=True).stdout
    x = np.frombuffer(raw, dtype="<f4").astype(np.float64)
    t_movie = np.arange(int(seconds * FS)) / FS * obs["source_to_audio_scale"] + obs["source_to_movie_offset_s"]
    return np.interp(t_movie * FS, np.arange(len(x)), x, left=0.0, right=0.0)


def wav24_to_40k(path: Path) -> np.ndarray:
    """A 24 kHz mono s16 fixture WAV, resampled 5/3 to 40 kHz (scipy polyphase) so it enters k1_poly24."""
    from scipy.io import wavfile
    from scipy.signal import resample_poly
    rate, x = wavfile.read(path)
    assert rate == 24000 and x.ndim == 1, (rate, x.shape)
    return resample_poly(x.astype(np.float64) / 32768.0, 5, 3)


def device_raw_rms_median(dirname: str, lo: float, hi: float) -> float:
    obs = json.loads((CANON / dirname / "observation.json").read_text())
    vals = [s["inhouse_raw_rms"] for s in obs["samples"]
            if s.get("inhouse_raw_rms") is not None and lo <= s.get("t_mir", -1) <= hi]
    return float(np.median(vals)) if vals else float("nan")


def hop_rms_median(x: np.ndarray, gain: float, lo: float, hi: float, hop: int = 300) -> float:
    """Median per-7.5 ms RMS of the replayed PCM (the in-house raw_rms analogue, 40 kHz domain)."""
    seg = np.clip(x[int(lo * FS):int(hi * FS)] * gain, -1.0, 32767 / 32768)
    n = len(seg) // hop
    return float(np.median(np.sqrt(np.mean(seg[: n * hop].reshape(n, hop) ** 2, axis=1))))
