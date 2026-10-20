#pragma once

#include "core/visual/ribbon_material.h"
#include <cstddef>

namespace k1::core::visual {

// Already treated, limited, master-scaled and quantised. A borrowed view valid
// until the next render/reset of its owning profile. Never a pre-master frame.
class RibbonNativeFrame final {
 public:
  const RibbonFrame16& pixels() const noexcept { return *pixels_; }
  bool valid() const noexcept { return valid_; }
  bool stage(std::uint8_t* first_half, std::size_t first_size,
             std::uint8_t* second_half, std::size_t second_size) const noexcept;
 private:
  friend class RibbonProfile;
  RibbonNativeFrame(const RibbonFrame16& pixels, bool valid) noexcept
      : pixels_(&pixels), valid_(valid) {}
  const RibbonFrame16* pixels_;
  bool valid_;
};

// One instance per logical channel; physical DIN halves never own mode state.
// The runtime selects this profile explicitly. No mode IDs or SMC mappings are
// allocated here. Canonical palette/brightness controls remain caller-owned.
class RibbonProfile final {
 public:
  bool configure(const RibbonSettings& base, std::uint64_t time_us) noexcept;
  bool setLiveiness(float value, std::uint64_t time_us) noexcept;
  void reset(std::uint64_t time_us = 0) noexcept { material_.reset(time_us); }
  void consume(const VisualAudioFrameView& audio, std::uint64_t time_us) noexcept {
    material_.consume(audio, time_us);
  }
  RibbonNativeFrame render(std::uint64_t time_us,
                           std::uint8_t brightness = 24U) noexcept;
  const RibbonSettings& settings() const noexcept { return base_; }
  const RibbonMaterial& material() const noexcept { return material_; }
  const RibbonTelemetry& telemetry() const noexcept { return material_.telemetry(); }
  float liveiness() const noexcept { return liveiness_; }
  float effectiveVelocity() const noexcept { return material_.settings().velocity_px_s; }
 private:
  RibbonMaterial material_{};
  RibbonSettings base_{};
  float liveiness_ = 0.5F;
};

} // namespace k1::core::visual
