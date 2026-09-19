#!/usr/bin/env python3
"""Bounded exclusive-CDC colour controls. Native RGB and GRB hashes are not wire proof."""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import time
import zlib
from datetime import datetime, timezone
from gpt_fault_witness import decode_v2
from run_led_smoke import UID, packet, read_exact
from verify_imports import ROOT, PIN, safe_path

SETTINGS = ('active automatic_cycle emit_enabled palette_a palette_b mode_a mode_b '
            'brightness output_channel transition_ms travel_ms direction showcase').split()
PDM_BAD = 'rearm_denied pair_skew_drops asrc_starved gain_clip_pos gain_clip_neg'.split()
LANE_BAD = ('error_callbacks overflow_events drop_events recovery_count '
            'packing_mismatch').split()


def integer(record, key):
    value = record[key]
    if type(value) is not int or not 0 <= value <= 0xffffffffffffffff:
        raise ValueError('missing/invalid counter: ' + key)
    return value


def delta(first, second, key):
    value = integer(second, key) - integer(first, key)
    if value < 0:
        raise ValueError('counter reset/wrap: ' + key)
    return value


def check_gpt(gpt, priority):
    if gpt['first_fault'] or gpt['errors'] or gpt['fault_witness']['valid']:
        raise ValueError('transmitter fault; preserve first witness, do not reset/retry')
    if gpt['dmctl'] & 1 != priority or gpt['bits'] != 3072:
        raise ValueError('DMA policy or 128-pixel GRB24 profile mismatch')
    if gpt['frames'] > min(gpt['dma_irqs'], gpt['stop_irqs']):
        raise ValueError('frame credited without both completion events')


def score_phase(g0, g1, p0, p1, priority, lane_map='pdm-first'):
    for g in (g0, g1):
        check_gpt(g, priority)
    advance = {k: delta(g0, g1, k) for k in ('attempts', 'dma_irqs', 'stop_irqs', 'frames')}
    if not all(advance.values()):
        raise ValueError('GPT did not advance')
    pdms = [p['pdm_target'] for p in (p0, p1)]
    for pdm in pdms:
        if (pdm['initialised'] is not True or pdm['running'] is not True
                or pdm['sample_rate_hz'] != 40000 or pdm['slot_elements'] != 296
                or pdm['profile'] != 'ap_40k_asrc24' or pdm['last_fsp_error'] != 0
                or len(pdm['lanes']) != 2):
            raise ValueError('dual PDM capture configuration/status invalid')
    first, second = pdms
    pipeline = {k: delta(first, second, k) for k in PDM_BAD}
    if any(pipeline.values()):
        raise ValueError('PDM/AP guardrail changed: ' + repr(pipeline))
    progress = {k: delta(first, second, k) for k in
                ('ap_hops', 'paired_slots', 'last_capture_end_us')}
    if not all(progress.values()):
        raise ValueError('PDM/AP capture did not advance')
    lanes = []
    expected_channels = (1, 2) if lane_map == 'led-first' else (0, 1)
    for index, (a, b) in enumerate(zip(first['lanes'], second['lanes'])):
        if a['dma_channel'] != expected_channels[index] or b['dma_channel'] != expected_channels[index]:
            raise ValueError('PDM DMA channel identity changed')
        bad = {k: delta(a, b, k) for k in LANE_BAD}
        if integer(a, 'error_flags') or integer(b, 'error_flags') or any(bad.values()):
            raise ValueError('PDM lane guardrail changed: ' + repr(bad))
        if not all(delta(a, b, k) > 0 for k in ('data_callbacks', 'processed_slots')):
            raise ValueError('PDM lane did not advance')
        lanes.append(dict(index=index, guardrails=bad,
                          sat_neg_delta=delta(a, b, 'sat_neg'),
                          sat_pos_delta=delta(a, b, 'sat_pos')))
    # Raw rails are retained, not silently interpreted as acoustic qualification.
    return dict(gpt=advance, pdm_guardrails=pipeline, pdm_progress=progress, lanes=lanes,
                maximum_prepare_cycles=g1['maximum_prepare_cycles'],
                gpt_clock_hz=g1['clock_hz'], waveform='NOT_CAPTURED', optical='NOT_CLAIMED')


