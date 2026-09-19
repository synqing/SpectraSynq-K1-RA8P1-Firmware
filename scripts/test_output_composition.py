#!/usr/bin/env python3
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT

names = [
    'core/visual/frame_blend.cpp',
    'core/visual/product_runtime_policy.cpp',
]
with tempfile.TemporaryDirectory(prefix='k1-output-composition-') as temp:
    binary = Path(temp) / 'test'
    command = [
        'c++', '-std=c++17', '-O2', '-ffp-contract=off',
        '-I' + str(ROOT / 'src/k1'),
        '-I' + str(ROOT / 'platform/ra8p1'),
        str(ROOT / 'tests/host/test_output_composition.cpp'),
    ]
    command += [str(ROOT / 'src/k1' / name) for name in names]
    command += ['-o', str(binary)]
    run(command)
    print(run([str(binary)]), end='')
