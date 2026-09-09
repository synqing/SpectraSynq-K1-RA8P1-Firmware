#include "core/visual/product_catalogue.h"

#include <cstddef>

namespace k1::core::visual {
namespace {

// Frozen from legacy system/config_types.h at source commit
// 71520ebd312558ae99c48a521e144a268f777192. Ordinals are append-only.
constexpr std::array<ProductModeDescriptor, kProductModeCount> kModes{{
    {0U, "GDFT", false},
    {1U, "GDFT_CHROMAGRAM", false},
    {2U, "GDFT_CHROMAGRAM_DOTS", false},
    {3U, "BLOOM", true},
    {4U, "VU_DOT", false},
    {5U, "KALEIDOSCOPE", false},
    {6U, "QUANTUM_COLLAPSE", false},
    {7U, "WAVEFORM_FAST", true},
    {8U, "WAVEFORM", true},
    {9U, "BLOOM_FAST", true},
    {10U, "VU", false},
    {11U, "WAVEFORM_HYBRID", true},
    {12U, "AURORA", true},
    {13U, "COMET", true},
    {14U, "SPECTRUM_RIVER", true},
    {15U, "SPECTRUM_RIVER_V2", true},
    {16U, "EMBER", true},
    {17U, "EMBER_V2", false},
    {18U, "WAVEFORM_TEMPO", true},
    {19U, "TEMPO_RIVER", true},
    {20U, "TEMPO_COMET", true},
    {21U, "DENSE_FORGE", true},
    {22U, "SNAPWAVE", true},
    {23U, "PULSE_PRISM", true},
    {24U, "DENSE_FORGE_CHORD", true},
    {25U, "CHROMA_CONSTELLATION", true},
    {26U, "PERCUSSION_BURST", true},
    {27U, "TEMPO_COMET_ANTICIPATE", true},
    {28U, "RIVER_SURGE", true},
    {29U, "TEMPO_RIVER_WALK", true},
    {30U, "BEAT_PULSE", false},
    {31U, "BLOOM_BT", false},
    {32U, "WAVEFORM_HYBRID_K1", true},
    {33U, "WFHYB_K1_FLUX", false},
    {34U, "WFHYB_K1_NOTE", false},
    {35U, "WFHYB_K1_WIDE", false},
    {36U, "WFHYB_K1_SUM", false},
    {37U, "WFHYB_K1_STEP", false},
}};

static_assert(kModes.size() == kProductModeCount,
              "product mode catalogue must preserve all stable ordinals");

}  // namespace

const std::array<ProductModeDescriptor, kProductModeCount>&
productModeCatalogue() noexcept {
  return kModes;
}

const ProductModeDescriptor* productMode(const std::uint16_t id) noexcept {
  return id < kModes.size() ? &kModes[id] : nullptr;
}

bool productModeEnabled(const std::uint16_t id) noexcept {
  const ProductModeDescriptor* const descriptor = productMode(id);
  return descriptor != nullptr && descriptor->product_enabled;
}

std::uint16_t sanitiseProductMode(const std::uint16_t id) noexcept {
  if (id >= kModes.size()) {
    return kDefaultProductModeId;
  }
  if (productModeEnabled(id)) {
    return id;
  }
  for (std::size_t offset = 1U; offset < kModes.size(); ++offset) {
    const std::size_t candidate = (id + offset) % kModes.size();
    if (kModes[candidate].product_enabled) {
      return static_cast<std::uint16_t>(candidate);
    }
  }
  return id;
}

std::uint16_t sanitiseProductPalette(const std::uint16_t id) noexcept {
  return id < kProductPaletteCount ? id : kDefaultProductPaletteId;
}

}  // namespace k1::core::visual
