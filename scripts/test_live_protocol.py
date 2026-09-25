#!/usr/bin/env python3
"""Host compile-and-run for the live wire codec plus Python unpacker."""
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from verify_imports import ROOT

NAMES = json.loads((ROOT / "docs/import-slices.json").read_text())["product"]


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="k1-live-protocol-") as temp:
        out = Path(temp)
        sources = out / "k1"
        shutil.copytree(ROOT / "src/k1", sources)
        exe = out / "test_live_protocol"
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
            str(ROOT / "tests/host/test_live_protocol.cpp"),
            "-o",
            str(exe),
        ]
        subprocess.run(command, check=True)
        output = subprocess.check_output([str(exe)], text=True)
        sys.stdout.write(output)
        if "LIVE_PROTOCOL_HOST_PASS" not in output:
            raise SystemExit("missing LIVE_PROTOCOL_HOST_PASS")
    sys.path.insert(0, str(ROOT / "tools/serial-studio"))
    import live_protocol as proto

    packed = proto.pack_config({"mode_a": 32, "emit_on": 0, "flags": 1})
    decoded = proto.unpack_config(packed)
    if decoded["mode_a"] != 32 or decoded["emit_on"] != 0:
        raise SystemExit("python config roundtrip failed")
    if proto.SCHEMA_SHA256 != proto.CONTRACT.get("schema_version", 1) and len(proto.SCHEMA_SHA256) != 64:
        raise SystemExit("schema hash missing")
    print("LIVE_PROTOCOL_PY_PASS", proto.SCHEMA_SHA256)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
