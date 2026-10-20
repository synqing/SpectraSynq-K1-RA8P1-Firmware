#!/usr/bin/env python3
"""Host compile of the pair-fixture admission gate. Does not open a device."""
from pathlib import Path
import tempfile

from run_host import run
from verify_imports import ROOT

with tempfile.TemporaryDirectory(prefix="k1-pair-fixture-admit-") as temp:
    binary = Path(temp) / "test"
    run(
        [
            "cc",
            "-std=c11",
            "-Wall",
            "-Werror",
            "-I" + str(ROOT / "platform/ra8p1"),
            str(ROOT / "tests/host/test_pair_fixture_admit.c"),
            str(ROOT / "platform/ra8p1/pair_fixture_admit.c"),
            "-o",
            str(binary),
        ]
    )
    print(run([str(binary)]), end="")
