#!/usr/bin/env python3
"""Compile the paired GPT/DMA driver against the register/FSP seam."""
from pathlib import Path
import subprocess, tempfile
ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temp:
    exe = Path(temp) / 'gpt-pair-hw-test'
    subprocess.run([
        'cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-I' + str(ROOT / 'tests/target_mock'),
        '-I' + str(ROOT / 'platform/ra8p1'),
        str(ROOT / 'tests/test_ws281x_gpt_dma_hw_pair.c'),
        str(ROOT / 'platform/ra8p1/ws281x_waveform.c'),
        '-o', str(exe),
    ], check=True, timeout=120)
    subprocess.run([str(exe)], check=True, timeout=60)
