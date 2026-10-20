#!/usr/bin/env python3
"""Mode 32 capture while a real recording plays. Observation, not acceptance."""
from __future__ import annotations
import argparse, hashlib, json, struct, subprocess, time, zlib
from datetime import datetime, timezone
from pathlib import Path
import serial
from serial.tools import list_ports
from run_colour_integrity import SETTINGS, check_settings, settings_payload
from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

NATIVE_RGB_BYTES = 480
REQUIRED_PDM = ('ap_hops', 'paired_slots', 'measured_hz', 'rate_locked', 'last_hop_peak')


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


def authored_stats(frame: bytes):
    if len(frame) != NATIVE_RGB_BYTES:
        raise ValueError(f'incomplete native RGB frame: {len(frame)} bytes')
    nz = 0
    mx = 0
    acc = 0.0
    lo = None
    hi = None
    for i in range(80, 160):
        r, g, b = frame[i * 3:i * 3 + 3]
        v = max(r, g, b)
        if v:
            nz += 1
            acc += i
            lo = i if lo is None else min(lo, i)
            hi = i if hi is None else max(hi, i)
            if v > mx:
                mx = v
    return {
        'nz': nz,
        'max': mx,
        'centroid': (acc / nz if nz else None),
        'span': (0 if lo is None else hi - lo),
    }


def rail_pair(pdm: dict):
    lanes = pdm.get('lanes') or [{}, {}]
    return {
        'sat_pos': [ln.get('sat_pos') for ln in lanes[:2]],
        'sat_neg': [ln.get('sat_neg') for ln in lanes[:2]],
        'gain_clip_pos': pdm.get('gain_clip_pos'),
        'gain_clip_neg': pdm.get('gain_clip_neg'),
    }


def require_counters(pdm: dict):
    missing = [key for key in REQUIRED_PDM if pdm.get(key) is None]
    if missing:
        raise ValueError('missing PDM counters: ' + ','.join(missing))


def write_failure(out: Path, payload: dict) -> None:
    payload = dict(payload)
    payload['label'] = 'OBSERVATION_FAIL'
    payload['accepted'] = False
    (out / 'real-audio.json').write_text(json.dumps(payload, indent=2) + '\n')


