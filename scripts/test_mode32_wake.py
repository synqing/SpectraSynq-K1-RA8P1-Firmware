#!/usr/bin/env python3
import json, os, shutil, subprocess, tempfile
from pathlib import Path
from verify_imports import ROOT
from palette_renderer_overlay import apply_palette_overlay

NAMES = json.loads((ROOT / 'docs/import-slices.json').read_text())['product']


def compile_run(label, palette_runtime, k1_root, overlay, extra_defines=None):
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
            *(extra_defines or []),
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


# Lane-work only: never compile against a foreign stage path (V-MODE32-WAKE-ISOLATION).
current_code, current_out, current_err = compile_run(
    'CURRENT', ROOT / 'platform/ra8p1/palette_runtime.cpp',
    ROOT / 'src/k1', True)
live_code, live_out, live_err = compile_run(
    'LIVE_RUNTIME', ROOT / 'platform/ra8p1/palette_runtime.cpp',
    ROOT / 'src/k1', True, extra_defines=['-DK1_LIVE_RUNTIME=1'])
print('CURRENT_PASS', current_code == 0)
print('LIVE_RUNTIME_PASS', live_code == 0)
print('RESIDENT_WAKE', 'SKIPPED_LANE_WORK_ONLY')
if current_code != 0 or live_code != 0:
    raise SystemExit(current_code or live_code)
