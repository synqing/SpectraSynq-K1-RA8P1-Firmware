#include "core/visual/wide/wide_material.h"

#include <cmath>

#include "core/visual/wide/wide_temporal.h"

namespace k1::core::visual::wide {
namespace {

constexpr float kInverseSqrtTwoPi = 0.3989422804014327F;
constexpr float kLastDistance = static_cast<float>(kPixelsPerHalf - 1U);

bool finiteIn(const float value, const float low, const float high) noexcept {
  return std::isfinite(value) && value >= low && value <= high;
}

bool validRole(const PulseRoleV1 role) noexcept {
  return role == PulseRoleV1::kPrimary || role == PulseRoleV1::kDecorative;
}

bool finished(const PulseSampleV1& sample) noexcept {
  if (!(sample.amplitude >= kPulseRetireAmplitude)) {
    return true;
  }
  const float sigma = std::sqrt(sample.variance_px2);
  return sample.radius_px - kPulseSupportSigmas * sigma > kLastDistance;
}

void reanchor(PulseObjectV1& object, const PulseSampleV1& sample,
              const std::uint64_t t_us) noexcept {
  object.anchor_us = t_us;
  object.anchor_radius_px = sample.radius_px;
  object.anchor_variance_px2 = sample.variance_px2;
  object.anchor_log2_amplitude = sample.log2_amplitude;
}

std::size_t freeSlot(const PulsePoolV1& pool) noexcept {
  for (std::size_t index = 0U; index < kPulseCapacity; ++index) {
    if (!pool.slots[index].live) {
      return index;
    }
  }
  return kPulseCapacity;
}

}  // namespace

MaterialValidationV1 validateMaterialLawV1(const MaterialLawV1& law) noexcept {
  if (!finiteIn(law.velocity_px_s, 0.0F, kPulseMaximumVelocityPxS)) {
    return MaterialValidationV1::kVelocity;
  }
  if (!finiteIn(law.diffusion_px2_s, 0.0F, kPulseMaximumDiffusionPx2S)) {
    return MaterialValidationV1::kDiffusion;
  }
  if (!finiteIn(law.half_life_s, kPulseMinimumHalfLifeS,
                kPulseMaximumHalfLifeS)) {
    return MaterialValidationV1::kHalfLife;
  }
  return MaterialValidationV1::kValid;
}

void resetPulsePoolV1(PulsePoolV1& pool, const MaterialLawV1& law,
                      const std::uint64_t now_us) noexcept {
  pool = PulsePoolV1{};
  pool.law = law;
  pool.law_anchor_us = now_us;
}

PulseSampleV1 samplePulseV1(const MaterialLawV1& law,
                            const PulseObjectV1& object,
                            const std::uint64_t t_us) noexcept {
  const float dt = secondsBetweenV1(object.anchor_us, t_us);
  PulseSampleV1 sample{};
  sample.radius_px = object.anchor_radius_px + law.velocity_px_s * dt;
  sample.variance_px2 =
      object.anchor_variance_px2 + 2.0F * law.diffusion_px2_s * dt;
  sample.log2_amplitude =
      object.anchor_log2_amplitude - dt / law.half_life_s;
  sample.amplitude = std::exp2(sample.log2_amplitude);
  return sample;
}

MaterialEditResultV1 editMaterialLawV1(PulsePoolV1& pool,
                                       const MaterialLawV1& law,
                                       const std::uint64_t effective_us) noexcept {
  if (validateMaterialLawV1(law) != MaterialValidationV1::kValid) {
    ++pool.counters.rejected_edits;
    return MaterialEditResultV1::kRejectedInvalid;
  }
  if (effective_us < pool.law_anchor_us) {
    ++pool.counters.rejected_edits;
    return MaterialEditResultV1::kRejectedPast;
  }
  for (PulseObjectV1& object : pool.slots) {
    if (!object.live || object.anchor_us >= effective_us) {
      continue;  // objects born after the edit already follow the next law
    }
    reanchor(object, samplePulseV1(pool.law, object, effective_us),
             effective_us);
  }
  pool.law = law;
  pool.law_anchor_us = effective_us;
  ++pool.counters.law_edits;
  return MaterialEditResultV1::kApplied;
}

std::uint32_t retireFinishedPulsesV1(PulsePoolV1& pool,
                                     const std::uint64_t t_us) noexcept {
  std::uint32_t retired = 0U;
  for (PulseObjectV1& object : pool.slots) {
    if (!object.live) {
      continue;
    }
    const PulseSampleV1 sample = samplePulseV1(pool.law, object, t_us);
    if (!(sample.amplitude >= kPulseRetireAmplitude)) {
      object.live = false;
      ++pool.counters.retired_amplitude;
      ++retired;
    } else if (finished(sample)) {
      object.live = false;
      ++pool.counters.retired_support;
      ++retired;
    }
  }
  return retired;
}

std::uint32_t livePulseCountV1(const PulsePoolV1& pool,
                               const PulseRoleV1 role) noexcept {
  std::uint32_t count = 0U;
  for (const PulseObjectV1& object : pool.slots) {
    if (object.live && object.role == role) {
      ++count;
    }
  }
  return count;
}

SpawnResultV1 spawnPulseV1(PulsePoolV1& pool, const PulseSpawnV1& spawn) noexcept {
  if (spawn.event_us == 0U || !validRole(spawn.role) ||
      !finiteIn(spawn.amplitude, kPulseRetireAmplitude, kLayerComponentMax) ||
      !finiteIn(spawn.radius_px, 0.0F, kLastDistance) ||
      !finiteIn(spawn.width_px, kPulseMinimumWidthPx, kPulseMaximumWidthPx)) {
    ++pool.counters.rejected_invalid;
    return SpawnResultV1::kRejectedInvalid;
  }
  std::uint64_t birth_us = spawn.event_us;
  if (birth_us < pool.law_anchor_us) {
    birth_us = pool.law_anchor_us;  // declared: late spawn anchored at the edit
    ++pool.counters.late_spawns;
  }
  static_cast<void>(retireFinishedPulsesV1(pool, birth_us));

  SpawnResultV1 result = SpawnResultV1::kAdmitted;
  std::size_t slot = freeSlot(pool);
  if (spawn.role == PulseRoleV1::kDecorative) {
    if (slot == kPulseCapacity ||
        livePulseCountV1(pool, PulseRoleV1::kDecorative) >=
            kPulseDecorativeLimit) {
      ++pool.counters.rejected_decorative;
      return SpawnResultV1::kRejectedDecorativeFull;
    }
  } else if (slot == kPulseCapacity) {
    // Primary at capacity: evict the weakest decorative (oldest id on ties).
    float weakest = 0.0F;
    for (std::size_t index = 0U; index < kPulseCapacity; ++index) {
      const PulseObjectV1& object = pool.slots[index];
      if (!object.live || object.role != PulseRoleV1::kDecorative) {
        continue;
      }
      const float amplitude =
          samplePulseV1(pool.law, object, birth_us).amplitude;
      if (slot == kPulseCapacity || amplitude < weakest ||
          (amplitude == weakest && object.id < pool.slots[slot].id)) {
        slot = index;
        weakest = amplitude;
      }
    }
    if (slot != kPulseCapacity) {
      ++pool.counters.evicted_decorative;
      result = SpawnResultV1::kAdmittedEvictedDecorative;
    } else {
      // Only primaries live: v1 does not permit merging; replace the oldest.
      for (std::size_t index = 0U; index < kPulseCapacity; ++index) {
        if (slot == kPulseCapacity ||
            pool.slots[index].id < pool.slots[slot].id) {
          slot = index;
        }
      }
      ++pool.counters.replaced_primary;
      result = SpawnResultV1::kAdmittedReplacedPrimary;
    }
  }

  PulseObjectV1& object = pool.slots[slot];
  object = PulseObjectV1{};
  object.id = pool.next_id;
  pool.next_id = pool.next_id == 0xFFFFFFFFU ? 1U : pool.next_id + 1U;
  object.role = spawn.role;
  object.live = true;
  object.anchor_us = birth_us;
  object.anchor_radius_px = spawn.radius_px;
  object.anchor_variance_px2 = spawn.width_px * spawn.width_px;
  object.anchor_log2_amplitude = std::log2(spawn.amplitude);
  object.colour = sanitiseWorking(spawn.colour, kLayerComponentMax,
                                  pool.counters.colour_boundary);
  ++pool.counters.admitted;
  return result;
}

void rasterisePulsesV1(const PulsePoolV1& pool, const std::uint64_t t_us,
                       const PulseNormalisationV1 normalisation,
                       WorkingFrame& layer, RasterCostV1& cost) noexcept {
  for (const PulseObjectV1& object : pool.slots) {
    if (!object.live) {
      continue;
    }
    const PulseSampleV1 sample = samplePulseV1(pool.law, object, t_us);
    if (!(sample.amplitude >= kPulseRetireAmplitude) ||
        !(sample.variance_px2 > 0.0F)) {
      continue;
    }
    const float sigma = std::sqrt(sample.variance_px2);
    const float reach = kPulseSupportSigmas * sigma;
    const float low = std::ceil(sample.radius_px - reach);
    const float high = std::floor(sample.radius_px + reach);
    if (high < 0.0F || low > kLastDistance) {
      continue;
    }
    const std::size_t first =
        low > 0.0F ? static_cast<std::size_t>(low) : 0U;
    const std::size_t last =
        high < kLastDistance ? static_cast<std::size_t>(high)
                             : kPixelsPerHalf - 1U;
    const float peak = normalisation == PulseNormalisationV1::kIntegrated
                           ? sample.amplitude * kInverseSqrtTwoPi / sigma
                           : sample.amplitude;
    const float inverse_two_variance = 0.5F / sample.variance_px2;
    ++cost.objects;
    for (std::size_t distance = first; distance <= last; ++distance) {
      const float offset = static_cast<float>(distance) - sample.radius_px;
      const float value =
          peak * std::exp(-offset * offset * inverse_two_variance);
      ++cost.exp_calls;
      const MirroredPair pair = mirroredPair(distance);
      WorkingRgbF32V1& right = layer[pair.right];
      WorkingRgbF32V1& left = layer[pair.left];
      right.red += value * object.colour.red;
      right.green += value * object.colour.green;
      right.blue += value * object.colour.blue;
      left.red += value * object.colour.red;
      left.green += value * object.colour.green;
      left.blue += value * object.colour.blue;
    }
  }
}

}  // namespace k1::core::visual::wide
