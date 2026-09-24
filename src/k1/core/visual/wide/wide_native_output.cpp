#include "core/visual/wide/wide_native_output.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace k1::core::visual::wide {
namespace {

// Provenance: the constants and stage order below reproduce
// core/visual/product_output_treatment.cpp (DualMCU 9792248) so that the FP32
// treatment keeps the legacy law; test_visual_wide_native_output proves the
// relation to the legacy bytes.
constexpr float kIncandescentRed = 1.0000F;
constexpr float kIncandescentGreen = 0.4453F;
constexpr float kIncandescentBlue = 0.1562F;
constexpr float kBulbCover[4] = {0.25F, 1.00F, 0.25F, 0.00F};

constexpr std::array<float, 256> makeCode8Table() {
  std::array<float, 256> table{};
  for (std::size_t code = 0U; code < table.size(); ++code) {
    table[code] = static_cast<float>(code) / 255.0F;
  }
  return table;
}

constexpr std::array<float, 256> kCode8ToUnit = makeCode8Table();

float legacyClamp01(const float value) noexcept {
  return std::clamp(value, 0.0F, 1.0F);
}

bool pixel8MatchesRoundedDisplay(const ConstPixelSpan pixels,
                                 const WorkingFrame& display) noexcept {
  StageBoundaryCountersV1 discard{};
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    const WorkingRgbF32V1 bounded = sanitiseWorking(display[index], 1.0F, discard);
    const Pixel8 pixel = pixels[index];
    if (quantiseUnorm8(bounded.red) != pixel.red ||
        quantiseUnorm8(bounded.green) != pixel.green ||
        quantiseUnorm8(bounded.blue) != pixel.blue) {
      return false;
    }
  }
  return true;
}

WorkingRgbF32V1 scaled(const WorkingRgbF32V1& value, const float red,
                       const float green, const float blue) noexcept {
  return {value.red * red, value.green * green, value.blue * blue};
}

void applyPrismOverlayWide(WorkingFrame& frame, const float prism_count) noexcept {
  const std::uint8_t count =
      static_cast<std::uint8_t>(std::clamp(prism_count, 0.0F, 8.0F));
  if (count == 0U) {
    return;
  }
  constexpr std::size_t kLeft = (kPixelsPerChannel / 2U) - 1U;
  constexpr std::size_t kRight = kPixelsPerChannel / 2U;
  for (std::uint8_t prism = 0U; prism < count; ++prism) {
    const std::size_t distance = 2U + (static_cast<std::size_t>(prism) * 3U);
    if (distance > kLeft || (kRight + distance) >= kPixelsPerChannel) {
      break;
    }
    const std::uint8_t gain = static_cast<std::uint8_t>(48U - (prism * 4U));
    const float full = kCode8ToUnit[gain];
    const float half = kCode8ToUnit[static_cast<std::uint8_t>(gain / 2U)];
    WorkingRgbF32V1& left = frame[kLeft - distance];
    WorkingRgbF32V1& right = frame[kRight + distance];
    left.red = std::max(left.red, full);
    left.blue = std::max(left.blue, half);
    right.green = std::max(right.green, full);
    right.blue = std::max(right.blue, half);
  }
}

}  // namespace

float unitFromCode8V1(const std::uint8_t code) noexcept {
  return kCode8ToUnit[code];
}

WideCaptureProvenanceV1 captureRenderedFrameV1(const ChannelRenderState& channel,
                                               const WideRoutedChannelV1& wide,
                                               WorkingFrame& out) noexcept {
  const ConstPixelSpan pixels = channel.frame();
  if (wide.previous_frame_wide && pixels.size() == kPixelsPerChannel &&
      pixel8MatchesRoundedDisplay(pixels, wide.display)) {
    out = wide.display;
    return WideCaptureProvenanceV1::kWideRenderer;
  }
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    const Pixel8 pixel = pixels[index];
    out[index] = {kCode8ToUnit[pixel.red], kCode8ToUnit[pixel.green],
                  kCode8ToUnit[pixel.blue]};
  }
  return WideCaptureProvenanceV1::kLiftedPixel8;
}

