#!/usr/bin/env python3
"""Live two-lane energy while one capsule is covered. No flash."""
from __future__ import annotations
import argparse, json, struct, time
from datetime import datetime, timezone
from pathlib import Path
from serial.tools import list_ports
import serial
from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

BUILD = 'c65c6fca29eda8d5674db6301a8eef8f8a8dae9fe16a9ee8f3e666b00f5918f5'


def main() -> None:
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--output', type=Path, required=True)
    a.add_argument('--build-id', default=BUILD)
    a.add_argument('--seconds', type=float, default=18.0)
    a.add_argument('--period-s', type=float, default=0.15)
    a.add_argument('--label', default='cover_u13')
    args = a.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
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

    samples = []
    prev = None
    t0 = time.monotonic()
    try:
        ident = json.loads(transact(1))
        if ident.get('uid') != UID or ident.get('build') != args.build_id:
            raise RuntimeError('wrong board/image')
        while time.monotonic() - t0 < args.seconds:
            mark = time.monotonic()
            pdm = json.loads(transact(6)).get('pdm_target') or {}
            lanes = pdm.get('lanes') or [{}, {}]
            now = {
                't': round(time.monotonic() - t0, 3),
                'last_hop_peak': pdm.get('last_hop_peak'),
                'u14': {
                    'samples': lanes[0].get('processed_samples'),
                    'energy': lanes[0].get('sample_square_sum'),
                    'hash': lanes[0].get('sample_hash'),
                },
                'u13': {
                    'samples': lanes[1].get('processed_samples'),
                    'energy': lanes[1].get('sample_square_sum'),
                    'hash': lanes[1].get('sample_hash'),
                },
            }
            if prev:
                for name in ('u14', 'u13'):
                    ds = (now[name]['samples'] or 0) - (prev[name]['samples'] or 0)
                    de = (now[name]['energy'] or 0) - (prev[name]['energy'] or 0)
                    now[name]['d_samples'] = ds
                    now[name]['rms'] = int((de / ds) ** 0.5) if ds > 0 and de > 0 else 0
            samples.append(now)
            print(json.dumps({
                't': now['t'],
                'hop': now['last_hop_peak'],
                'u14_rms': now['u14'].get('rms'),
                'u13_rms': now['u13'].get('rms'),
            }), flush=True)
            prev = now
            remain = args.period_s - (time.monotonic() - mark)
            if remain > 0:
                time.sleep(remain)
    finally:
        port.close()

    def med(name, start, stop):
        vals = [s[name].get('rms') or 0 for s in samples
                if start <= s['t'] < stop and s[name].get('rms') is not None]
        if not vals:
            return 0
        vals.sort()
        return vals[len(vals)//2]

    # skip first 1.5 s (finger not yet on), compare 2–18 s vs hop
    hops = [s['last_hop_peak'] or 0 for s in samples if s['t'] >= 2]
    receipt = {
        'label': 'ON-SILICON',
        'cover': args.label,
        'build_id': args.build_id,
        'uid': UID,
        'identity': ident,
        'n': len(samples),
        'u14_rms_med_after_2s': med('u14', 2, 99),
        'u13_rms_med_after_2s': med('u13', 2, 99),
        'hop_med_after_2s': sorted(hops)[len(hops)//2] if hops else 0,
        'samples': samples,
        'start': datetime.now(timezone.utc).isoformat(),
    }
    args.output.write_text(json.dumps(receipt, indent=2) + '\n')
    print('MIC_COVER', json.dumps({
        k: receipt[k] for k in (
            'cover', 'u14_rms_med_after_2s', 'u13_rms_med_after_2s',
            'hop_med_after_2s', 'n')
    }))


if __name__ == '__main__':
    main()
