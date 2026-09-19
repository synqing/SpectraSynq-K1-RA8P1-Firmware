#!/usr/bin/env python3
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT

with tempfile.TemporaryDirectory(prefix='k1-strength-shape-') as temp:
    binary = Path(temp) / 'test'
    run([
        'c++', '-std=c++17', '-O2',
        '-I' + str(ROOT / 'src/k1'),
        str(ROOT / 'tests/host/test_feature_strength_shape.cpp'),
        str(ROOT / 'src/k1/core/audio/feature_strength_shape.cpp'),
        '-o', str(binary),
    ])
    print(run([str(binary)]), end='')