void applyProductOutputTreatmentWideV1(WorkingFrame& frame,
                                       const ChannelVisualControls& controls,
                                       ProductOutputTreatmentState& state) noexcept {
  const float incandescent_mix = controls.vp_fix_secondary_clean
                                     ? 0.0F
                                     : legacyClamp01(controls.incandescent_filter);
  const float bulb_opacity = legacyClamp01(controls.bulb_opacity);
  float base = 0.0F;
  if (controls.base_coat && !controls.vp_fix_secondary_clean) {
    const float intensity = std::isfinite(controls.base_coat_intensity)
                                ? legacyClamp01(controls.base_coat_intensity)
                                : 0.0F;
    base = intensity * 20.0F / 255.0F;
  }
  const float mix_red = (1.0F - incandescent_mix) + (incandescent_mix * kIncandescentRed);
  const float mix_green =
      (1.0F - incandescent_mix) + (incandescent_mix * kIncandescentGreen);
  const float mix_blue =
      (1.0F - incandescent_mix) + (incandescent_mix * kIncandescentBlue);

  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    WorkingRgbF32V1 pixel = frame[index];
    if (controls.incandescent_mode) {
      const float peak = std::max({pixel.red, pixel.green, pixel.blue});
      pixel = {peak, peak * kIncandescentGreen, peak * kIncandescentBlue};
    } else if (incandescent_mix > 0.0F) {
      pixel = scaled(pixel, mix_red, mix_green, mix_blue);
    }
    if (bulb_opacity > 0.0F) {
      const float cover = kBulbCover[index & 0x03U];
      const float scale = (1.0F - bulb_opacity) + (cover * bulb_opacity);
      pixel = scaled(pixel, scale, scale, scale);
    }
    if (base > 0.0F) {
      pixel = {pixel.red + base, pixel.green + base, pixel.blue + base};
    }
    // Temporal dithering is an RGB8 device: not applied on the native path.
    frame[index] = pixel;
  }

  if (!controls.vp_fix_prism_off) {
    applyPrismOverlayWide(frame, controls.prism_count);
  }

  if (controls.reverse_order) {
    for (std::size_t left = 0U, right = kPixelsPerChannel - 1U; left < right;
         ++left, --right) {
      std::swap(frame[left], frame[right]);
    }
  }

  if (controls.temporal_dithering) {
    // Same advance as the legacy stage, so switching paths keeps the phase.
    state.dither_phase = static_cast<std::uint8_t>((state.dither_phase + 1U) & 0x03U);
  }
}

WideNativeOutputResultV1 resolveNativeOutputV1(
    ChannelRenderState& channel_a, const WideRoutedChannelV1& wide_a,
    ChannelRenderState& channel_b, const WideRoutedChannelV1& wide_b,
    const EndpointChannelIntentV1& intent_a,
    const EndpointChannelIntentV1& intent_b, const EndpointConfigV1& config,
    WideNativeOutputWorkspaceV1& workspace, DeviceRgb16Frame& out_a,
    DeviceRgb16Frame& out_b) noexcept {
  WideNativeOutputResultV1 result{};
  result.provenance[0] =
      captureRenderedFrameV1(channel_a, wide_a, workspace.working[0]);
  result.provenance[1] =
      captureRenderedFrameV1(channel_b, wide_b, workspace.working[1]);
  applyProductOutputTreatmentWideV1(workspace.working[0], channel_a.controls(),
                                    channel_a.outputTreatmentState());
  applyProductOutputTreatmentWideV1(workspace.working[1], channel_b.controls(),
                                    channel_b.outputTreatmentState());
  result.endpoint = resolveWideEndpointV1(
      workspace.working[0], workspace.working[1], intent_a, intent_b, config,
      workspace.endpoint, out_a, out_b);
  return result;
}

}  // namespace k1::core::visual::wide
