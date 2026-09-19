#!/usr/bin/env python3
"""Mode 32 occupancy/travel while DualMCU is held on. Fade unchanged. No flash."""
from __future__ import annotations
import argparse, json, math, struct, subprocess, time, wave, zlib
from datetime import datetime, timezone
from pathlib import Path
import serial
from serial.tools import list_ports
from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

RATE = 48000


def write_tone(path: Path, seconds: float, amp: float, hz: float) -> None:
    n = int(RATE * seconds)
    with wave.open(str(path), 'w') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        frames = bytearray()
        for i in range(n):
            s = amp * math.sin(2 * math.pi * hz * i / RATE)
            frames += struct.pack('<h', int(max(-1.0, min(1.0, s)) * 32767))
        w.writeframes(bytes(frames))


def write_perc(path: Path, seconds: float = 8.0) -> None:
    n = int(RATE * seconds)
    out = [0.0] * n
    kick_p = int(RATE * 0.5)
    for start in range(0, n, kick_p):
        for i in range(int(RATE * 0.04)):
            if start + i >= n:
                break
            env = math.exp(-i / (RATE * 0.01))
            out[start + i] += env * math.sin(2 * math.pi * 90 * i / RATE)
    peak = max(abs(x) for x in out) or 1.0
    with wave.open(str(path), 'w') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        frames = bytearray()
        for x in out:
            frames += struct.pack('<h', int(max(-1.0, min(1.0, 0.95 * x / peak)) * 32767))
        w.writeframes(bytes(frames))


def authored_stats(frame: bytes):
    if len(frame) < 480:
        return {'nz': 0, 'max': 0, 'centroid': None, 'span': 0}
    nz = 0
    mx = 0
    acc = 0.0
    lo = None
    hi = None
    for i in range(80, 160):
        r, g, b = frame[i * 3:i * 3 + 3]
        v = max(r, g, b)
        if v:
            nz += 1
            acc += i
            lo = i if lo is None else min(lo, i)
            hi = i if hi is None else max(hi, i)
            if v > mx:
                mx = v
    return {
        'nz': nz,
        'max': mx,
        'centroid': (acc / nz if nz else None),
        'span': (0 if lo is None else hi - lo),
    }


