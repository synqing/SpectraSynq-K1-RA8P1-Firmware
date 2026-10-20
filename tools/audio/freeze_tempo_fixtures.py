#!/usr/bin/env python3
"""Freeze causal PCM fixtures before scoring; never opens a device."""
import argparse
import array
import hashlib
import json
import math
from pathlib import Path
import random
import shutil
import sys
import wave

RATE = 24000
ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path('/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/host-product-01')


def build(output):
    output.mkdir(parents=True, exist_ok=True)
    if (output / 'manifest.json').exists():
        raise SystemExit('Refusing to replace frozen fixture manifest')
    rows = []
    recipes = ['clear120', 'syncopation', 'half_time', 'sparse_reentry', 'dense', 'tempo_step', 'silence', 'fractional12743']
    for name in recipes:
        seconds = 36
        samples = [0.0] * (RATE * seconds)
        events = []
        if name == 'tempo_step':
            times = [(2 + n * 60 / 90, 0.7) for n in range(24)] + [(18 + n * 60 / 135, 0.7) for n in range(36)]
        elif name == 'fractional12743':
            times = [(2 + n * 6000 / 12743, 0.7) for n in range(68)]
        elif name == 'half_time':
            times = [(2 + n / 3, 0.7 if n % 2 == 0 else 0.35) for n in range(96)]
        elif name == 'syncopation':
            times = [(2 + bar * 2 + offset, 0.7) for bar in range(16) for offset in (0.0, 0.75, 1.25, 1.5)]
        elif name == 'silence':
            times = []
        else:
            times = [(2 + n / 2, 0.7) for n in range(64)]
            if name == 'sparse_reentry':
                times = [(t, a) for t, a in times if t < 12 or t >= 22]
        for t, amplitude in times:
            start = int(math.floor(t * RATE + 0.5))
            events.append({'sample': start, 'media_frame_48k': start * 2, 'amplitude': amplitude})
            for j in range(720):
                if start+j >= len(samples):
                    break
                # Explicit broadband attack plus decaying 90 Hz body.
                value = amplitude * math.exp(-j / 150) * (0.5 * math.cos(2*math.pi*90*j/RATE) + 0.5 * (1 if j % 2 else -1))
                samples[start+j] += value
        if name == 'dense':
            rng = random.Random(20260924)
            for i in range(2*RATE, 34*RATE):
                samples[i] += 0.07*math.sin(2*math.pi*220*i/RATE) + 0.03*rng.uniform(-1, 1)
        pcm = array.array('h', [int(max(-1, min(1, x))*28000) for x in samples])
        if sys.byteorder != 'little':
            pcm.byteswap()
        path = output / (name+'.pcm')
        path.write_bytes(pcm.tobytes())
        rows.append({'id': name, 'path': path.name, 'kind': 'synthetic PCM full-chain', 'split': 'development', 'events': events, 'duration_seconds': seconds,
                     'reference_bpm': 127.43 if name == 'fractional12743' else 90 if name == 'half_time' else 120 if name not in ('tempo_step','silence') else None,
                     'ambiguity': [90,180] if name == 'half_time' else [], 'annotation': 'generated attacks; correlation phase is not a calibrated beat instant'})
    for index, title in [(0,'Cristina Vane - So Easy'),(1,'Louis Cressy Band - Good Time')]:
        name = f'song-{index}.pcm'
        shutil.copy2(SOURCE/name, output/name)
        rows.append({'id':f'song-{index}','path':name,'kind':'authorised local MUSDB mixture','title':title,'split':'held-out' if index else 'development','events':None,'reference_bpm':None,
                     'duration_seconds':(output/name).stat().st_size/(RATE*2),'annotation':'NONE: musical accuracy pending independent attack/beat annotations', 'source_receipt':str(SOURCE/'receipt.json')})
    for row in rows:
        data=(output/row['path']).read_bytes()
        row['sha256']=hashlib.sha256(data).hexdigest()
        wav=(output/row['path']).with_suffix('.wav')
        with wave.open(str(wav),'wb') as w:
            w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes(data)
    manifest={'schema':'tempo-field-pcm-fixtures.v1','sample_rate_hz':RATE,'hop_samples':180,'media_rate_hz':48000,'format':'mono signed16 little endian',
              'generator_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'rounding':'floor(exact event seconds * 24000 + 0.5); no resampling of existing PCM',
              'scoring_frozen':{'incumbent_parity':'all serialised fields exactly identical','tempo':'exact absolute BPM error separate from min(error to half/double family); no real-song truth available',
              'attack':'nearest unused annotated attack, causal detection within 0..150ms; report misses/false positives, no detector accuracy admission threshold',
              'appearance':'same fixed scene/palette; paired total output level matched; preference requires operator judgement',
              'held_out':'song-1 never used for tuning'},'fixtures':rows}
    (output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(f'Frozen {len(rows)} fixtures at {output}')


if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,default=ROOT/'tests/fixtures/tempo_field/v1/pcm')
    build(parser.parse_args().output)
