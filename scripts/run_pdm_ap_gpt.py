#!/usr/bin/env python3
"""Score 40 kHz PDM → 24 kHz/180 AP hops plus GPT frames. No 16 kHz repeat."""
from __future__ import annotations
import argparse, hashlib, json, struct, subprocess, time, zlib
from datetime import datetime, timezone
from pathlib import Path
from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

def score(first, second):
    pdm0 = first.get('pdm_target') or {}
    pdm1 = second.get('pdm_target') or {}
    if pdm1.get('sample_rate_hz') != 40000:
        raise ValueError('capture is not 40 kHz')
    if pdm1.get('slot_elements') != 296:
        raise ValueError('slot is not 296 samples')
    if pdm1.get('profile') != 'ap_40k_asrc24':
        raise ValueError('admission profile missing')
    if (pdm1.get('ap_hops') or 0) <= (pdm0.get('ap_hops') or 0):
        raise ValueError('AP hops did not advance')
    if (pdm1.get('last_capture_end_us') or 0) <= (pdm0.get('last_capture_end_us') or 0):
        raise ValueError('capture end time did not advance')
    if (pdm1.get('rearm_denied') or 0) > (pdm0.get('rearm_denied') or 0):
        raise ValueError('DMA rearm denied during observation')
    lanes0 = pdm0.get('lanes') or [{}, {}]
    lanes = pdm1.get('lanes') or []
    if len(lanes) != 2:
        raise ValueError('expected two PDM lanes')
    for i, lane in enumerate(lanes):
        prev = lanes0[i] if i < len(lanes0) else {}
        if (lane.get('overflow_events') or 0) > (prev.get('overflow_events') or 0):
            raise ValueError(f'lane {i} overflow during observation')
        if (lane.get('drop_events') or 0) > (prev.get('drop_events') or 0):
            raise ValueError(f'lane {i} drop during observation')
        if (lane.get('recovery_count') or 0) > (prev.get('recovery_count') or 0):
            raise ValueError(f'lane {i} capture restart during observation')
    hops = (pdm1.get('ap_hops') or 0) - (pdm0.get('ap_hops') or 0)
    capture_us = (pdm1.get('last_capture_end_us') or 0) - (pdm0.get('last_capture_end_us') or 0)
    capture_s = capture_us / 1e6 if capture_us else 0.0
    hop_rate = hops / capture_s if capture_s else 0.0
    return {
        'ap_hops': (pdm1.get('ap_hops') or 0) - (pdm0.get('ap_hops') or 0),
        'paired_slots': (pdm1.get('paired_slots') or 0) - (pdm0.get('paired_slots') or 0),
        'sat_neg': [lane.get('sat_neg', 0) for lane in lanes],
        'sat_pos': [lane.get('sat_pos', 0) for lane in lanes],
        'packing_mismatch': [lane.get('packing_mismatch', 0) for lane in lanes],
        'first_sat_raw': [lane.get('first_sat_raw', 0) for lane in lanes],
        'asrc_starved': pdm1.get('asrc_starved', 0),
        'acoustic_identity': pdm1.get('acoustic_identity', 'unproven'),
        'capture_s': capture_s,
        'hop_rate_from_capture': hop_rate,
        'expected_ap_hops_per_s': 24000 / 180,
        'expected_slot_per_s': 40000 / 296,
        'last_hop_dt_us': pdm1.get('last_hop_dt_us'),
        'max_pdm_gap_us': pdm1.get('max_pdm_gap_us'),
        'max_gpt_gap_us': pdm1.get('max_gpt_gap_us'),
        'max_usb_gap_us': pdm1.get('max_usb_gap_us'),
    }

