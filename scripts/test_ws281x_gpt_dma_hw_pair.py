#!/usr/bin/env python3
"""Compile the paired GPT/DMA driver against the register/FSP seam."""
from pathlib import Path
import subprocess, tempfile
ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temp:
    for donor in (False, True):
        exe = Path(temp) / f'gpt-pair-hw-test-donor-{int(donor)}'
        definitions = ['-DK1_DUAL_EDGE_DONOR=1'] if donor else []
        subprocess.run([
            'cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
            '-fsanitize=address', '-fno-omit-frame-pointer', *definitions,
            '-I' + str(ROOT / 'tests/target_mock'),
            '-I' + str(ROOT / 'platform/ra8p1'),
            str(ROOT / 'tests/test_ws281x_gpt_dma_hw_pair.c'),
            str(ROOT / 'platform/ra8p1/ws281x_waveform.c'),
            '-o', str(exe),
        ], check=True, timeout=120)
        subprocess.run([str(exe)], check=True, timeout=60)
        print(f'Paired driver: 80 pixels per DIN, donor={donor}, late-poll/reset/timeout regressions passed')
