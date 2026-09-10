#!/usr/bin/env python3
"""Exercise every palette in the native VP and actual Titan protocol composition."""
import json
from pathlib import Path
import subprocess
import tempfile
from verify_imports import ROOT

names = json.loads((ROOT/'docs/import-slices.json').read_text())['product']
with tempfile.TemporaryDirectory(prefix='k1-palettes-') as temp:
    out = Path(temp)
    (out/'build_identity.h').write_text('#define K1_BUILD_ID "HOST-PALETTES"\n#define K1_SOURCE_PIN "HOST-PALETTES"\n')
    common = ['c++','-std=c++17','-O2','-ffp-contract=off','-fno-fast-math',
              '-I'+str(ROOT/'src/k1'),'-I'+str(ROOT/'platform/ra8p1'),
              '-I'+str(ROOT/'tests/target'),'-I'+str(out),
              str(ROOT/'platform/ra8p1/palette_runtime.cpp'),
              *[str(ROOT/'src/k1'/p) for p in names if p.endswith('.cpp')]]
    for suite in ('runtime','protocol'):
        executable=out/suite
        command=common+[str(ROOT/f'tests/host/test_palette_{suite}.cpp'),'-o',str(executable)]
        if suite=='protocol':
            command += ['-DK1_PALETTE_RUNTIME=1',str(ROOT/'platform/ra8p1/fixture_app.cpp')]
        subprocess.run(command,check=True)
        subprocess.run([str(executable)],check=True)
