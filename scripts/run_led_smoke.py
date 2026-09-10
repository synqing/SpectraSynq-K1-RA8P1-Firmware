#!/usr/bin/env python3
"""UID-gated opcode-11 LED smoke. CRC proves emission, not photons."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
import zlib
from datetime import datetime, timezone
from verify_imports import PIN

UID = '545433931bd25436593630352d068363'


def packet(op, request, sequence=0, payload=b''):
    header = struct.pack('<4s6I', b'K1S1', op, request, sequence, len(payload),
                         zlib.crc32(payload), 1)
    return header + struct.pack('<I', zlib.crc32(header)) + payload


def read_exact(port, size, timeout):
    data = bytearray()
    deadline = time.monotonic() + timeout
    while len(data) < size and time.monotonic() < deadline:
        data.extend(port.read(size - len(data)))
    if len(data) != size:
        raise RuntimeError(f'USB timeout {len(data)}/{size}')
    return bytes(data)


def pack_pixel(red, green, blue):
    return bytes([green >> 8, green & 0xFF, red >> 8, red & 0xFF, blue >> 8,
                  blue & 0xFF])


def expected_lanes():
    lane_a = bytearray(480)
    lane_b = bytearray(480)
    lane_a[0:6] = pack_pixel(0x12AB, 0, 0)
    lane_b[0:6] = pack_pixel(0, 0, 0x12AB)
    vis_a = pack_pixel(0x7A3C, 0, 0)
    vis_b = pack_pixel(0, 0, 0x7A3C)
    assert vis_a[3] == 0x3C and vis_a[3] != 0x7A
    for i in range(1, 8):
        lane_a[i * 6:(i + 1) * 6] = vis_a
        lane_b[i * 6:(i + 1) * 6] = vis_b
    assert lane_a[3] == 0xAB and lane_a[3] != 0x12
    return bytes(lane_a), bytes(lane_b)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = dict(
        label='ON-SILICON',
        start=datetime.now(timezone.utc).isoformat(),
        crc_proves='emission_not_reception',
        photons='NOT_CLAIMED',
        level_shifter='74HCT2G34GW',
        photon_disagreement_first_suspect='bit_timing_or_data_break',
        data_break='CAPTAIN_REPORTED_UNVERIFIED',
        cpu_blocked_budget='G0.1_1000us_MISS_EXPECTED',
        **{'pass': False})
    port = None
    try:
        import serial
        from serial.tools import list_ports
        build = json.loads((args.build / 'receipt.json').read_text())
        if not build['pass']:
            raise RuntimeError('build receipt failed')
        hex_path = args.build / 'rtthread.hex'
        hex_sha = hashlib.sha256(hex_path.read_bytes()).hexdigest()
        if hex_sha != build['artifacts']['rtthread.hex']:
            raise RuntimeError('image changed')
        matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x045b, 0x5310)]
        if len(matches) != 1:
            raise RuntimeError('expected exactly one Titan application USB')
        device = matches[0].device
        owners = subprocess.run(['lsof', '-t', device], capture_output=True, text=True)
        if owners.returncode not in (0, 1) or owners.stdout.strip():
            raise RuntimeError('USB has another owner')
        # Opcode 11 holds IRQs for ~4.8 ms bits + ~300 µs latch. Inter-byte
        # timeout must exceed ~10 ms so the first reply byte is not a false USB fault.
        port = serial.Serial(device, 115200, timeout=0.5, write_timeout=2, exclusive=True)
        receipt['usb'] = dict(path=device, location=matches[0].location,
                              vid=matches[0].vid, pid=matches[0].pid)
        request = 0

        def transact(op, payload=b'', expected_status=0, timeout=15):
            nonlocal request
            request += 1
            data = packet(op, request, 0, payload)
            if port.write(data) != len(data):
                raise RuntimeError('short USB write')
            port.flush()
            header = read_exact(port, 32, timeout)
            magic, status, rid, seq, size, cycles, crc, header_crc = struct.unpack(
                '<4s7I', header)
            if magic != b'K1R1' or zlib.crc32(header[:28]) != header_crc or rid != request or size > 19968:
                raise RuntimeError('response identity/header/length invalid')
            body = read_exact(port, size, timeout)
            if zlib.crc32(body) != crc or status != expected_status:
                raise RuntimeError(
                    f'response rejected: status={status} expected={expected_status}')
            return body, seq, cycles

        body, _, _ = transact(1)
        info = json.loads(body)
        receipt['runtime'] = info
        if info['uid'] != UID or info['build'] != build['build_id'] or info['source'] != PIN:
            raise RuntimeError('runtime UID/build/source mismatch')
        if info['protocol'] != 1 or info['clock_hz'] <= 0:
            raise RuntimeError('runtime protocol/clock invalid')
        lane_a, lane_b = expected_lanes()
        expect_a = zlib.crc32(lane_a) & 0xffffffff
        expect_b = zlib.crc32(lane_b) & 0xffffffff
        body, _, cycles = transact(11, timeout=15)
        payload = json.loads(body)
        receipt['led'] = payload
        receipt['usb_cycles'] = cycles
        receipt['expected_crc_a'] = expect_a
        receipt['expected_crc_b'] = expect_b
        receipt['image_sha256'] = hex_sha
        receipt['CURRENT_TARGET'] = True
        if payload.get('op') != 11:
            raise RuntimeError('opcode 11 reply missing')
        if payload.get('din_a') != 'P601' or payload.get('din_b') != 'P004':
            raise RuntimeError('pin identity mismatch')
        if payload.get('pixels_a') != 80 or payload.get('pixels_b') != 80:
            raise RuntimeError('pixel count mismatch')
        if payload.get('crc_a') != expect_a or payload.get('crc_b') != expect_b:
            raise RuntimeError('packed CRC mismatch (emission)')
        if payload.get('true16') != '0x12AB':
            raise RuntimeError('TRUE16 fixture missing')
        if int(payload.get('bit_period_min_cycles') or 0) <= 0:
            raise RuntimeError('bit-period min missing')
        if int(payload['bit_period_max_cycles']) < int(payload['bit_period_min_cycles']):
            raise RuntimeError('bit-period max < min')
        clock = int(info['clock_hz'])
        emit_us = (int(payload['emit_cycles']) * 1000000 + clock - 1) // clock
        receipt['emit_us'] = emit_us
        if not (3500 <= emit_us <= 8000):
            raise RuntimeError(f'emit_us {emit_us} outside 3.5–8.0 ms PRE-SILICON band')
        receipt['pass'] = True
    except Exception as error:
        receipt['error'] = str(error)
        raise
    finally:
        if port is not None:
            port.close()
        receipt['end'] = datetime.now(timezone.utc).isoformat()
        (args.output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')


if __name__ == '__main__':
    main()
