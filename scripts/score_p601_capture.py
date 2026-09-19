#!/usr/bin/env python3
"""Predicted 3072-bit P601/DIN reference and fail-closed capture scorer.

Firmware accounting of the retained colour-integrity frame. Packed GRB is not
wire evidence. WAVEFORM_CAPTURED is refused unless a physical capture names
both P601 and first-LED DIN with the required pulse fields and file hashes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

from gpt_fault_witness import PROFILES, fnv1a32

DEFAULT_WITNESS = Path(
    '/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/'
    'k1-ra8p1-002/colour-integrity-led-first-run-20260916-01/'
    'zero-output-first-fault-gpt.json')
DEFAULT_RUN = DEFAULT_WITNESS.parent / 'receipt.json'
RETAINED_BUILD = (
    'a3f37e8a49ac1afbc13448c835f2dd21f07aa20e41c7f696c2a6a11c8f4c9717')
RETAINED_UID = '545433931bd25436593630352d068363'
WIRE_BITS = 3072
DMA_WORDS = 3070
PIN = 'P601'
GTIO = 'GTIOC6A'
PROBE_FIELDS = (
    'bit_count',
    'idle_low_before_first_ns',
    'first_bit_start_ns',
    'first_bit_high_ns',
    'last_bit_start_ns',
    'last_bit_high_ns',
    'last_bit_clipped',
    'reset_low_ns',
    'capture_sha256',
)


class CaptureScoreError(ValueError):
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code


def reconstruct_duty(payload, profile_id, clock_hz):
    profile = PROFILES.get(profile_id)
    if not profile:
        raise CaptureScoreError('K1_P601_PROFILE', 'unknown WS281x profile')
    if not clock_hz:
        raise CaptureScoreError('K1_P601_CLOCK', 'missing GPT clock')
    duty = bytearray()
    bits = []
    for byte in payload:
        for bit in range(8):
            one = bool(byte & (0x80 >> bit))
            bits.append(1 if one else 0)
            counts = (clock_hz * profile[one] + 999999999) // 1000000000
            duty.extend(struct.pack('<I', counts - 1))
    return bytes(duty), bits, profile


def predict_from_witness(witness_path=DEFAULT_WITNESS, run_path=DEFAULT_RUN):
    gpt = json.loads(Path(witness_path).read_text())
    run = json.loads(Path(run_path).read_text()) if Path(run_path).is_file() else {}
    witness = gpt['fault_witness']
    payload = bytes.fromhex(witness['packed_grb_hex'])
    if len(payload) != witness['payload_bytes']:
        raise CaptureScoreError('K1_P601_PAYLOAD', 'payload length mismatch')
    if hashlib.sha256(payload).hexdigest() != witness['packed_grb_sha256']:
        raise CaptureScoreError('K1_P601_PAYLOAD', 'packed GRB hash mismatch')
    if fnv1a32(payload) != witness['payload_fnv1a32']:
        raise CaptureScoreError('K1_P601_PAYLOAD', 'packed GRB FNV mismatch')
    duty, bits, profile = reconstruct_duty(
        payload, witness['profile'], gpt['clock_hz'])
    if fnv1a32(duty) != witness['duty_fnv1a32']:
        raise CaptureScoreError('K1_P601_DUTY', 'duty reconstruction mismatch')
    if gpt['bits'] != WIRE_BITS or len(bits) != WIRE_BITS:
        raise CaptureScoreError('K1_P601_BITS', 'retained frame is not 3072 bits')
    if witness['expected_dma_words'] != DMA_WORDS:
        raise CaptureScoreError('K1_P601_DMA', 'expected DMA words are not 3070')
    t0h_counts = (gpt['clock_hz'] * profile[0] + 999999999) // 1000000000
    t1h_counts = (gpt['clock_hz'] * profile[1] + 999999999) // 1000000000
    build_id = run.get('build_id') or run.get('identity', {}).get('build')
    uid = run.get('identity', {}).get('uid')
    predicted = {
        'label': 'PREDICTED_WIRE',
        'waveform': 'NOT_CAPTURED',
        'physical_claims': False,
        'authority': 'firmware_accounting',
        'build_id': build_id,
        'uid': uid,
        'frame_id': witness['frame_id'],
        'profile': witness['profile'],
        'clock_hz': gpt['clock_hz'],
        'pixels': len(payload) // profile[3],
        'payload_bytes': len(payload),
        'bits': WIRE_BITS,
        'expected_dma_words': DMA_WORDS,
        'pin': PIN,
        'gtio': GTIO,
        'packed_grb_sha256': witness['packed_grb_sha256'],
        'payload_fnv1a32': witness['payload_fnv1a32'],
        'duty_fnv1a32': witness['duty_fnv1a32'],
        't0h_ns': profile[0],
        't1h_ns': profile[1],
        'period_ns': profile[2],
        't0h_duty_counts': t0h_counts - 1,
        't1h_duty_counts': t1h_counts - 1,
        'all_zero_grb': payload == bytes(len(payload)),
        'first_bit': bits[0],
        'last_bit': bits[-1],
        'first_high_ns': profile[bits[0]],
        'last_high_ns': profile[bits[-1]],
        'dma_remaining_at_fault': witness['terminal']['dmac_count'],
        'source_witness': str(Path(witness_path).resolve()),
        'source_run': str(Path(run_path).resolve()) if Path(run_path).is_file() else None,
        'note': 'Predicted GPT duty of the retained submitted frame. Not wire evidence.',
    }
    if predicted['build_id'] != RETAINED_BUILD:
        raise CaptureScoreError('K1_P601_IDENTITY', 'witness is not retained a3f37e8a')
    if predicted['uid'] != RETAINED_UID:
        raise CaptureScoreError('K1_P601_IDENTITY', 'witness UID is not the identified Titan')
    if not predicted['all_zero_grb'] or predicted['pixels'] != 128:
        raise CaptureScoreError('K1_P601_FRAME', 'retained frame is not 128 zero GRB pixels')
    return predicted


def _probe(capture, name):
    probe = capture.get(name)
    if not isinstance(probe, dict):
        raise CaptureScoreError('K1_P601_PROBE_MISSING', f'{name} measurement missing')
    missing = [field for field in PROBE_FIELDS if field not in probe]
    if missing:
        raise CaptureScoreError(
            'K1_P601_PROBE_FIELDS',
            f'{name} missing {",".join(missing)}')
    digest = probe['capture_sha256']
    if not isinstance(digest, str) or len(digest) != 64:
        raise CaptureScoreError('K1_P601_PROBE_HASH', f'{name} capture hash missing')
    if digest == '0' * 64:
        raise CaptureScoreError('K1_P601_PROBE_HASH', f'{name} capture hash is a placeholder')
    return probe


def score_capture(capture, predicted):
    if not isinstance(capture, dict):
        raise CaptureScoreError('K1_P601_CAPTURE_MISSING', 'capture object missing')
    claimed = capture.get('waveform')
    source = capture.get('source')
    if capture.get('build_id') != predicted['build_id']:
        raise CaptureScoreError('K1_P601_IDENTITY', 'capture build is not the retained image')
    if capture.get('uid') != predicted['uid']:
        raise CaptureScoreError('K1_P601_IDENTITY', 'capture UID is not the identified Titan')
    if capture.get('packed_grb_sha256') != predicted['packed_grb_sha256']:
        raise CaptureScoreError('K1_P601_FRAME', 'capture is not the retained packed GRB')
    if capture.get('frame_id') != predicted['frame_id']:
        raise CaptureScoreError('K1_P601_FRAME', 'capture frame_id is not the retained fault frame')
    p601 = _probe(capture, 'p601')
    din = _probe(capture, 'din')
    bit_counts = (p601['bit_count'], din['bit_count'])
    bit_count_ok = bit_counts == (WIRE_BITS, WIRE_BITS)
    last_clipped = bool(p601['last_bit_clipped'] or din['last_bit_clipped'])
    if source != 'physical':
        if claimed == 'WAVEFORM_CAPTURED':
            raise CaptureScoreError(
                'K1_P601_OVERCLAIM',
                'WAVEFORM_CAPTURED requires a physical P601+DIN capture')
        return {
            'pass': True,
            'waveform': 'NOT_CAPTURED',
            'physical_claims': False,
            'bit_count_ok': bit_count_ok,
            'bit_counts': {'p601': p601['bit_count'], 'din': din['bit_count']},
            'last_clipped': last_clipped,
            'source': source,
            'note': 'Fields complete on a non-physical object. Not wire evidence.',
        }
    stamp = 'WAVEFORM_CAPTURED'
    return {
        'pass': True,
        'waveform': stamp,
        'physical_claims': True,
        'bit_count_ok': bit_count_ok,
        'bit_counts': {'p601': p601['bit_count'], 'din': din['bit_count']},
        'last_clipped': last_clipped,
        'source': 'physical',
        'first_divergence': (
            None if bit_count_ok and not last_clipped else
            'count_short_or_last_clipped'),
    }


def complete_synthetic(predicted, bit_count=WIRE_BITS, source='synthetic'):
    digest = hashlib.sha256(f'synthetic-{bit_count}'.encode()).hexdigest()
    probe = {
        'bit_count': bit_count,
        'idle_low_before_first_ns': 2000,
        'first_bit_start_ns': 0,
        'first_bit_high_ns': predicted['first_high_ns'],
        'last_bit_start_ns': (bit_count - 1) * predicted['period_ns'],
        'last_bit_high_ns': predicted['last_high_ns'],
        'last_bit_clipped': False,
        'reset_low_ns': 50000,
        'capture_sha256': digest,
    }
    return {
        'build_id': predicted['build_id'],
        'uid': predicted['uid'],
        'packed_grb_sha256': predicted['packed_grb_sha256'],
        'frame_id': predicted['frame_id'],
        'source': source,
        'p601': dict(probe),
        'din': dict(probe),
    }


def score_path(path, predicted):
    target = Path(path)
    if not target.is_file():
        raise CaptureScoreError('K1_P601_CAPTURE_MISSING', f'capture missing: {target}')
    return score_capture(json.loads(target.read_text()), predicted)


def self_test():
    predicted = predict_from_witness()
    results = []

    try:
        score_path(Path('/tmp/k1-p601-capture-absent.json'), predicted)
        raise AssertionError('missing capture did not fail')
    except CaptureScoreError as error:
        if error.code != 'K1_P601_CAPTURE_MISSING':
            raise
        results.append('MISSING_FAIL')

    historical = Path(
        '/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/'
        'k1-ra8p1-002/ws2812-p601-live-01/receipt.json')
    try:
        score_path(historical, predicted)
        raise AssertionError('historical WS2812 diagnostic did not fail')
    except CaptureScoreError as error:
        if error.code not in ('K1_P601_IDENTITY', 'K1_P601_FRAME', 'K1_P601_PROBE_MISSING'):
            raise
        results.append('HISTORICAL_FAIL')

    short = complete_synthetic(predicted, bit_count=WIRE_BITS - 1)
    short_score = score_capture(short, predicted)
    if short_score['waveform'] != 'NOT_CAPTURED' or short_score['bit_count_ok']:
        raise CaptureScoreError('K1_P601_SELFTEST', 'short synthetic was not fail-closed')
    results.append('SHORT_SYNTHETIC_NOT_CAPTURED')

    complete = complete_synthetic(predicted)
    complete_score = score_capture(complete, predicted)
    if complete_score['waveform'] != 'NOT_CAPTURED' or not complete_score['bit_count_ok']:
        raise CaptureScoreError('K1_P601_SELFTEST', 'complete synthetic stamped wire evidence')
    results.append('COMPLETE_SYNTHETIC_NOT_CAPTURED')

    overclaim = dict(complete, waveform='WAVEFORM_CAPTURED')
    try:
        score_capture(overclaim, predicted)
        raise AssertionError('synthetic WAVEFORM_CAPTURED was accepted')
    except CaptureScoreError as error:
        if error.code != 'K1_P601_OVERCLAIM':
            raise
        results.append('OVERCLAIM_FAIL')

    physical = dict(complete_synthetic(predicted, source='physical'),
                    waveform='WAVEFORM_CAPTURED')
    physical_score = score_capture(physical, predicted)
    if physical_score['waveform'] != 'WAVEFORM_CAPTURED':
        raise CaptureScoreError('K1_P601_SELFTEST', 'physical complete capture did not stamp')
    results.append('PHYSICAL_COMPLETE_STAMP')

    return {
        'pass': True,
        'K1_P601_SELFTEST': 'PASS',
        'waveform': 'NOT_CAPTURED',
        'physical_claims': False,
        'predicted_bits': predicted['bits'],
        'predicted_dma_words': predicted['expected_dma_words'],
        'packed_grb_sha256': predicted['packed_grb_sha256'],
        'proofs': results,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--witness', type=Path, default=DEFAULT_WITNESS)
    parser.add_argument('--run', type=Path, default=DEFAULT_RUN)
    parser.add_argument('--predict', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--capture', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    try:
        predicted = predict_from_witness(args.witness, args.run)
        if args.self_test:
            result = self_test()
        elif args.predict:
            result = predicted
        elif args.capture:
            result = score_path(args.capture, predicted)
        else:
            parser.error('specify --predict, --self-test or --capture')
        text = json.dumps(result, indent=2) + '\n'
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            if args.output.exists():
                raise CaptureScoreError('K1_P601_OUTPUT', f'refusing to reuse {args.output}')
            args.output.write_text(text)
        sys.stdout.write(text)
        return 0
    except CaptureScoreError as error:
        sys.stdout.write(json.dumps({
            'pass': False,
            'code': error.code,
            'error': str(error),
            'waveform': 'NOT_CAPTURED',
            'physical_claims': False,
        }, indent=2) + '\n')
        return 1


if __name__ == '__main__':
    sys.exit(main())
