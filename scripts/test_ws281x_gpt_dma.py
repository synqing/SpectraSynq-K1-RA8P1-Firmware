#!/usr/bin/env python3
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT

with tempfile.TemporaryDirectory(prefix="k1-ws281x-gpt-dma-") as temp:
    binary = Path(temp) / "test"
    run(
        [
            "c++",
            "-std=c++17",
            "-O2",
            "-I" + str(ROOT / "platform/ra8p1"),
            str(ROOT / "tests/host/test_ws281x_gpt_dma.cpp"),
            str(ROOT / "platform/ra8p1/ws281x_gpt_dma.c"),
            str(ROOT / "platform/ra8p1/ws281x_waveform.c"),
            "-o",
            str(binary),
        ]
    )
    print(run([str(binary)]), end="")
