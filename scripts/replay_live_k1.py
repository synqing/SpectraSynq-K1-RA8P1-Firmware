#!/usr/bin/env python3
"""Offline LiveAudioRuntime replay. Never opens CDC."""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from verify_imports import ROOT

NAMES = json.loads((ROOT / "docs/import-slices.json").read_text())["product"]


def compile_replay(tmp: Path) -> Path:
    sources = tmp / "k1"
    shutil.copytree(ROOT / "src/k1", sources)
    exe = tmp / "replay_live_k1"
    command = [
        "c++", "-std=c++17", "-O2", "-ffp-contract=off",
        "-I" + str(sources),
        "-I" + str(ROOT / "src"),
        "-I" + str(ROOT / "platform/ra8p1"),
        "-I" + str(ROOT / "tests/target"),
        str(ROOT / "platform/ra8p1/k1_live_clock.c"),
        str(ROOT / "platform/ra8p1/k1_live_runtime.cpp"),
        str(ROOT / "platform/ra8p1/k1_live_protocol.cpp"),
        str(ROOT / "src/k1/core/audio/k1_audio_hop.c"),
        *[str(sources / name) for name in NAMES if name.endswith(".cpp")],
        str(ROOT / "tests/host/replay_live_k1.cpp"),
        "-o", str(exe),
    ]
    subprocess.run(command, check=True)
    return exe


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pcm", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--repeat", type=int, default=2)
    args = parser.parse_args()
    pcm = args.pcm.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="k1-replay-") as temp:
        exe = compile_replay(Path(temp))
        results = []
        for _ in range(args.repeat):
            results.append(json.loads(subprocess.check_output([str(exe), str(pcm)], text=True)))
        delayed = subprocess.run([str(exe), str(pcm), "delay"], capture_output=True, text=True)
        delay_payload = None
        if delayed.returncode == 0:
            delay_payload = json.loads(delayed.stdout)
        same = all(row == results[0] for row in results)
        payload = {
            "pcm": str(pcm),
            "pcm_sha256": hashlib.sha256(pcm.read_bytes()).hexdigest(),
            "device_mutated": False,
            "repeatable": same,
            "runs": results,
            "delay_case": True,
            "delay_exit": delayed.returncode,
            "delay_run": delay_payload,
            "call_path": "LiveAudioRuntime::consume",
        }
    (args.output / "REPLAY.json").write_text(json.dumps(payload, indent=2) + "\n")
    print("REPLAY_OK repeatable", same, "hops", results[0].get("hops"))
    return 0 if same and results[0].get("hops", 0) > 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
