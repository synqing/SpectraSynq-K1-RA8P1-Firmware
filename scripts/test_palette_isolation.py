#!/usr/bin/env python3
"""Host isolation: palette changes with audio, time and renderer start held."""
import json
import subprocess
import tempfile
from pathlib import Path

from verify_imports import ROOT

COMPILE_TIMEOUT = 120
RUN_TIMEOUT = 60

names = json.loads((ROOT / "docs/import-slices.json").read_text())["product"]


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="k1-palette-iso-") as temp:
        out = Path(temp)
        exe = out / "test_palette_isolation"
        command = [
            "c++", "-std=c++17", "-O2", "-ffp-contract=off", "-fno-fast-math",
            "-DK1_LIVE_RUNTIME=1", "-DK1_PALETTE_RUNTIME=1",
            "-I" + str(ROOT / "src/k1"),
            "-I" + str(ROOT / "platform/ra8p1"),
            "-I" + str(ROOT / "tests/target"),
            str(ROOT / "platform/ra8p1/palette_runtime.cpp"),
            *[str(ROOT / "src/k1" / p) for p in names if p.endswith(".cpp")],
            str(ROOT / "src/k1/core/visual/frame_blend.cpp"),
            str(ROOT / "src/k1/core/visual/product_runtime_policy.cpp"),
            str(ROOT / "tests/host/test_palette_isolation.cpp"),
            "-o", str(exe),
        ]
        subprocess.run(command, check=True, timeout=COMPILE_TIMEOUT)
        text = subprocess.check_output([str(exe)], text=True, timeout=RUN_TIMEOUT)
        print(text, end="")
        for stamp in (
            "PALETTE_ISOLATION_PREVIEW_PASS",
            "PALETTE_ISOLATION_EFFECT_PASS",
            "PALETTE_ISOLATION_RETUNE_PASS",
            "PALETTE_ISOLATION_HOLD_PASS",
            "PALETTE_ISOLATION_VISUAL_CONTROLS_PASS",
            "PALETTE_ISOLATION_SPECTRUM_FOCUS_PASS",
            "PALETTE_ISOLATION_HOST_IDLE_PREVIEW_PASS",
            "PALETTE_ISOLATION_TARGET_IDLE_PREVIEW=untested",
            "PALETTE_ISOLATION_PASS",
        ):
            if stamp not in text:
                raise SystemExit(f"missing {stamp}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
