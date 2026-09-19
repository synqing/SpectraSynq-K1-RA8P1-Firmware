#!/usr/bin/env python3
"""Score Titan LED2 PHY identity and its four-state control sequence on silicon.

This checks PHY identity and firmware-reported channel states.
It does not independently verify register readback or emitted light.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import time
import zlib

from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN


PHY_ID = 0x001CC916
EXPECTED_DWT_HZ = 1_000_000_000
EXPECTED_CHANNELS = [(0, 0), (1, 0), (0, 0), (0, 1), (0, 0), (1, 1), (0, 0)]
TRACE_RECORD = struct.Struct('<IIIHBBBBBx')


def parse_firmware_trace(body):
    if len(body) < 8 or body[:4] != b'L2T1':
        raise RuntimeError('LED2 trace header invalid')
    count = struct.unpack_from('<I', body, 4)[0]
    if count > 96 or len(body) != 8 + count * TRACE_RECORD.size:
        raise RuntimeError('LED2 trace size invalid')
    result = []
    previous = 0
    for offset in range(8, len(body), TRACE_RECORD.size):
        sequence, attempt, reset_age_ms, value, address, reg, ack, io_error, value_valid = (
            TRACE_RECORD.unpack_from(body, offset))
        if sequence <= previous or ack > 2 or io_error > 1 or value_valid > 1:
            raise RuntimeError('LED2 trace record invalid')
        previous = sequence
        result.append({
            'sequence': sequence,
            'attempt': attempt,
            'reset_age_ms': reset_age_ms,
            'address': address,
            'register': reg,
            'ack': ack,
            'io_error': io_error,
            'value': value,
            'value_valid': bool(value_valid),
        })
    return result


def compact_channels(samples):
    result = []
    for sample in samples:
        channels = (int(sample['g']), int(sample['y']))
        if not result or result[-1] != channels:
            result.append(channels)
    return result


def validate_phy_identity(sample):
    """Reject an unexpected responder; a readiness bit alone is insufficient."""
    phy_id = int(sample.get('id', 0))
    if phy_id & 0xfffffff0 != PHY_ID & 0xfffffff0:
        raise RuntimeError('RTL8211F identity mismatch')
    id1, id2 = int(sample.get('id1', 0)), int(sample.get('id2', 0))
    if id1 in (0, 0xffff) or id2 in (0, 0xffff):
        raise RuntimeError('invalid PHY identity word')
    if not 0 < id1 < 0xffff or not 0 < id2 < 0xffff or (id1 << 16 | id2) != phy_id:
        raise RuntimeError('PHY identity words disagree with combined identity')
    if int(sample.get('addr', -1)) != 1:
        raise RuntimeError('PHY address strap mismatch')


def validate_ready(sample):
    if sample.get('ok') is not True:
        raise RuntimeError('PHY control path became unavailable')
    validate_phy_identity(sample)
    if int(sample.get('io', 0)) != 0 or int(sample.get('err', -1)) != 0:
        raise RuntimeError(f"PHY status error {sample.get('err')} io={sample.get('io', 0)}")
    if sample.get('applied_valid') is not True or sample.get('reg_readback') is not True:
        raise RuntimeError('PHY register readback is not valid')
    if sample.get('page_unknown') is True:
        raise RuntimeError('PHY register page is uncertain')
    if sample.get('dwt_ok') is not True or int(sample.get('dwt_hz', 0)) != EXPECTED_DWT_HZ:
        raise RuntimeError('DWT timing authority is not valid at 1 GHz')
    if int(sample.get('pins', -1)) != 0xff:
        raise RuntimeError('GPIO/PIDR preflight did not pass')
    if sample.get('txc_requested') is not True or sample.get('txc_state') != 'unverified':
        raise RuntimeError('TXC route state is not explicit')


def observe_readiness(read_status, receipt, seconds, sample_ms,
                      monotonic=time.monotonic, sleep=time.sleep):
    """Observe bounded readiness; host polls are not proof of firmware retries.

    read_status returns the existing opcode-20 JSON reply. This helper sends no
    sequence command. History is attached before I/O so main's finally block
    preserves every completed observation even if a later transaction fails.
    A blocking transaction may finish after the deadline; it cannot be accepted
    as timely readiness and no additional transaction starts after the deadline.
    """
    if not math.isfinite(seconds) or not 0 < seconds <= 60:
        raise ValueError('ready-seconds must be finite, > 0 and <= 60')
    if not 25 <= sample_ms <= 200:
        raise ValueError('sample-ms must be 25..200')
    started = monotonic()
    deadline = started + seconds
    history = []
    observation = receipt['readiness'] = {
        'timeout_seconds': seconds,
        'sample_ms': sample_ms,
        'firmware_retry_count': 'NOT_OBSERVED',
        'history': history,
        'ready': False,
    }
    while monotonic() < deadline:
        poll_started = monotonic()
        try:
            status = read_status()
        except Exception as error:
            observation['error'] = str(error)
            observation['elapsed_ms'] = round((monotonic() - started) * 1000)
            raise
        sample = dict(status.get('led2', {}))
        completed = monotonic()
        history.append({
            'elapsed_ms': round((completed - started) * 1000),
            'poll_started_ms': round((poll_started - started) * 1000),
            'observed_at': datetime.now(timezone.utc).isoformat(),
            'led2': sample,
        })
        if 'before' not in receipt:
            receipt['before'] = status
        # Zero identity is expected while discovery has no valid responder.
        # A claimed nonzero identity must never bypass identity/address checks.
        if int(sample.get('id', 0)) != 0:
            validate_phy_identity(sample)
        if int(sample.get('io', 0)) != 0 or int(sample.get('err', -1)) not in (0, 1, 4):
            raise RuntimeError(f'PHY readiness failed: {sample}')
        if sample.get('ok') is True:
            validate_ready(sample)
            if completed >= deadline:
                break
            observation['ready'] = True
            observation['elapsed_ms'] = round((completed - started) * 1000)
            receipt['ready_status'] = status
            return status
        remaining = deadline - monotonic()
        if remaining > 0:
            sleep(min(sample_ms / 1000.0, remaining))
    observation['elapsed_ms'] = round((monotonic() - started) * 1000)
    observation['error'] = 'PHY identity/control readiness timeout'
    raise RuntimeError('PHY identity/control readiness timeout')


def collect_samples(read_status, receipt, seconds, sample_ms,
                    monotonic=time.monotonic, sleep=time.sleep):
    """Keep partial sequence evidence attached to the receipt during I/O."""
    samples = receipt['samples'] = []
    started = monotonic()
    deadline = started + seconds
    while monotonic() < deadline:
        status = read_status()
        sample = dict(status.get('led2', {}))
        sample['elapsed_ms'] = round((monotonic() - started) * 1000)
        samples.append(sample)
        remaining = deadline - monotonic()
        if remaining > 0:
            sleep(min(sample_ms / 1000.0, remaining))
    return samples


def score_samples(samples):
    if not samples:
        raise RuntimeError('no LED2 status samples')
    for sample in samples:
        validate_ready(sample)
    observed = compact_channels(samples)
    matched = any(observed[start:start + len(EXPECTED_CHANNELS)] == EXPECTED_CHANNELS
                  for start in range(len(observed)))
    if not matched:
        raise RuntimeError(f'four-state sequence incomplete: {observed}')
    return observed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seconds', type=float, default=5.0)
    parser.add_argument('--sample-ms', type=int, default=75)
    parser.add_argument('--ready-seconds', type=float, default=5.0,
                        help='bounded readiness observation before sequence (default: 5 s)')
    args = parser.parse_args()
    if (not math.isfinite(args.seconds) or not 4.2 <= args.seconds <= 60
            or not math.isfinite(args.ready_seconds) or not 0 < args.ready_seconds <= 60
            or not 25 <= args.sample_ms <= 200):
        raise SystemExit('seconds must be finite 4.2..60, ready-seconds finite > 0 and <= 60, '
                         'and sample-ms 25..200')
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = {
        'label': 'ON-SILICON_CONTROL_PLANE',
        'start': datetime.now(timezone.utc).isoformat(),
        'pass': False,
        'optical': 'NOT_CLAIMED',
        'proves': 'PHY_IDENTITY_AND_FIRMWARE_REPORTED_CHANNEL_SEQUENCE',
        'register_readback': 'FIRMWARE_READBACK_ONLY_NOT_INDEPENDENT',
    }
    port = None
    try:
        import serial
        from serial.tools import list_ports

        build = json.loads((args.build / 'receipt.json').read_text())
        if not build.get('pass'):
            raise RuntimeError('build receipt failed')
        hex_path = args.build / 'rtthread.hex'
        image_sha = hashlib.sha256(hex_path.read_bytes()).hexdigest()
        if image_sha != build['artifacts']['rtthread.hex']:
            raise RuntimeError('image changed after build')
        receipt['build_id'] = build['build_id']
        receipt['image_sha256'] = image_sha

        matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x045b, 0x5310)]
        if len(matches) != 1:
            raise RuntimeError('expected exactly one Titan application USB')
        device = matches[0].device
        owners = subprocess.run(['lsof', '-t', device], capture_output=True, text=True)
        if owners.returncode not in (0, 1) or owners.stdout.strip():
            raise RuntimeError('USB has another owner')
        port = serial.Serial(device, 115200, timeout=0.5, write_timeout=2, exclusive=True)
        receipt['usb'] = {'path': device, 'location': matches[0].location,
                          'vid': matches[0].vid, 'pid': matches[0].pid}
        request = 0

        def transact(op, payload=b'', expected_status=0):
            nonlocal request
            request += 1
            data = packet(op, request, payload=payload)
            if port.write(data) != len(data):
                raise RuntimeError('short USB write')
            port.flush()
            header = read_exact(port, 32, 15)
            magic, status, rid, sequence, size, cycles, crc, header_crc = struct.unpack(
                '<4s7I', header)
            if magic != b'K1R1' or zlib.crc32(header[:28]) != header_crc:
                raise RuntimeError('response header invalid')
            if rid != request or size > 19968:
                raise RuntimeError('response identity or length invalid')
            body = read_exact(port, size, 15)
            if zlib.crc32(body) != crc or status != expected_status:
                raise RuntimeError(f'response rejected: status={status} expected={expected_status}')
            return body, sequence, cycles

        body, _, _ = transact(1)
        info = json.loads(body)
        receipt['runtime'] = info
        if info.get('uid') != UID or info.get('build') != build.get('build_id'):
            raise RuntimeError('runtime UID/build mismatch')
        if info.get('source') != PIN:
            raise RuntimeError('runtime import-source mismatch')
        if int(info.get('clock_hz', 0)) != EXPECTED_DWT_HZ:
            raise RuntimeError('runtime core clock is not the expected 1 GHz')

        def read_status():
            return json.loads(transact(20)[0])

        def capture_trace(label, required=False):
            try:
                receipt[label] = parse_firmware_trace(transact(20, bytes([9]))[0])
            except Exception as trace_error:
                receipt[label + '_error'] = str(trace_error)
                if required:
                    raise

        capture_trace('trace_before_readiness', required=True)
        try:
            observe_readiness(read_status, receipt, args.ready_seconds, args.sample_ms)
        except Exception:
            capture_trace('trace_after_readiness_failure')
            raise
        response = transact(20, bytes([8]))[0]
        if response != b'LED2SEQ':
            raise RuntimeError('LED2 sequence restart rejected')

        samples = collect_samples(read_status, receipt, args.seconds, args.sample_ms)
        receipt['observed_channels'] = score_samples(samples)
        capture_trace('trace_after_sequence', required=True)
        receipt['build_id'] = build['build_id']
        receipt['image_sha256'] = image_sha
        receipt['CURRENT_TARGET'] = True
        receipt['pass'] = True
        print('LED2_PHY_CONTROL_SEQUENCE_PASS')
    except Exception as error:
        receipt['error'] = str(error)
        raise
    finally:
        try:
            if port is not None:
                port.close()
        finally:
            receipt['end'] = datetime.now(timezone.utc).isoformat()
            (args.output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')


if __name__ == '__main__':
    main()
