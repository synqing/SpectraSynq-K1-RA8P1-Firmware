// Lane P6 C5b (TOLERANCE-FROZEN-P6-C5b.json): discriminating hue check on overlapping
// v9 deposits of two distinct palette positions, through the renderer's own v9 release,
// deposit and output code (probeV9Deposit). Build with K1_INHOUSE_MAPPING_V9 and
// K1_INHOUSE_V9_HOST_VARIANTS.
//   p6_c5b_hue_probe none|channel_max|rgb_blend [asis|lifted|range|mapped]  -> one JSON line per board
// Iteration 4: the oracle encodes the declared variant stops / fold independently
// (P6-IT4-PALETTE-VARIANTS-FROZEN.json); it does not call the renderer's sampler.
#include "core/visual/inhouse_waveform.h"
#include "core/visual/product_palette.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

using namespace k1::core::visual;

namespace {
std::string g_palette = "asis";
constexpr ProductPaletteStop kOracleLifted32[] = {{0, {150, 0, 0}}, {42, {200, 0, 0}}, {84, {255, 0, 0}},
    {127, {255, 0, 45}}, {170, {255, 0, 255}}, {212, {255, 55, 45}}, {255, {255, 255, 0}}};
constexpr ProductPaletteStop kOracleLifted39[] = {{0, {150, 0, 125}}, {32, {190, 0, 131}}, {64, {220, 0, 121}},
    {96, {220, 0, 70}}, {128, {255, 36, 24}}, {160, {255, 92, 0}}, {192, {255, 145, 0}},
    {218, {255, 190, 40}}, {240, {255, 70, 120}}, {255, {150, 0, 125}}};

// The renderer's sampler is internal; this mirrors its stop interpolation for the oracle.
void paletteColour(std::uint16_t id, float x, float out[3]) {
  const ProductPaletteStop* stops = productPalette(id).stops;
  unsigned count = productPalette(id).stop_count;
  if (g_palette == "lifted" && (id == 31U || id == 38U)) {
    stops = id == 31U ? kOracleLifted32 : kOracleLifted39;
    count = id == 31U ? 7U : 10U;
  }
  if (g_palette == "range" && (id == 31U || id == 38U)) {
    const float lo = id == 31U ? 0.3294F : 0.2510F, hi = id == 31U ? 1.0F : 0.9412F;
    x = std::min(lo + (hi - lo) * (1.0F - std::fabs(2.0F * x - 1.0F)), 65535.0F / 65536.0F);
  }
  struct { const ProductPaletteStop* stops; unsigned stop_count; } p{stops, count};
  unsigned i = 0;
  while (i + 1U < p.stop_count && p.stops[i + 1U].position / 255.0F < x) ++i;
  if (i + 1U >= p.stop_count || x <= p.stops[0].position / 255.0F) {
    const auto& s = p.stops[x <= p.stops[0].position / 255.0F ? 0U : p.stop_count - 1U].colour;
    out[0] = s.red; out[1] = s.green; out[2] = s.blue;
  } else {
    const auto& a = p.stops[i].colour; const auto& b = p.stops[i + 1U].colour;
    const float pos = p.stops[i].position / 255.0F, span = p.stops[i + 1U].position / 255.0F - pos;
    const float t = span > 1e-6F ? std::clamp((x - pos) / span, 0.0F, 1.0F) : 0.0F;
    out[0] = a.red + (b.red - a.red) * t; out[1] = a.green + (b.green - a.green) * t; out[2] = a.blue + (b.blue - a.blue) * t;
  }
  const float m = std::max(out[0], std::max(out[1], out[2]));
  for (int k = 0; k < 3; ++k) out[k] = m > 0.0F ? out[k] / m : 0.0F;
}

float distance(const Pixel16& px, const float ref[3]) {
  const float c[3] = {px.red / 65535.0F, px.green / 65535.0F, px.blue / 65535.0F};
  const float m = std::max(c[0], std::max(c[1], c[2]));
  float d = 0.0F;
  for (int k = 0; k < 3; ++k) d = std::max(d, std::fabs(c[k] / m - ref[k]));
  return d;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 && argc != 3) return 2;
  const std::string name = argv[1];
  if (argc == 3) g_palette = argv[2];
  const InhouseV9Palette palette = g_palette == "asis" ? InhouseV9Palette::kAsIs
      : g_palette == "lifted" ? InhouseV9Palette::kLiftedLowEnd
      : g_palette == "range" ? InhouseV9Palette::kRestrictedRange
      : g_palette == "mapped" ? InhouseV9Palette::kPerPixelMapped : static_cast<InhouseV9Palette>(255);
  const InhouseV9Fault fault = name == "none" ? InhouseV9Fault::kNone
      : name == "channel_max" ? InhouseV9Fault::kPerChannelMax
      : name == "rgb_blend" ? InhouseV9Fault::kRgbBlend : static_cast<InhouseV9Fault>(255);
  if (static_cast<unsigned>(fault) > 3U) return 2;
  static InhouseWaveformRenderer renderer;
  static Pixel16 frame[160];
  const float levels[3] = {0.5F, 0.8F, 1.0F};
  for (unsigned board = 0; board < 2; ++board) {
    InhouseWaveformControls controls{};
    controls.mode = board == 0 ? 32U : 18U;
    controls.palette_id = board == 0 ? 31U : 38U;
    controls.mirror = true;
    controls.saturation = 1.0F;
    unsigned long scenarios = 0, lit = 0, off = 0, scenarios_off = 0;
    for (int ia = 1; ia <= 9; ++ia) for (int ib = 1; ib <= 9; ++ib) {
      if (ia == ib) continue;
      const float a = ia / 10.0F, b = ib / 10.0F;
      float ca[3], cb[3];
      paletteColour(controls.palette_id, a, ca);
      paletteColour(controls.palette_id, b, cb);
      for (float level : levels) for (int d = 0; d <= 3; ++d) {
        renderer.reset();
        if (!renderer.setV9HostConfig(kInhouseV9Variant, fault, kInhouseV9Gain32, kInhouseV9Gain18)) return 4;
        if (!renderer.setV9HostPalette(palette)) return 4;
        if (!renderer.probeV9Deposit(controls, a, 1.0F, 120.0F, 0.0F, frame)) return 5;
        if (!renderer.probeV9Deposit(controls, b, level, 120.0F + d, 0.010F, frame)) return 5;
        ++scenarios;
        unsigned long here = 0;
        for (unsigned i = 0; i < 160U; ++i) {
          const auto& px = frame[i];
          if (g_palette == "mapped") {
            // PAL extended oracle: pixel i shows HD(a + d_i) or HD(b + d_i), d_i its mapped place.
            const unsigned src = i < 80U ? 159U - i : i;
            const float d = static_cast<float>(src - 80U) / 80.0F;
            const auto wrap1 = [](float v) { return v - std::floor(v); };
            paletteColour(controls.palette_id, wrap1(a + d), ca);
            paletteColour(controls.palette_id, wrap1(b + d), cb);
          }
          if (std::max(px.red, std::max(px.green, px.blue)) < 655U) continue;
          ++lit;
          if (std::min(distance(px, ca), distance(px, cb)) > 0.02F) { ++off; ++here; }
        }
        scenarios_off += here ? 1U : 0U;
      }
    }
    std::printf("{\"palette\":\"%s\",\"fault\":\"%s\",\"board\":%u,\"mode\":%u,\"palette_id\":%u,\"scenarios\":%lu,"
                "\"lit_px\":%lu,\"off_px\":%lu,\"scenarios_with_off\":%lu}\n",
                g_palette.c_str(), name.c_str(), board, static_cast<unsigned>(controls.mode), static_cast<unsigned>(controls.palette_id),
                scenarios, lit, off, scenarios_off);
  }
  return 0;
}
