#!/usr/bin/env python3
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT

with tempfile.TemporaryDirectory(prefix='k1-cycle-clock-') as temp:
    binary = Path(temp) / 'test'
    run([
        'cc', '-std=c11', '-O2',
        '-I' + str(ROOT / 'platform/ra8p1'),
        str(ROOT / 'tests/host/test_cycle_clock.c'),
        str(ROOT / 'platform/ra8p1/k1_asrc_24k.c'),
        '-o', str(binary),
    ])
    print(run([str(binary)]), end='')
