#!/usr/bin/env python3
"""Host gate for the live-runtime clock and hop-descriptor contract."""
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="k1-live-clock-") as temp:
        executable = Path(temp) / "test_k1_live_clock"
        command = [
            "c++",
            "-std=c++17",
            "-O2",
            "-I" + str(ROOT / "src"),
            "-I" + str(ROOT / "platform/ra8p1"),
            str(ROOT / "platform/ra8p1/k1_live_clock.c"),
            str(ROOT / "src/k1/core/audio/k1_audio_hop.c"),
            str(ROOT / "tests/host/test_k1_live_clock.cpp"),
            "-o",
            str(executable),
        ]
        subprocess.run(command, check=True)
        output = subprocess.check_output([str(executable)], text=True)
        sys.stdout.write(output)
        if "LIVE_CLOCK_CONTRACT_PASS" not in output:
            raise SystemExit("missing LIVE_CLOCK_CONTRACT_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
