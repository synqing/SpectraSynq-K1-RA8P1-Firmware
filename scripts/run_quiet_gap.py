#!/usr/bin/env python3
"""Quiet-gate → one hit → hop-floor gap. Record room, Pixel8, emit. No flash."""
from __future__ import annotations
import argparse, hashlib, json, math, struct, subprocess, time, wave, zlib
from datetime import datetime, timezone
from pathlib import Path
from serial.tools import list_ports
import serial
from emit_continuity import emit_continuity_ok
from quiet_gap_score import (
    INCONCLUSIVE_ROOM, QUIET_HOLD_S, QUIET_HOP_ABS, SILENCE_HOLD_S,
    classify, first_hold, hop_floor, last_hold_tail)
from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

BUILD = 'd13cd520dafbc3e7d22955a5a4b71cbe8a61f8009f02414873fc756d2c83c8d3'
RATE = 48000


def start_room(path: Path, seconds: float):
    cmd = [
        'ffmpeg', '-y', '-hide_banner', '-loglevel', 'error',
        '-f', 'avfoundation', '-i', ':1',
        '-ac', '1', '-ar', str(RATE), '-t', f'{seconds:.1f}', str(path),
    ]
    try:
        proc = subprocess.Popen(
            cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except OSError:
        return None, None
    return proc, time.monotonic()


def wav_rms(path: Path, start_s: float, end_s: float):
    if not path.exists() or path.stat().st_size < 44:
        return None
    with wave.open(str(path), 'r') as w:
        rate = w.getframerate()
        nch = w.getnchannels()
        sw = w.getsampwidth()
        start = max(0, int(start_s * rate))
        end = min(w.getnframes(), int(end_s * rate))
        if end <= start:
            return None
        w.setpos(start)
        raw = w.readframes(end - start)
    if sw != 2:
        return None
    n = len(raw) // 2
    if n == 0:
        return None
    acc = 0.0
    peak = 0
    for i in range(0, len(raw), 2):
        v = struct.unpack_from('<h', raw, i)[0]
        acc += v * v
        peak = max(peak, abs(v))
    rms = math.sqrt(acc / (n / nch))
    return {'n': n // nch, 'rms': round(rms, 1), 'peak': peak}


def frame_stats(frame: bytes):
    if len(frame) != 160 * 3:
        raise ValueError('expected one complete 160-pixel native RGB frame')
    nz = ones = mx = total = 0
    for b in frame:
        total += b
        if b:
            nz += 1
        if b == 1:
            ones += 1
        if b > mx:
            mx = b
    return total, mx, nz, ones, hashlib.sha256(frame).hexdigest()[:16]


def main() -> None:
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--output', type=Path, required=True)
    a.add_argument('--build-id', default=BUILD)
    a.add_argument('--arm-s', type=float, default=12.0)
    a.add_argument('--after-s', type=float, default=12.0)
    a.add_argument('--period-s', type=float, default=0.12)
    a.add_argument('--quiet-hop-abs', type=int, default=QUIET_HOP_ABS)
    args = a.parse_args()
    out = args.output
    out.mkdir(parents=True, exist_ok=False)
    room = out / 'room.wav'
    frames_path = out / 'frames.bin'
    matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x045b, 0x5310)]
    if len(matches) != 1:
        raise SystemExit('need one Titan CDC')
    device = matches[0].device
    owners = subprocess.run(
        ['lsof', '-t', device, device.replace('/cu.', '/tty.')],
        capture_output=True, text=True)
    if owners.returncode not in (0, 1) or owners.stdout.strip():
        raise SystemExit('CDC owned; no commands sent')
    room_proc, room_t0 = start_room(room, args.arm_s + args.after_s + 6.0)
    port = serial.Serial(device, 115200, timeout=.5, write_timeout=2,
                         exclusive=True)
    req = 0
    samples = []
    overhead = []
    clicked = False
    hit_at_s = None
    quiet_max = None
    floor = None
    t0 = time.monotonic()

    def transact(op, payload=b''):
        nonlocal req
        req += 1
        outb = packet(op, req, payload=payload)
        if port.write(outb) != len(outb):
            raise RuntimeError('short write')
        port.flush()
        head = read_exact(port, 32, 15)
        magic, status, rid, seq, size, cycles, crc, hcrc = struct.unpack(
            '<4s7I', head)
        if magic != b'K1R1' or rid != req or zlib.crc32(head[:28]) != hcrc:
            raise RuntimeError('invalid envelope')
        body = read_exact(port, size, 15)
        if zlib.crc32(body) != crc or status:
            raise RuntimeError(f'op {op} status {status}')
        return body

    try:
        discarded = 0
        quiet = 0
        deadline = time.monotonic() + 5
        while quiet < 2 and time.monotonic() < deadline:
            stale = port.read(1024)
            discarded += len(stale)
            quiet = 0 if stale else quiet + 1
        ident = json.loads(transact(1))
        if ident.get('uid') != UID or ident.get('source') != PIN:
            raise RuntimeError('wrong board')
        if ident.get('build') != args.build_id:
            raise RuntimeError('wrong image: ' + str(ident.get('build')))
        cfg = transact(16, struct.pack('<8I', 1, 0, 1, 32, 32, 5, 255, 0))
        status0 = json.loads(transact(17))
        first6 = json.loads(transact(6))
        t0 = time.monotonic()
        with frames_path.open('wb') as frame_out:
            while True:
                now = time.monotonic() - t0
                if hit_at_s is None and now >= args.arm_s:
                    break
                if hit_at_s is not None and now >= hit_at_s + args.after_s:
                    break
                mark = time.monotonic()
                frame = transact(18, struct.pack('<I', 0))
                status = json.loads(transact(17))
                pdm = json.loads(transact(6)).get('pdm_target') or {}
                overhead.append(time.monotonic() - mark)
                led_sum, led_max, led_nz, led_ones, led_hash = frame_stats(frame)
                hop = pdm.get('last_hop_peak')
                row = {
                    't': round(now, 3),
                    'clicked': clicked,
                    'led_sum': led_sum,
                    'led_max': led_max,
                    'led_nz': led_nz,
                    'led_ones': led_ones,
                    'led_hash': led_hash,
                    'frame_bytes': len(frame),
                    'frame_a_crc': status.get('frame_a_crc'),
                    'mode_a': status.get('mode_a'),
                    'emitted': status.get('emitted'),
                    'emit_errors': status.get('emit_errors'),
                    'waiting_for_audio': status.get('waiting_for_audio'),
                    'last_hop_peak': hop,
                    'last_hop_gain_q8': pdm.get('last_hop_gain_q8'),
                    'ap_hops': pdm.get('ap_hops'),
                    'measured_hz': pdm.get('measured_hz'),
                    'rate_locked': pdm.get('rate_locked'),
                    'asrc_starved': pdm.get('asrc_starved'),
                    'gain_clip_pos': pdm.get('gain_clip_pos'),
                    'gain_clip_neg': pdm.get('gain_clip_neg'),
                    'visual_path': status.get('visual_path'),
                    'musical': status.get('musical'),
                    'in_dwell': status.get('in_dwell'),
                    'live_age_us': status.get('live_age_us'),
                    'peak_milli': status.get('peak_milli'),
                    'vu_milli': status.get('vu_milli'),
                    'chroma_milli': status.get('chroma_milli'),
                    'wave_milli': status.get('wave_milli'),
                    'dwell_armed': status.get('dwell_armed'),
                    'dwell_reinit': status.get('dwell_reinit'),
                    'effect_frames': status.get('effect_frames'),
                    'dwell_frames': status.get('dwell_frames'),
                    'dwell_reinits': status.get('dwell_reinits'),
                }
                lanes = pdm.get('lanes') or [{}, {}]
                row['lane_peak'] = [ln.get('sample_peak') for ln in lanes[:2]]
                if hit_at_s is None:
                    row['phase'] = 'ARM'
                elif now < hit_at_s + 0.4:
                    row['phase'] = 'HIT'
                else:
                    row['phase'] = 'AFTER'
                samples.append(row)
                frame_out.write(struct.pack('<fH', now, len(frame)))
                frame_out.write(frame)
                print(json.dumps(row), flush=True)
                if hit_at_s is None:
                    if floor is None:
                        hold = first_hold(
                            samples,
                            lambda s: ((s.get('last_hop_peak') or 0) <= args.quiet_hop_abs
                                       and (s.get('led_nz') or 0) == 0),
                            QUIET_HOLD_S)
                        if hold:
                            q0, q1 = hold
                            quiet_rows = [s for s in samples if q0 <= s['t'] <= q1]
                            quiet_max = max(
                                (s.get('last_hop_peak') or 0) for s in quiet_rows)
                            floor = hop_floor(quiet_max)
                    else:
                        hop = row.get('last_hop_peak') or 0
                        pcm_need = max(3 * (quiet_max or 0), (quiet_max or 0) + 500)
                        if row.get('musical') or hop >= pcm_need:
                            hit_at_s = samples[-1]['t']
                            clicked = True
                            samples[-1]['clicked'] = True
                            samples[-1]['phase'] = 'HIT'
                remain = args.period_s - (time.monotonic() - mark)
                if remain > 0:
                    time.sleep(remain)
        end_status = json.loads(transact(17))
        end_pdm = json.loads(transact(6))
    finally:
        port.close()
        if room_proc is not None and room_proc.poll() is None:
            try:
                room_proc.wait(timeout=8)
            except subprocess.TimeoutExpired:
                room_proc.kill()

    if hit_at_s is None:
        scored = {
            'cause': INCONCLUSIVE_ROOM,
            'reason': (
                f'live gate never saw {QUIET_HOLD_S:.1f}s hop<='
                f'{args.quiet_hop_abs} before arm timeout'),
            'emit_ok': True,
            'quiet_hold': None,
            'clicked': False,
        }
        emit_ok, stall_s, stall_at, stall_val = emit_continuity_ok(samples)
        scored.update(emit_ok=emit_ok, emit_stall_s=stall_s,
                      emit_stall_at_s=stall_at, emit_stall_val=stall_val)
        if not emit_ok:
            scored['cause'] = 'FAIL_EMIT'
            scored['reason'] = f'emit stalled {stall_s:.3f}s at t={stall_at}'
        hops = [s.get('last_hop_peak') or 0 for s in samples]
        scored['arm_hop_peak'] = {
            'n': len(hops), 'min': min(hops) if hops else None,
            'max': max(hops) if hops else None,
        }
    else:
        scored = classify(
            samples, hit_at_s, quiet_hop_abs=args.quiet_hop_abs)
    room_offset = 0.0 if room_t0 is None else max(0.0, t0 - room_t0)
    room_windows = {}
    if room.exists():
        if scored.get('quiet_hold'):
            q0, q1 = scored['quiet_hold']
            room_windows['quiet'] = wav_rms(
                room, room_offset + q0, room_offset + q1)
        if hit_at_s is not None:
            room_windows['hit'] = wav_rms(
                room, room_offset + hit_at_s, room_offset + hit_at_s + 1.5)
            if scored.get('silence_hold'):
                s0, s1 = scored['silence_hold']
                room_windows['silence'] = wav_rms(
                    room, room_offset + s0, room_offset + s1)
            else:
                room_windows['after'] = wav_rms(
                    room, room_offset + hit_at_s + 0.4,
                    room_offset + (samples[-1]['t'] if samples else hit_at_s + 1))
        elif samples:
            room_windows['arm'] = wav_rms(
                room, room_offset, room_offset + samples[-1]['t'])
    receipt = {
        'result': scored.get('cause'),
        'reason': scored.get('reason'),
        'label': 'ON-SILICON',
        'build_id': args.build_id,
        'uid': UID,
        'source_pin': PIN,
        'identity': ident,
        'configure': json.loads(cfg) if cfg[:1] == b'{' else {'raw': cfg.hex()},
        'boot_status': status0,
        'first_pdm': first6.get('pdm_target'),
        'stimulus': 'live_acoustic_only',
        'hit_at_s': hit_at_s,
        'quiet_max_observed': quiet_max,
        'hop_floor_live': floor,
        'arm_s': args.arm_s,
        'after_s': args.after_s,
        'period_s': args.period_s,
        'stale_discarded': discarded,
        'n': len(samples),
        'emitted_delta': (samples[-1]['emitted'] - samples[0]['emitted'])
        if samples else 0,
        'mean_sample_overhead_s': (
            sum(overhead) / len(overhead) if overhead else None),
        'room_wav': str(room) if room.exists() else None,
        'room_windows': room_windows,
        'frames_bin': str(frames_path),
        'end_status': end_status,
        'end_pdm': end_pdm.get('pdm_target') if isinstance(end_pdm, dict) else None,
        'score': scored,
        'samples': samples,
        'start': datetime.now(timezone.utc).isoformat(),
    }
    (out / 'quiet-gap.json').write_text(
        json.dumps(receipt, indent=2, default=str) + '\n')
    print('QUIET_GAP', json.dumps({
        k: receipt[k] for k in (
            'result', 'reason', 'hit_at_s', 'n', 'emitted_delta')
    }))


if __name__ == '__main__':
    main()
