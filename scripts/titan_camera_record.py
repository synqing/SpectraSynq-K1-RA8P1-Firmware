#!/usr/bin/env python3
"""Record the Titan through the Captain's iPhone (Continuity Camera) so the agent can see the light.

Lane P6 evaluation aid (Captain, 15 Oct 2026: "reinstate the iPhone camera to record all these
evaluation/test runs so YOU can see clearly what I see"). It is a recording, not an instrument:
nothing here is a latency or colour measurement.

  titan_camera_record.py probe                       # 2 s test clip; fails fast if the camera stalls
  titan_camera_record.py record --seconds 80 --label boom-host-006
  titan_camera_record.py sheet --video V --every 0.5 # contact sheet of frames for viewing

Heavy video is written to the external drive (storage policy, >50 MB). A stalled camera open
(locked phone not mounted, macOS permission prompt) is killed after --open-timeout seconds.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
from pathlib import Path

DEVICE = 'EiP Camera'
ROOT = Path('/Volumes/Disk Storage/SpectraSynq-Scratch/titan-lagfree-20261014/camera')


def record(out: Path, seconds: float, open_timeout: float) -> dict:
    out.parent.mkdir(parents=True, exist_ok=True)
    cmd = ['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-f', 'avfoundation',
           '-framerate', '30', '-video_size', '1920x1080', '-i', f'{DEVICE}:none',
           '-t', f'{seconds:.1f}', '-c:v', 'h264_videotoolbox', '-b:v', '12M', str(out)]
    start = time.monotonic()
    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    # The file appears once frames flow. If it never appears, the camera open has stalled.
    while proc.poll() is None:
        if not out.exists() and time.monotonic() - start > open_timeout:
            proc.kill()
            proc.wait()
            return {'ok': False, 'error': f'camera open stalled > {open_timeout:.0f} s '
                    '(mount and lock the iPhone; allow the Mac camera prompt)', 'cmd': cmd}
        if time.monotonic() - start > seconds + open_timeout + 30:
            proc.kill()
            proc.wait()
            return {'ok': False, 'error': 'recording overran its deadline', 'cmd': cmd}
        time.sleep(0.5)
    err = proc.stderr.read().decode(errors='replace')[-800:] if proc.stderr else ''
    ok = proc.returncode == 0 and out.exists() and out.stat().st_size > 0
    return {'ok': ok, 'returncode': proc.returncode, 'bytes': out.stat().st_size if out.exists() else 0,
            'stderr_tail': err, 'path': str(out), 'started_monotonic': start}


def sheet(video: Path, every: float, out: Path) -> dict:
    out.parent.mkdir(parents=True, exist_ok=True)
    cmd = ['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-i', str(video),
           '-vf', f'fps=1/{every},scale=480:-1,tile=6x6', '-frames:v', '1', str(out)]
    rc = subprocess.run(cmd).returncode
    return {'ok': rc == 0 and out.exists(), 'path': str(out)}


QUEUE = ROOT / 'queue'


def serve(open_timeout: float) -> int:
    """Run from a plain Cursor terminal tab (Cursor holds the macOS camera grant).

    Agent sessions are not attributed to Cursor by macOS (camera authorisation reads
    notDetermined there and the open hangs), so they queue requests here instead.
    """
    QUEUE.mkdir(parents=True, exist_ok=True)
    print(f'camera server: watching {QUEUE} (Ctrl-C to stop)', flush=True)
    while True:
        (QUEUE / 'heartbeat').write_text(str(time.time()))
        for req in sorted(QUEUE.glob('*.request.json')):
            spec = json.loads(req.read_text())
            label = spec['label']
            print(f'recording {label} for {spec["seconds"]} s', flush=True)
            res = record(ROOT / f'{label}.mp4', float(spec['seconds']), open_timeout)
            (QUEUE / f'{label}.result.json').write_text(json.dumps(res))
            req.unlink()
            print(json.dumps(res), flush=True)
        time.sleep(0.5)


def request(label: str, seconds: float, wait: bool) -> dict:
    hb = QUEUE / 'heartbeat'
    if not hb.exists() or time.time() - float(hb.read_text() or 0) > 10:
        return {'ok': False, 'error': 'camera server not running (start it in a Cursor terminal tab)'}
    (QUEUE / f'{label}.request.json').write_text(json.dumps({'label': label, 'seconds': seconds}))
    if not wait:
        return {'ok': True, 'queued': label}
    out = QUEUE / f'{label}.result.json'
    deadline = time.time() + seconds + 90
    while time.time() < deadline and not out.exists():
        time.sleep(0.5)
    return json.loads(out.read_text()) if out.exists() else {'ok': False, 'error': 'no result before deadline'}


def main() -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest='cmd', required=True)
    sv = sub.add_parser('serve')
    sv.add_argument('--open-timeout', type=float, default=20.0)
    rq = sub.add_parser('request')
    rq.add_argument('--label', required=True)
    rq.add_argument('--seconds', type=float, required=True)
    rq.add_argument('--no-wait', action='store_true')
    p = sub.add_parser('probe')
    p.add_argument('--open-timeout', type=float, default=20.0)
    r = sub.add_parser('record')
    r.add_argument('--seconds', type=float, required=True)
    r.add_argument('--label', required=True)
    r.add_argument('--open-timeout', type=float, default=20.0)
    s = sub.add_parser('sheet')
    s.add_argument('--video', type=Path, required=True)
    s.add_argument('--every', type=float, default=0.5)
    a = ap.parse_args()
    if a.cmd == 'serve':
        return serve(a.open_timeout)
    if a.cmd == 'request':
        res = request(a.label, a.seconds, not a.no_wait)
    elif a.cmd == 'probe':
        res = record(ROOT / 'probe.mp4', 2.0, a.open_timeout)
    elif a.cmd == 'record':
        res = record(ROOT / f'{a.label}.mp4', a.seconds, a.open_timeout)
    else:
        res = sheet(a.video, a.every, a.video.with_suffix('.sheet.jpg'))
    print(json.dumps(res))
    return 0 if res.get('ok') else 1


if __name__ == '__main__':
    sys.exit(main())
