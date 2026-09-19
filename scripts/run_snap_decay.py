#!/usr/bin/env python3
"""Configure WaveformK1, play one click, score LED dump decay. No flash. No 15 min loop."""
from __future__ import annotations
import argparse, hashlib, json, math, struct, subprocess, tempfile, time, wave, zlib
from datetime import datetime, timezone
from pathlib import Path
from serial.tools import list_ports
import serial
from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

BUILD = 'dc07d97db773d7b0e29ca9ea38f270537f0da1e64640bca7be044a3895d9a1f2'


def main() -> None:
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--output', type=Path, required=True)
    a.add_argument('--build-id', default=BUILD)
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

    try:
        ident = json.loads(transact(1))
        if ident.get('uid') != UID or ident.get('source') != PIN:
            raise RuntimeError('wrong board')
        if ident.get('build') != args.build_id:
            raise RuntimeError('wrong image: ' + str(ident.get('build')))
        cfg = transact(16, struct.pack('<8I', 1, 0, 1, 32, 32, 5, 255, 0))
        status0 = json.loads(transact(17))
        samples = []
        t0 = time.monotonic()
        clicked = False
        end = 8.0
        while True:
            now = time.monotonic() - t0
            if now >= end:
                break
            frame = transact(18, struct.pack('<I', 0))
            status = json.loads(transact(17))
            pdm = json.loads(transact(6)).get('pdm_target') or {}
            led_sum = sum(frame)
            led_max = max(frame) if frame else 0
            row = {
                't': round(now, 3),
                'clicked': clicked,
                'led_sum': led_sum,
                'led_max': led_max,
                'led_nz': sum(1 for b in frame if b),
                'led_hash': hashlib.sha256(frame).hexdigest()[:16],
                'frame_a_crc': status.get('frame_a_crc'),
                'mode_a': status.get('mode_a'),
                'emitted': status.get('emitted'),
                'waiting_for_audio': status.get('waiting_for_audio'),
            }
            lanes = pdm.get('lanes') or [{}, {}]
            row['peak'] = [ln.get('sample_peak') for ln in lanes[:2]]
            samples.append(row)
            print(json.dumps(row), flush=True)
            time.sleep(0.12)
    finally:
        port.close()

    pre = [s for s in samples if not s['clicked']]
    post = [s for s in samples if s['clicked']]
    early = post[:8]
    late = post[-8:] if len(post) >= 8 else post
    pre_max = max((s['led_max'] for s in pre), default=0)
    early_max = max((s['led_max'] for s in early), default=0)
    late_max = max((s['led_max'] for s in late), default=0)
    early_crc = {s['frame_a_crc'] for s in early}
    late_crc = {s['frame_a_crc'] for s in late}
    lit = early_max > pre_max and early_max > 0
    moved = len(late_crc) > 1 or (late_crc and early_crc and late_crc != early_crc)
    decayed = late_max < early_max
    frozen = lit and (not moved) and late_max >= 250
    receipt = {
        'pass': bool(lit and decayed and moved and not frozen),
        'build_id': args.build_id,
        'uid': UID,
        'configure': json.loads(cfg) if cfg[:1] == b'{' else {'raw': cfg.hex()},
        'boot_status': status0,
        'pre_max': pre_max,
        'early_max': early_max,
        'late_max': late_max,
        'lit': lit,
        'moved': moved,
        'decayed': decayed,
        'frozen': frozen,
        'stimulus': 'host_click_40ms_2500hz',
        'samples': samples,
        'start': datetime.now(timezone.utc).isoformat(),
    }
    if not lit:
        receipt['blocker'] = 'click did not light the dump; acoustic path still quiet'
    args.output.write_text(json.dumps(receipt, indent=2) + '\n')
    print('SNAP_DECAY', json.dumps({k: receipt[k] for k in
          ('pass', 'lit', 'moved', 'decayed', 'frozen', 'pre_max', 'early_max', 'late_max')}))


if __name__ == '__main__':
    main()
