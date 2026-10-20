#!/usr/bin/env python3
"""DBS pilot step 1: the pre-AGC probe reads only at or after publication.

Builds and runs test_p6_onset_chronology.cpp (one valid read and the earlier
future-publication read, which InhouseAudio::acquire must reject). Host only.
"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FLAGS = ["-O2", "-fno-fast-math", "-DK1_EMOTISCOPE_PORT=1", "-DK1_INHOUSE_MAPPING_V9=1",
         "-Iplatform/ra8p1", "-Isrc", "-Isrc/k1"]
SOURCES = ["tests/host/test_p6_onset_chronology.cpp", "platform/ra8p1/inhouse_audio.cpp",
           "src/k1/core/audio/gdft_goertzel.cpp", "src/k1/core/audio/tempo_tracker.cpp",
           "src/k1/core/audio/tempo_acf.cpp"]


def test_onset_chronology() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        exe = Path(tmp) / "chronology"
        subprocess.run(["c++", "-std=c++17", *FLAGS, *SOURCES, "-o", str(exe)], cwd=ROOT, check=True)
        result = subprocess.run([str(exe)], cwd=ROOT, capture_output=True, text=True)
        sys.stdout.write(result.stdout)
        assert result.returncode == 0, result.stdout


if __name__ == "__main__":
    test_onset_chronology()
