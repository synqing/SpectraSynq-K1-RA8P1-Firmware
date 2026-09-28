#!/usr/bin/env python3
"""Package actual host-renderer pixels + a causal trace + fixed PCM into one HTML.

No detector or scene implementation lives here. scene_variants must be generated
by renderTempoReferencePair for masks 0..7 (accent, clock, field). Each half-strip
contains the first 80 values; the renderer's mirror invariant is tested in C++.
"""
from __future__ import annotations
import argparse
import array
import base64
import gzip
import hashlib
import io
import json
import math
from pathlib import Path
import sys
import wave


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path: Path) -> list[dict]:
    rows = [json.loads(line) for line in path.read_text().splitlines() if line.strip()]
    if not rows:
        raise ValueError(f"empty trace: {path}")
    if any(b['available_us'] < a['available_us'] for a, b in zip(rows, rows[1:])):
        raise ValueError(f"non-monotonic availability: {path}")
    return rows


def package(args: argparse.Namespace) -> dict:
    baseline, candidate = load(args.baseline), load(args.candidate)
    if len(baseline) != len(candidate):
        raise ValueError('baseline/candidate hop count mismatch')
    parity_keys = ('hop', 'media_end_48k', 'available_us', 'features', 'tempo',
                   'valid', 'musical_time_valid', 'predicted_next_beat_valid', 'gdft_overflows')
    pixels = array.array('H')
    compact = []
    max_energy_error = max_quantised_energy_error = max_level = 0.0
    for n, (old, new) in enumerate(zip(baseline, candidate)):
        for key in parity_keys:
            if key not in old or key not in new or old[key] != new[key]:
                raise ValueError(f'incumbent parity failed at hop {n}, field {key}')
        variants = new.get('scene_variants')
        if not isinstance(variants, list) or len(variants) != 8:
            raise ValueError(f'hop {n}: require eight actual C++ scene_variants')
        for mask, variant in enumerate(variants):
            pair, quantised_pair = [], []
            for lane in ('baseline', 'candidate'):
                half = variant[lane]
                if len(half) == 160:
                    if any(abs(a-b) > 1e-5 for a,b in zip(half, reversed(half))):
                        raise ValueError(f'hop {n}/{mask}: non-mirrored renderer output')
                    half = half[:80]
                if len(half) != 80 or any(not math.isfinite(x) or x < 0 or x > 24.0001 for x in half):
                    raise ValueError(f'hop {n}/{mask}: invalid renderer pixels')
                quantised = [round(min(24, x) * 65535 / 24) for x in half]
                pixels.extend(quantised)
                pair.append(2 * sum(half))
                quantised_pair.append(2 * sum(quantised) * 24 / 65535)
                max_level = max(max_level, max(half))
            max_energy_error = max(max_energy_error, abs(pair[0]-pair[1]))
            max_quantised_energy_error = max(max_quantised_energy_error, abs(quantised_pair[0]-quantised_pair[1]))
        f = new['features']
        tf = new.get('tempo_field', {})
        compact.append({
            'available_us': new['available_us'], 'media_end_48k': new['media_end_48k'],
            'onset_event_ms': f['onset_event_ms'], 'onset_event_id': f['onset_event_id'],
            'onset_strength': f['onset_strength'], 'onset_age_ms': f['onset_event_age_ms'],
            'tempo': new['tempo'], 'field': tf,
        })
    if max_energy_error > 0.02:
        raise ValueError(f'renderer energy mismatch: {max_energy_error}')
    pcm = args.pcm.read_bytes()
    if len(pcm) % 2:
        raise ValueError('PCM must be mono signed 16-bit little endian')
    duration = len(pcm) / (2 * args.sample_rate)
    if candidate[-1]['media_end_48k'] > duration * 48000 + 360:
        raise ValueError('trace exceeds supplied audio duration')
    buffer = io.BytesIO()
    with wave.open(buffer, 'wb') as wav:
        wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(args.sample_rate)
        wav.writeframes(pcm)
    if sys.byteorder != 'little':
        pixels.byteswap()
    manifest = {
        'schema': 'k1.tempo.host-audition.v1', 'label': args.label,
        'held_out': args.held_out, 'sample_rate': args.sample_rate,
        'duration_seconds': duration, 'hops': len(candidate),
        'pcm_sha256': sha(args.pcm), 'baseline_sha256': sha(args.baseline),
        'candidate_sha256': sha(args.candidate), 'builder_sha256': sha(Path(__file__)),
        'renderer_maximum_level': max_level,
        'maximum_renderer_total_difference': max_energy_error,
        'maximum_packaged_total_difference': max_quantised_energy_error,
        'pixel_encoding': 'gzip of little-endian uint16: hop, mask 0..7, baseline/candidate, first80; full_scale=24',
        'presentation': 'audio.currentTime, last trace row available_us <= playback time; no future shift',
        'clock_mapping': 'host replay origin zero, 48000 media frames per second; not target timing',
        'availability_disclosure': 'Simulated host replay availability at hop release; no measured computation latency. Causal ordering is tested; physical audio-to-light latency is not measured.',
        'template_sha256': sha(Path(__file__).with_name('tempo_audition_template.html')),
        'scope': 'Host expressive audition. No optical, target timing, beat annotation or preference acceptance claim.',
    }
    payload = {
        'manifest': manifest, 'rows': compact,
        'pixels': base64.b64encode(gzip.compress(pixels.tobytes(), mtime=0)).decode(),
        'wav': base64.b64encode(buffer.getvalue()).decode(),
    }
    template = Path(__file__).with_name('tempo_audition_template.html').read_text()
    html = template.replace('__AUDITION_DATA__', json.dumps(payload, separators=(',', ':'), allow_nan=False).replace('<', '\\u003c'))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(html)
    manifest['html_sha256'] = sha(args.output)
    args.output.with_suffix('.manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', required=True, type=Path)
    parser.add_argument('--candidate', required=True, type=Path)
    parser.add_argument('--pcm', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--sample-rate', type=int, default=24000, choices=(24000,))
    parser.add_argument('--label', default='Fixed music excerpt')
    parser.add_argument('--held-out', action='store_true')
    print(json.dumps(package(parser.parse_args()), indent=2))


if __name__ == '__main__':
    main()
