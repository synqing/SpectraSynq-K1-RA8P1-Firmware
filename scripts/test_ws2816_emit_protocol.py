#!/usr/bin/env python3
"""Compile the lockstep WS2816 bit protocol on HOST with a recording sink."""
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT
with tempfile.TemporaryDirectory(prefix='k1-ws2816-emit-') as temp:
    directory = Path(temp)
    command = ['c++', '-std=c++17', '-O2', '-ffp-contract=off', '-fno-fast-math',
               '-I' + str(ROOT / 'src/k1'),
               '-I' + str(ROOT / 'platform/ra8p1'),
               str(ROOT / 'tests/host/test_ws2816_emit_protocol.cpp'),
               '-o', str(directory / 'test')]
    run(command)
    print(run([str(directory / 'test')]), end='')
