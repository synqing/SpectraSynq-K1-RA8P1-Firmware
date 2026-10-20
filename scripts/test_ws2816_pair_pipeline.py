#!/usr/bin/env python3
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT

with tempfile.TemporaryDirectory(prefix="k1-ws2816-pair-pipeline-") as temp:
    binary = Path(temp) / "test"
    run(
        [
            "c++",
            "-std=c++17",
            "-O2",
            "-I" + str(ROOT / "platform/ra8p1"),
            "-I" + str(ROOT / "src/k1"),
            str(ROOT / "tests/host/test_ws2816_pair_pipeline.cpp"),
            str(ROOT / "platform/ra8p1/ws281x_gpt_dma_pair.c"),
            str(ROOT / "platform/ra8p1/ws281x_gpt_dma.c"),
            str(ROOT / "platform/ra8p1/ws281x_waveform.c"),
            "-o",
            str(binary),
        ]
    )
    print(run([str(binary)]), end="")
