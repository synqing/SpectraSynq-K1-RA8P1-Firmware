#pragma once

// Native 16-bit output path (DUR-011, VP part): the renderer's
// pre-quantisation frame reaches the WS2816 endpoint without an RGB8 round
// trip.
//
// Per channel, per frame, in this order:
//   1. renderRoutedProductChannelV1()            (wide_bloom.h)
//   2. captureRenderedFrameV1()                  pre-quantisation frame
//   3. applyProductOutputTreatmentWideV1()       FP32 output treatment
//   4. resolveWideEndpointV1() for both channels (E1-E5, wide_endpoint.h)
// resolveNativeOutputV1() performs 2-4 for both channels with caller-owned
// buffers. Everything is allocation-free and portable.
//
// Provenance is explicit. kWideRenderer means the frame is the wide route's
// FP32 display, bit for bit. kLiftedPixel8 means the frame is the
// pre-treatment Pixel8 frame lifted exactly as code / 255 and carries no
// renderer precision; callers must not present it as wide.

#include <array>
#include <cstdint>

#include "core/visual/channel_render_state.h"
#include "core/visual/wide/wide_bloom.h"
#include "core/visual/wide/wide_endpoint.h"
#include "core/visual/wide/wide_types.h"

namespace k1::core::visual::wide {

enum class WideCaptureProvenanceV1 : std::uint8_t {
  kWideRenderer = 1U,
  kLiftedPixel8 = 2U,
};

// The float nearest to code / 255 (constant table, independent of the
// reciprocal-math rewriting that -ffast-math permits for runtime division).
[[nodiscard]] float unitFromCode8V1(std::uint8_t code) noexcept;

// Call after renderRoutedProductChannelV1() for this frame and before any
// Pixel8 output treatment. Returns the wide display bit for bit only when the
// channel took the wide route on its latest render AND the channel's Pixel8
// frame still equals that display's rounded compatibility bytes. A stale
// display (the channel was not re-rendered) or an already treated Pixel8
// frame therefore falls back to kLiftedPixel8, never a false wide claim.
WideCaptureProvenanceV1 captureRenderedFrameV1(const ChannelRenderState& channel,
                                               const WideRoutedChannelV1& wide,
                                               WorkingFrame& out) noexcept;

// FP32 counterpart of applyProductOutputTreatment (product_output_treatment.cpp)
// with no byte truncation. Same stage order: incandescent mode or mix ->
// bulb cover -> base coat -> prism overlay -> reverse order. Differences,
// all declared:
//   - values are not saturated at 1.0 between stages; the endpoint's
//     artistic clamp applies it once
//   - base coat adds clamp(intensity, 0, 1) * 20 / 255 without truncating
//     the byte step (the legacy adds floor(intensity * 20) codes)
//   - prism gains are the legacy byte gains lifted exactly (48 - 4p and
//     (48 - 4p) / 2 with integer division), so prism levels match exactly
//   - temporal dithering is an RGB8 device and is NOT applied, but
//     state.dither_phase advances exactly as the legacy stage advances it
//     (once per call while temporal_dithering is set)
// Use exactly one of the two treatments per channel per frame on the same
// ProductOutputTreatmentState. If both outputs are produced in one frame,
// give one of them a copy of the state so the phase advances once.
void applyProductOutputTreatmentWideV1(WorkingFrame& frame,
                                       const ChannelVisualControls& controls,
                                       ProductOutputTreatmentState& state) noexcept;

// Caller-owned buffers for the two-channel helper (7680 + 3840 bytes).
struct WideNativeOutputWorkspaceV1 final {
  std::array<WorkingFrame, kChannelCount> working{};
  EndpointWorkspaceV1 endpoint{};
};

struct WideNativeOutputResultV1 final {
  std::array<WideCaptureProvenanceV1, kChannelCount> provenance{
      WideCaptureProvenanceV1::kLiftedPixel8,
      WideCaptureProvenanceV1::kLiftedPixel8};
  EndpointReportV1 endpoint{};
};

// Capture -> FP32 treatment -> resolveWideEndpointV1 for both channels.
// Advances each channel's outputTreatmentState() dither phase exactly once
// when temporal_dithering is set (see the treatment's note above).
WideNativeOutputResultV1 resolveNativeOutputV1(
    ChannelRenderState& channel_a, const WideRoutedChannelV1& wide_a,
    ChannelRenderState& channel_b, const WideRoutedChannelV1& wide_b,
    const EndpointChannelIntentV1& intent_a,
    const EndpointChannelIntentV1& intent_b, const EndpointConfigV1& config,
    WideNativeOutputWorkspaceV1& workspace, DeviceRgb16Frame& out_a,
    DeviceRgb16Frame& out_b) noexcept;

}  // namespace k1::core::visual::wide
