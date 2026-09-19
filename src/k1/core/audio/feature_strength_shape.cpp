#include "core/audio/feature_strength_shape.h"

#include <cmath>

namespace k1::core::audio {
namespace {

float clamp01(const float value) noexcept {
  if (!std::isfinite(value) || value <= 0.0F) {
    return 0.0F;
  }
  return value >= 1.0F ? 1.0F : value;
}

}  // namespace

void StrengthShapeAdapter::reset(const std::uint32_t epoch) noexcept {
  for (std::size_t i = 0; i < contract::kSpectrumBinCount; ++i) {
    previous_shape_[i] = 0.0F;
    predicted_shape_[i] = 0.0F;
  }
  previous_strength_ = 0.0F;
  epoch_ = epoch;
  count_ = 0;
  have_previous_ = false;
}

StrengthShapeObservation StrengthShapeAdapter::observe(
    const contract::AudioFeaturesV1& audio) noexcept {
  StrengthShapeObservation out{};
  out.sequence = audio.sequence;
  out.source_frame_ms = audio.source_frame_ms;
  out.epoch = epoch_;
  float energy = 0.0F;
  float shape[contract::kSpectrumBinCount]{};
  for (std::size_t i = 0; i < contract::kSpectrumBinCount; ++i) {
    const float bin = std::isfinite(audio.spectrum[i]) && audio.spectrum[i] > 0.0F
                          ? audio.spectrum[i]
                          : 0.0F;
    energy += bin;
    shape[i] = bin;
  }
  if (energy <= 1.0e-8F || (audio.event_flags & contract::kEventSilence) != 0U) {
    out.valid = false;
    have_previous_ = false;
    previous_strength_ = 0.0F;
    return out;
  }
  for (std::size_t i = 0; i < contract::kSpectrumBinCount; ++i) {
    shape[i] /= energy;
  }
  out.strength = clamp01(audio.spectral_energy > 0.0F ? audio.spectral_energy : energy);
  float distance = 0.0F;
  float surprise = 0.0F;
  if (have_previous_) {
    for (std::size_t i = 0; i < contract::kSpectrumBinCount; ++i) {
      const float d = shape[i] - previous_shape_[i];
      distance += d * d;
      const float p = shape[i] - predicted_shape_[i];
      surprise += p * p;
    }
    distance = std::sqrt(distance);
    surprise = std::sqrt(surprise);
  }
  out.shape_distance = distance;
  out.surprise = surprise;
  out.confidence = have_previous_ ? clamp01(1.0F - surprise) : 0.0F;
  out.valid = true;
  if (have_previous_) {
    for (std::size_t i = 0; i < contract::kSpectrumBinCount; ++i) {
      predicted_shape_[i] = shape[i];
    }
  } else {
    for (std::size_t i = 0; i < contract::kSpectrumBinCount; ++i) {
      predicted_shape_[i] = shape[i];
    }
  }
  for (std::size_t i = 0; i < contract::kSpectrumBinCount; ++i) {
    previous_shape_[i] = shape[i];
  }
  previous_strength_ = out.strength;
  have_previous_ = true;
  count_ += 1U;
  return out;
}

}  // namespace k1::core::audio
