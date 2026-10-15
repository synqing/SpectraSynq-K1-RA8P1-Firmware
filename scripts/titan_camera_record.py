#!/usr/bin/env python3
"""Record the Titan through the Captain's iPhone (Continuity Camera) so the agent can see the light.

Lane P6 evaluation aid (Captain, 15 Oct 2026: "reinstate the iPhone camera to record all these
evaluation/test runs so YOU can see clearly what I see"). It is a recording, not an instrument:
nothing here is a latency or colour measurement.

  titan_camera_record.py probe                       # 2 s test clip; fails fast if the camera stalls
  titan_camera_record.py record --seconds 80 --label boom-host-006
  titan_camera_record.py sheet --video V --every 0.5 # contact sheet of frames for viewing
  titan_camera_record.py request --label L --seconds S --audio 'MacBook Pro Microphone'

Heavy video is written to the external drive (storage policy, >50 MB). A stalled camera open
(locked phone not mounted, macOS permission prompt) is killed after --open-timeout seconds.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import signal
import subprocess
import sys
import time
from pathlib import Path

DEVICE = 'EiP Camera'
ROOT = Path('/Volumes/Disk Storage/SpectraSynq-Scratch/titan-lagfree-20261014/camera')
# Grace beyond a recording's own -t before it counts as hung (open stall + finalise).
OVERDUE_MARGIN_S = 60.0


def _etime_seconds(etime: str) -> float:
    """ps etime [[dd-]hh:]mm:ss -> seconds."""
    days, _, rest = etime.strip().rpartition('-')
    parts = [float(p) for p in rest.split(':')]
    while len(parts) < 3:
        parts.insert(0, 0.0)
    return (float(days) if days else 0.0) * 86400 + parts[0] * 3600 + parts[1] * 60 + parts[2]


def camera_holders() -> list[dict]:
    """Every ffmpeg reading DEVICE, with its age and its own -t duration.

    -ww: without it ps truncates long command lines and "-i EiP Camera" is cut off
    (caught by the planted hung-recording test, 15 Oct)."""
    out = subprocess.run(['ps', '-axww', '-o', 'pid=,etime=,command='], capture_output=True, text=True).stdout
    holders = []
    for line in out.splitlines():
        m = re.match(r'\s*(\d+)\s+(\S+)\s+(.*)$', line)
        if not m or 'ffmpeg' not in m.group(3) or f'-i {DEVICE}' not in m.group(3):
            continue
        t = re.search(r'\s-t\s+([0-9.]+)', m.group(3))
        holders.append({'pid': int(m.group(1)), 'age_s': _etime_seconds(m.group(2)),
                        'duration_s': float(t.group(1)) if t else None, 'command': m.group(3)[:200]})
    return holders


def reap_overdue(margin: float = OVERDUE_MARGIN_S) -> list[dict]:
    """Kill camera ffmpeg processes past their own -t plus margin (or with no -t at all).

    13-15 Oct 2026: a server tab closed mid-recording orphaned its ffmpeg; the camera
    then stalled, so the 90 s recording never ended and held the iPhone for 13 hours,
    making every later open hang. The in-process deadline died with its parent.
    """
    reaped = []
    for h in camera_holders():
        limit = (h['duration_s'] or 0.0) + margin
        if h['duration_s'] is None or h['age_s'] > limit:
            try:
                os.kill(h['pid'], signal.SIGKILL)
                h['reaped'] = True
            except ProcessLookupError:
                h['reaped'] = 'already gone'
            reaped.append(h)
    return reaped


def record(out: Path, seconds: float, open_timeout: float, audio: str | None = None) -> dict:
    out.parent.mkdir(parents=True, exist_ok=True)
    # Audio in the same file lets light be timed against the room sound (atlas
    # Experiment 3 step 0). Bluetooth playback lags by a variable 100-250 ms, so
    # the played file's clock is not the room's clock.
    cmd = ['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-f', 'avfoundation',
           '-thread_queue_size', '1024', '-framerate', '30', '-video_size', '1920x1080',
           '-i', f'{DEVICE}:{audio or "none"}',
           '-t', f'{seconds:.1f}', '-c:v', 'h264_videotoolbox', '-b:v', '12M',
           *(['-c:a', 'aac', '-b:a', '160k'] if audio else []),
           # Fragmented MP4: a recording cut short (server tab closed) is still readable.
           '-movflags', '+frag_keyframe+empty_moov', str(out)]
    global _CHILD
    reaped = reap_overdue()
    busy = camera_holders()
    if busy:
        return {'ok': False, 'error': 'camera held by another live recording', 'holders': busy, 'reaped': reaped}
    # Hard wall-clock deadline that survives this process dying: perl's alarm is
    # inherited across exec, so SIGALRM ends ffmpeg even if the server is killed.
    hard = int(seconds + open_timeout + 30)
    cmd = ['/usr/bin/perl', '-e', 'alarm shift; exec @ARGV or die', str(hard), *cmd]
    start = time.monotonic()
    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    _CHILD = proc
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
    _CHILD = None
    err = proc.stderr.read().decode(errors='replace')[-800:] if proc.stderr else ''
    ok = proc.returncode == 0 and out.exists() and out.stat().st_size > 0
    streams = []
    if out.exists():
        probe = subprocess.run(['ffprobe', '-v', 'error', '-show_entries', 'stream=codec_type',
                                '-of', 'csv=p=0', str(out)], capture_output=True, text=True)
        streams = probe.stdout.split()
    if audio and 'audio' not in streams:
        ok = False
    return {'ok': ok, 'returncode': proc.returncode, 'bytes': out.stat().st_size if out.exists() else 0,
            'streams': streams, 'audio_device': audio,
            'stderr_tail': err, 'path': str(out), 'started_monotonic': start}


def sheet(video: Path, every: float, out: Path) -> dict:
    out.parent.mkdir(parents=True, exist_ok=True)
    cmd = ['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-i', str(video),
           '-vf', f'fps=1/{every},scale=480:-1,tile=6x6', '-frames:v', '1', str(out)]
    rc = subprocess.run(cmd).returncode
    return {'ok': rc == 0 and out.exists(), 'path': str(out)}


QUEUE = ROOT / 'queue'
_CHILD: subprocess.Popen | None = None


def _stop_child(signum, _frame):
    """Closing the tab (SIGHUP), Ctrl-C or SIGTERM never leaves a recording orphaned."""
    if _CHILD is not None and _CHILD.poll() is None:
        _CHILD.kill()
    raise SystemExit(128 + signum)


def preflight() -> dict:
    """Agent check BEFORE asking the Captain anything about the camera."""
    reaped = reap_overdue()
    hb = QUEUE / 'heartbeat'
    age = time.time() - float(hb.read_text() or 0) if hb.exists() else None
    return {'ok': age is not None and age <= 10 and not camera_holders(),
            'server_heartbeat_age_s': None if age is None else round(age, 1),
            'server_running': age is not None and age <= 10,
            'reaped_overdue_ffmpeg': reaped, 'live_holders': camera_holders()}


def serve(open_timeout: float) -> int:
    """Run from a plain Cursor terminal tab (Cursor holds the macOS camera grant).

    Agent sessions are not attributed to Cursor by macOS (camera authorisation reads
    notDetermined there and the open hangs), so they queue requests here instead.
    """
    QUEUE.mkdir(parents=True, exist_ok=True)
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, _stop_child)
    for h in reap_overdue():
        print(f'camera server: killed hung recording pid {h["pid"]} '
              f'(age {h["age_s"]:.0f} s, -t {h["duration_s"]})', flush=True)
    print(f'camera server: RUNNING - watching {QUEUE}\n'
          'Leave this tab open. It is idle until the agent requests a recording; '
          'it prints "alive" every 15 s. Ctrl-C stops it.', flush=True)
    last_alive = time.monotonic()
    while True:
        (QUEUE / 'heartbeat').write_text(str(time.time()))
        if time.monotonic() - last_alive >= 15:
            print(f'camera server: alive {time.strftime("%H:%M:%S")} - waiting for requests', flush=True)
            last_alive = time.monotonic()
        for req in sorted(QUEUE.glob('*.request.json')):
            spec = json.loads(req.read_text())
            label = spec['label']
            print(f'recording {label} for {spec["seconds"]} s', flush=True)
            res = record(ROOT / f'{label}.mp4', float(spec['seconds']), open_timeout, spec.get('audio'))
            (QUEUE / f'{label}.result.json').write_text(json.dumps(res))
            req.unlink()
            print(json.dumps(res), flush=True)
        time.sleep(0.5)


def request(label: str, seconds: float, wait: bool, audio: str | None = None) -> dict:
    hb = QUEUE / 'heartbeat'
    if not hb.exists() or time.time() - float(hb.read_text() or 0) > 10:
        return {'ok': False, 'error': 'camera server not running (start it in a Cursor terminal tab)'}
    (QUEUE / f'{label}.request.json').write_text(json.dumps({'label': label, 'seconds': seconds, 'audio': audio}))
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
    rq.add_argument('--audio', default=None, help="audio device to record with the video, e.g. 'MacBook Pro Microphone'")
    p = sub.add_parser('probe')
    p.add_argument('--open-timeout', type=float, default=20.0)
    sub.add_parser('preflight', help='agent: reap hung recordings, report server state (run before asking the Captain)')
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
    if a.cmd == 'preflight':
        res = preflight()
    elif a.cmd == 'request':
        reap_overdue()
        res = request(a.label, a.seconds, not a.no_wait, a.audio)
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
