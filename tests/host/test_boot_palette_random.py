"""Random start-up palettes (Captain, 15 Oct 2026): the two edges never share a colour family.

1. The generated pair header matches the frozen catalogue (generator --check).
2. Every pair is one eligible warm and one eligible cool palette, >= 120 degrees apart,
   checked from the palette stops. Planted: a same-family pair (37, 39, both warm,
   the old preset pair that looked alike) must be rejected by that audit.
3. The C++ picker: over 20,000 seeds every pair is used, the warm palette goes to
   each edge about half the time, the two picked palettes always come from one pair,
   and the same seed always gives the same pick; different microphone noise gives
   different picks.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import boot_palette_pairs as gen  # noqa: E402


def _audit(pairs: list[tuple[int, int]]) -> list[str]:
    desc = {d["value"]: d for d in (gen.describe(p) for p in gen.palettes())}
    bad = []
    for warm, cool in pairs:
        w, c = desc[warm], desc[cool]
        gap = abs((w["hue"] - c["hue"] + 180) % 360 - 180)
        if not (w["eligible"] and w["family"] == "warm" and c["eligible"] and c["family"] == "cool" and gap >= 120):
            bad.append(f"{warm}/{cool}")
    return bad


def _header_pairs() -> list[tuple[int, int]]:
    import re
    text = (ROOT / "src/k1/core/visual/boot_palette_pairs.h").read_text()
    return [(int(a), int(b)) for a, b in re.findall(r"\{(\d+)U, (\d+)U\}", text)]


def test_header_is_current():
    assert subprocess.run([sys.executable, str(ROOT / "scripts/boot_palette_pairs.py"), "--check"],
                          capture_output=True, text=True).returncode == 0


def test_every_pair_is_warm_plus_cool_and_plant_is_caught():
    pairs = _header_pairs()
    assert len(pairs) >= 20, len(pairs)
    assert _audit(pairs) == []
    assert _audit(pairs + [(37, 39)]) == ["37/39"], "planted same-family pair not caught"


def test_picker(tmp_path):
    n = len(_header_pairs())
    src = tmp_path / "pick.cpp"
    src.write_text(r'''
#include "core/visual/boot_palette_random.h"
#include <cstdio>
#include <vector>
using namespace k1::core::visual;
int main() {
  const unsigned n = kBootPalettePairs.size();
  std::vector<unsigned> used(n, 0);
  unsigned warm_upper = 0, bad = 0, total = 20000;
  std::uint32_t s = 12345U;
  for (unsigned i = 0; i < total; ++i) {
    s = s * 1664525U + 1013904223U;
    const auto a = bootPalettePick(s), b = bootPalettePick(s);
    if (a.lower != b.lower || a.upper != b.upper) ++bad;          // deterministic
    bool found = false;
    for (unsigned k = 0; k < n; ++k) {
      const auto& p = kBootPalettePairs[k];
      if ((a.lower == p.warm && a.upper == p.cool) || (a.lower == p.cool && a.upper == p.warm)) {
        ++used[k]; found = true; if (a.upper == p.warm) ++warm_upper; break;
      }
    }
    if (!found) ++bad;
  }
  unsigned unused = 0; for (unsigned u : used) if (!u) ++unused;
  // Two different noise traces through the real mixer give different picks.
  BootPaletteSeed x{}, y{};
  for (unsigned f = 0; f < kBootPaletteFrames; ++f) {
    for (unsigned k = 0; k < 80; ++k) { bootPaletteMixFloat(x, 1e-4F * float((f * 80 + k) % 97)); bootPaletteMixFloat(y, 1e-4F * float((f * 80 + k) % 89)); }
  }
  const auto px = bootPalettePick(x.hash), py = bootPalettePick(y.hash);
  const bool differ = px.lower != py.lower || px.upper != py.upper;
  std::printf("n=%u unused=%u bad=%u warm_upper=%.3f differ=%d\n", n, unused, bad, double(warm_upper) / total, differ);
  return (unused == 0 && bad == 0 && warm_upper > 0.45 * total && warm_upper < 0.55 * total && differ) ? 0 : 1;
}
''')
    exe = tmp_path / "pick"
    subprocess.run(["clang++", "-std=c++17", "-O2", f"-I{ROOT}/src", f"-I{ROOT}/src/k1", str(src), "-o", str(exe)],
                   check=True)
    out = subprocess.run([str(exe)], capture_output=True, text=True)
    print(out.stdout)
    assert out.returncode == 0, out.stdout
    assert f"n={n}" in out.stdout
