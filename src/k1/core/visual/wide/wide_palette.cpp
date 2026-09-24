#include "core/visual/wide/wide_palette.h"

#include <cmath>

#include "core/visual/product_palette.h"

namespace k1::core::visual::wide {
namespace {

bool finiteIn(const float value, const float low, const float high) noexcept {
  return std::isfinite(value) && value >= low && value <= high;
}

// Same law as the legacy finiteClamp01 used by sampleProductPaletteHd.
float legacyClamp01(const float value) noexcept {
  if (!std::isfinite(value) || value <= 0.0F) {
    return 0.0F;
  }
  return value >= 1.0F ? 1.0F : value;
}

LegacyPaletteCodeV1 stopCode(const ProductPaletteStop& stop) noexcept {
  return {static_cast<float>(stop.colour.red) / 255.0F,
          static_cast<float>(stop.colour.green) / 255.0F,
          static_cast<float>(stop.colour.blue) / 255.0F};
}

struct MixEntry final {
  std::uint16_t palette_id;
  float weight;
};

void addToMix(MixEntry (&mix)[kPaletteContributorCapacity + 1U],
              std::size_t& count, const std::uint16_t palette_id,
              const float weight) noexcept {
  if (!(weight > 0.0F)) {
    return;
  }
  for (std::size_t index = 0U; index < count; ++index) {
    if (mix[index].palette_id == palette_id) {
      mix[index].weight += weight;
      return;
    }
  }
  if (count < kPaletteContributorCapacity + 1U) {
    mix[count] = {palette_id, weight};
    ++count;
  }
}

}  // namespace

PaletteWindowValidationV1 validatePaletteWindowV1(
    const PaletteWindowV1& window) noexcept {
  if (!finiteIn(window.position, 0.0F, 1.0F)) {
    return PaletteWindowValidationV1::kPosition;
  }
  if (!finiteIn(window.span, 0.0F, 1.0F)) {
    return PaletteWindowValidationV1::kSpan;
  }
  if (!finiteIn(window.travel_depth, 0.0F, 1.0F)) {
    return PaletteWindowValidationV1::kTravelDepth;
  }
  return PaletteWindowValidationV1::kValid;
}

float paletteTravelOffsetV1(const float travel_depth, float travel_q,
                            StageBoundaryCountersV1& counters) noexcept {
  if (!std::isfinite(travel_q)) {
    ++counters.nonfinite;
    travel_q = 0.0F;
  } else if (travel_q < -1.0F) {
    ++counters.overrange;
    travel_q = -1.0F;
  } else if (travel_q > 1.0F) {
    ++counters.overrange;
    travel_q = 1.0F;
  }
  if (travel_depth == 0.0F || travel_q == 0.0F) {
    return 0.0F;
  }
  return kPaletteTravelExcursionV1 * travel_depth * travel_q;
}

float paletteWindowCoordinateV1(const PaletteWindowV1& window, const float u,
                                const float travel_offset) noexcept {
  const float coordinate =
      window.position + window.span * (u - 0.5F) + travel_offset;
  if (!(coordinate > 0.0F)) {
    return 0.0F;
  }
  return coordinate < 1.0F ? coordinate : 1.0F;
}

LegacyPaletteCodeV1 samplePaletteCodeClampedV1(const std::uint16_t palette_id,
                                               const float coordinate) noexcept {
  const ProductPaletteDescriptor& palette = productPalette(palette_id);
  if (palette.stop_count == 0U || palette.stops == nullptr) {
    return {};
  }
  float phase = coordinate;
  if (!std::isfinite(phase) || phase <= 0.0F) {
    phase = 0.0F;
  } else if (phase > 1.0F) {
    phase = 1.0F;
  }
  // Stop search and interpolation follow sampleProductPaletteHd() operation
  // for operation so that the colour law is identical on [0, 1).
  std::uint8_t first = 0U;
  while (static_cast<std::uint8_t>(first + 1U) < palette.stop_count &&
         static_cast<float>(palette.stops[first + 1U].position) / 255.0F <
             phase) {
    ++first;
  }
  if (first + 1U >= palette.stop_count) {
    return stopCode(palette.stops[palette.stop_count - 1U]);
  }
  const ProductPaletteStop& selected = palette.stops[first];
  if (phase <= static_cast<float>(palette.stops[0].position) / 255.0F) {
    return stopCode(selected);
  }
  const ProductPaletteStop& next = palette.stops[first + 1U];
  const float first_position = static_cast<float>(selected.position) / 255.0F;
  const float next_position = static_cast<float>(next.position) / 255.0F;
  const float span = next_position - first_position;
  float amount = span > 1.0e-6F ? (phase - first_position) / span : 0.0F;
  amount = legacyClamp01(amount);
  return {(static_cast<float>(selected.colour.red) +
           (static_cast<float>(next.colour.red) -
            static_cast<float>(selected.colour.red)) * amount) / 255.0F,
          (static_cast<float>(selected.colour.green) +
           (static_cast<float>(next.colour.green) -
            static_cast<float>(selected.colour.green)) * amount) / 255.0F,
          (static_cast<float>(selected.colour.blue) +
           (static_cast<float>(next.colour.blue) -
            static_cast<float>(selected.colour.blue)) * amount) / 255.0F};
}

void initialisePaletteTransitionV1(PaletteTransitionStateV1& state,
                                   const std::uint16_t palette_id) noexcept {
  state = PaletteTransitionStateV1{};
  state.target = palette_id;
  state.progress = 1.0F;
}

bool paletteTransitionActiveV1(const PaletteTransitionStateV1& state) noexcept {
  return state.progress < 1.0F && state.from_count > 0U;
}

PaletteSelectResultV1 selectPaletteV1(PaletteTransitionStateV1& state,
                                      const std::uint16_t palette_id,
                                      const float duration_s) noexcept {
  if (!(duration_s == 0.0F ||
        finiteIn(duration_s, kPaletteTransitionMinimumSeconds,
                 kPaletteTransitionMaximumSeconds))) {
    return PaletteSelectResultV1::kRejectedDuration;
  }
  ++state.selections;
  if (palette_id == state.target) {
    ++state.repeated_selections;
    return PaletteSelectResultV1::kUnchanged;
  }
  if (duration_s == 0.0F) {
    state.from = {};
    state.from_count = 0U;
    state.target = palette_id;
    state.progress = 1.0F;
    return PaletteSelectResultV1::kCut;
  }
  // Snapshot the resolved mix: the fade starts from what is visible now.
  MixEntry mix[kPaletteContributorCapacity + 1U]{};
  std::size_t count = 0U;
  if (paletteTransitionActiveV1(state)) {
    ++state.interruptions;
    const float remaining = 1.0F - state.progress;
    for (std::size_t index = 0U; index < state.from_count; ++index) {
      addToMix(mix, count, state.from[index].palette_id,
               state.from[index].weight * remaining);
    }
    addToMix(mix, count, state.target, state.progress);
  } else {
    addToMix(mix, count, state.target, 1.0F);
  }
  bool dropped = false;
  while (count > kPaletteContributorCapacity) {
    std::size_t lightest = 0U;
    for (std::size_t index = 1U; index < count; ++index) {
      if (mix[index].weight < mix[lightest].weight) {
        lightest = index;
      }
    }
    for (std::size_t index = lightest + 1U; index < count; ++index) {
      mix[index - 1U] = mix[index];
    }
    --count;
    ++state.merges;
    dropped = true;
  }
  float total = 0.0F;
  for (std::size_t index = 0U; index < count; ++index) {
    total += mix[index].weight;
  }
  state.from = {};
  for (std::size_t index = 0U; index < count; ++index) {
    state.from[index] = {mix[index].palette_id,
                         dropped && total > 0.0F ? mix[index].weight / total
                                                 : mix[index].weight};
  }
  state.from_count = static_cast<std::uint8_t>(count);
  state.target = palette_id;
  state.progress = 0.0F;
  state.duration_s = duration_s;
  return PaletteSelectResultV1::kStarted;
}

void advancePaletteTransitionV1(PaletteTransitionStateV1& state,
                                const float delta_seconds) noexcept {
  if (!paletteTransitionActiveV1(state) || !std::isfinite(delta_seconds) ||
      delta_seconds <= 0.0F) {
    return;
  }
  state.progress += delta_seconds / state.duration_s;
  if (!(state.progress < 1.0F)) {
    state.progress = 1.0F;
    state.from = {};
    state.from_count = 0U;
  }
}

float paletteResolvedWeightV1(const PaletteTransitionStateV1& state,
                              const std::uint16_t palette_id) noexcept {
  if (!paletteTransitionActiveV1(state)) {
    return palette_id == state.target ? 1.0F : 0.0F;
  }
  float weight = palette_id == state.target ? state.progress : 0.0F;
  const float remaining = 1.0F - state.progress;
  for (std::size_t index = 0U; index < state.from_count; ++index) {
    if (state.from[index].palette_id == palette_id) {
      weight += state.from[index].weight * remaining;
    }
  }
  return weight;
}

LegacyPaletteCodeV1 samplePaletteTransitionV1(
    const PaletteTransitionStateV1& state, const float coordinate) noexcept {
  if (!paletteTransitionActiveV1(state)) {
    return samplePaletteCodeClampedV1(state.target, coordinate);
  }
  const LegacyPaletteCodeV1 target =
      samplePaletteCodeClampedV1(state.target, coordinate);
  LegacyPaletteCodeV1 result{target.red * state.progress,
                             target.green * state.progress,
                             target.blue * state.progress};
  const float remaining = 1.0F - state.progress;
  for (std::size_t index = 0U; index < state.from_count; ++index) {
    const float weight = state.from[index].weight * remaining;
    const LegacyPaletteCodeV1 sample =
        samplePaletteCodeClampedV1(state.from[index].palette_id, coordinate);
    result.red += weight * sample.red;
    result.green += weight * sample.green;
    result.blue += weight * sample.blue;
  }
  return result;
}

}  // namespace k1::core::visual::wide
