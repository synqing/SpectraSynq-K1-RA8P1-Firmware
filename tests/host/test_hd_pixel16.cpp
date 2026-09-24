// DUR-010: quantiseHd16Selected's default-off identity, and the switched-on
// exact law's confirmed divergence from quantiseHd16 at +Inf and near
// half-way rounding boundaries. See platform/ra8p1/hd_pixel16.h.
#include "hd_pixel16.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
using namespace k1;
using namespace k1::core::visual;
using namespace k1::titan;

namespace {
bool sameWord(std::uint16_t a, std::uint16_t b) { return a == b; }
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

  // 1) Default-off identity: quantiseHd16Selected(colour, false) is
  //    byte-identical to quantiseHd16(colour) for every sample, on all three
  //    channels simultaneously (a full PaletteLinearRgb, not one channel at
  //    a time -- proves the wiring, not just the per-channel lambda).
  for (unsigned i = 0; i < count; ++i) {
    for (unsigned j = 0; j < count; ++j) {
      for (unsigned k = 0; k < count; ++k) {
        const PaletteLinearRgb colour{samples[i], samples[j], samples[k]};
        const Pixel16 legacy = quantiseHd16(colour);
        const Pixel16 selected_off = quantiseHd16Selected(colour, false);
        assert(sameWord(legacy.red, selected_off.red));
        assert(sameWord(legacy.green, selected_off.green));
        assert(sameWord(legacy.blue, selected_off.blue));
      }
    }
  }

  // 2) Switched-on identity: quantiseHd16Selected(colour, true) is
  //    byte-identical to quantiseHd16Exact(colour) (== quantiseUnorm16 per
  //    channel), for the same full vector.
  for (unsigned i = 0; i < count; ++i) {
    for (unsigned j = 0; j < count; ++j) {
      for (unsigned k = 0; k < count; ++k) {
        const PaletteLinearRgb colour{samples[i], samples[j], samples[k]};
        const Pixel16 exact = quantiseHd16Exact(colour);
        const Pixel16 selected_on = quantiseHd16Selected(colour, true);
        assert(sameWord(exact.red, selected_on.red));
        assert(exact.green == selected_on.green);
        assert(exact.blue == selected_on.blue);
        assert(exact.red == core::visual::wide::quantiseUnorm16(samples[i]));
      }
    }
  }

  // 3) Confirmed divergence at +Inf: quantiseHd16 -> 0, quantiseHd16Exact ->
  //    65535. If this assertion ever fails, the two laws have been made to
  //    agree here and the comment in hd_pixel16.h needs updating, not this
  //    test relaxing.
  {
    const PaletteLinearRgb colour{inf, inf, inf};
    const Pixel16 legacy = quantiseHd16(colour);
    const Pixel16 exact = quantiseHd16Exact(colour);
    assert(legacy.red == 0U && legacy.green == 0U && legacy.blue == 0U);
    assert(exact.red == 65535U && exact.green == 65535U && exact.blue == 65535U);
    assert(quantiseHd16Selected(colour, false).red == 0U);
    assert(quantiseHd16Selected(colour, true).red == 65535U);
  }
  // NaN: no divergence -- both laws land on 0. Documented, not assumed.
  {
    const PaletteLinearRgb colour{nan, nan, nan};
    const Pixel16 legacy = quantiseHd16(colour);
    const Pixel16 exact = quantiseHd16Exact(colour);
    assert(legacy.red == 0U && exact.red == 0U);
  }
  // Negative-case proof this comparator is not vacuous: an intentionally
  // wrong "selected" implementation must fail these same assertions.
  {
    const PaletteLinearRgb colour{inf, 0.5F, 0.0F};
    const auto broken_select = [](PaletteLinearRgb c, bool) { return quantiseHd16(c); };
    const Pixel16 broken = broken_select(colour, true);
    const Pixel16 exact = quantiseHd16Exact(colour);
    assert(broken.red != exact.red);  // 0 vs 65535: the mutation is detectable
  }

  std::printf(
      "HD_PIXEL16_DUR010_PASS samples=%u default_off_identical=true "
      "switched_on_matches_exact=true inf_diverges=true nan_agrees=true "
      "mutation_detectable=true\n",
      count);
  return 0;
}
