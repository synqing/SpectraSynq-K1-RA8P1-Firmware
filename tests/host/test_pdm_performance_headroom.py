"""Compile the production ring at the exact bounded Titan slot count."""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[2]
def test_performance_headroom_and_paired_recovery(tmp_path):
    exe=tmp_path/'pdm-headroom'
    subprocess.run(['clang','-std=c11','-DK1_PDM_STREAM_SLOT_COUNT=4u','-I'+str(ROOT/'platform/ra8p1'),str(ROOT/'tests/host/test_pdm_performance_headroom.c'),str(ROOT/'platform/ra8p1/pdm_capture.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
    build=(ROOT/'scripts/build_scalar.py').read_text()
    assert "'-DK1_PDM_STREAM_SLOT_COUNT=4u'" in build
