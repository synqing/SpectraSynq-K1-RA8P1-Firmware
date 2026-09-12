#!/usr/bin/env python3
"""Host tests for Titan status LED patterns, GPIO inversion and lifecycle."""
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT
with tempfile.TemporaryDirectory(prefix='k1-status-led-') as temp:
    directory=Path(temp)
    command=['cc','-std=c11','-O2','-DK1_STATUS_GPIO_STUB=1',
             '-I'+str(ROOT/'platform/ra8p1'),
             str(ROOT/'tests/host/test_status_led.c'),
             str(ROOT/'platform/ra8p1/k1_status_led.c'),
             str(ROOT/'platform/ra8p1/titan_status_gpio.c'),
             '-o',str(directory/'test')]
    run(command)
    print(run([str(directory/'test')]),end='')
