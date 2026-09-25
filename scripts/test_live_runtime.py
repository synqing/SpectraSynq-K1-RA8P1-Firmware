#!/usr/bin/env python3
"""Aggregate host gate for the live-runtime source candidate."""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPTS = ROOT / "scripts"


def run(script: str) -> None:
    command = [sys.executable, str(SCRIPTS / script)]
    print("RUN", script, flush=True)
    subprocess.run(command, check=True)


def main() -> int:
    run("test_k1_live_clock.py")
    run("test_asrc_24k.py")
    run("test_cycle_clock.py")
    run("test_k1_live_runtime.py")
    run("test_live_protocol.py")
    run("test_live_lease.py")
    run("test_score_live_k1.py")
    print("LIVE_RUNTIME_HOST_SUITE_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
