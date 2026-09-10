#!/usr/bin/env python3
"""Compile the RA8P1-local WS2816 packer on HOST and exercise TRUE16 negatives."""
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT
with tempfile.TemporaryDirectory(prefix='k1-ws2816-pack-') as temp:
    directory = Path(temp)
    command = ['c++', '-std=c++17', '-O2', '-ffp-contract=off', '-fno-fast-math',
               '-I' + str(ROOT / 'src/k1'),
               str(ROOT / 'tests/host/test_ws2816_pack.cpp'),
               '-o', str(directory / 'test')]
    run(command)
    print(run([str(directory / 'test')]), end='')
