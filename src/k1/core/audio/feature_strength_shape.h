#pragma once

#include <cstddef>
#include <cstdint>

#include "contract/audio_features_v1.h"

namespace k1::core::audio {

/* Past-only strength / normalised shape / surprise over the admitted 80-bin
   GDFT. Does not replace onset/energy, does not consume future frames. */

struct StrengthShapeObservation final {
  float strength = 0.0F;
  float shape_distance = 0.0F;
  float surprise = 0.0F;
  float confidence = 0.0F;
  std::uint32_t sequence = 0;
  std::uint32_t source_frame_ms = 0;
  std::uint32_t epoch = 0;
  bool valid = false;
};

class StrengthShapeAdapter final {
 public:
  void reset(std::uint32_t epoch = 0) noexcept;
  StrengthShapeObservation observe(const contract::AudioFeaturesV1& audio) noexcept;
  [[nodiscard]] std::uint32_t epoch() const noexcept { return epoch_; }

 private:
  float previous_shape_[contract::kSpectrumBinCount]{};
  float predicted_shape_[contract::kSpectrumBinCount]{};
  float previous_strength_ = 0.0F;
  std::uint32_t epoch_ = 0;
  std::uint32_t count_ = 0;
  bool have_previous_ = false;
};

}  // namespace k1::core::audio
