#!/usr/bin/env python3
import json, subprocess, tempfile
from pathlib import Path
from verify_imports import ROOT

NAMES = json.loads((ROOT / 'docs/import-slices.json').read_text())['product']
audio = [p for p in NAMES if p.startswith('core/audio/') and p.endswith('.cpp')]
with tempfile.TemporaryDirectory(prefix='k1-audio-exp-') as temp:
    out = Path(temp) / 'exp'
    cmd = [
        'c++', '-std=c++17', '-O2', '-ffp-contract=off', '-fno-fast-math',
        '-I' + str(ROOT / 'src/k1'),
        *[str(ROOT / 'src/k1' / p) for p in audio],
        str(ROOT / 'tests/host/test_audio_experiment_baseline.cpp'),
        '-o', str(out),
    ]
    subprocess.run(cmd, check=True)
    ran = subprocess.run([str(out)], capture_output=True, text=True)
    print(ran.stdout, end='')
    if ran.returncode:
        raise SystemExit(ran.returncode)
