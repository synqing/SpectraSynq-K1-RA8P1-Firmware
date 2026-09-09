#!/usr/bin/env python3
"""Compile and run the bounded semantic-sidecar failure contract on HOST."""
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT

with tempfile.TemporaryDirectory(prefix='k1-semantic-sidecar-') as temp:
    executable=Path(temp)/'test'
    run(['c++','-std=c++17','-O2','-ffp-contract=off','-fno-fast-math',
         '-I'+str(ROOT/'platform/ra8p1'),str(ROOT/'tests/host/test_semantic_sidecar.cpp'),
         str(ROOT/'platform/ra8p1/semantic_sidecar.cpp'),'-o',str(executable)])
    print(run([str(executable)]),end='')
