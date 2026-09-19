#!/usr/bin/env python3
"""Record mic RMS, palette status and LED frame energy. No flash."""
from __future__ import annotations
import argparse, hashlib, json, math, struct, subprocess, time, zlib
from datetime import datetime, timezone
from pathlib import Path
from serial.tools import list_ports
import serial
from run_led_smoke import UID, packet, read_exact

def main():
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--output', type=Path, required=True)
    a.add_argument('--phase-s', type=float, default=12)
    args = a.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    device = [p.device for p in list_ports.comports() if (p.vid, p.pid) == (0x045b, 0x5310)]
    if len(device) != 1:
        raise SystemExit('need one Titan CDC')
    port = serial.Serial(device[0], 115200, timeout=.5, write_timeout=2, exclusive=True)
    req = 0
    def transact(op, payload=b''):
        nonlocal req
        req += 1
        out = packet(op, req, payload=payload)
        if port.write(out) != len(out):
            raise RuntimeError('short write')
        port.flush()
        head = read_exact(port, 32, 15)
        magic, status, rid, seq, size, cycles, crc, hcrc = struct.unpack('<4s7I', head)
        body = read_exact(port, size, 15)
        if status:
            raise RuntimeError(f'op {op} status {status}')
        return body
    samples = []
    prev6 = None
    t0 = time.monotonic()
    phases = [('MUSIC1', 0.0), ('SILENCE', args.phase_s), ('MUSIC2', args.phase_s * 2)]
    end = args.phase_s * 3
    try:
        while True:
            now = time.monotonic() - t0
            if now >= end:
                break
            phase = 'MUSIC1'
            for name, start in phases:
                if now >= start:
                    phase = name
            raw6 = transact(6)
            pdm = json.loads(raw6)['pdm_target']
            status = json.loads(transact(17))
            frame = transact(18, struct.pack('<I', 0))
            led_sum = sum(frame)
            led_hash = hashlib.sha256(frame).hexdigest()[:16]
            led_nz = sum(1 for b in frame if b)
            rms = []
            if prev6:
                for i in range(2):
                    a, b = prev6['lanes'][i], pdm['lanes'][i]
                    dn = (b.get('processed_samples') or 0) - (a.get('processed_samples') or 0)
                    dss = (b.get('sample_square_sum') or 0) - (a.get('sample_square_sum') or 0)
                    rms.append(math.sqrt(dss / dn) if dn and dss >= 0 else None)
            else:
                rms = [None, None]
            prev6 = pdm
            row = {
                't': round(now, 3),
                'phase': phase,
                'u14_rms': rms[0],
                'u13_rms': rms[1],
                'ap_hops': pdm.get('ap_hops'),
                'last_hop_dt_us': pdm.get('last_hop_dt_us'),
                'waiting_for_audio': status.get('waiting_for_audio'),
                'mode_a': status.get('mode_a'),
                'palette_frames': status.get('frames'),
                'emitted': status.get('emitted'),
                'frame_a_crc': status.get('frame_a_crc'),
                'led_sum': led_sum,
                'led_hash': led_hash,
                'led_nonzero': led_nz,
            }
            samples.append(row)
            print(json.dumps(row), flush=True)
            time.sleep(0.45)
    finally:
        port.close()
    args.output.write_text(json.dumps({
        'start': datetime.now(timezone.utc).isoformat(),
        'uid': UID,
        'phase_s': args.phase_s,
        'samples': samples,
    }, indent=2) + '\n')

if __name__ == '__main__':
    main()
