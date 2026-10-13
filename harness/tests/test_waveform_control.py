#!/usr/bin/env python3
"""Production C++ control parser/apply against a frozen Python-shared wire vector."""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'_scratch'/'test-waveform-control';OUT.mkdir(parents=True,exist_ok=True)
(OUT/'golden.bin').write_bytes(bytes.fromhex((ROOT/'harness/tests/waveform-v8-golden.wire').read_text()))
sources=['harness/tests/test_waveform_control.cpp','platform/ra8p1/emotiscope_port.cpp','platform/ra8p1/emotiscope_secondary.cpp','platform/ra8p1/emotiscope_ml.cpp','src/k1/core/visual/product_palette.cpp','src/k1/core/visual/product_palette_data.cpp']
cmd=['clang++','-std=c++17','-O1','-g','-Wno-narrowing','-fsanitize=address,undefined','-fno-omit-frame-pointer','-DK1_EMOTISCOPE_PORT=1','-DK1_TITAN_TOP_OUTPUT=1','-Isrc/k1','-Iplatform/ra8p1','-Iplatform/ra8p1/emotiscope_donor','-Iplatform/ra8p1/emotiscope_shims',*sources,'-o',str(OUT/'control')]
subprocess.run(cmd,cwd=ROOT,check=True);subprocess.run([str(OUT/'control'),str(OUT/'golden.bin')],cwd=ROOT,check=True)
