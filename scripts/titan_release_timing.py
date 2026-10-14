#!/usr/bin/env python3
"""Light minus sound at the drop, from one camera recording that carries room audio.

Sound: the recording's own audio track is cross-correlated with the played clip
(both 8 kHz mono, 100 Hz-3 kHz band), which places the clip's first sample on the
video's clock; the drop is then clip_start + --drop-s. The correlation peak's
sharpness is reported (peak / second-best outside ±20 ms).
Light: the plate's centre band (middle 20 % of its lit width, panel rows) is
averaged per 30 fps frame. The release is the first frame, within ±0.5 s of the
sound drop, whose centre brightness crosses halfway from its median over
[-1.0, -0.2] s to its maximum over [0, +0.6] s; the crossing is interpolated
between frames.

Not instrument-grade: 30 fps frames (33 ms), and avfoundation's audio/video
alignment is assumed, not measured. The answer calibrates a host-timed arm to
about one frame.

  titan_release_timing.py --video V.mp4 --clip WAV --drop-s 15.766 --out DIR [--panel 107,220]
"""
from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from scipy import signal

W, H, FPS, SR = 640, 360, 30, 8000


def pcm(path: Path, stream: str = "a:0") -> np.ndarray:
    raw = subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(path), "-map", stream,
                          "-ac", "1", "-ar", str(SR), "-f", "f32le", "-"], capture_output=True, check=True).stdout
    x = np.frombuffer(raw, np.float32).astype(np.float64)
    sos = signal.butter(4, [100, 3000], btype="band", fs=SR, output="sos")
    return signal.sosfiltfilt(sos, x)


def clip_start(room: np.ndarray, clip: np.ndarray) -> dict:
    corr = signal.correlate(room, clip, mode="full", method="fft")
    norm = np.abs(corr)
    k = int(np.argmax(norm))
    guard = int(0.020 * SR)
    rest = norm.copy()
    rest[max(0, k - guard):k + guard] = 0
    lag = k - (len(clip) - 1)  # samples from the room track's start to the clip's first sample
    return {"start_s": lag / SR, "sharpness": float(norm[k] / max(rest.max(), 1e-12))}


def frames(video: Path, panel: tuple[int, int]) -> np.ndarray:
    raw = subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(video), "-vf",
                          f"fps={FPS},scale={W}:{H}", "-pix_fmt", "rgb24", "-f", "rawvideo", "-"],
                         capture_output=True, check=True).stdout
    f = np.frombuffer(raw, np.uint8).reshape(-1, H, W, 3)
    return f[:, panel[0]:panel[1]].astype(np.float32).mean(axis=(1, 3))  # (n, W) brightness profile


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--video", type=Path, required=True)
    ap.add_argument("--clip", type=Path, required=True)
    ap.add_argument("--drop-s", type=float, required=True)
    ap.add_argument("--panel", default="107,220")
    ap.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    panel = tuple(int(v) for v in a.panel.split(","))
    sound = clip_start(pcm(a.video), pcm(a.clip))
    # Decoded PCM starts at the audio stream's first timestamp and decoded frames
    # at the video stream's; put both on the video's clock.
    starts = {}
    for kind in ("a:0", "v:0"):
        out = subprocess.run(["ffprobe", "-v", "error", "-select_streams", kind, "-show_entries",
                              "stream=start_time", "-of", "csv=p=0", str(a.video)],
                             capture_output=True, text=True, check=True).stdout.strip()
        starts[kind] = float(out) if out not in ("", "N/A") else 0.0
    sound["audio_minus_video_start_s"] = starts["a:0"] - starts["v:0"]
    t_drop = sound["start_s"] + sound["audio_minus_video_start_s"] + a.drop_s
    prof = frames(a.video, panel)
    t = (np.arange(len(prof)) + 0.5) / FPS
    col = prof.mean(axis=0)
    lit = np.where(col > col.min() + 0.25 * (col.max() - col.min()))[0]
    x0, x1 = int(lit.min()), int(lit.max())
    c0, c1 = int(x0 + 0.4 * (x1 - x0)), int(x0 + 0.6 * (x1 - x0))
    centre = prof[:, c0:c1 + 1].mean(axis=1)
    pre = centre[(t >= t_drop - 1.0) & (t <= t_drop - 0.2)]
    post = centre[(t >= t_drop) & (t <= t_drop + 0.6)]
    lo, hi = float(np.median(pre)), float(post.max())
    half = lo + 0.5 * (hi - lo)
    idx = np.where((t >= t_drop - 0.5) & (t <= t_drop + 0.5) & (centre >= half))[0]
    release = None
    if len(idx) and idx[0] > 0:
        i = int(idx[0])
        f = (half - centre[i - 1]) / max(centre[i] - centre[i - 1], 1e-9)
        release = float(t[i - 1] + f * (t[i] - t[i - 1]))
    res = {"video": str(a.video), "clip": str(a.clip), "drop_s": a.drop_s, "sound": sound,
           "t_drop_video_s": t_drop, "plate_x": [x0, x1], "centre_x": [c0, c1], "centre_pre": lo,
           "centre_post_max": hi, "contrast": (hi - lo) / max(hi, 1e-9), "t_release_video_s": release,
           "light_minus_sound_ms": None if release is None else 1000 * (release - t_drop)}
    a.out.mkdir(parents=True, exist_ok=True)
    stem = a.video.stem
    (a.out / f"{stem}.timing.json").write_text(json.dumps(res, indent=2) + "\n")
    m = (t >= t_drop - 2.0) & (t <= t_drop + 2.0)
    fig, ax = plt.subplots(2, 1, figsize=(11, 7), gridspec_kw={"height_ratios": [1, 2]})
    ax[0].plot(1000 * (t[m] - t_drop), centre[m], ".-", lw=1)
    ax[0].axvline(0, color="#c8361f", label="sound drop (room audio)")
    if release is not None:
        ax[0].axvline(1000 * (release - t_drop), color="#1d4e89", ls="--", label=f"light release {res['light_minus_sound_ms']:+.0f} ms")
    ax[0].axhline(half, color="0.6", lw=0.8)
    ax[0].set_xlabel("ms from the sound drop")
    ax[0].set_ylabel("plate centre brightness")
    ax[0].legend(fontsize=8)
    ax[1].imshow(prof[m][:, x0:x1 + 1], aspect="auto", cmap="magma",
                 extent=[x0, x1, 1000 * (t[m][-1] - t_drop), 1000 * (t[m][0] - t_drop)])
    ax[1].axhline(0, color="#c8361f", lw=1)
    ax[1].set_xlabel("plate x (camera px)")
    ax[1].set_ylabel("ms from the sound drop")
    ax[1].set_title("plate brightness across its width, one row per frame")
    fig.suptitle(f"{stem}: light minus sound at the drop")
    fig.tight_layout()
    fig.savefig(a.out / f"{stem}.timing.png", dpi=110)
    print(json.dumps({k: res[k] for k in ("sound", "t_drop_video_s", "t_release_video_s", "light_minus_sound_ms", "contrast")}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
