#!/usr/bin/env python3
"""Compile the actual platform fixture parser on HOST and exercise its negatives."""
import json
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT
with tempfile.TemporaryDirectory(prefix='k1-protocol-') as temp:
    directory=Path(temp)
    (directory/'build_identity.h').write_text('#define K1_BUILD_ID "HOST-TEST"\n#define K1_SOURCE_PIN "HOST-TEST"\n')
    names=json.loads((ROOT/'docs/import-slices.json').read_text())['product']
    command=['c++','-std=c++17','-O2','-ffp-contract=off','-fno-fast-math','-DK1_STATUS_GPIO_STUB=1',
             '-I'+str(ROOT/'src/k1'),'-I'+str(ROOT/'tests/target'),'-I'+str(ROOT/'platform/ra8p1'),'-I'+str(directory),
             str(ROOT/'tests/host/test_fixture_protocol.cpp'),str(ROOT/'platform/ra8p1/fixture_app.cpp'),
             str(ROOT/'platform/ra8p1/k1_status_led.c'),str(ROOT/'platform/ra8p1/titan_status_gpio.c'),
             *[str(ROOT/'src/k1'/p) for p in names if p.endswith('.cpp')],'-o',str(directory/'test')]
    run(command)
    print(run([str(directory/'test')]),end='')
