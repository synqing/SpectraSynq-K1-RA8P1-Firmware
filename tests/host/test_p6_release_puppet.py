"""The release puppet (TEST IMAGE ONLY) must draw what was scored.

K1_V9_RELEASE_PUPPET lets the host arm the R01+R03 release on the device by
setting board 0 speed to 0.501; the drop lands 6 s after the arm. The device
runs core/visual/v9_release_puppet.h v9PuppetStep, and so does the host probe's
"puppet@T" mode. These tests require:

1. Armed at T, the puppet draws what the scored host run "r01+r03@(T + 6)" draws
   on the BOOM file (both boards). It cannot be bit-exact: the device clock is
   seconds since the arm (6.0 at the drop) and the scored clock is file time
   (55.77), so float rounding moves the front's edge by a hair. First run,
   15 Oct: 6,774 of 2,655,680 pixels differ, all inside the release window
   55.77-58.45 s, median 1 count, max 22 of 65,535. The bound below (64
   counts, 0.1 % of full scale, a tenth of a 1 % luminance step) was set after
   seeing that 22. It is not fitted to pass: a 10 ms early or late arm differs
   by about 53,500 counts on about 25,000 pixels.
2. Never armed, the puppet build draws exactly what the plain scored host argv
   draws, so the code is inert until the host speaks.
3. Planted: arms 10 ms late and 10 ms early must break the bound.
4. The code parser accepts 0.501 only, and arms only on a rising edge.
"""
from __future__ import annotations

import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/host"))
sys.path.insert(0, str(ROOT / "scripts"))
import p6_effect_mapping as mapping  # noqa: E402
from test_p6_product_path_p import HOST_V9, _diff, _song_files  # noqa: E402

DROP = 55.77   # on the 10 ms render grid; the measured BOOM onset is 55.766
ARM = 49.77
BOUND = 64     # counts of 65,535
WINDOW = (DROP, DROP + 3.0)


def _check(a: bytes, b: bytes) -> dict:
    x = np.frombuffer(a, dtype="<u2").reshape(-1, 2, 160, 3).astype(np.int32)
    y = np.frombuffer(b, dtype="<u2").reshape(-1, 2, 160, 3).astype(np.int32)
    assert x.shape == y.shape
    d = np.abs(x - y).max(axis=-1)
    frames = np.where(d.max(axis=(1, 2)) > 0)[0]
    t = frames / 100.0
    inside = bool(len(frames) == 0 or (t.min() >= WINDOW[0] - 1e-9 and t.max() <= WINDOW[1] + 1e-9))
    return {"max": int(d.max()), "pixels": int((d > 0).sum()), "inside_window": inside,
            "ok": bool(d.max() <= BOUND and inside)}


def _run(exe: Path, tmp: Path, song: Path, tag: str, extra: list[str]) -> bytes:
    frames = tmp / f"{tag}.bin"
    mapping.run_probe(exe, song, tmp / f"{tag}.csv", frames, HOST_V9 + extra)
    return frames.read_bytes()


def test_puppet_matches_scored_release_and_is_inert_unarmed(tmp_path):
    song = _song_files()["boom_source"]
    host = tmp_path / "host"
    mapping.build("v9", host)
    scored = _run(host, tmp_path, song, "scored", [f"r01+r03@{DROP:.2f}"])
    puppet = _run(host, tmp_path, song, "puppet", [f"puppet@{ARM:.2f}"])
    armed = _check(puppet, scored)
    print("armed:", armed)
    assert armed["ok"], armed

    plain = _run(host, tmp_path, song, "plain", [])
    unarmed = _run(host, tmp_path, song, "unarmed", ["puppet@999"])
    report = _diff(unarmed, plain)
    print("unarmed:", report)
    assert "mismatches=0" in report, report

    for tag, at in (("late", ARM + 0.01), ("early", ARM - 0.01)):
        planted = _check(_run(host, tmp_path, song, tag, [f"puppet@{at:.2f}"]), scored)
        print(f"planted {tag} arm:", planted)
        assert not planted["ok"], f"a 10 ms {tag} arm was not seen"


def test_puppet_code_parser(tmp_path):
    src = tmp_path / "parser.cpp"
    src.write_text(r'''
#include "core/visual/v9_release_puppet.h"
#include <cstdio>
using namespace k1::core::visual;
int main() {
  int bad = 0;
  bad += v9PuppetCode(0.5F) != 0;
  bad += v9PuppetCode(0.501F) != 1;
  bad += v9PuppetCode(0.5012F) != 0;
  bad += v9PuppetCode(0.55F) != 0;
  bad += v9PuppetCode(0.502F) != 0;
  V9PuppetState s{};
  bad += !v9PuppetUpdate(s, 0.501F, 100U);
  bad += v9PuppetUpdate(s, 0.501F, 200U);      // held: no re-arm
  bad += s.arm_us != 100U;
  bad += v9PuppetUpdate(s, 0.5F, 300U);
  bad += !v9PuppetUpdate(s, 0.501F, 400U);     // a new rising edge re-arms
  bad += s.arm_us != 400U;
  bad += v9PuppetClock(V9PuppetState{}, 5U) > -999.0F;
  std::printf("bad=%d\n", bad);
  return bad;
}
''')
    import subprocess
    exe = tmp_path / "parser"
    subprocess.run(["clang++", "-std=c++17", "-O2", "-DK1_EMOTISCOPE_PORT=1", "-DK1_INHOUSE_MAPPING_V9=1",
                    "-DK1_INHOUSE_V9_HOST_VARIANTS=1", f"-I{ROOT}/platform/ra8p1", f"-I{ROOT}/src",
                    f"-I{ROOT}/src/k1", str(src), "-o", str(exe)], check=True, cwd=ROOT)
    out = subprocess.run([str(exe)], capture_output=True, text=True)
    assert out.returncode == 0, out.stdout + out.stderr
