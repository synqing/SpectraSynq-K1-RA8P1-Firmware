#pragma once

// Analytic centre-origin pulse material (first ribbon profile, accent role).
//
// Each object stores its state at an anchor time. Between law edits it is
// evaluated in closed form from the anchor, so the state at a given
// timestamp does not depend on render cadence:
//   r(t)          = r_a + v * dt                  (logical px from centre)
//   variance(t)   = var_a + 2 * D * dt            (px^2)
//   log2 amp(t)   = log2 amp_a - dt / h           (half-life h)
// A law edit at its recorded effective time first re-anchors every live
// object with the old law, then installs the next law for future time, so a
// speed/diffusion/lifetime edit never teleports, rewrites the past or
// relights an object. Width is a birth property (sigma at spawn); a width
// edit affects later births only. Edits must be time-ordered; an edit
// earlier than the last installed edit is rejected.
//
// Rasterisation v1: point-sampled Gaussian at pixel distances d = 0..79
// (d = 0 is the centre pair 79/80), mirrored onto both halves, support
// |d - r| <= 6 sigma. Normalisation is declared per call: kPeak (amplitude is
// the peak) or kIntegrated (amplitude is the integral); widening never
// switches it silently.
//
// Capacity (fixed, per channel): 32 slots; 8 are reserved for primary
// gestures, so decorative objects never exceed 24. Admission order:
//   1. objects finished at the spawn time retire (amplitude below 1e-6 or
//      whole 6-sigma support beyond distance 79); retirement is final
//   2. a free slot (lowest index) admits the object
//   3. decorative with no free slot or at 24 decoratives: rejected, counted
//   4. primary with no free slot: evicts the decorative with the lowest
//      current amplitude (oldest id breaks ties), counted
//   5. primary with only primaries live: merging is not permitted by the v1
//      law, so it replaces the oldest primary and increments the visible
//      capacity counter `replaced_primary`
// Allocator order never chooses the result; AP event accounting is not
// touched here.

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/visual/wide/wide_types.h"

namespace k1::core::visual::wide {

inline constexpr std::size_t kPulseCapacity = 32U;
inline constexpr std::size_t kPulsePrimaryReserve = 8U;
inline constexpr std::size_t kPulseDecorativeLimit =
    kPulseCapacity - kPulsePrimaryReserve;
inline constexpr float kPulseSupportSigmas = 6.0F;
inline constexpr float kPulseRetireAmplitude = 1.0e-6F;
inline constexpr float kPulseMaximumVelocityPxS = 480.0F;
inline constexpr float kPulseMaximumDiffusionPx2S = 1024.0F;
inline constexpr float kPulseMinimumHalfLifeS = 0.02F;
inline constexpr float kPulseMaximumHalfLifeS = 60.0F;
inline constexpr float kPulseMinimumWidthPx = 0.25F;
inline constexpr float kPulseMaximumWidthPx = 24.0F;

enum class PulseRoleV1 : std::uint8_t {
  kPrimary = 1U,
  kDecorative = 2U,
};

enum class PulseNormalisationV1 : std::uint8_t {
  kPeak = 1U,
  kIntegrated = 2U,
};

struct MaterialLawV1 final {
  float velocity_px_s = 80.0F;
  float diffusion_px2_s = 0.0F;
  float half_life_s = 0.35F;
};

enum class MaterialValidationV1 : std::uint8_t {
  kValid = 0U,
  kVelocity = 1U,
  kDiffusion = 2U,
  kHalfLife = 3U,
};

[[nodiscard]] MaterialValidationV1 validateMaterialLawV1(
    const MaterialLawV1& law) noexcept;

struct PulseSpawnV1 final {
  std::uint64_t event_us = 0U;
  PulseRoleV1 role = PulseRoleV1::kDecorative;
  float amplitude = 1.0F;
  float radius_px = 0.0F;
  float width_px = 2.0F;
  WorkingRgbF32V1 colour{};
};

struct PulseObjectV1 final {
  std::uint32_t id = 0U;
  PulseRoleV1 role = PulseRoleV1::kDecorative;
  bool live = false;
  std::uint64_t anchor_us = 0U;
  float anchor_radius_px = 0.0F;
  float anchor_variance_px2 = 0.0F;
  float anchor_log2_amplitude = 0.0F;
  WorkingRgbF32V1 colour{};
};

struct PulseSampleV1 final {
  float radius_px = 0.0F;
  float variance_px2 = 0.0F;
  float log2_amplitude = 0.0F;
  float amplitude = 0.0F;
};

struct PulsePoolCountersV1 final {
  std::uint32_t admitted = 0U;
  std::uint32_t evicted_decorative = 0U;
  std::uint32_t replaced_primary = 0U;
  std::uint32_t rejected_decorative = 0U;
  std::uint32_t rejected_invalid = 0U;
  std::uint32_t retired_amplitude = 0U;
  std::uint32_t retired_support = 0U;
  std::uint32_t late_spawns = 0U;
  std::uint32_t law_edits = 0U;
  std::uint32_t rejected_edits = 0U;
  StageBoundaryCountersV1 colour_boundary{};
};

struct PulsePoolV1 final {
  std::array<PulseObjectV1, kPulseCapacity> slots{};
  MaterialLawV1 law{};
  std::uint64_t law_anchor_us = 0U;
  std::uint32_t next_id = 1U;
  PulsePoolCountersV1 counters{};
};

// Clears every object and counter and installs `law` (assumed validated).
void resetPulsePoolV1(PulsePoolV1& pool, const MaterialLawV1& law,
                      std::uint64_t now_us) noexcept;

// Closed-form state at t_us under `law`; t_us earlier than the anchor
// returns the anchor state (no backward extrapolation).
[[nodiscard]] PulseSampleV1 samplePulseV1(const MaterialLawV1& law,
                                          const PulseObjectV1& object,
                                          std::uint64_t t_us) noexcept;

enum class MaterialEditResultV1 : std::uint8_t {
  kApplied = 0U,
  kRejectedInvalid = 1U,
  kRejectedPast = 2U,
};

MaterialEditResultV1 editMaterialLawV1(PulsePoolV1& pool,
                                       const MaterialLawV1& law,
                                       std::uint64_t effective_us) noexcept;

enum class SpawnResultV1 : std::uint8_t {
  kAdmitted = 0U,
  kAdmittedEvictedDecorative = 1U,
  kAdmittedReplacedPrimary = 2U,
  kRejectedDecorativeFull = 3U,
  kRejectedInvalid = 4U,
};

SpawnResultV1 spawnPulseV1(PulsePoolV1& pool, const PulseSpawnV1& spawn) noexcept;

std::uint32_t retireFinishedPulsesV1(PulsePoolV1& pool,
                                     std::uint64_t t_us) noexcept;

[[nodiscard]] std::uint32_t livePulseCountV1(const PulsePoolV1& pool,
                                             PulseRoleV1 role) noexcept;

struct RasterCostV1 final {
  std::uint32_t objects = 0U;
  std::uint32_t exp_calls = 0U;
};

// Accumulates (+=) the live objects at t_us into `layer`.
void rasterisePulsesV1(const PulsePoolV1& pool, std::uint64_t t_us,
                       PulseNormalisationV1 normalisation, WorkingFrame& layer,
                       RasterCostV1& cost) noexcept;

}  // namespace k1::core::visual::wide
