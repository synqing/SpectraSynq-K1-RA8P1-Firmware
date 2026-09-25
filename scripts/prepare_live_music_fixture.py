#!/usr/bin/env python3
"""Hash a named 24 kHz S16LE fixture. Does not invent music."""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import wave
from pathlib import Path


def convert(src: Path, dst: Path) -> dict:
    with wave.open(str(src), "rb") as wav:
        channels = wav.getnchannels()
        width = wav.getsampwidth()
        rate = wav.getframerate()
        frames = wav.getnframes()
        pcm = wav.readframes(frames)
    note = {
        "source": str(src),
        "source_sha256": hashlib.sha256(src.read_bytes()).hexdigest(),
        "channels": channels,
        "width": width,
        "rate_in": rate,
        "frames_in": frames,
    }
    dst.parent.mkdir(parents=True, exist_ok=True)
    if channels == 1 and width == 2 and rate == 24000:
        dst.write_bytes(pcm)
        note["conversion"] = "copy"
    else:
        sox = shutil.which("sox")
        if not sox:
            raise SystemExit("need sox to convert to 24 kHz mono S16LE, or supply that format")
        subprocess.run(
            [sox, str(src), "-r", "24000", "-c", "1", "-b", "16", "-e", "signed-integer", str(dst)],
            check=True,
        )
        note["conversion"] = "sox"
    note["rate_hz"] = 24000
    note["sample_format"] = "s16le"
    note["sha256"] = hashlib.sha256(dst.read_bytes()).hexdigest()
    note["bytes"] = dst.stat().st_size
    note["named_music"] = False
    return note


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--named-music", action="store_true")
    args = parser.parse_args()
    pcm = args.output / "music_24k_s16le.pcm"
    meta = convert(args.input.resolve(), pcm)
    if args.named_music:
        meta["named_music"] = True
    meta["commands"] = {
        "prepare": "scripts/prepare_live_music_fixture.py",
        "replay": "scripts/replay_live_k1.py",
        "compare": "scripts/compare_live_k1.py",
        "score": "scripts/score_live_k1.py",
        "control": "tools/serial-studio/titan_live_control.py",
    }
    (args.output / "FIXTURE.json").write_text(json.dumps(meta, indent=2) + "\n")
    print("FIXTURE_OK", meta["sha256"], "named_music", meta["named_music"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
