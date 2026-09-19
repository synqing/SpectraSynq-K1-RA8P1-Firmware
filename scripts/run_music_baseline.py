#!/usr/bin/env python3
"""Finite labelled music-like baseline on resident mode 32. No flash. No loop."""
from __future__ import annotations
import argparse, hashlib, json, math, struct, subprocess, tempfile, time, wave
from datetime import datetime, timezone
from pathlib import Path
from serial.tools import list_ports
import serial
from emit_continuity import emit_continuity_ok
from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

BUILD = '55320f891d37211f8401d61d62d85427bcb4a7c9b277638d374af1ba1d738986'
RATE = 48000


def write_wav(path: Path, samples) -> None:
    with wave.open(str(path), 'w') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        frames = bytearray()
        for x in samples:
            frames += struct.pack('<h', max(-32767, min(32767, int(32767 * x))))
        w.writeframes(bytes(frames))


def sparse_clicks(seconds=8.0, bpm=120):
    n = int(RATE * seconds)
    out = [0.0] * n
    period = int(RATE * 60 / bpm)
    width = int(RATE * 0.05)
    for start in range(0, n, period):
        for i in range(width):
            if start + i >= n:
                break
            env = 1.0 - i / width
            out[start + i] = env * math.sin(2 * math.pi * 1800 * i / RATE)
    return out


def pad_chord(seconds=8.0):
    n = int(RATE * seconds)
    freqs = (261.63, 329.63, 392.00)
    out = []
    for i in range(n):
        t = i / RATE
        env = min(1.0, t / 0.04)
        if i > n - RATE * 0.2:
            env *= max(0.0, (n - i) / (RATE * 0.2))
        s = sum(math.sin(2 * math.pi * f * t) for f in freqs) / 3.0
        out.append(0.45 * env * s)
    return out


def percussion(seconds=8.0, bpm=120):
    n = int(RATE * seconds)
    out = [0.0] * n
    kick_p = int(RATE * 60 / bpm)
    hat_p = kick_p // 2
    for start in range(0, n, kick_p):
        for i in range(int(RATE * 0.03)):
            if start + i >= n:
                break
            env = math.exp(-i / (RATE * 0.008))
            out[start + i] += env * math.sin(2 * math.pi * 80 * i / RATE)
    seed = 1
    for start in range(hat_p, n, hat_p):
        for i in range(int(RATE * 0.008)):
            if start + i >= n:
                break
            seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF
            noise = (seed / 0x40000000) - 1.0
            env = math.exp(-i / (RATE * 0.002))
            out[start + i] += 0.25 * env * noise
    peak = max(abs(x) for x in out) or 1.0
    return [0.9 * x / peak for x in out]