def main() -> None:
    raise SystemExit(
        'HARD FAIL: never play white noise or computer-generated tones '
        'into the room')
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--output', type=Path, required=True)
    a.add_argument('--build-id', required=True)
    args = a.parse_args()
    out = args.output
    out.mkdir(parents=True, exist_ok=False)
    tone = out / 'tone.wav'
    perc = out / 'perc.wav'
    write_tone(tone, 6.0, 0.95, 1000.0)
    write_perc(perc, 8.0)
    timeline = [
        ('QUIET', 0.0, 2.0, None),
        ('TONE', 2.0, 8.0, tone),
        ('Q2', 8.0, 10.0, None),
        ('PERC', 10.0, 18.0, perc),
        ('SILENCE', 18.0, 24.0, None),
    ]
    matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x045b, 0x5310)]
    if len(matches) != 1:
        raise SystemExit('need one Titan CDC')
    device = matches[0].device
    owners = subprocess.run(
        ['lsof', '-t', device, device.replace('/cu.', '/tty.')],
        capture_output=True, text=True)
    if owners.returncode not in (0, 1) or owners.stdout.strip():
        raise SystemExit('CDC owned')
    port = serial.Serial(device, 115200, timeout=.5, write_timeout=2,
                         exclusive=True)
    req = 0
    samples = []

    def transact(op, payload=b''):
        nonlocal req
        req += 1
        outb = packet(op, req, payload=payload)
        if port.write(outb) != len(outb):
            raise RuntimeError('short write')
        port.flush()
        head = read_exact(port, 32, 15)
        magic, status, rid, seq, size, cycles, crc, hcrc = struct.unpack(
            '<4s7I', head)
        if magic != b'K1R1' or rid != req or zlib.crc32(head[:28]) != hcrc:
            raise RuntimeError('invalid envelope')
        body = read_exact(port, size, 15)
        if zlib.crc32(body) != crc or status:
            raise RuntimeError(f'op {op} status {status}')
        return body

    def phase_at(now):
        name = timeline[0][0]
        for n, start, stop, _ in timeline:
            if start <= now < stop:
                return n
            name = n
        return name

    try:
        quiet = 0
        deadline = time.monotonic() + 5
        while quiet < 2 and time.monotonic() < deadline:
            stale = port.read(1024)
            quiet = 0 if stale else quiet + 1
        ident = json.loads(transact(1))
        if ident.get('uid') != UID or ident.get('build') != args.build_id:
            raise RuntimeError('wrong image')
        transact(16, struct.pack('<8I', 1, 0, 1, 32, 32, 5, 255, 0))
        first = json.loads(transact(6)).get('pdm_target') or {}
        t0 = time.monotonic()
        started = set()
        frames_path = out / 'frames.bin'
        with frames_path.open('wb') as frame_out:
            while True:
                now = time.monotonic() - t0
                if now >= 24.0:
                    break
                ph = phase_at(now)
                for name, start, stop, clip in timeline:
                    if clip and name not in started and now >= start:
                        raise SystemExit(
                            'HARD FAIL: never play white noise or '
                            'computer-generated tones into the room')
                mark = time.monotonic()
                frame = transact(18, struct.pack('<I', 0))
                status = json.loads(transact(17))
                pdm = json.loads(transact(6)).get('pdm_target') or {}
                geom = authored_stats(frame)
                lanes = pdm.get('lanes') or [{}, {}]
                row = {
                    't': round(now, 3),
                    'phase': ph,
                    'led_max': geom['max'],
                    'led_nz': geom['nz'],
                    'centroid': geom['centroid'],
                    'span': geom['span'],
                    'emitted': status.get('emitted'),
                    'emit_errors': status.get('emit_errors'),
                    'visual_path': status.get('visual_path'),
                    'musical': status.get('musical'),
                    'effect_frames': status.get('effect_frames'),
                    'last_hop_peak': pdm.get('last_hop_peak'),
                    'sat_pos': [ln.get('sat_pos', 0) for ln in lanes[:2]],
                    'gain_clip_pos': pdm.get('gain_clip_pos'),
                }
                samples.append(row)
                frame_out.write(struct.pack('<fH', now, len(frame)))
                frame_out.write(frame)
                print(json.dumps(row), flush=True)
                remain = 0.08 - (time.monotonic() - mark)
                if remain > 0:
                    time.sleep(remain)
        end = json.loads(transact(6)).get('pdm_target') or {}
    finally:
        port.close()

    def bucket(name):
        rows = [s for s in samples if s['phase'] == name]
        cents = [s['centroid'] for s in rows if s.get('centroid') is not None]
        return {
            'n': len(rows),
            'musical': sum(1 for s in rows if s.get('musical')),
            'effect': sum(1 for s in rows if s.get('visual_path') == 'effect'),
            'hop_max': max((s.get('last_hop_peak') or 0) for s in rows) if rows else 0,
            'led_max': max((s.get('led_max') or 0) for s in rows) if rows else 0,
            'centroid_min': (min(cents) if cents else None),
            'centroid_max': (max(cents) if cents else None),
            'centroid_span': ((max(cents) - min(cents)) if cents else 0),
        }

    start_sat = [ln.get('sat_pos', 0) for ln in (first.get('lanes') or [{}, {}])[:2]]
    end_sat = [ln.get('sat_pos', 0) for ln in (end.get('lanes') or [{}, {}])[:2]]
    tone = bucket('TONE')
    perc = bucket('PERC')
    travel = max(tone.get('centroid_span') or 0, perc.get('centroid_span') or 0)
    receipt = {
        'label': 'ON-SILICON',
        'build_id': args.build_id,
        'identity': ident,
        'tone': tone,
        'perc': perc,
        'quiet': bucket('QUIET'),
        'silence': bucket('SILENCE'),
        'travel_px': travel,
        'travels': travel >= 8.0,
        'sat_delta': [end_sat[i] - start_sat[i] for i in (0, 1)],
        'samples': samples,
        'start': datetime.now(timezone.utc).isoformat(),
    }
    (out / 'motion.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('MOTION', json.dumps({
        'travels': receipt['travels'],
        'travel_px': travel,
        'tone': {k: tone[k] for k in ('musical', 'effect', 'hop_max', 'led_max',
                                      'centroid_span')},
        'perc': {k: perc[k] for k in ('musical', 'effect', 'hop_max', 'led_max',
                                      'centroid_span')},
        'sat_delta': receipt['sat_delta'],
    }))


if __name__ == '__main__':
    main()
