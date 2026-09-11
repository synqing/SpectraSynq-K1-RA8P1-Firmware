#include "core/visual/product_output_treatment.h"

#include "core/visual/channel_render_state.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace k1::core::visual {
namespace {

constexpr float kIncandescentRed = 1.0000F;
constexpr float kIncandescentGreen = 0.4453F;
constexpr float kIncandescentBlue = 0.1562F;
constexpr float kBulbCover[4] = {0.25F, 1.00F, 0.25F, 0.00F};
constexpr std::uint8_t kDitherThreshold[4] = {64U, 128U, 192U, 255U};

float clamp01(const float value) noexcept {
  return std::clamp(value, 0.0F, 1.0F);
}

std::uint8_t clampByte(const float value) noexcept {
  return static_cast<std::uint8_t>(
      std::clamp(value, 0.0F, 255.0F));
}

std::uint8_t incandescentChannel(
    const std::uint8_t value,
    const float coefficient,
    const float mix) noexcept {
  const float scale = (1.0F - mix) + (mix * coefficient);
  return clampByte(static_cast<float>(value) * scale);
}

std::uint8_t ditherChannel(
    const std::uint8_t value,
    const std::size_t pixel_index,
    const std::uint8_t phase) noexcept {
  const std::uint16_t scaled = static_cast<std::uint16_t>(value) * 254U;
  std::uint16_t whole = scaled / 255U;
  const std::uint8_t remainder = static_cast<std::uint8_t>(scaled % 255U);
  const std::size_t threshold_index = (pixel_index + phase) & 0x03U;
  if (remainder >= kDitherThreshold[threshold_index] && whole < 254U) {
    ++whole;
  }
  return static_cast<std::uint8_t>(whole);
}

void applyPrismOverlay(
    const PixelSpan frame,
    const float prism_count) noexcept {
  const std::uint8_t count = static_cast<std::uint8_t>(
      std::clamp(prism_count, 0.0F, 8.0F));
  if (count == 0U || frame.size() < 2U) {
    return;
  }

  const std::size_t centre_left = (frame.size() / 2U) - 1U;
  const std::size_t centre_right = frame.size() / 2U;
  for (std::uint8_t prism = 0U; prism < count; ++prism) {
    const std::size_t distance = 2U + (static_cast<std::size_t>(prism) * 3U);
    if (distance > centre_left || (centre_right + distance) >= frame.size()) {
      break;
    }
    const std::uint8_t gain = static_cast<std::uint8_t>(48U - (prism * 4U));
    Pixel8& left = frame[centre_left - distance];
    Pixel8& right = frame[centre_right + distance];
    left.red = std::max(left.red, gain);
    left.blue = std::max(left.blue, static_cast<std::uint8_t>(gain / 2U));
    right.green = std::max(right.green, gain);
    right.blue = std::max(right.blue, static_cast<std::uint8_t>(gain / 2U));
  }
}

}  // namespace

void applyProductOutputTreatment(
    const PixelSpan frame,
    const ChannelVisualControls& controls,
    ProductOutputTreatmentState& state) noexcept {
  const float incandescent_mix = controls.vp_fix_secondary_clean
      ? 0.0F
      : clamp01(controls.incandescent_filter);
  const float bulb_opacity = clamp01(controls.bulb_opacity);
  const std::uint8_t base_add = controls.base_coat &&
                                        !controls.vp_fix_secondary_clean
      ? static_cast<std::uint8_t>(
            std::clamp(controls.base_coat_intensity, 0.0F, 1.0F) * 20.0F)
      : 0U;

  for (std::size_t index = 0U; index < frame.size(); ++index) {
    Pixel8 pixel = frame[index];
    if (controls.incandescent_mode) {
      const std::uint8_t peak = std::max({pixel.red, pixel.green, pixel.blue});
      pixel.red = peak;
      pixel.green = clampByte(static_cast<float>(peak) * kIncandescentGreen);
      pixel.blue = clampByte(static_cast<float>(peak) * kIncandescentBlue);
    } else if (incandescent_mix > 0.0F) {
      pixel.red = incandescentChannel(pixel.red, kIncandescentRed, incandescent_mix);
      pixel.green = incandescentChannel(
          pixel.green, kIncandescentGreen, incandescent_mix);
      pixel.blue = incandescentChannel(pixel.blue, kIncandescentBlue, incandescent_mix);
    }

    if (bulb_opacity > 0.0F) {
      const float cover = kBulbCover[index & 0x03U];
      const float scale = (1.0F - bulb_opacity) + (cover * bulb_opacity);
      pixel.red = clampByte(static_cast<float>(pixel.red) * scale);
      pixel.green = clampByte(static_cast<float>(pixel.green) * scale);
      pixel.blue = clampByte(static_cast<float>(pixel.blue) * scale);
    }

    pixel.red = static_cast<std::uint8_t>(
        std::min<std::uint16_t>(255U, pixel.red + base_add));
    pixel.green = static_cast<std::uint8_t>(
        std::min<std::uint16_t>(255U, pixel.green + base_add));
    pixel.blue = static_cast<std::uint8_t>(
        std::min<std::uint16_t>(255U, pixel.blue + base_add));

    if (controls.temporal_dithering) {
      pixel.red = ditherChannel(pixel.red, index, state.dither_phase);
      pixel.green = ditherChannel(pixel.green, index, state.dither_phase);
      pixel.blue = ditherChannel(pixel.blue, index, state.dither_phase);
    }
    frame[index] = pixel;
  }

  if (!controls.vp_fix_prism_off) {
    applyPrismOverlay(frame, controls.prism_count);
  }

  if (controls.reverse_order) {
    for (std::size_t left = 0U, right = frame.size() - 1U;
         left < right;
         ++left, --right) {
      std::swap(frame[left], frame[right]);
    }
  }

  if (controls.temporal_dithering) {
    // Advance after use. The stored value at entry is the index this frame.
    state.dither_phase = static_cast<std::uint8_t>((state.dither_phase + 1U) & 0x03U);
  }
}

}  // namespace k1::core::visual