def main():
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--build', type=Path, required=True)
    a.add_argument('--output', type=Path, required=True)
    a.add_argument('--pause-s', type=float, default=45)
    args = a.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = {
        'pass': False,
        'start': datetime.now(timezone.utc).isoformat(),
        'operation': 'TITAN_PDM_AP_GPT',
        'photons': 'NOT_CLAIMED',
        'waveform': 'WAVEFORM_NOT_CAPTURED',
        'g4': 'NOT_RUN_THIS_IMAGE',
        'optical': 'OPERATOR_MAY_OBSERVE_LIGHT_AND_RESPONSE',
        'pause_s': args.pause_s,
    }
    port = None
    try:
        b = json.loads((args.build / 'receipt.json').read_text())
        if not b.get('pass') or not b.get('pdm_target') or not b.get('palette_gpt_dma'):
            raise ValueError('requires combined PDM+GPT image')
        sha = hashlib.sha256((args.build / 'rtthread.hex').read_bytes()).hexdigest()
        if sha != b['artifacts']['rtthread.hex']:
            raise ValueError('image hash changed')
        receipt.update(build_id=b['build_id'], hex_sha256=sha, source_pin=b['source_pin'])
        import serial
        from serial.tools import list_ports
        matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x045b, 0x5310)]
        if len(matches) != 1:
            raise ValueError('expected one Titan app CDC')
        device = matches[0].device
        owners = subprocess.run(['lsof', '-t', device, device.replace('/cu.', '/tty.')],
                                capture_output=True, text=True)
        if owners.returncode not in (0, 1) or owners.stdout.strip():
            raise ValueError('CDC owned; no commands sent')
        port = serial.Serial(device, 115200, timeout=.5, write_timeout=2, exclusive=True)
        req = 0
        def transact(op):
            nonlocal req
            req += 1
            out = packet(op, req, payload=b'')
            if port.write(out) != len(out):
                raise ValueError('short USB write')
            port.flush()
            head = read_exact(port, 32, 15)
            magic, status, rid, seq, size, cycles, crc, hcrc = struct.unpack('<4s7I', head)
            if magic != b'K1R1' or rid != req or zlib.crc32(head[:28]) != hcrc or size > 19968:
                raise ValueError('invalid response envelope')
            body = read_exact(port, size, 15)
            if zlib.crc32(body) != crc or status:
                raise ValueError(f'opcode {op} failed: {status}')
            return body
        info = json.loads(transact(1))
        receipt['runtime'] = info
        if info.get('uid') != UID or info.get('build') != b['build_id'] or info.get('source') != PIN:
            raise ValueError('UID/build/source mismatch')
        first = json.loads(transact(6))
        receipt['first'] = first
        time.sleep(args.pause_s)
        second = json.loads(transact(6))
        receipt['second'] = second
        again = json.loads(transact(1))
        receipt['runtime_end'] = again
        if any(again.get(k) != info.get(k) for k in ['uid', 'build', 'source']):
            raise ValueError('identity changed during run')
        receipt['delta'] = score(first, second)
        gpt = None
        try:
            from run_gpt_dma_autonomous import decode_snapshot, score as gpt_score
            g0 = decode_snapshot(transact(22))
            time.sleep(max(2.0, min(args.pause_s, 10.0)))
            g1 = decode_snapshot(transact(22))
            gpt = gpt_score(g0, g1)
            if g1.get('frames', 0) < 4378:
                raise ValueError(f'GPT frames {g1.get("frames")} did not pass prior freeze at 4377')
            receipt['gpt'] = {'first': g0, 'second': g1, 'delta': gpt}
        except Exception as error:
            receipt['gpt_error'] = str(error)
            raise
        receipt['pass'] = True
        print(json.dumps({
            'pass': True,
            'build': b['build_id'],
            'delta': receipt['delta'],
            'gpt': gpt,
            'g4': receipt['g4'],
        }, indent=2))
    except Exception as error:
        receipt['error'] = str(error)
        raise
    finally:
        if port is not None:
            port.close()
            receipt['cdc_released'] = True
        receipt['end'] = datetime.now(timezone.utc).isoformat()
        (args.output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')

if __name__ == '__main__':
    main()
