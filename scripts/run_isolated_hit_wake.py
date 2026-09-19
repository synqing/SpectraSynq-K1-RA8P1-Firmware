#!/usr/bin/env python3
"""Quiet → distinct hit → gap → silence. Score isolated-hit wake. No flash."""
from __future__ import annotations
import argparse, hashlib, json, math, struct, subprocess, tempfile, time, wave
from datetime import datetime, timezone
from pathlib import Path
from serial.tools import list_ports
import serial
from emit_continuity import emit_continuity_ok
from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

BUILD = '55320f891d37211f8401d61d62d85427bcb4a7c9b277638d374af1ba1d738986'


def main() -> None:
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--output', type=Path, required=True)
    a.add_argument('--build-id', default=BUILD)
    a.add_argument('--quiet-s', type=float, default=3.0)
    a.add_argument('--hit-at-s', type=float, default=4.0)
    a.add_argument('--end-s', type=float, default=14.0)
    a.add_argument('--period-s', type=float, default=0.05)
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
    overhead = []
    clicked = False
    t0 = time.monotonic()
    try:
        ident = json.loads(transact(1))
        if ident.get('uid') != UID or ident.get('source') != PIN:
            raise RuntimeError('wrong board')
        if ident.get('build') != args.build_id:
            raise RuntimeError('wrong image: ' + str(ident.get('build')))
        cfg = transact(16, struct.pack('<8I', 1, 0, 1, 32, 32, 5, 255, 0))
        status0 = json.loads(transact(17))
        first6 = json.loads(transact(6))
        while True:
            now = time.monotonic() - t0
            if now >= args.end_s:
                break
            mark = time.monotonic()
            frame = transact(18, struct.pack('<I', 0))
            status = json.loads(transact(17))
            pdm = json.loads(transact(6)).get('pdm_target') or {}
            hop = pdm.get('last_hop_peak') or 0
            if not clicked and now >= args.quiet_s and hop >= 2000:
                clicked = True
            overhead.append(time.monotonic() - mark)
            phase = 'QUIET' if not clicked else (
                'HIT' if now < args.quiet_s + 0.4 else (
                    'GAP' if now < args.quiet_s + 2.5 else 'SILENCE'))
            row = {
                't': round(now, 3),
                'phase': phase,
                'clicked': clicked,
                'led_sum': sum(frame),
                'led_max': max(frame) if frame else 0,
                'led_nz': sum(1 for b in frame if b),
                'led_ones': sum(1 for b in frame if b == 1),
                'led_hash': hashlib.sha256(frame).hexdigest()[:16],
                'frame_a_crc': status.get('frame_a_crc'),
                'mode_a': status.get('mode_a'),
                'emitted': status.get('emitted'),
                'emit_errors': status.get('emit_errors'),
                'waiting_for_audio': status.get('waiting_for_audio'),
                'last_hop_peak': pdm.get('last_hop_peak'),
                'last_hop_gain_q8': pdm.get('last_hop_gain_q8'),
                'last_hop_dt_us': pdm.get('last_hop_dt_us'),
                'ap_hops': pdm.get('ap_hops'),
                'measured_hz': pdm.get('measured_hz'),
                'rate_locked': pdm.get('rate_locked'),
                'asrc_starved': pdm.get('asrc_starved'),
            }
            lanes = pdm.get('lanes') or [{}, {}]
            row['lane_peak'] = [ln.get('sample_peak') for ln in lanes[:2]]
            samples.append(row)
            print(json.dumps(row), flush=True)
            remain = args.period_s - (time.monotonic() - mark)
            if remain > 0:
                time.sleep(remain)
        end_status = json.loads(transact(17))
        end_pdm = json.loads(transact(6))
    finally:
        port.close()

    quiet = [s for s in samples if s['t'] < args.hit_at_s]
    after = [s for s in samples if s['t'] >= args.hit_at_s]
    gap = [s for s in samples if args.hit_at_s + 0.4 <= s['t'] < args.hit_at_s + 2.5]
    silence = [s for s in samples if s['t'] >= args.hit_at_s + 6.0]
    def stats(rows, key):
        vals = [s[key] for s in rows if s.get(key) is not None]
        if not vals:
            return None
        vals = sorted(vals)
        return {'n': len(vals), 'min': vals[0], 'med': vals[len(vals)//2],
                'max': vals[-1]}
    baseline_peak = stats(quiet, 'last_hop_peak') or {'med': 0, 'max': 0}
    baseline_led = stats(quiet, 'led_max') or {'med': 0, 'max': 0}
    hit_window = [s for s in after if s['t'] < args.hit_at_s + 1.5]
    hit_peak = stats(hit_window, 'last_hop_peak') or {'max': 0}
    hit_led = stats(hit_window, 'led_max') or {'max': 0}
    pcm_floor = max(3 * (baseline_peak.get('med') or 0),
                    (baseline_peak.get('max') or 0) + 500)
    led_floor = max((baseline_led.get('max') or 0) + 8, 16)
    event_pcm = (hit_peak.get('max') or 0) >= pcm_floor
    event_led = (hit_led.get('max') or 0) >= led_floor
    distinguishable = event_pcm and event_led
    black = [s for s in samples if s['led_nz'] == 0]
    run = best = 0
    for s in samples:
        if s['led_nz'] == 0:
            run += 1
            best = max(best, run)
        else:
            run = 0
    gap_hold = bool(gap) and min(s['led_nz'] for s in gap) > 0
    late_black = bool(silence) and all(s['led_max'] <= 1 for s in silence[-6:])
    stuck = False
    if gap:
        ones = [s['led_ones'] for s in gap]
        nz = [s['led_nz'] for s in gap]
        stuck = (max(s['led_max'] for s in gap) <= 1 and min(nz) > 0
                 and ones[-1] >= ones[0])
    emit_ok, emit_stall_s, emit_stall_at, emit_stall_val = emit_continuity_ok(
        samples, max_stall_s=2.0)
    plateau = 0.0
    run_t0 = None
    last_hash = None
    for s in samples:
        if s.get('led_hash') == last_hash and s.get('led_max', 0) <= 1:
            if run_t0 is None:
                run_t0 = s['t']
            plateau = max(plateau, s['t'] - run_t0)
        else:
            last_hash = s.get('led_hash')
            run_t0 = s['t']
    verdict = 'INCONCLUSIVE'
    if not distinguishable:
        verdict = 'INCONCLUSIVE'
        reason = 'hit did not exceed predeclared PCM/LED margin over quiet baseline'
    elif not emit_ok:
        verdict = 'FAIL'
        reason = f'emit stalled {emit_stall_s:.3f}s at t={emit_stall_at}'
    elif not gap_hold:
        verdict = 'FAIL'
        reason = 'history did not remain through the 0.4–2.5 s gap'
    elif stuck:
        verdict = 'FAIL'
        reason = 'one-count trail did not decay'
    else:
        verdict = 'PASS'
        reason = 'distinct hit, gap hold, emit continuous, not frozen'
    receipt = {
        'result': verdict,
        'reason': reason,
        'label': 'ON-SILICON',
        'build_id': args.build_id,
        'uid': UID,
        'source_pin': PIN,
        'identity': ident,
        'configure': json.loads(cfg) if cfg[:1] == b'{' else {'raw': cfg.hex()},
        'boot_status': status0,
        'first_pdm': first6.get('pdm_target'),
        'stimulus': 'host_180ms_880plus2640_once',
        'quiet_s': args.quiet_s,
        'hit_at_s': args.hit_at_s,
        'end_s': args.end_s,
        'period_s': args.period_s,
        'n': len(samples),
        'black_rows': len(black),
        'longest_black_run': best,
        'baseline_hop_peak': baseline_peak,
        'baseline_led_max': baseline_led,
        'hit_hop_peak': hit_peak,
        'hit_led_max': hit_led,
        'pcm_margin_required': pcm_floor,
        'led_margin_required': led_floor,
        'event_pcm': event_pcm,
        'event_led': event_led,
        'distinguishable': distinguishable,
        'gap_hold': gap_hold,
        'late_near_black': late_black,
        'lsb_stuck': stuck,
        'one_count_plateau_s': plateau,
        'emit_ok': emit_ok,
        'emit_stall_s': emit_stall_s,
        'emit_stall_at_s': emit_stall_at,
        'late_near_black': late_black,
        'mean_sample_overhead_s': sum(overhead) / len(overhead) if overhead else None,
        'emitted_delta': (samples[-1]['emitted'] - samples[0]['emitted'])
        if samples else 0,
        'end_status': end_status if 'end_status' in dir() else None,
        'end_pdm': end_pdm.get('pdm_target') if isinstance(end_pdm, dict) else None,
        'mode_a': {s['mode_a'] for s in samples} and list({s['mode_a'] for s in samples}),
        'samples': samples,
        'start': datetime.now(timezone.utc).isoformat(),
    }
    args.output.write_text(json.dumps(receipt, indent=2, default=str) + '\n')
    print('ISOLATED_HIT', json.dumps({
        k: receipt[k] for k in (
            'result', 'reason', 'distinguishable', 'event_pcm', 'event_led',
            'gap_hold', 'lsb_stuck', 'longest_black_run', 'n',
            'pcm_margin_required', 'led_margin_required')
    }))


if __name__ == '__main__':
    main()