def main() -> None:
    raise SystemExit(
        'HARD FAIL: never play white noise or computer-generated tones '
        'into the room')
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--output', type=Path, required=True)
    a.add_argument('--build-id', default=BUILD)
    a.add_argument('--period-s', type=float, default=0.12)
    args = a.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    tmp = Path(tempfile.mkdtemp(prefix='k1-music-'))
    clips = {
        'SPARSE': tmp / 'sparse.wav',
        'PAD': tmp / 'pad.wav',
        'PERC': tmp / 'perc.wav',
    }
    write_wav(clips['SPARSE'], sparse_clicks())
    write_wav(clips['PAD'], pad_chord())
    write_wav(clips['PERC'], percussion())
    timeline = [
        ('QUIET', 0.0, 3.0, None),
        ('SPARSE', 3.0, 11.0, clips['SPARSE']),
        ('Q2', 11.0, 14.0, None),
        ('PAD', 14.0, 22.0, clips['PAD']),
        ('Q3', 22.0, 25.0, None),
        ('PERC', 25.0, 33.0, clips['PERC']),
        ('SILENCE', 33.0, 50.0, None),
    ]
    end_s = 50.0
    device = [p.device for p in list_ports.comports()
              if (p.vid, p.pid) == (0x045b, 0x5310)]
    if len(device) != 1:
        raise SystemExit('need one Titan CDC')
    port = serial.Serial(device[0], 115200, timeout=.5, write_timeout=2,
                         exclusive=True)
    req = 0

    def transact(op, payload=b''):
        nonlocal req
        req += 1
        out = packet(op, req, payload=payload)
        if port.write(out) != len(out):
            raise RuntimeError('short write')
        port.flush()
        head = read_exact(port, 32, 15)
        magic, status, rid, seq, size, cycles, crc, hcrc = struct.unpack(
            '<4s7I', head)
        body = read_exact(port, size, 15)
        if status:
            raise RuntimeError(f'op {op} status {status}')
        return body

    def phase_at(now):
        name = timeline[0][0]
        for n, start, stop, _ in timeline:
            if start <= now < stop:
                return n
            name = n
        return name

    samples = []
    started = set()
    t0 = time.monotonic()
    try:
        ident = json.loads(transact(1))
        if ident.get('uid') != UID or ident.get('source') != PIN:
            raise RuntimeError('wrong board')
        if ident.get('build') != args.build_id:
            raise RuntimeError('wrong image: ' + str(ident.get('build')))
        cfg = transact(16, struct.pack('<8I', 1, 0, 1, 32, 32, 5, 255, 0))
        status0 = json.loads(transact(17))
        first6 = json.loads(transact(6))
        while True:
            now = time.monotonic() - t0
            if now >= end_s:
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
            row = {
                't': round(now, 3),
                'phase': ph,
                'led_sum': sum(frame),
                'led_max': max(frame) if frame else 0,
                'led_nz': sum(1 for b in frame if b),
                'led_ones': sum(1 for b in frame if b == 1),
                'led_hash': hashlib.sha256(frame).hexdigest()[:16],
                'frame_a_crc': status.get('frame_a_crc'),
                'mode_a': status.get('mode_a'),
                'emitted': status.get('emitted'),
                'emit_errors': status.get('emit_errors'),
                'waiting_for_audio': status.get('waiting_for_audio'),
                'last_hop_peak': pdm.get('last_hop_peak'),
                'last_hop_gain_q8': pdm.get('last_hop_gain_q8'),
                'ap_hops': pdm.get('ap_hops'),
                'measured_hz': pdm.get('measured_hz'),
                'rate_locked': pdm.get('rate_locked'),
                'asrc_starved': pdm.get('asrc_starved'),
            }
            samples.append(row)
            print(json.dumps(row), flush=True)
            remain = args.period_s - (time.monotonic() - mark)
            if remain > 0:
                time.sleep(remain)
        end_status = json.loads(transact(17))
        end_pdm = json.loads(transact(6))
    finally:
        port.close()

    def bucket(name):
        rows = [s for s in samples if s['phase'] == name]
        if not rows:
            return None
        hops = [s['last_hop_peak'] or 0 for s in rows]
        leds = [s['led_max'] for s in rows]
        nz = [s['led_nz'] for s in rows]
        return {
            'n': len(rows),
            'hop_med': sorted(hops)[len(hops)//2],
            'hop_max': max(hops),
            'led_max': max(leds),
            'nz_min': min(nz),
            'nz_max': max(nz),
            'black': sum(1 for s in rows if s['led_nz'] == 0),
            'crc': len({s['frame_a_crc'] for s in rows}),
        }

    phases = {name: bucket(name) for name, *_ in timeline}
    quiet = phases['QUIET'] or {}
    sparse = phases['SPARSE'] or {}
    pad = phases['PAD'] or {}
    perc = phases['PERC'] or {}
    silence = phases['SILENCE'] or {}
    emit0 = samples[0]['emitted'] if samples else 0
    emit1 = samples[-1]['emitted'] if samples else 0
    hop_rate = ((samples[-1]['ap_hops'] - samples[0]['ap_hops']) /
                (samples[-1]['t'] - samples[0]['t'])) if samples else 0
    attacks = (
        (sparse.get('hop_max') or 0) >= 3 * max(1, quiet.get('hop_med') or 0)
        or ((quiet.get('led_max') or 0) == 0 and (sparse.get('led_max') or 0) >= 16)
    )
    pad_hold = (pad.get('nz_min') or 0) > 0
    late = [s for s in samples if s['phase'] == 'SILENCE' and s['t'] >= 39.0]
    silence_dark = bool(late) and max(s['led_max'] for s in late) <= 1 and (
        sum(1 for s in late if s['led_nz'] == 0) >= len(late) // 2
        or max(s['led_max'] for s in late) == 0)
    emit_ok, emit_stall_s, emit_stall_at, emit_stall_val = emit_continuity_ok(
        samples, max_stall_s=2.0)
    receipt = {
        'label': 'ON-SILICON',
        'result': 'PASS' if attacks and pad_hold and silence_dark and emit_ok
        else 'NEEDS_WORK',
        'build_id': args.build_id,
        'uid': UID,
        'identity': ident,
        'configure': json.loads(cfg) if cfg[:1] == b'{' else {'raw': cfg.hex()},
        'boot_status': status0,
        'first_pdm': first6.get('pdm_target'),
        'stimulus': 'labelled_8s_sparse120_pad_perc_once_each',
        'n': len(samples),
        'emitted_delta': emit1 - emit0,
        'hop_rate': hop_rate,
        'phases': phases,
        'attacks_on_sparse': attacks,
        'pad_presence': pad_hold,
        'silence_recovery': silence_dark,
        'transmit_continued': emit_ok,
        'emit_stall_s': emit_stall_s,
        'emit_stall_at_s': emit_stall_at,
        'emit_stall_value': emit_stall_val,
        'end_status': end_status,
        'end_pdm': end_pdm.get('pdm_target') if isinstance(end_pdm, dict) else None,
        'samples': samples,
        'start': datetime.now(timezone.utc).isoformat(),
    }
    args.output.write_text(json.dumps(receipt, indent=2, default=str) + '\n')
    print('MUSIC_BASELINE', json.dumps({
        k: receipt[k] for k in (
            'result', 'attacks_on_sparse', 'pad_presence', 'silence_recovery',
            'transmit_continued', 'emitted_delta', 'hop_rate', 'n')
    }))
    print('PHASES', json.dumps(phases))


if __name__ == '__main__':
    main()
