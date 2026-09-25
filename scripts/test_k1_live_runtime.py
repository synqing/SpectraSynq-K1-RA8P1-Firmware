#!/usr/bin/env python3
"""Host gate for live AP owner, hop timestamps and MIR snapshot."""
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from verify_imports import ROOT

NAMES = json.loads((ROOT / "docs/import-slices.json").read_text())["product"]


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="k1-live-runtime-") as temp:
        out = Path(temp)
        sources = out / "k1"
        shutil.copytree(ROOT / "src/k1", sources)
        exe = out / "test_k1_live_runtime"
        command = [
            "c++",
            "-std=c++17",
            "-O2",
            "-ffp-contract=off",
            "-I" + str(sources),
            "-I" + str(ROOT / "src"),
            "-I" + str(ROOT / "platform/ra8p1"),
            "-I" + str(ROOT / "tests/target"),
            str(ROOT / "platform/ra8p1/k1_live_clock.c"),
            str(ROOT / "platform/ra8p1/k1_live_runtime.cpp"),
            str(ROOT / "platform/ra8p1/k1_live_protocol.cpp"),
            str(ROOT / "src/k1/core/audio/k1_audio_hop.c"),
            *[str(sources / name) for name in NAMES if name.endswith(".cpp")],
            str(ROOT / "tests/host/test_k1_live_runtime.cpp"),
            "-o",
            str(exe),
        ]
        subprocess.run(command, check=True)
        output = subprocess.check_output([str(exe)], text=True)
        sys.stdout.write(output)
        if "LIVE_RUNTIME_HOST_PASS" not in output:
            raise SystemExit("missing LIVE_RUNTIME_HOST_PASS")
        if "phase8=scheduled" not in output:
            raise SystemExit("Phase 8 recording must remain scheduled")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
