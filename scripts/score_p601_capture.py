#!/usr/bin/env python3
"""Predicted 3072-bit P601/DIN reference and fail-closed capture scorer.

Firmware accounting of a verified witness is not wire evidence.
WAVEFORM_CAPTURED requires hashed raw files, same-acquisition correlation,
and pulse conformance. A captured bad waveform stays evidence; it is not a pass.
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
RESET_LOW_MIN_NS = 50000
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


def file_sha256(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


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


def retained_zero_frame_prediction():
    """Host-only 128-pixel all-zero accounting. Does not read Mac receipts."""
    payload = bytes(384)
    clock_hz = 300000000
    duty, bits, profile = reconstruct_duty(payload, 1, clock_hz)
    t0h_counts = (clock_hz * profile[0] + 999999999) // 1000000000
    t1h_counts = (clock_hz * profile[1] + 999999999) // 1000000000
    return {
        'label': 'PREDICTED_WIRE',
        'waveform': 'NOT_CAPTURED',
        'physical_claims': False,
        'authority': 'firmware_accounting',
        'build_id': RETAINED_BUILD,
        'uid': RETAINED_UID,
        'frame_id': 2030,
        'profile': 1,
        'clock_hz': clock_hz,
        'pixels': 128,
        'payload_bytes': len(payload),
        'bits': len(bits),
        'expected_dma_words': len(bits) - 2,
        'pin': PIN,
        'gtio': GTIO,
        'packed_grb_sha256': hashlib.sha256(payload).hexdigest(),
        'payload_fnv1a32': fnv1a32(payload),
        'duty_fnv1a32': fnv1a32(duty),
        't0h_ns': profile[0],
        't1h_ns': profile[1],
        'period_ns': profile[2],
        't0h_duty_counts': t0h_counts - 1,
        't1h_duty_counts': t1h_counts - 1,
        'all_zero_grb': True,
        'first_bit': bits[0],
        'last_bit': bits[-1],
        'first_high_ns': profile[bits[0]],
        'last_high_ns': profile[bits[-1]],
        'dma_remaining_at_fault': 1,
        'source_witness': 'retained_zero_frame_constants',
        'source_run': None,
        'require_retained': True,
        'note': 'Predicted GPT duty of the retained submitted frame. Not wire evidence.',
    }


def predict_from_witness(witness_path=DEFAULT_WITNESS, run_path=DEFAULT_RUN,
                         require_retained=True):
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
    if gpt['bits'] != len(bits):
        raise CaptureScoreError('K1_P601_BITS', 'witness bit count mismatch')
    expected_words = len(bits) - 2
    if witness['expected_dma_words'] != expected_words:
        raise CaptureScoreError('K1_P601_DMA', 'two-preload DMA word count mismatch')
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
        'bits': len(bits),
        'expected_dma_words': expected_words,
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
        'require_retained': require_retained,
        'note': 'Predicted GPT duty of the retained submitted frame. Not wire evidence.',
    }
    if predicted['uid'] != RETAINED_UID:
        raise CaptureScoreError('K1_P601_IDENTITY', 'witness UID is not the identified Titan')
    if not predicted['build_id']:
        raise CaptureScoreError('K1_P601_IDENTITY', 'witness run is missing build identity')
    if require_retained:
        if predicted['build_id'] != RETAINED_BUILD:
            raise CaptureScoreError('K1_P601_IDENTITY', 'witness is not retained a3f37e8a')
        if predicted['bits'] != WIRE_BITS or predicted['expected_dma_words'] != DMA_WORDS:
            raise CaptureScoreError('K1_P601_BITS', 'retained frame is not 3072 bits / 3070 DMA words')
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


def _raw_verified(probe, name):
    path = probe.get('capture_path')
    if not path:
        return False, f'{name} capture_path missing'
    target = Path(path)
    if not target.is_file():
        return False, f'{name} raw file missing: {target}'
    digest = file_sha256(target)
    claimed = str(probe['capture_sha256']).lower()
    if digest != claimed:
        return False, f'{name} SHA-256 mismatch'
    return True, digest


def _pulse_divergences(probe, predicted, resolution_ns):
    labels = []
    if probe['bit_count'] != predicted['bits']:
        labels.append('bit_count')
    if probe['last_bit_clipped']:
        labels.append('last_clipped')
    for key in (
            'idle_low_before_first_ns', 'first_bit_start_ns', 'first_bit_high_ns',
            'last_bit_start_ns', 'last_bit_high_ns', 'reset_low_ns'):
        if probe[key] < 0:
            labels.append('negative_timing')
            break
    if probe['reset_low_ns'] < RESET_LOW_MIN_NS:
        labels.append('reset_low')
    if resolution_ns is None or resolution_ns <= 0:
        labels.append('instrument_resolution_unspecified')
        return labels
    distinguishable = abs(predicted['t1h_ns'] - predicted['t0h_ns']) // 2
    if distinguishable and resolution_ns >= distinguishable:
        labels.append('uncertain_margin')
    for measured, expected, name in (
            (probe['first_bit_high_ns'], predicted['first_high_ns'], 'first_high'),
            (probe['last_bit_high_ns'], predicted['last_high_ns'], 'last_high')):
        if abs(measured - expected) > resolution_ns:
            labels.append(name)
    bits = predicted['bits']
    if bits > 1:
        span = probe['last_bit_start_ns'] - probe['first_bit_start_ns']
        period = span / (bits - 1)
        if abs(period - predicted['period_ns']) > resolution_ns:
            labels.append('period')
    return labels


def _first(items):
    return items[0] if items else None


def score_capture(capture, predicted):
    if not isinstance(capture, dict):
        raise CaptureScoreError('K1_P601_CAPTURE_MISSING', 'capture object missing')
    claimed = capture.get('waveform')
    source = capture.get('source')
    if capture.get('build_id') != predicted['build_id']:
        raise CaptureScoreError('K1_P601_IDENTITY', 'capture build is not the predicted image')
    if capture.get('uid') != predicted['uid']:
        raise CaptureScoreError('K1_P601_IDENTITY', 'capture UID is not the identified Titan')
    if capture.get('packed_grb_sha256') != predicted['packed_grb_sha256']:
        raise CaptureScoreError('K1_P601_FRAME', 'capture is not the predicted packed GRB')
    if capture.get('frame_id') != predicted['frame_id']:
        raise CaptureScoreError('K1_P601_FRAME', 'capture frame_id is not the predicted fault frame')
    p601 = _probe(capture, 'p601')
    din = _probe(capture, 'din')
    bit_counts = (p601['bit_count'], din['bit_count'])
    bit_count_ok = bit_counts == (predicted['bits'], predicted['bits'])
    last_clipped = bool(p601['last_bit_clipped'] or din['last_bit_clipped'])
    p601_raw, p601_raw_note = _raw_verified(p601, 'p601')
    din_raw, din_raw_note = _raw_verified(din, 'din')
    raw_files_verified = bool(p601_raw and din_raw)
    acquisition = capture.get('acquisition_id')
    p601_acq = p601.get('acquisition_id', acquisition)
    din_acq = din.get('acquisition_id', acquisition)
    frame_correlated = bool(
        acquisition and p601_acq and din_acq
        and p601_acq == din_acq == acquisition)
    resolution_ns = capture.get('instrument_resolution_ns')
    divergences = []
    if not frame_correlated:
        divergences.append('uncorrelated')
    divergences.extend(_pulse_divergences(p601, predicted, resolution_ns))
    divergences.extend(_pulse_divergences(din, predicted, resolution_ns))
    # Unique while preserving first-seen order.
    seen = []
    for item in divergences:
        if item not in seen:
            seen.append(item)
    waveform_conforms = not seen
    if source != 'physical':
        if claimed == 'WAVEFORM_CAPTURED':
            raise CaptureScoreError(
                'K1_P601_OVERCLAIM',
                'WAVEFORM_CAPTURED requires a physical P601+DIN capture')
        return {
            'pass': False,
            'waveform': 'NOT_CAPTURED',
            'physical_claims': False,
            'capture_present': True,
            'raw_files_verified': False,
            'frame_correlated': False,
            'waveform_conforms': False,
            'bit_count_ok': bit_count_ok,
            'bit_counts': {'p601': p601['bit_count'], 'din': din['bit_count']},
            'last_clipped': last_clipped,
            'first_divergence': None,
            'source': source,
            'note': 'Fields complete on a non-physical object. Not wire evidence.',
        }
    qualified = raw_files_verified and frame_correlated and waveform_conforms
    result = {
        'pass': qualified,
        'waveform': 'WAVEFORM_CAPTURED' if qualified else 'NOT_CAPTURED',
        'physical_claims': raw_files_verified,
        'capture_present': True,
        'raw_files_verified': raw_files_verified,
        'frame_correlated': frame_correlated,
        'waveform_conforms': waveform_conforms,
        'bit_count_ok': bit_count_ok,
        'bit_counts': {'p601': p601['bit_count'], 'din': din['bit_count']},
        'last_clipped': last_clipped,
        'first_divergence': None if qualified else _first(
            (['raw_files'] if not raw_files_verified else []) + seen),
        'source': 'physical',
        'raw_notes': {'p601': p601_raw_note, 'din': din_raw_note},
    }
    if claimed == 'WAVEFORM_CAPTURED' and not qualified:
        result['note'] = 'Physical object retained; waveform qualification refused'
    return result


def complete_synthetic(predicted, bit_count=None, source='synthetic'):
    if bit_count is None:
        bit_count = predicted['bits']
    digest = hashlib.sha256(f'synthetic-{bit_count}'.encode()).hexdigest()
    probe = {
        'bit_count': bit_count,
        'idle_low_before_first_ns': 2000,
        'first_bit_start_ns': 0,
        'first_bit_high_ns': predicted['first_high_ns'],
        'last_bit_start_ns': (bit_count - 1) * predicted['period_ns'],
        'last_bit_high_ns': predicted['last_high_ns'],
        'last_bit_clipped': False,
        'reset_low_ns': RESET_LOW_MIN_NS,
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


def physical_with_files(predicted, directory, *, bit_count=None, first_high=None,
                        last_high=None, reset_low=None, resolution_ns=1):
    capture = complete_synthetic(predicted, bit_count=bit_count, source='physical')
    if first_high is not None:
        capture['p601']['first_bit_high_ns'] = first_high
        capture['din']['first_bit_high_ns'] = first_high
    if last_high is not None:
        capture['p601']['last_bit_high_ns'] = last_high
        capture['din']['last_bit_high_ns'] = last_high
    if reset_low is not None:
        capture['p601']['reset_low_ns'] = reset_low
        capture['din']['reset_low_ns'] = reset_low
    root = Path(directory)
    root.mkdir(parents=True, exist_ok=True)
    acquisition = 'acq-' + predicted['packed_grb_sha256'][:16]
    capture['acquisition_id'] = acquisition
    capture['instrument_resolution_ns'] = resolution_ns
    for name in ('p601', 'din'):
        path = root / f'{name}.bin'
        payload = f'{name}:{acquisition}:{capture[name]["bit_count"]}'.encode()
        path.write_bytes(payload)
        capture[name]['capture_path'] = str(path)
        capture[name]['capture_sha256'] = hashlib.sha256(payload).hexdigest()
        capture[name]['acquisition_id'] = acquisition
    return capture


def score_path(path, predicted):
    target = Path(path)
    if not target.is_file():
        raise CaptureScoreError('K1_P601_CAPTURE_MISSING', f'capture missing: {target}')
    return score_capture(json.loads(target.read_text()), predicted)


def self_test():
    predicted = retained_zero_frame_prediction()
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
    if historical.is_file():
        try:
            score_path(historical, predicted)
            raise AssertionError('historical WS2812 diagnostic did not fail')
        except CaptureScoreError as error:
            if error.code not in (
                    'K1_P601_IDENTITY', 'K1_P601_FRAME', 'K1_P601_PROBE_MISSING'):
                raise
            results.append('HISTORICAL_FAIL')
    else:
        results.append('HISTORICAL_RECEIPT_ABSENT')

    short = complete_synthetic(predicted, bit_count=predicted['bits'] - 1)
    short_score = score_capture(short, predicted)
    if short_score['waveform'] != 'NOT_CAPTURED' or short_score['bit_count_ok'] or short_score['pass']:
        raise CaptureScoreError('K1_P601_SELFTEST', 'short synthetic was not fail-closed')
    results.append('SHORT_SYNTHETIC_NOT_CAPTURED')

    complete = complete_synthetic(predicted)
    complete_score = score_capture(complete, predicted)
    if complete_score['waveform'] != 'NOT_CAPTURED' or not complete_score['bit_count_ok'] or complete_score['pass']:
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
    if (physical_score['pass'] or physical_score['waveform'] == 'WAVEFORM_CAPTURED'
            or physical_score['physical_claims']):
        raise CaptureScoreError(
            'K1_P601_SELFTEST',
            'physical object without raw files was qualified')
    results.append('PHYSICAL_WITHOUT_FILES_REJECTED')

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
    parser.add_argument('--admit-current-witness', action='store_true',
                        help='accept a verified new witness/run pair; do not replace historic a3f37e8a')
    parser.add_argument('--predict', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--capture', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    try:
        if args.self_test:
            result = self_test()
        else:
            if args.witness.is_file():
                predicted = predict_from_witness(
                    args.witness, args.run,
                    require_retained=not args.admit_current_witness)
            else:
                predicted = retained_zero_frame_prediction()
            if args.predict:
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
        return 0 if result.get('pass', False) or args.self_test or args.predict else 1
    except CaptureScoreError as error:
        sys.stdout.write(json.dumps({
            'pass': False,
            'code': error.code,
            'error': str(error),
            'waveform': 'NOT_CAPTURED',
            'physical_claims': False,
            'capture_present': False,
            'raw_files_verified': False,
            'frame_correlated': False,
            'waveform_conforms': False,
        }, indent=2) + '\n')
        return 1


if __name__ == '__main__':
    sys.exit(main())
