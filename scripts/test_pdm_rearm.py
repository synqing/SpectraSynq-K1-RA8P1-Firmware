#!/usr/bin/env python3
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT
with tempfile.TemporaryDirectory(prefix='k1-pdm-rearm-') as temp:
    out=Path(temp)/'test'
    run(['cc','-std=c11','-O2','-I'+str(ROOT/'platform/ra8p1'),
         str(ROOT/'tests/host/test_pdm_rearm.c'),
         str(ROOT/'platform/ra8p1/pdm_capture.c'),
         '-o',str(out)])
    print(run([str(out)]),end='')
