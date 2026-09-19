#!/usr/bin/env python3
"""HOST: print original vs sliced tempo fields for frozen hops 0-7."""
import shutil
import subprocess
import tempfile
from pathlib import Path
from tempo_acf_slice_overlay import apply_tempo_acf_slice_overlay
from verify_imports import ROOT

AUDIO = ROOT / 'src/k1/core/audio'
RESIDENT = Path('/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/resident-controls-01')
TEST = ROOT / 'tests/host/test_first_tempo_crc_field.cpp'

def compile_run(k1: Path, label: str) -> str:
    cmd = ['c++', '-std=c++17', '-O2', '-ffp-contract=off', '-fno-fast-math',
           '-I' + str(k1), '-I' + str(RESIDENT), '-I' + str(ROOT / 'src/k1'), str(TEST)]
    cmd += [str(p) for p in sorted((k1 / 'core/audio').glob('*.cpp'))]
    out = k1.parent / label
    subprocess.run(cmd + ['-o', str(out)], check=True)
    return subprocess.check_output([str(out)], text=True)

def main():
    with tempfile.TemporaryDirectory(prefix='k1-first-diff-') as temp:
        temp = Path(temp)
        orig = temp / 'orig' / 'k1'
        sliced = temp / 'sliced' / 'k1'
        shutil.copytree(ROOT / 'src/k1', orig)
        shutil.copytree(ROOT / 'src/k1', sliced)
        apply_tempo_acf_slice_overlay(sliced)
        a = compile_run(orig, 'orig.bin')
        b = compile_run(sliced, 'sliced.bin')
        print('ORIGINAL')
        print(a)
        print('SLICED')
        print(b)
        al, bl = a.strip().splitlines(), b.strip().splitlines()
        for i, (x, y) in enumerate(zip(al, bl)):
            if x != y:
                print(f'FIRST_LINE_DIFF hop_line={i}')
                print(' orig ', x)
                print(' slice', y)
                break
        else:
            print('NO_LINE_DIFF')

if __name__ == '__main__':
    main()
