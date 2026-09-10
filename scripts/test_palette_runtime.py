#!/usr/bin/env python3
"""Compare pinned VP composition with the optional palette-transition derivative."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from verify_imports import ROOT
from palette_renderer_overlay import apply_palette_overlay

names=json.loads((ROOT/'docs/import-slices.json').read_text())['product']
digests=[]
with tempfile.TemporaryDirectory(prefix='k1-palettes-') as temp:
    out=Path(temp)
    (out/'build_identity.h').write_text('#define K1_BUILD_ID "HOST-PALETTES"\n#define K1_SOURCE_PIN "HOST-PALETTES"\n')
    for morph in (False,True):
        sources=ROOT/'src/k1'
        if morph:
            sources=out/'k1'
            shutil.copytree(ROOT/'src/k1',sources)
            apply_palette_overlay(sources)
            # Overlay rejects unexpected/already-transformed source.
            try: apply_palette_overlay(sources)
            except RuntimeError: pass
            else: raise AssertionError('overlay accepted a source mutation')
        common=['c++','-std=c++17','-O2','-ffp-contract=off','-fno-fast-math',
                '-I'+str(sources),'-I'+str(ROOT/'platform/ra8p1'),
                '-I'+str(ROOT/'tests/target'),'-I'+str(out),
                str(ROOT/'platform/ra8p1/palette_runtime.cpp'),
                *[str(sources/p) for p in names if p.endswith('.cpp')]]
        if morph: common+=['-DK1_PALETTE_MORPH=1']
        for suite in (('runtime','protocol','transition') if morph else ('runtime','protocol')):
            executable=out/(suite+str(morph))
            command=common+[str(ROOT/f'tests/host/test_palette_{suite}.cpp'),'-o',str(executable)]
            if suite=='protocol': command+=['-DK1_PALETTE_RUNTIME=1',str(ROOT/'platform/ra8p1/fixture_app.cpp')]
            subprocess.run(command,check=True)
            result=subprocess.check_output([str(executable)],text=True)
            print(('MORPH ' if morph else 'PINNED ')+result,end='')
            if suite=='runtime':
                digests.append(next(line for line in result.splitlines() if line.startswith('COMPATIBILITY_DIGEST=')))
    assert len(digests)==2 and digests[0]==digests[1], 'disabled-transition VP differs from pinned renderer'
    print('PALETTE_COMPATIBILITY_PASS independent_executables=2 overlay_mutation_rejected=true')
