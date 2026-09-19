#!/usr/bin/env python3
"""Stepped speaker tones vs raw PDM rail counts. Does not change gain."""
from __future__ import annotations
import argparse, json, math, struct, subprocess, time, wave, zlib
from datetime import datetime, timezone
from pathlib import Path
import serial
from serial.tools import list_ports
from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

RATE = 48000
LEVELS = (('quiet', 0.0), ('low', 0.08), ('mid', 0.25), ('high', 0.55), ('full', 0.95))


def tone_wav(path: Path, amp: float, seconds: float) -> None:
    n = int(RATE * seconds)
    with wave.open(str(path), 'w') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        frames = bytearray()
        for i in range(n):
            s = amp * math.sin(2 * math.pi * 1000.0 * i / RATE)
            frames += struct.pack('<h', int(max(-1.0, min(1.0, s)) * 32767))
        w.writeframes(bytes(frames))


def lane_sat(pdm):
    lanes = pdm.get('lanes') or [{}, {}]
    return {
        'sat_pos': [ln.get('sat_pos', 0) for ln in lanes[:2]],
        'sat_neg': [ln.get('sat_neg', 0) for ln in lanes[:2]],
        'sample_peak': [ln.get('sample_peak', 0) for ln in lanes[:2]],
    }


def main() -> None:
    raise SystemExit(
        'HARD FAIL: never play white noise or computer-generated tones '
        'into the room')
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--output', type=Path, required=True)
    a.add_argument('--build-id', required=True)
    a.add_argument('--hold-s', type=float, default=2.0)
    args = a.parse_args()
    out = args.output
    out.mkdir(parents=True, exist_ok=False)
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

    try:
        quiet = 0
        deadline = time.monotonic() + 5
        while quiet < 2 and time.monotonic() < deadline:
            stale = port.read(1024)
            quiet = 0 if stale else quiet + 1
        ident = json.loads(transact(1))
        if ident.get('uid') != UID or ident.get('build') != args.build_id:
            raise RuntimeError('wrong image')
        start = json.loads(transact(6)).get('pdm_target') or {}
        t0 = time.monotonic()
        for name, amp in LEVELS:
            wav = out / f'{name}.wav'
            tone_wav(wav, amp, args.hold_s)
            player = None
            if amp > 0.0:
                raise SystemExit(
                    'HARD FAIL: never play white noise or '
                    'computer-generated tones into the room')
            end_t = time.monotonic() + args.hold_s
            while time.monotonic() < end_t:
                pdm = json.loads(transact(6)).get('pdm_target') or {}
                sat = lane_sat(pdm)
                row = {
                    't': round(time.monotonic() - t0, 3),
                    'level': name,
                    'amp': amp,
                    'last_hop_peak': pdm.get('last_hop_peak'),
                    'last_hop_gain_q8': pdm.get('last_hop_gain_q8'),
                    'gain_clip_pos': pdm.get('gain_clip_pos'),
                    'gain_clip_neg': pdm.get('gain_clip_neg'),
                    **sat,
                }
                samples.append(row)
                print(json.dumps(row), flush=True)
                time.sleep(0.15)
            if player is not None:
                player.wait(timeout=5)
            time.sleep(0.4)
        end = json.loads(transact(6)).get('pdm_target') or {}
    finally:
        port.close()

    start_sat = lane_sat(start)
    end_sat = lane_sat(end)
    by_level = {}
    for name, _ in LEVELS:
        rows = [s for s in samples if s['level'] == name]
        if not rows:
            continue
        by_level[name] = {
            'n': len(rows),
            'hop_max': max(s.get('last_hop_peak') or 0 for s in rows),
            'gain_q8_min': min(s.get('last_hop_gain_q8') or 0 for s in rows),
            'raw_peak_max': [max((s['sample_peak'][i] or 0) for s in rows)
                             for i in (0, 1)],
        }
    receipt = {
        'label': 'ON-SILICON',
        'build_id': args.build_id,
        'identity': ident,
        'start_sat': start_sat,
        'end_sat': end_sat,
        'sat_delta': {
            'sat_pos': [end_sat['sat_pos'][i] - start_sat['sat_pos'][i]
                        for i in (0, 1)],
            'sat_neg': [end_sat['sat_neg'][i] - start_sat['sat_neg'][i]
                        for i in (0, 1)],
        },
        'gain_clip_delta': {
            'pos': (end.get('gain_clip_pos') or 0) - (start.get('gain_clip_pos') or 0),
            'neg': (end.get('gain_clip_neg') or 0) - (start.get('gain_clip_neg') or 0),
        },
        'by_level': by_level,
        'samples': samples,
        'start': datetime.now(timezone.utc).isoformat(),
    }
    (out / 'headroom.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('HEADROOM', json.dumps({
        'sat_delta': receipt['sat_delta'],
        'gain_clip_delta': receipt['gain_clip_delta'],
        'by_level': {k: {'hop_max': v['hop_max'], 'raw_peak_max': v['raw_peak_max']}
                     for k, v in by_level.items()},
    }))


if __name__ == '__main__':
    main()
