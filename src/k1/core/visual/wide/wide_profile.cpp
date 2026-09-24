#include "core/visual/wide/wide_profile.h"

#include <cmath>

#include "core/visual/product_catalogue.h"

namespace k1::core::visual::wide {
namespace {

bool finiteIn(const float value, const float low, const float high) noexcept {
  return std::isfinite(value) && value >= low && value <= high;
}

std::size_t distanceOf(const std::size_t pixel) noexcept {
  return pixel >= kCentreRight ? pixel - kCentreRight : kCentreLeft - pixel;
}

bool sameLaw(const MaterialLawV1& left, const MaterialLawV1& right) noexcept {
  return left.velocity_px_s == right.velocity_px_s &&
         left.diffusion_px2_s == right.diffusion_px2_s &&
         left.half_life_s == right.half_life_s;
}

class StageClock final {
 public:
  StageClock(const WideTapSinkV1* const sink,
             WideCostCountersV1& cost) noexcept
      : sink_(sink != nullptr && sink->cycles != nullptr ? sink : nullptr),
        cost_(cost) {
    if (sink_ != nullptr) {
      last_ = sink_->cycles(sink_->context);
    }
  }

  void mark(const RibbonStageV1 stage) noexcept {
    if (sink_ == nullptr) {
      return;
    }
    const std::uint32_t now = sink_->cycles(sink_->context);
    cost_.stage_cycles[static_cast<std::size_t>(stage)] = now - last_;
    last_ = now;
  }