def main() -> None:
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--output', type=Path, required=True)
    a.add_argument('--build-id', required=True)
    a.add_argument('--audio', type=Path, required=True,
                   help='Existing real recording. Not a generated tone.')
    a.add_argument('--play-s', type=float, default=20.0)
    a.add_argument('--after-s', type=float, default=8.0)
    a.add_argument('--volume', type=float, default=0.5)
    a.add_argument('--period-s', type=float, default=0.15)
    args = a.parse_args()
    audio = args.audio.resolve()
    if not audio.is_file():
        raise SystemExit('audio file missing')
    out = args.output
    out.mkdir(parents=True, exist_ok=False)
    matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x045b, 0x5310)]
    if len(matches) != 1:
        raise SystemExit('need one Titan CDC')
    device = matches[0].device
    owners = subprocess.run(
        ['lsof', '-t', device, device.replace('/cu.', '/tty.')],
        capture_output=True, text=True)
    if owners.returncode not in (0, 1) or owners.stdout.strip():
        raise SystemExit('CDC owned')
    port = serial.Serial(device, 115200, timeout=.5, write_timeout=2,
                         exclusive=True, dsrdtr=False, rtscts=False)
    try:
        port.dtr = False
        port.rts = False
    except OSError:
        pass
    req = 0
    samples = []
    player = None
    restore = None
    restore_armed = False
    ident = None
    receipt = {
        'label': 'OBSERVATION',
        'accepted': False,
        'build_id': args.build_id,
        'audio': str(audio),
        'audio_name': audio.name,
        'audio_sha256': file_sha256(audio),
        'play_s': args.play_s,
        'start': datetime.now(timezone.utc).isoformat(),
    }

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
        quiet = 0
        deadline = time.monotonic() + 5
        while quiet < 2 and time.monotonic() < deadline:
            stale = port.read(1024)
            quiet = 0 if stale else quiet + 1
        ident = json.loads(transact(1))
        if (ident.get('uid') != UID or ident.get('build') != args.build_id
                or ident.get('source') != PIN or ident.get('protocol') != 1):
            raise RuntimeError('wrong image')
        receipt['identity'] = ident
        before = json.loads(transact(17))
        restore = dict(before)
        settings_payload(restore)
        (out / 'original-settings.json').write_text(json.dumps(before, indent=2) + '\n')
        restore_armed = True
        wanted = dict(before, mode_a=32, mode_b=32, active=True,
                      automatic_cycle=False, emit_enabled=True, showcase=False)
        transact(16, settings_payload(wanted))
        configured = json.loads(transact(17))
        check_settings(configured, {k: wanted[k] for k in SETTINGS})
        receipt['configured_settings'] = {k: configured.get(k) for k in SETTINGS}
        first = json.loads(transact(6)).get('pdm_target') or {}
        require_counters(first)
        t0 = time.monotonic()
        player = subprocess.Popen(['afplay', '-v', str(args.volume), str(audio)])
        frames_path = out / 'frames.bin'
        end_s = args.play_s + args.after_s
        with frames_path.open('wb') as frame_out:
            while True:
                now = time.monotonic() - t0
                if now >= end_s:
                    break
                if player is not None and now >= args.play_s:
                    player.terminate()
                    player = None
                mark = time.monotonic()
                frame = transact(18, struct.pack('<I', 0))
                geom = authored_stats(frame)
                status = json.loads(transact(17))
                pdm = json.loads(transact(6)).get('pdm_target') or {}
                require_counters(pdm)
                rails = rail_pair(pdm)
                row = {
                    't': round(now, 3),
                    'phase': 'MUSIC' if now < args.play_s else 'AFTER',
                    'led_max': geom['max'],
                    'led_nz': geom['nz'],
                    'centroid': geom['centroid'],
                    'span': geom['span'],
                    'emitted': status.get('emitted'),
                    'emit_errors': status.get('emit_errors'),
                    'visual_path': status.get('visual_path'),
                    'musical': status.get('musical'),
                    'effect_frames': status.get('effect_frames'),
                    'last_hop_peak': pdm.get('last_hop_peak'),
                    'sat_pos': rails['sat_pos'],
                    'sat_neg': rails['sat_neg'],
                    'gain_clip_pos': rails['gain_clip_pos'],
                    'gain_clip_neg': rails['gain_clip_neg'],
                    'ap_hops': pdm.get('ap_hops'),
                    'measured_hz': pdm.get('measured_hz'),
                    'native_rgb_bytes': len(frame),
                    'native_rgb_sha256': hashlib.sha256(frame).hexdigest(),
                }
                samples.append(row)
                frame_out.write(struct.pack('<fH', now, len(frame)))
                frame_out.write(frame)
                print(json.dumps(row), flush=True)
                remain = args.period_s - (time.monotonic() - mark)
                if remain > 0:
                    time.sleep(remain)
        end = json.loads(transact(6)).get('pdm_target') or {}
        require_counters(end)
        music = [s for s in samples if s['phase'] == 'MUSIC']
        after = [s for s in samples if s['phase'] == 'AFTER']
        cents = [s['centroid'] for s in music if s.get('centroid') is not None]
        start_rails = rail_pair(first)
        end_rails = rail_pair(end)
        receipt.update({
            'identity': ident,
            'music_musical': sum(1 for s in music if s.get('musical')),
            'music_effect': sum(1 for s in music if s.get('visual_path') == 'effect'),
            'last_hop_peak_max': max((s.get('last_hop_peak') or 0) for s in music) if music else 0,
            'led_max': max((s.get('led_max') or 0) for s in music) if music else 0,
            'centroid_span': ((max(cents) - min(cents)) if cents else 0),
            'after_led_max': max((s.get('led_max') or 0) for s in after) if after else 0,
            'after_last_nz': after[-1]['led_nz'] if after else None,
            'sat_pos_delta': [
                (end_rails['sat_pos'][i] or 0) - (start_rails['sat_pos'][i] or 0)
                for i in (0, 1)],
            'sat_neg_delta': [
                (end_rails['sat_neg'][i] or 0) - (start_rails['sat_neg'][i] or 0)
                for i in (0, 1)],
            'emit_errors': samples[-1].get('emit_errors') if samples else None,
            'samples': samples,
            'note': 'Observation only. Not a live-audio acceptance gate.',
        })
        (out / 'real-audio.json').write_text(json.dumps(receipt, indent=2) + '\n')
        print('REAL_AUDIO', json.dumps({
            k: receipt[k] for k in (
                'audio_name', 'music_musical', 'music_effect', 'last_hop_peak_max',
                'led_max', 'centroid_span', 'after_last_nz', 'sat_pos_delta',
                'sat_neg_delta', 'emit_errors', 'accepted')
        }))
    except Exception as error:
        receipt['error'] = str(error)
        write_failure(out, receipt)
        raise
    finally:
        if player is not None:
            player.terminate()
        try:
            if restore_armed and restore is not None:
                transact(16, settings_payload(restore))
                restored = json.loads(transact(17))
                check_settings(restored, {k: restore[k] for k in SETTINGS})
                (out / 'restored-settings.json').write_text(
                    json.dumps(restored, indent=2) + '\n')
                receipt['restore_verified'] = True
                if (out / 'real-audio.json').exists() and receipt.get('label') == 'OBSERVATION':
                    (out / 'real-audio.json').write_text(json.dumps(receipt, indent=2) + '\n')
        except Exception as restore_error:
            receipt['restore_error'] = str(restore_error)
            write_failure(out, receipt)
        port.close()


if __name__ == '__main__':
    main()
