#!/usr/bin/env python3
"""Compile the resident target scheduler on HOST and exercise its control protocol."""
import json
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT
with tempfile.TemporaryDirectory(prefix='k1-schedule-protocol-') as temp:
    directory=Path(temp)
    (directory/'build_identity.h').write_text('#define K1_BUILD_ID "HOST-TEST"\n#define K1_SOURCE_PIN "HOST-TEST"\n')
    names=json.loads((ROOT/'docs/import-slices.json').read_text())['product']
    base=['c++','-std=c++17','-O2','-ffp-contract=off','-fno-fast-math','-DK1_RESIDENT_SCHEDULE=1','-DK1_STATUS_GPIO_STUB=1',
          '-DK1_DOUBLE_PROBE_HOST=1',
          '-I'+str(ROOT/'tests/host'),'-I'+str(ROOT/'src/k1'),'-I'+str(ROOT/'tests/target'),
          '-I'+str(ROOT/'platform/ra8p1'),'-I'+str(directory),
          str(ROOT/'tests/host/test_schedule_protocol.cpp'),str(ROOT/'platform/ra8p1/fixture_app.cpp'),
          str(ROOT/'platform/ra8p1/semantic_sidecar.cpp'),
          str(ROOT/'platform/ra8p1/k1_double_probe.c'),
          str(ROOT/'platform/ra8p1/k1_status_led.c'),
          str(ROOT/'platform/ra8p1/titan_status_gpio.c'),
          *[str(ROOT/'src/k1'/p) for p in names if p.endswith('.cpp')]]
    for label,extra in [('scalar',[]),('npu',['-DK1_NPU_LOAD=1']),
                        ('profile',['-DK1_ENABLE_STAGE_PROBE=1'])]:
        executable=directory/('test-'+label)
        run([*base,*extra,'-o',str(executable)])
        print(label.upper()+' '+run([str(executable)]),end='')