 private:
  const WideTapSinkV1* sink_;
  WideCostCountersV1& cost_;
  std::uint32_t last_ = 0U;
};

void tap(const WideTapSinkV1* const sink, const PixelChannelId channel,
         const WideTapPointV1 point, const WorkingFrame& surface) noexcept {
  if (sink != nullptr && sink->capture != nullptr) {
    sink->capture(sink->context, channel, point, surface);
  }
}

std::uint32_t paletteEvaluations(const PaletteTransitionStateV1& palette) noexcept {
  return paletteTransitionActiveV1(palette) ? 1U + palette.from_count : 1U;
}

PersistenceControlsV1 gatedPersistence(const PersistenceControlsV1& controls,
                                       const bool layer_live) noexcept {
  PersistenceControlsV1 gated = controls;
  gated.enabled = controls.enabled && layer_live;
  return gated;
}

void clearPulses(PulsePoolV1& pool) noexcept {
  for (PulseObjectV1& object : pool.slots) {
    object.live = false;
  }
}

void applyChannelControls(RibbonChannelStateV1& state,
                          const RibbonControlsV1& next,
                          const std::uint64_t effective_us) noexcept {
  const RibbonControlsV1& previous = state.controls;
  if (!sameLaw(previous.accent_law, next.accent_law)) {
    static_cast<void>(
        editMaterialLawV1(state.accent_pool, next.accent_law, effective_us));
  }
  if (next.palette_id != previous.palette_id) {
    static_cast<void>(selectPaletteV1(state.palette, next.palette_id,
                                      next.palette_transition_s));
  }
  if (next.body_meaning != previous.body_meaning) {
    setHistoryMeaningV1(state.body_history, next.body_meaning);
  }
  state.controls = next;
}

bool transactionTimeValid(const RibbonChannelStateV1& state,
                          const std::uint64_t effective_us) noexcept {
  return effective_us >= state.timeline.last_us &&
         effective_us >= state.accent_pool.law_anchor_us;
}

}  // namespace

RibbonValidationV1 validateRibbonControlsV1(
    const RibbonControlsV1& controls) noexcept {
  if (!finiteIn(controls.body_level, 0.0F, kLayerComponentMax)) {
    return RibbonValidationV1::kBodyLevel;
  }
  if (controls.body_meaning != HistoryMeaningV1::kScalarRecolouredNow &&
      controls.body_meaning != HistoryMeaningV1::kDepositedRgb) {
    return RibbonValidationV1::kBodyMeaning;
  }
  if (!finiteIn(controls.body_motion_px_s, 0.0F, kRibbonMotionMaximumPxS)) {
    return RibbonValidationV1::kBodyMotion;
  }
  if (!finiteIn(controls.body_half_life_s, kPersistenceHalfLifeMinimumS,
                kPersistenceHalfLifeMaximumS)) {
    return RibbonValidationV1::kBodyHalfLife;
  }
  if (!finiteIn(controls.rhythm_mix, 0.0F, 1.0F)) {
    return RibbonValidationV1::kRhythmMix;
  }
  if (validateMaterialLawV1(controls.accent_law) !=
      MaterialValidationV1::kValid) {
    return RibbonValidationV1::kAccentLaw;
  }
  if (!finiteIn(controls.accent_width_px, kPulseMinimumWidthPx,
                kPulseMaximumWidthPx)) {
    return RibbonValidationV1::kAccentWidth;
  }
  if (controls.accent_normalisation != PulseNormalisationV1::kPeak &&
      controls.accent_normalisation != PulseNormalisationV1::kIntegrated) {
    return RibbonValidationV1::kAccentNormalisation;
  }
  if (!finiteIn(controls.atmosphere_level, 0.0F, kRibbonAtmosphereMaximum)) {
    return RibbonValidationV1::kAtmosphere;
  }
  for (const PersistenceControlsV1& persistence : controls.persistence) {
    if (validatePersistenceV1(persistence) != PersistenceValidationV1::kValid) {
      return RibbonValidationV1::kPersistence;
    }
  }
  if (controls.palette_id >= kProductPaletteCount) {
    return RibbonValidationV1::kPalette;
  }
  if (validatePaletteWindowV1(controls.window) !=
      PaletteWindowValidationV1::kValid) {
    return RibbonValidationV1::kWindow;
  }
  if (!(controls.palette_transition_s == 0.0F ||
        finiteIn(controls.palette_transition_s,
                 kPaletteTransitionMinimumSeconds,
                 kPaletteTransitionMaximumSeconds))) {
    return RibbonValidationV1::kTransition;
  }
  if (validateColourTreatmentV1(controls.colour) != ColourValidationV1::kValid) {
    return RibbonValidationV1::kColour;
  }
  return RibbonValidationV1::kValid;
}

bool initialiseRibbonChannelV1(RibbonChannelStateV1& state,
                               const PixelChannelId channel,
                               const RibbonControlsV1& controls) noexcept {
  if (!validChannel(channel) ||
      validateRibbonControlsV1(controls) != RibbonValidationV1::kValid) {
    return false;
  }
  state = RibbonChannelStateV1{};
  state.channel = channel;
  state.controls = controls;
  initialisePaletteTransitionV1(state.palette, controls.palette_id);
  state.body_history.meaning = controls.body_meaning;
  resetPulsePoolV1(state.accent_pool, controls.accent_law, 0U);
  state.body_enabled_last = controls.body_enabled;
  state.accent_enabled_last = controls.accent_enabled;
  return true;
}

RibbonFrameResultV1 renderRibbonChannelV1(RibbonChannelStateV1& state,
                                          const RibbonFrameInputV1& input,
                                          const WideTapSinkV1* const taps) noexcept {
  RibbonFrameResultV1 result{};
  StageClock clock(taps, state.cost);
  // S0 time. A rejected timestamp mutates nothing and renders nothing.
  result.step = advanceTimelineV1(state.timeline, input.presentation_us);
  if (!result.step.accepted()) {
    return result;
  }
  const std::uint64_t now = input.presentation_us;
  const float dt = result.step.delta_seconds;
  const RibbonControlsV1& controls = state.controls;
  clock.mark(RibbonStageV1::kTime);

  // S1 palette window table.
  advancePaletteTransitionV1(state.palette, dt);
  const float travel_offset = paletteTravelOffsetV1(
      controls.window.travel_depth, input.travel_q, state.boundary);
  for (std::size_t distance = 0U; distance < kPixelsPerHalf; ++distance) {
    const float u = static_cast<float>(distance) /
                    static_cast<float>(kPixelsPerHalf - 1U);
    const float coordinate =
        paletteWindowCoordinateV1(controls.window, u, travel_offset);
    state.colour_by_distance[distance] =
        legacyCodeAsIntentV1(samplePaletteTransitionV1(state.palette, coordinate));
  }
  state.cost.palette_samples +=
      static_cast<std::uint32_t>(kPixelsPerHalf) * paletteEvaluations(state.palette);
  clock.mark(RibbonStageV1::kPalette);

  // S2 body: typed history transported outward from the centre pair.
  if (controls.body_enabled != state.body_enabled_last) {
    if (!controls.body_enabled) {
      clearHistoryV1(state.body_history);
      state.body_emission_valid = false;
      ++state.body_resets;
    }
    state.body_enabled_last = controls.body_enabled;
  }
  if (controls.body_enabled) {
    const float amplitude =
        sanitiseComponent(input.body_amplitude, kLayerComponentMax,
                          state.boundary) *
        controls.body_level;
    const WorkingRgbF32V1& centre = state.colour_by_distance[0];
    const WorkingRgbF32V1 emission{centre.red * amplitude,
                                   centre.green * amplitude,
                                   centre.blue * amplitude};
    if (!state.body_emission_valid) {
      state.body_previous_amplitude = amplitude;
      state.body_previous_emission = emission;
      state.body_emission_valid = true;
    }
    const float retention = halfLifeRetentionV1(dt, controls.body_half_life_s);
    const float shift = controls.body_motion_px_s * dt;
    ++state.cost.exp2_calls;
    state.cost.exp2_calls += static_cast<std::uint32_t>(std::ceil(shift));
    static_cast<void>(advectBodyOutwardV1(
        state.body_history, shift, retention, emission, amplitude,
        state.body_previous_emission, state.body_previous_amplitude,
        state.layer));
    state.body_previous_amplitude = amplitude;
    state.body_previous_emission = emission;
    renderHistoryLayerV1(state.body_history, state.colour_by_distance,
                         state.layer);
    state.cost.pixel_passes += 3U;
  } else {
    clearFrameV1(state.layer);
  }
  tap(taps, state.channel, WideTapPointV1::kBody, state.layer);
  clock.mark(RibbonStageV1::kBody);

  // S3 body persistence, then S4 body contribution.
  applyRgbPersistenceV1(
      state.layer, state.persistence[0],
      gatedPersistence(controls.persistence[0], controls.body_enabled), dt);
  tap(taps, state.channel, WideTapPointV1::kBodyPersisted, state.layer);
  clearFrameV1(state.output);
  composeAddV1(state.output, state.layer, 1.0F - controls.rhythm_mix);
  state.cost.pixel_passes += 3U;
  clock.mark(RibbonStageV1::kBodyPersistence);

  // S5 accent: analytic pulses.
  if (controls.accent_enabled != state.accent_enabled_last) {
    if (!controls.accent_enabled) {
      clearPulses(state.accent_pool);
      ++state.accent_resets;
    }
    state.accent_enabled_last = controls.accent_enabled;
  }
  clearFrameV1(state.layer);
  const std::size_t spawn_count =
      input.spawn_count < kRibbonMaximumSpawnsPerFrame
          ? input.spawn_count
          : kRibbonMaximumSpawnsPerFrame;
  if (controls.accent_enabled) {
    for (std::size_t index = 0U; index < spawn_count; ++index) {
      const AccentSpawnV1& request = input.spawns[index];
      if (request.event_us > now) {
        ++state.spawns_rejected_future;
        continue;
      }
      PulseSpawnV1 spawn{};
      spawn.event_us = request.event_us;
      spawn.role = request.role;
      spawn.amplitude = request.amplitude;
      spawn.radius_px = 0.0F;
      spawn.width_px = controls.accent_width_px;
      const float u = sanitiseComponent(request.palette_u, 1.0F, state.boundary);
      spawn.colour = legacyCodeAsIntentV1(samplePaletteTransitionV1(
          state.palette,
          paletteWindowCoordinateV1(controls.window, u, travel_offset)));
      state.cost.palette_samples += paletteEvaluations(state.palette);
      static_cast<void>(spawnPulseV1(state.accent_pool, spawn));
    }
    static_cast<void>(retireFinishedPulsesV1(state.accent_pool, now));
    RasterCostV1 raster{};
    rasterisePulsesV1(state.accent_pool, now, controls.accent_normalisation,
                      state.layer, raster);
    state.cost.exp_calls += raster.exp_calls;
    state.cost.objects_rasterised += raster.objects;
  } else {
    state.spawns_ignored += static_cast<std::uint32_t>(spawn_count);
  }
  tap(taps, state.channel, WideTapPointV1::kAccent, state.layer);
  clock.mark(RibbonStageV1::kAccent);

  // S6 accent persistence, then S7 accent contribution.
  applyRgbPersistenceV1(
      state.layer, state.persistence[1],
      gatedPersistence(controls.persistence[1], controls.accent_enabled), dt);
  tap(taps, state.channel, WideTapPointV1::kAccentPersisted, state.layer);
  composeAddV1(state.output, state.layer, controls.rhythm_mix);
  state.cost.pixel_passes += 3U;
  clock.mark(RibbonStageV1::kAccentPersistence);

  // S8 atmosphere: bounded base, suppressed by the quiet-space policy.
  const bool atmosphere_live = controls.atmosphere_enabled && !input.quiet;
  if (atmosphere_live) {
    for (std::size_t pixel = 0U; pixel < kPixelsPerChannel; ++pixel) {
      const WorkingRgbF32V1& colour = state.colour_by_distance[distanceOf(pixel)];
      state.layer[pixel] = {colour.red * controls.atmosphere_level,
                            colour.green * controls.atmosphere_level,
                            colour.blue * controls.atmosphere_level};
    }
  } else {
    clearFrameV1(state.layer);
  }
  applyRgbPersistenceV1(state.layer, state.persistence[2],
                        gatedPersistence(controls.persistence[2], atmosphere_live),
                        dt);
  tap(taps, state.channel, WideTapPointV1::kAtmosphere, state.layer);
  composeAddV1(state.output, state.layer, 1.0F);
  state.cost.pixel_passes += 3U;
  clock.mark(RibbonStageV1::kAtmosphere);

  // S9 accumulator bound.
  boundAccumulatorV1(state.output, state.boundary);
  tap(taps, state.channel, WideTapPointV1::kComposite, state.output);
  clock.mark(RibbonStageV1::kBound);

  // S10 artistic colour treatment.
  if (controls.colour.contrast_enabled &&
      controls.colour.contrast != kContrastNeutral) {
    state.cost.pow_calls += static_cast<std::uint32_t>(kPixelsPerChannel);
  }
  applyColourTreatmentV1(state.output, controls.colour, state.boundary);
  tap(taps, state.channel, WideTapPointV1::kColourTreated, state.output);
  clock.mark(RibbonStageV1::kColour);

  ++state.frames;
  result.rendered = true;
  return result;
}

RibbonTransactionResultV1 applyRibbonTransactionV1(
    RibbonChannelStateV1& channel_a, RibbonChannelStateV1& channel_b,
    const RibbonTransactionV1& transaction) noexcept {
  if (!transaction.has_a && !transaction.has_b) {
    return RibbonTransactionResultV1::kEmpty;
  }
  if (transaction.has_a &&
      validateRibbonControlsV1(transaction.a) != RibbonValidationV1::kValid) {
    return RibbonTransactionResultV1::kRejectedA;
  }
  if (transaction.has_b &&
      validateRibbonControlsV1(transaction.b) != RibbonValidationV1::kValid) {
    return RibbonTransactionResultV1::kRejectedB;
  }
  if (transaction.effective_us == 0U ||
      (transaction.has_a &&
       !transactionTimeValid(channel_a, transaction.effective_us)) ||
      (transaction.has_b &&
       !transactionTimeValid(channel_b, transaction.effective_us))) {
    return RibbonTransactionResultV1::kRejectedTime;
  }
  if (transaction.has_a) {
    applyChannelControls(channel_a, transaction.a, transaction.effective_us);
  }
  if (transaction.has_b) {
    applyChannelControls(channel_b, transaction.b, transaction.effective_us);
  }
  return RibbonTransactionResultV1::kApplied;
}

}  // namespace k1::core::visual::wide
