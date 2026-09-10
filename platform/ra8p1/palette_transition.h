#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include "core/visual/product_palette.h"

namespace k1::core::visual {
// Fixed catalogue weights preserve interrupted fades without retaining frames
// or advancing an effect twice. At rest, use the original sampler directly.
class PaletteTransition final {
 public:
  static constexpr std::uint32_t kMaximumDurationMs = 10000U;
  void select(std::uint16_t id, std::uint32_t duration_ms,
              std::uint64_t now_us) noexcept {
    if (id >= kProductPaletteCount || duration_ms > kMaximumDurationMs) return;
    advance(now_us);
    if (!initialised_ || duration_ms == 0U) {
      target_ = id; initialised_ = true; duration_us_ = 0U;
      weights_.fill(0.0F); weights_[id] = 1.0F;
      phase_ = brightest(id); progress_ = 65535U;
      return;
    }
    if (id == target_) return; // Repeated controls do not restart a fade.
    source_ = weights_; source_phase_ = phase_; target_ = id;
    target_phase_ = brightest(id); start_us_ = now_us;
    duration_us_ = duration_ms * 1000U; progress_ = 0U;
  }
  void advance(std::uint64_t now_us) noexcept {
    if (!duration_us_) return;
    const auto elapsed = now_us > start_us_ ? now_us - start_us_ : 0U;
    if (elapsed >= duration_us_) {
      duration_us_ = 0U; weights_.fill(0.0F); weights_[target_] = 1.0F;
      phase_ = target_phase_; progress_ = 65535U;
      return;
    }
    const float t = static_cast<float>(elapsed) / static_cast<float>(duration_us_);
    const float amount = t * t * (3.0F - 2.0F * t);
    for (unsigned i = 0; i < kProductPaletteCount; ++i)
      weights_[i] = source_[i] * (1.0F - amount);
    weights_[target_] += amount;
    float delta = target_phase_ - source_phase_;
    if (delta > 0.5F) delta -= 1.0F;
    if (delta < -0.5F) delta += 1.0F;
    phase_ = source_phase_ + delta * amount;
    if (elapsed) phase_ -= ::floorf(phase_);
    progress_ = static_cast<std::uint32_t>(amount * 65535.0F);
  }
  bool active() const noexcept { return duration_us_ != 0U; }
  unsigned progress() const noexcept { return progress_; }
  float brightestPhase() const noexcept { return phase_; }
  unsigned contributors() const noexcept {
    unsigned count = 0U;
    for (float w : weights_) if (w > 0.0F) ++count;
    return count;
  }
  PaletteLinearRgb hd(float phase, float level = 1.0F) const noexcept {
    if (!active()) return sampleProductPaletteHd(target_, phase, level);
    PaletteLinearRgb out{};
    for (unsigned i = 0; i < kProductPaletteCount; ++i) {
      if (weights_[i] == 0.0F) continue;
      const auto p = sampleProductPaletteHd(i, phase, level);
      out.red += weights_[i] * p.red;
      out.green += weights_[i] * p.green;
      out.blue += weights_[i] * p.blue;
    }
    return out;
  }
  Pixel8 fast(std::uint8_t index) const noexcept {
    if (!active()) return sampleProductPaletteFastLed16(target_, index);
    float red = 0.0F, green = 0.0F, blue = 0.0F;
    for (unsigned i = 0; i < kProductPaletteCount; ++i) {
      if (weights_[i] == 0.0F) continue;
      const auto p = sampleProductPaletteFastLed16(i, index);
      red += weights_[i] * p.red; green += weights_[i] * p.green;
      blue += weights_[i] * p.blue;
    }
    const auto quantise = [](float v) -> std::uint8_t {
      return static_cast<std::uint8_t>(v >= 254.5F ? 255U : v + 0.5F);
    };
    return {quantise(red), quantise(green), quantise(blue)};
  }
 private:
  static float brightest(std::uint16_t id) noexcept {
    const auto& p = productPalette(id);
    float maximum = -1.0F, phase = 0.0F;
    for (unsigned i = 0U; i < p.stop_count; ++i) {
      const auto c = p.stops[i].colour;
      const float light = 0.30F * c.red + 0.59F * c.green + 0.11F * c.blue;
      if (light > maximum) { maximum = light; phase = float(p.stops[i].position) / 255.0F; }
    }
    return phase;
  }
  std::array<float, kProductPaletteCount> source_{}, weights_{};
  std::uint64_t start_us_ = 0U;
  std::uint32_t duration_us_ = 0U, progress_ = 65535U;
  std::uint16_t target_ = 0U;
  float source_phase_ = 0.0F, target_phase_ = 0.0F, phase_ = 0.0F;
  bool initialised_ = false;
};
static_assert(sizeof(PaletteTransition) <= 400U, "transition state exceeds budget");
} // namespace k1::core::visual
