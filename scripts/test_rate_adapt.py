#!/usr/bin/env python3
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT

with tempfile.TemporaryDirectory(prefix="k1-rate-adapt-") as temp:
    binary = Path(temp) / "test"
    run(
        [
            "c++",
            "-std=c++17",
            "-O2",
            "-I" + str(ROOT / "platform/ra8p1"),
            str(ROOT / "tests/host/test_rate_adapt.cpp"),
            str(ROOT / "platform/ra8p1/k1_rate_adapt.c"),
            "-o",
            str(binary),
        ]
    )
    print(run([str(binary)]), end="")
