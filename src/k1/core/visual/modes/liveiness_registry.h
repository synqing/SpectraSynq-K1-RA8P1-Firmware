#pragma once

// Liveiness adapter registry: one row per enabled catalogue mode, indexed by
// the stable product mode ordinal. The renderer hook is liveinessEffective();
// applied-state feedback uses liveinessStatus(). Header-only and constexpr so
// that the hook adds no translation unit to any existing build.

#include <array>
#include <cstddef>
#include <cstdint>

#include "contract/audio_features_v1.h"
#include "contract/tempo_field_v1.h"
#include "core/visual/modes/liveiness_contract.h"
#include "core/visual/modes/material_liveiness.h"
#include "core/visual/modes/rhythm_liveiness.h"
#include "core/visual/modes/spectrum_liveiness.h"
#include "core/visual/modes/waveform_liveiness.h"
#include "core/visual/product_catalogue.h"

namespace k1::core::visual::modes {

// Registered snapshot of the product-enabled catalogue (product_catalogue.cpp
// sha256 6154c0ec84c8581e...). The contract fixture compares it with
// productModeEnabled() so a catalogue change cannot silently shrink or grow
// the Liveiness denominator.
inline constexpr std::array<std::uint16_t, 23U> kLiveinessEnabledModes{
    {3U, 7U, 8U, 9U, 11U, 12U, 13U, 14U, 15U, 16U, 18U, 19U, 20U, 21U, 22U,
     23U, 24U, 25U, 26U, 27U, 28U, 29U, 32U}};

namespace detail {

inline constexpr std::size_t kLiveinessAdapterCount =
    kMaterialLiveinessAdapters.size() + kWaveformLiveinessAdapters.size() +
    kSpectrumLiveinessAdapters.size() + kRhythmLiveinessAdapters.size();
inline constexpr std::uint8_t kNoAdapter = 0xFFU;

template <std::size_t Count>
constexpr std::size_t appendAdapters(
    std::array<LiveinessAdapterDescriptor, kLiveinessAdapterCount>& target,
    std::size_t cursor,
    const std::array<LiveinessAdapterDescriptor, Count>& source) {
  for (std::size_t index = 0U; index < Count; ++index) {
    target[cursor] = source[index];
    ++cursor;
  }
  return cursor;
}

constexpr std::array<LiveinessAdapterDescriptor, kLiveinessAdapterCount>
combineAdapters() {
  std::array<LiveinessAdapterDescriptor, kLiveinessAdapterCount> all{};
  std::size_t cursor = 0U;
  cursor = appendAdapters(all, cursor, kMaterialLiveinessAdapters);
  cursor = appendAdapters(all, cursor, kWaveformLiveinessAdapters);
  cursor = appendAdapters(all, cursor, kSpectrumLiveinessAdapters);
  cursor = appendAdapters(all, cursor, kRhythmLiveinessAdapters);
  return all;
}

inline constexpr std::array<LiveinessAdapterDescriptor, kLiveinessAdapterCount>
    kAllLiveinessAdapters = combineAdapters();

constexpr std::array<std::uint8_t, kProductModeCount> buildAdapterIndex() {
  std::array<std::uint8_t, kProductModeCount> index{};
  for (std::size_t mode = 0U; mode < kProductModeCount; ++mode) {
    index[mode] = kNoAdapter;
  }
  for (std::size_t slot = 0U; slot < kLiveinessAdapterCount; ++slot) {
    const std::uint16_t mode = kAllLiveinessAdapters[slot].mode_id;
    if (mode < kProductModeCount) {
      index[mode] = static_cast<std::uint8_t>(slot);
    }
  }
  return index;
}

inline constexpr std::array<std::uint8_t, kProductModeCount>
    kLiveinessAdapterIndex = buildAdapterIndex();

inline constexpr LiveinessAdapterDescriptor kUnsupportedLiveinessAdapter{};

constexpr bool enabledMode(const std::uint16_t mode) {
  for (const std::uint16_t enabled : kLiveinessEnabledModes) {
    if (enabled == mode) {
      return true;
    }
  }
  return false;
}

constexpr bool adaptersWellFormed() {
  for (std::size_t slot = 0U; slot < kLiveinessAdapterCount; ++slot) {
    const LiveinessAdapterDescriptor& row = kAllLiveinessAdapters[slot];
    if (row.mode_id >= kProductModeCount || !enabledMode(row.mode_id)) {
      return false;
    }
    for (std::size_t other = slot + 1U; other < kLiveinessAdapterCount;
         ++other) {
      if (kAllLiveinessAdapters[other].mode_id == row.mode_id) {
        return false;
      }
    }
    if (row.support == LiveinessSupport::kUnsupported ||
        row.family == LiveinessFamily::kNone ||
        row.dimension == LiveinessDimension::kNone ||
        !(row.exponent_gain > 0.0F) || row.minimum < 0.0F ||
        !(row.maximum > row.minimum)) {
      return false;
    }
    const bool admitted = row.support == LiveinessSupport::kSupported;
    if (admitted != (row.adapter_version >= 1U)) {
      return false;
    }
    // Only a wired adapter is scored; a recorded failure names its cause.
    if (admitted == (row.acceptance == LiveinessAcceptance::kNotScored)) {
      return false;
    }
    if ((row.acceptance == LiveinessAcceptance::kFailReferenceBound) ==
        (row.acceptance_note[0] == '\0')) {
      return false;
    }
  }
  return true;
}

constexpr bool everyEnabledModeHasARow() {
  for (const std::uint16_t mode : kLiveinessEnabledModes) {
    if (kLiveinessAdapterIndex[mode] == kNoAdapter) {
      return false;
    }
  }
  return true;
}

static_assert(kLiveinessAdapterCount == kLiveinessEnabledModes.size(),
              "one Liveiness row per enabled catalogue mode");
static_assert(adaptersWellFormed(),
              "Liveiness rows must be unique, enabled and well formed");
static_assert(everyEnabledModeHasARow(),
              "every enabled mode needs a registered or pending row");

}  // namespace detail

[[nodiscard]] inline const LiveinessAdapterDescriptor& liveinessAdapter(
    const std::uint16_t mode_id) noexcept {
  if (mode_id >= kProductModeCount) {
    return detail::kUnsupportedLiveinessAdapter;
  }
  const std::uint8_t slot = detail::kLiveinessAdapterIndex[mode_id];
  return slot == detail::kNoAdapter ? detail::kUnsupportedLiveinessAdapter
                                    : detail::kAllLiveinessAdapters[slot];
}

// Renderer hook: the effective value for a mode's Liveiness quantity. Exact
// identity for unsupported/pending modes, a disabled input or a neutral amount.
[[nodiscard]] inline float liveinessEffective(
    const std::uint16_t mode_id, const float base,
    const LiveinessInput& input) noexcept {
  return resolveLiveiness(liveinessAdapter(mode_id), base, input).effective;
}

// Modes whose registered criteria pass on VP's wide render route (route
// accepted separately from the legacy Pixel8 path). Bloom v1: legacy path
// recorded FAIL (cadence, reference-bound); wide route PASS (ORCH: admits
// Bloom on the wide route only). Evidence: native_mode_liveiness_wide_bloom.
inline constexpr std::array<std::uint16_t, 1U> kLiveinessWideRouteAcceptedModes{
    {3U}};

[[nodiscard]] constexpr bool liveinessAcceptedOnWideRoute(
    const std::uint16_t mode_id) noexcept {
  for (const std::uint16_t mode : kLiveinessWideRouteAcceptedModes) {
    if (mode == mode_id) {
      return true;
    }
  }
  return false;
}

// Wired adapters (hook live), whether or not accepted.
[[nodiscard]] constexpr std::size_t liveinessAdmittedCount() noexcept {
  std::size_t count = 0U;
  for (const LiveinessAdapterDescriptor& row : detail::kAllLiveinessAdapters) {
    count += row.support == LiveinessSupport::kSupported ? 1U : 0U;
  }
  return count;
}

// Adapters counted towards MOD-04 coverage: wired and every registered host
// criterion passed. A wired adapter with a recorded failure is not counted.
[[nodiscard]] constexpr std::size_t liveinessAcceptedCount() noexcept {
  std::size_t count = 0U;
  for (const LiveinessAdapterDescriptor& row : detail::kAllLiveinessAdapters) {
    count += row.support == LiveinessSupport::kSupported &&
                     row.acceptance == LiveinessAcceptance::kHostPass
                 ? 1U
                 : 0U;
  }
  return count;
}

// Read-only projection of the audio evidence a status decision needs: the
// AudioFeaturesV1 flags plus AP's TempoFieldV1 (nullable until the visual
// frame carries it; absent means no tempo evidence). Nothing here can write
// audio-processor state.
struct LiveinessEvidenceView final {
  std::uint32_t validity_flags = 0U;
  std::uint32_t event_flags = contract::kEventSilence;
  const contract::TempoFieldV1* tempo_field = nullptr;
};

// Tempo evidence counts only as AP publishes it: an enabled, valid, available
// field with valid non-zero activity and a locked canonical clock. A coasting
// clock is prediction, never observation, so it does not count.
[[nodiscard]] inline bool liveinessTempoObserved(
    const contract::TempoFieldV1* field) noexcept {
  if (field == nullptr) {
    return false;
  }
  constexpr std::uint32_t kRequired = contract::kTempoFieldEnabled |
                                      contract::kTempoFieldValid |
                                      contract::kTempoFieldActivityValid |
                                      contract::kTempoFieldCanonicalLocked;
  constexpr std::uint32_t kExcluded = contract::kTempoFieldCanonicalCoasting |
                                      contract::kTempoFieldSilence |
                                      contract::kTempoFieldInputSilence |
                                      contract::kTempoFieldInvalidEvidence;
  return (field->flags & kRequired) == kRequired &&
         (field->flags & kExcluded) == 0U &&
         field->reason ==
             static_cast<std::uint8_t>(contract::TempoFieldReason::kAvailable) &&
         std::isfinite(field->activity) && field->activity > 0.0F;
}

[[nodiscard]] inline bool liveinessTempoCoasting(
    const contract::TempoFieldV1* field) noexcept {
  return field != nullptr &&
         (field->flags & contract::kTempoFieldCanonicalCoasting) != 0U;
}

[[nodiscard]] inline std::uint8_t liveinessEvidencePresent(
    const LiveinessEvidenceView& view) noexcept {
  const bool audible = (view.event_flags & contract::kEventSilence) == 0U;
  std::uint8_t present = evidence::kNone;
  if (audible) {
    present = static_cast<std::uint8_t>(present | evidence::kAmplitude);
    if ((view.validity_flags & contract::kValidChroma) != 0U) {
      present = static_cast<std::uint8_t>(present | evidence::kChroma);
    }
    if ((view.validity_flags & contract::kValidSpectrum) != 0U) {
      present = static_cast<std::uint8_t>(present | evidence::kSpectrum);
    }
    if ((view.validity_flags & contract::kValidOnsetV2) != 0U) {
      present = static_cast<std::uint8_t>(present | evidence::kOnsetEvents);
    }
  }
  if (liveinessTempoObserved(view.tempo_field)) {
    present = static_cast<std::uint8_t>(present | evidence::kTempoLock);
  }
  return present;
}

struct LiveinessStatus final {
  std::uint16_t mode_id = 0U;
  std::uint16_t adapter_version = 0U;
  LiveinessSupport support = LiveinessSupport::kUnsupported;
  bool active = false;
  LiveinessReason reason = LiveinessReason::kUnsupportedMode;
  std::uint8_t missing_evidence = evidence::kNone;
  const char* ui_key = "";
  LiveinessAcceptance acceptance = LiveinessAcceptance::kNotScored;
  const char* acceptance_note = "";
  bool tempo_coasting = false;  // qualifier: the tempo clock is predicting
};

// Capability truth for applied-state feedback: supported or not, and the
// specific reason a supported control is currently inactive.
[[nodiscard]] inline LiveinessStatus liveinessStatus(
    const std::uint16_t mode_id, const LiveinessInput& input,
    const LiveinessEvidenceView& view) noexcept {
  const LiveinessAdapterDescriptor& adapter = liveinessAdapter(mode_id);
  LiveinessStatus status{};
  status.mode_id = mode_id;
  status.adapter_version = adapter.adapter_version;
  status.support = adapter.support;
  status.ui_key = adapter.ui_key;
  status.acceptance = adapter.acceptance;
  status.acceptance_note = adapter.acceptance_note;
  status.tempo_coasting = (adapter.evidence & evidence::kTempoLock) != 0U &&
                          liveinessTempoCoasting(view.tempo_field);
  if (adapter.support != LiveinessSupport::kSupported) {
    status.reason = LiveinessReason::kUnsupportedMode;
    return status;
  }
  if (!input.enabled) {
    status.reason = LiveinessReason::kDisabled;
    return status;
  }
  if (!std::isfinite(input.amount)) {
    status.reason = LiveinessReason::kInvalidAmount;
    return status;
  }
  status.missing_evidence = static_cast<std::uint8_t>(
      adapter.evidence & static_cast<std::uint8_t>(
                             ~liveinessEvidencePresent(view)));
  if (status.missing_evidence != evidence::kNone) {
    status.reason = LiveinessReason::kNoEvidence;
    return status;
  }
  status.active = true;
  status.reason = LiveinessReason::kNone;
  return status;
}

}  // namespace k1::core::visual::modes
