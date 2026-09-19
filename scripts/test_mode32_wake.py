#!/usr/bin/env python3
import json, os, shutil, subprocess, tempfile
from pathlib import Path
from verify_imports import ROOT
from palette_renderer_overlay import apply_palette_overlay

NAMES = json.loads((ROOT / 'docs/import-slices.json').read_text())['product']
RESIDENT = Path(
    '/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/'
    'k1-ra8p1-002/pdm-ap-gpt-build-20260914-10/stage')


def compile_run(label, palette_runtime, k1_root, overlay):
    with tempfile.TemporaryDirectory(prefix='k1-wake-') as temp:
        out = Path(temp)
        sources = out / 'k1'
        shutil.copytree(k1_root, sources)
        if overlay:
            apply_palette_overlay(sources)
        (out / 'build_identity.h').write_text(
            '#define K1_BUILD_ID "HOST-WAKE"\n'
            '#define K1_SOURCE_PIN "HOST-WAKE"\n')
        exe = out / 'wake'
        includes = ['-I' + str(sources), '-I' + str(ROOT / 'platform/ra8p1')]
        if not overlay:
            includes = ['-I' + str(Path(palette_runtime).parent)] + includes
        cmd = [
            'c++', '-std=c++17', '-O2', '-ffp-contract=off',
            '-DK1_PALETTE_MORPH=1',
            *includes,
            '-I' + str(ROOT / 'tests/target'),
            '-I' + str(out),
            str(palette_runtime),
            *[str(sources / p) for p in NAMES if p.endswith('.cpp')],
            str(ROOT / 'tests/host/test_mode32_wake.cpp'),
            '-o', str(exe),
        ]
        subprocess.run(cmd, check=True)
        ran = subprocess.run([str(exe)], capture_output=True, text=True)
        print('LABEL', label)
        print('stdout', ran.stdout)
        print('stderr', ran.stderr)
        print('code', ran.returncode)
        return ran.returncode, ran.stdout, ran.stderr


current_code, current_out, current_err = compile_run(
    'CURRENT', ROOT / 'platform/ra8p1/palette_runtime.cpp',
    ROOT / 'src/k1', True)
if os.environ.get('SKIP_RESIDENT') == '1':
    resident_code, resident_out, resident_err = 1, '', 'skipped'
else:
    try:
        resident_code, resident_out, resident_err = compile_run(
            'RESIDENT_3526df5c', RESIDENT / 'src/palette_runtime.cpp',
            RESIDENT / 'src/k1', False)
    except subprocess.CalledProcessError as error:
        print('RESIDENT_COMPILE_FAIL', error)
        resident_code, resident_out, resident_err = 2, '', str(error)
print('CURRENT_PASS', current_code == 0)
print('RESIDENT_WAKE', 'FAIL' if resident_code else 'HOLD')
if current_code != 0:
    raise SystemExit(current_code)
