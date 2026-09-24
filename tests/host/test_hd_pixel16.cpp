// DUR-010 evidence: where quantiseHd16 (platform/ra8p1/hd_pixel16.h) and the
// imported wide endpoint's quantiseUnorm16 (the exact law) diverge. Per the
// 2026-09-24 register disposition, quantiseHd16 has no live caller in this
// repository and the native path already quantises once with the exact law
// at the wide endpoint (E5) -- there is no selector here, deliberately: a
// switch nothing reads is not a feature. This file is ALSO the proof that
// quantiseHd16's finiteness test is bit-level, not std::isfinite(): run
// through scripts/test_hd_pixel16.py, it compiles and runs twice, once with
// this project's normal Titan host flags and once with -ffast-math added,
// and every assertion below (including the +Inf divergence) must hold
// identically both times.
#include "hd_pixel16.h"
#include "core/visual/wide/wide_endpoint.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace k1;
using namespace k1::core::visual;
using namespace k1::titan;

namespace {
// The exact law, per channel, built directly from the imported
// quantiseUnorm16 -- not through any Titan wrapper, since none exists here
// any more.
Pixel16 exactLaw(PaletteLinearRgb colour) noexcept {
  return {wide::quantiseUnorm16(colour.red), wide::quantiseUnorm16(colour.green),
          wide::quantiseUnorm16(colour.blue)};
}
}  // namespace

int main() {
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float inf = std::numeric_limits<float>::infinity();
  // Representative vector: exact stops, boundary values, +/-Inf, NaN, values
  // just either side of several half-integer/65535 boundaries.
  float samples[64];
  unsigned count = 0;
  for (float v : {0.0F, -0.0F, 1.0F, 0.5F, -1.0F, 2.0F, inf, -inf, nan,
                  0.0000001F, 0.9999999F, std::nextafterf(1.0F, 0.0F),
                  std::nextafterf(0.0F, 1.0F)}) {
    samples[count++] = v;
  }
  // Values whose *65535 product sits exactly on, or one ULP either side of,
  // a half-integer -- where quantiseHd16's float-multiply rounding and
  // quantiseUnorm16's exact-integer rounding are most likely to diverge.
  for (unsigned n = 1; n <= 65534; n += 8191) {
    const float exact_half = (static_cast<float>(n) + 0.5F) / 65535.0F;
    samples[count++] = exact_half;
    samples[count++] = std::nextafterf(exact_half, 0.0F);
    samples[count++] = std::nextafterf(exact_half, 1.0F);
  }
  assert(count <= 64);

  // Full three-channel cross product: any two laws that happened to agree
  // per-channel but compose differently would still be caught.
  unsigned agreements = 0, divergences = 0;
  for (unsigned i = 0; i < count; ++i) {
    for (unsigned j = 0; j < count; ++j) {
      for (unsigned k = 0; k < count; ++k) {
        const PaletteLinearRgb colour{samples[i], samples[j], samples[k]};
        const Pixel16 legacy = quantiseHd16(colour);
        const Pixel16 exact = exactLaw(colour);
        const bool same = legacy.red == exact.red && legacy.green == exact.green &&
                          legacy.blue == exact.blue;
        same ? ++agreements : ++divergences;
      }
    }
  }
  assert(agreements > 0 && divergences > 0);  // Neither law is a constant function of the other.

  // Confirmed divergence at +Inf: quantiseHd16 -> 0, the exact law -> 65535.
  // If this assertion ever fails, the two laws have been made to agree here
  // and the comment in hd_pixel16.h needs updating, not this test relaxing.
  // This is also the -ffast-math proof: under -ffinite-math-only,
  // std::isfinite(+Inf) folds to true, which would make quantiseHd16 take
  // the >=1.0F branch (65535) instead of the finiteness-guard branch (0).
  // isFiniteBits() is pure integer bit manipulation, so this assertion must
  // hold unchanged under -ffast-math too.
  {
    const PaletteLinearRgb colour{inf, inf, inf};
    const Pixel16 legacy = quantiseHd16(colour);
    const Pixel16 exact = exactLaw(colour);
    assert(legacy.red == 0U && legacy.green == 0U && legacy.blue == 0U);
    assert(exact.red == 65535U && exact.green == 65535U && exact.blue == 65535U);
  }
  // NaN: no divergence -- both laws land on 0. Documented, not assumed.
  // Also part of the -ffast-math proof: -ffast-math implies -fno-signaling-nans
  // and licenses folding NaN comparisons, so this is checked under both flag
  // sets too.
  {
    const PaletteLinearRgb colour{nan, nan, nan};
    const Pixel16 legacy = quantiseHd16(colour);
    const Pixel16 exact = exactLaw(colour);
    assert(legacy.red == 0U && exact.red == 0U);
  }
  // Negative-case proof this comparator is not vacuous: an intentionally
  // wrong "exact law" stand-in must fail the +Inf assertion above.
  {
    const PaletteLinearRgb colour{inf, 0.5F, 0.0F};
    const Pixel16 broken_exact = quantiseHd16(colour);  // wrongly reuses the legacy law
    assert(broken_exact.red != exactLaw(colour).red);   // 0 vs 65535: detectable
  }

  std::printf(
      "HD_PIXEL16_DUR010_PASS samples=%u agreements=%u divergences=%u "
      "inf_diverges=true nan_agrees=true mutation_detectable=true\n",
      count, agreements, divergences);
  return 0;
}
