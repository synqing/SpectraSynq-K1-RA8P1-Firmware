#!/usr/bin/env python3
"""THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Build host-only comparison."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def build(output):
    output.mkdir(parents=True, exist_ok=False)
    compiler = shutil.which('g++-15')
    if not compiler:
        raise RuntimeError('GCC 15 required for unmodified FixedPoints 1.1.2 constexpr members')
    names = json.loads((ROOT/'docs/import-slices.json').read_text())['product']
    names += ['core/audio/tempo_field_publisher.cpp', 'core/audio/tempo_phase_tracks.cpp',
              'core/visual/tempo_reference_scene.cpp', 'core/visual/ribbon_material.cpp', 'core/visual/ribbon_profile.cpp', 'core/visual/paired_wave_scene.cpp']
    sources = [ROOT/'src/k1'/n for n in names if n.endswith('.cpp')]
    sources += [ROOT/'platform/ra8p1'/n for n in ['k1_live_clock.c', 'k1_live_runtime.cpp', 'k1_live_protocol.cpp']]
    sources += [ROOT/'src/k1/core/audio/k1_audio_hop.c', ROOT/'tests/host/ribbon_replay.cpp']
    exe = output/'ribbon_replay'
    command = [compiler, '-std=c++17', '-O2', '-ffp-contract=off', '-fno-fast-math',
               *['-I'+str(ROOT/p) for p in ['src/k1','src','platform/ra8p1','tests/reference']],
               *map(str,sources), '-o', str(exe)]
    result = subprocess.run(command, capture_output=True, text=True)
    (output/'build.log').write_text(result.stdout+result.stderr)
    if result.returncode:
        raise RuntimeError(result.stderr)
    paths = set(sources) | set((ROOT/'src/k1').rglob('*.h')) | set((ROOT/'tests/reference').rglob('*.h'))
    paths |= set((ROOT/'platform/ra8p1').glob('*.h')) | {Path(__file__)}
    (output/'build-receipt.json').write_text(json.dumps(dict(
        command=command, compiler=subprocess.check_output([compiler,'--version'],text=True),
        executable_sha256=sha(exe), sources={str(p.relative_to(ROOT)):sha(p) for p in sorted(paths)},
        target_executed=False),indent=2)+'\n')
    return exe

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    print(build(parser.parse_args().output.resolve()))
