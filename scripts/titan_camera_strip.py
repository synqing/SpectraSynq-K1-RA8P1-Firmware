#!/usr/bin/env python3
"""Turn an iPhone recording of the Titan into a space x time strip the agent can read.

Lane P6 evaluation aid (Captain, 15 Oct 2026). For each video frame, the light-guide panel
is cropped and its colour is averaged down the panel's height, giving one row of RGB per
frame across the panel's width. Rows stack into a strip (time downwards), directly comparable
with the host replay strips (figures/pal/*strip*). The audio clock is aligned by
cross-correlating panel brightness with the played WAV's loudness envelope.

This is a camera view (exposure, white balance and gamma are the phone's), not a photometric
measurement; use it to see motion, extent and colour spread, not absolute levels.

  titan_camera_strip.py --video V.mp4 --wav boom.wav --out strip.png [--panel y0,y1] [--t0 S --t1 S]
"""
from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path

import numpy as np

W, H, FPS = 640, 360, 30


def frames(video: Path) -> np.ndarray:
    cmd = ['ffmpeg', '-hide_banner', '-loglevel', 'error', '-i', str(video), '-vf',
           f'fps={FPS},scale={W}:{H}', '-pix_fmt', 'rgb24', '-f', 'rawvideo', '-']
    raw = subprocess.run(cmd, capture_output=True, check=True).stdout
    return np.frombuffer(raw, np.uint8).reshape(-1, H, W, 3)


def panel_rows(stack: np.ndarray) -> tuple[int, int]:
    # The panel is the brightest horizontal band on average over the clip.
    prof = stack.astype(np.float32).mean(axis=(0, 2, 3))
    thr = prof.min() + 0.5 * (prof.max() - prof.min())
    rows = np.where(prof > thr)[0]
    return int(rows.min()), int(rows.max()) + 1


def envelope_wav(wav: Path, rate: int) -> np.ndarray:
    cmd = ['ffmpeg', '-hide_banner', '-loglevel', 'error', '-i', str(wav), '-ac', '1', '-ar', '8000',
           '-f', 's16le', '-']
    x = np.frombuffer(subprocess.run(cmd, capture_output=True, check=True).stdout, np.int16).astype(np.float32)
    hop = 8000 // rate
    n = len(x) // hop
    return np.sqrt((x[:n * hop].reshape(n, hop) ** 2).mean(axis=1) + 1e-9)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--video', type=Path, required=True)
    ap.add_argument('--wav', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--panel', default=None, help='y0,y1 rows of the panel at 640x360')
    ap.add_argument('--t0', type=float, default=None, help='song seconds')
    ap.add_argument('--t1', type=float, default=None)
    ap.add_argument('--marks', default='', help='comma list of song seconds to mark')
    a = ap.parse_args()

    st = frames(a.video)
    y0, y1 = map(int, a.panel.split(',')) if a.panel else panel_rows(st)
    ym = (y0 + y1) // 2
    top = st[:, y0:ym].astype(np.float32).mean(axis=1)              # frames x W x 3 (upper half)
    bot = st[:, ym:y1].astype(np.float32).mean(axis=1)              # lower half
    lum = (top.mean(axis=(1, 2)) + bot.mean(axis=(1, 2))) / 2
    gap = np.zeros((len(top), 12, 3), np.float32)
    strip = np.concatenate([top, gap, bot], axis=1)
    env = envelope_wav(a.wav, FPS)
    # Lag search: song time = video time - offset. Correlate log-brightness with log-loudness.
    lv = np.log(lum + 1.0); lv = (lv - lv.mean()) / (lv.std() + 1e-9)
    le = np.log(env + 1.0); le = (le - le.mean()) / (le.std() + 1e-9)
    best = (-9.0, 0)
    for off in range(0, len(lv) - len(le) // 2):
        seg = lv[off:off + len(le)]
        m = min(len(seg), len(le))
        if m < FPS * 20:
            break
        r = float(np.dot(seg[:m], le[:m]) / m)
        if r > best[0]:
            best = (r, off)
    offset_s = best[1] / FPS
    t_song = np.arange(len(lum)) / FPS - offset_s

    sel = np.ones(len(t_song), bool)
    if a.t0 is not None:
        sel &= t_song >= a.t0
    if a.t1 is not None:
        sel &= t_song <= a.t1
    img = strip[sel]
    img = np.clip(img / max(np.percentile(img, 99.5), 1.0), 0, 1)

    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    ts = t_song[sel]
    fig, ax = plt.subplots(figsize=(7, 10))
    ax.imshow(img, aspect='auto', extent=[0, 1, ts[-1], ts[0]], interpolation='nearest')
    for m in [float(v) for v in a.marks.split(',') if v.strip()]:
        if ts[0] <= m <= ts[-1]:
            ax.axhline(m, color='cyan', lw=0.8, ls='--')
            ax.text(1.01, m, f'{m:g} s', color='black', va='center', fontsize=8, transform=ax.get_yaxis_transform())
    ax.set_xlabel('upper half of panel  |  lower half of panel   (each: left end to right end, camera view)')
    ax.set_ylabel('song time (s), aligned by loudness correlation')
    ax.set_title(f'{a.video.name}\ncamera strip, panel rows {y0}-{y1} of {H}; offset {offset_s:.2f} s (r {best[0]:.2f})', fontsize=9)
    fig.tight_layout()
    a.out.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(a.out, dpi=130)
    meta = {'video': str(a.video), 'wav': str(a.wav), 'panel_rows': [y0, y1], 'offset_s': offset_s,
            'align_r': best[0], 'frames': int(len(lum)), 'out': str(a.out)}
    a.out.with_suffix('.json').write_text(json.dumps(meta, indent=1))
    print(json.dumps(meta))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
