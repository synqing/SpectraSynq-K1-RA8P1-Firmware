#!/usr/bin/env python3
"""HOST-ONLY: sliced ACF matches one-shot computeTempoAcfAtRate; incomplete pump fails."""
import subprocess
import tempfile
from pathlib import Path
from verify_imports import ROOT

def main():
    with tempfile.TemporaryDirectory(prefix='k1-acf-slice-') as temp:
        out = Path(temp) / 'test_tempo_acf_slice'
        cmd = [
            'c++', '-std=c++17', '-O2', '-ffp-contract=off', '-fno-fast-math',
            '-I' + str(ROOT / 'src/k1'),
            str(ROOT / 'src/k1/core/audio/tempo_acf.cpp'),
            str(ROOT / 'src/k1/core/audio/tempo_acf_slice.cpp'),
            str(ROOT / 'tests/host/test_tempo_acf_slice.cpp'),
            '-o', str(out),
        ]
        subprocess.run(cmd, check=True)
        print(subprocess.check_output([str(out)], text=True), end='')

if __name__ == '__main__':
    main()