def settings_payload(settings):
    for key in ('active', 'automatic_cycle', 'emit_enabled', 'showcase'):
        if type(settings[key]) is not bool:
            raise ValueError('invalid setting: ' + key)
    if settings['direction'] not in ('centre_out', 'edges_in'):
        raise ValueError('invalid direction')
    values = [integer(settings, k) for k in
              ('palette_a', 'palette_b', 'mode_a', 'mode_b', 'brightness',
               'output_channel', 'transition_ms', 'travel_ms')]
    if (any(v > 0xffffffff for v in values) or values[4] > 255
            or values[5] > 1):
        raise ValueError('invalid settings range')
    flags = (int(settings['active']) | int(settings['automatic_cycle']) << 1
             | int(settings['emit_enabled']) << 2
             | int(settings['direction'] == 'edges_in') << 3
             | int(settings['showcase']) << 4)
    return struct.pack('<10I', 3, *values[:4], flags, *values[4:])


def check_settings(actual, expected):
    if any(actual[k] != expected[k] for k in SETTINGS):
        raise ValueError('settings readback mismatch')


def accepted_build(path):
    build = json.loads((path / 'receipt.json').read_text())
    if (build['pass'] is not True or not build['pdm_target'] or not build['palette_gpt_dma']
            or build['source_pin'] != PIN or build['dmac_priority'] not in ('fixed', 'round-robin')
            or build.get('dmac_lane_map') not in ('pdm-first', 'led-first')
            or build['dmac_control_initialisation']['live_policy_change'] != 'REFUSED'):
        raise ValueError('requires accepted source-bound colour/PDM candidate')
    sha = hashlib.sha256((path / 'rtthread.hex').read_bytes()).hexdigest()
    if sha != build['artifacts']['rtthread.hex']:
        raise ValueError('image hash mismatch')
    for name, expected in build['sources'].items():
        if hashlib.sha256(safe_path(ROOT, name).read_bytes()).hexdigest() != expected:
            raise ValueError('source drift: ' + name)
    return build, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--phase-s', type=float, default=20)
    parser.add_argument('--restore-settings', type=Path,
                        help='explicit declared leave-state, otherwise preserve captured settings')
    args = parser.parse_args()
    if not math.isfinite(args.phase_s) or not 10 <= args.phase_s <= 20:
        parser.error('phase-s must be finite and between 10 and 20 seconds')
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = dict(label='ON-SILICON', operation='TITAN_COLOUR_INTEGRITY',
                   start=datetime.now(timezone.utc).isoformat(), phases=[],
                   waveform='NOT_CAPTURED', optical='NOT_CLAIMED', **{'pass': False})
    port = None
    restore = None
    original_error = None

    def save(name, value):
        (args.output / name).write_text(json.dumps(value, indent=2) + '\n')

    try:
        build, sha = accepted_build(args.build)
        receipt.update(build_id=build['build_id'], hex_sha256=sha,
                       dmac_priority=build['dmac_priority'],
                       dmac_lane_map=build.get('dmac_lane_map', 'pdm-first'), phase_s=args.phase_s,
                       runner_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                       decoder_sha256=hashlib.sha256(Path(__file__).with_name('gpt_fault_witness.py').read_bytes()).hexdigest())
        if args.restore_settings:
            restore = json.loads(args.restore_settings.read_text())
            settings_payload(restore)
            receipt['declared_restore'] = str(args.restore_settings.resolve())
        import serial
        from serial.tools import list_ports
        matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x045b, 0x5310)]
        if len(matches) != 1:
            raise ValueError('expected one Titan application CDC')
        device = matches[0].device
        owners = subprocess.run(['lsof', '-t', device, device.replace('/cu.', '/tty.')],
                                capture_output=True, text=True)
        if owners.returncode not in (0, 1) or owners.stdout.strip():
            raise ValueError('CDC owned; no commands sent; explicit handoff required')
        port = serial.Serial(device, 115200, timeout=.5, write_timeout=2, exclusive=True)
        request = 0

        def transact(op, payload=b''):
            nonlocal request
            request += 1
            message = packet(op, request, payload=payload)
            if port.write(message) != len(message):
                raise ValueError('short USB write')
            port.flush()
            head = read_exact(port, 32, 3)
            magic, status, rid, seq, size, cycles, crc, hcrc = struct.unpack('<4s7I', head)
            if magic != b'K1R1' or rid != request or zlib.crc32(head[:28]) != hcrc or size > 19968:
                raise ValueError('invalid response envelope')
            body = read_exact(port, size, 3)
            if zlib.crc32(body) != crc or status:
                raise ValueError(f'opcode {op} rejected: {status}')
            return body

        def identity():
            info = json.loads(transact(1))
            if (info['uid'] != UID or info['build'] != build['build_id']
                    or info['source'] != PIN or info['protocol'] != 1):
                raise ValueError('full UID/build/source/protocol mismatch')
            return info

        # Only mark restore as armed after identity is verified: never configure
        # an unexpected resident image in the exception handler.
        declared = restore
        restore = None
        receipt['identity'] = identity()
        before = json.loads(transact(17))
        restore = declared if declared is not None else before
        settings_payload(restore)
        save('original-settings.json', before)
        receipt['restore_armed'] = True
        priority = int(build['dmac_priority'] == 'round-robin')
        for name, brightness, pause in [('zero-output', 0, 0), ('warm-low-load', 24, .04),
                                         ('warm-usb-stress', 24, 0)]:
            wanted = dict(before, palette_a=4, palette_b=4, mode_a=0, mode_b=0,
                          brightness=brightness, output_channel=0, active=True,
                          automatic_cycle=False, emit_enabled=True, showcase=False,
                          direction='centre_out', transition_ms=0, travel_ms=4000)
            transact(16, settings_payload(wanted))
            configured = json.loads(transact(17))
            check_settings(configured, wanted)
            if configured['bench_pixels'] != 128 or configured['wire_profile'] != 1:
                raise ValueError('unexpected pixel/wire profile')
            def diagnostic(suffix):
                body = transact(22, struct.pack('<I', 2))
                (args.output / (name + '-' + suffix + '-gpt.bin')).write_bytes(body)
                parsed = decode_v2(body)
                save(name + '-' + suffix + '-gpt.json', parsed)
                return parsed
            g0 = diagnostic('start')
            p0 = json.loads(transact(6))
            save(name + '-start-platform.json', p0)
            check_gpt(g0, priority)
            began = time.monotonic()
            samples = 0
            with (args.output / (name + '-native-rgb.bin')).open('wb') as bf, \
                    (args.output / (name + '-rows.jsonl')).open('w') as jf:
                while time.monotonic() - began < args.phase_s:
                    t = time.monotonic() - began
                    pixels = transact(18, struct.pack('<I', 0))
                    if len(pixels) != 480:
                        raise ValueError('incomplete native RGB frame')
                    g = decode_v2(transact(22, struct.pack('<I', 2)))
                    bf.write(struct.pack('<dH', t, len(pixels)) + pixels)
                    jf.write(json.dumps(dict(t=t, native_rgb_sha256=hashlib.sha256(pixels).hexdigest(),
                                             blue_max=max(pixels[2::3]), maximum=max(pixels),
                                             gpt=g)) + '\n')
                    jf.flush()
                    samples += 1
                    if g['first_fault'] or g['errors']:
                        diagnostic('first-fault')
                        save(name + '-fault-platform.json', json.loads(transact(6)))
                    check_gpt(g, priority)
                    if pause:
                        time.sleep(pause)
            g1 = diagnostic('end')
            p1 = json.loads(transact(6))
            save(name + '-end-platform.json', p1)
            result = score_phase(g0, g1, p0, p1, priority,
                                 build.get('dmac_lane_map', 'pdm-first'))
            result.update(phase=name, duration_s=time.monotonic() - began, samples=samples)
            receipt['phases'].append(result)
            save('receipt.json', receipt)
            print(json.dumps(result), flush=True)
        receipt['identity_end'] = identity()
        accepted_build(args.build)
        receipt['pass'] = True
    except Exception as error:
        original_error = error
        receipt['error'] = str(error)
    finally:
        if port is not None:
            try:
                if restore is not None and receipt.get('restore_armed'):
                    receipt['restore_identity'] = identity()
                    transact(16, settings_payload(restore))
                    restored = json.loads(transact(17))
                    save('restored-settings.json', restored)
                    check_settings(restored, restore)
                    receipt['restore_verified'] = True
            except Exception as error:
                receipt['pass'] = False
                receipt['restore_error'] = str(error)
                original_error = original_error or error
            finally:
                port.close()
                receipt['cdc_released'] = True
        receipt['end'] = datetime.now(timezone.utc).isoformat()
        save('receipt.json', receipt)
    if original_error:
        raise original_error


if __name__ == '__main__':
    main()
