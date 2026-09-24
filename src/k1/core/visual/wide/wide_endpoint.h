#pragma once

// Wide endpoint: the single final conversion from reference light intent to
// native UNORM16 words, plus the WS2816 word packer and the explicit RGB8
// evaluation/compatibility adapters.
//
// Declared endpoint stage order (resolveWideEndpointV1):
//   E1 configuration check      invalid configuration -> both channels black,
//                               report.config_valid = false
//   E2 blackout                 infrastructure; dominates every artistic tail
//   E3 per channel              disabled -> black; any non-finite component ->
//                               whole channel black and frame_valid = false
//                               (the platform applies its invalid-frame
//                               policy); otherwise the artistic boundary clamp
//                               to [0, 1] (negative/overrange counted), then
//                               x intensity x master, then the device transfer
//                               (identity by default; the only gamma/device
//                               mapping boundary), giving drive in [0, 1]
//   E4 shared current limit     estimated in the drive domain from a declared
//                               linear model (full component = 20 mA, the
//                               legacy Pixel8 model); one scale for A and B
//   E5 quantise once            q(x) = floor(clamp(x, 0, 1) * 65535 + 0.5)
//
// Native words never pass through RGB8 and carry no residual dither. The RGB8
// adapters are separately named evaluation/compatibility outputs.

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/pixel.h"
#include "core/visual/wide/wide_types.h"

namespace k1::core::visual::wide {

// q(x) = floor(clamp(x, 0, 1) * 65535 + 0.5), exact for every float: the
// multiply runs in integer arithmetic on the IEEE-754 significand, so no
// float rounding (or compiler contraction) can move a value across a code
// boundary. Callers validate finiteness first; defensively NaN -> 0 and
// +Inf -> 65535.
[[nodiscard]] std::uint16_t quantiseUnorm16(float value) noexcept;

// The same exact law with full scale 255, for explicit RGB8 adapters only.
[[nodiscard]] std::uint8_t quantiseUnorm8(float value) noexcept;

// ---------------------------------------------------------------------------
// Device transfer: the single declared gamma/device-mapping boundary.
// Identity is the default and is an exact bypass (no arithmetic). A LUT is a
// versioned calibration asset: 257 knots over [0, 1], validated before
// activation (finite, within [0, 1], knot 0 = 0, knot 256 = 1, monotonic).
// LUT interpolation is its own flag and is independent of every temporal
// operator.
enum class DeviceTransferModeV1 : std::uint8_t {
  kIdentity = 0U,
  kLut257 = 1U,
};

inline constexpr std::size_t kDeviceLutKnots = 257U;

enum class LutValidationV1 : std::uint8_t {
  kValid = 0U,
  kMissing = 1U,
  kNonFinite = 2U,
  kOutOfRange = 3U,
  kEndpoints = 4U,
  kNonMonotonic = 5U,
};

[[nodiscard]] LutValidationV1 validateDeviceLutV1(const float* knots) noexcept;

struct DeviceTransferV1 final {
  DeviceTransferModeV1 mode = DeviceTransferModeV1::kIdentity;
  bool interpolate = true;
  const float* knots = nullptr;  // kDeviceLutKnots entries when kLut257
};

// x must already be bounded to [0, 1] and finite.
[[nodiscard]] float applyDeviceTransferV1(const DeviceTransferV1& transfer,
                                          float x) noexcept;

// ---------------------------------------------------------------------------
// Endpoint configuration and report.
struct EndpointChannelIntentV1 final {
  bool enabled = true;
  float intensity = 1.0F;  // channel intent, [0, 1]
};

struct EndpointConfigV1 final {
  float master = 1.0F;       // global master intent, [0, 1]
  bool blackout = false;     // infrastructure, not an artistic toggle
  DeviceTransferV1 transfer{};
  float max_current_ma = 5000.0F;          // shared A+B budget, [100, 1e6]
  float full_component_current_ma = 20.0F; // drive->current model, [.001, 1000]
};

enum class EndpointValidationV1 : std::uint8_t {
  kValid = 0U,
  kMaster = 1U,
  kIntensity = 2U,
  kCurrentLimit = 3U,
  kCurrentModel = 4U,
  kTransfer = 5U,
};

// Reports the first failing field in the declared order master, intensity
// (A then B), current limit, current model, transfer.
[[nodiscard]] EndpointValidationV1 validateEndpointConfigV1(
    const EndpointConfigV1& config, const EndpointChannelIntentV1& intent_a,
    const EndpointChannelIntentV1& intent_b) noexcept;

// Caller-owned drive scratch (3840 B for both channels). After a resolve it
// holds each channel's PRE-LIMIT drive (after clamp, intent and transfer);
// the emitted drive is that value times report.limiter_scale, quantised.
// Never aliases history.
struct EndpointWorkspaceV1 final {
  std::array<DeviceDriveFrame, kChannelCount> drive{};
};

struct EndpointChannelReportV1 final {
  bool frame_valid = true;
  std::uint32_t nonfinite = 0U;
  std::uint32_t negative = 0U;
  std::uint32_t overrange = 0U;
  float requested_current_ma = 0.0F;
};

struct EndpointReportV1 final {
  std::array<EndpointChannelReportV1, kChannelCount> channel{};
  EndpointValidationV1 validation = EndpointValidationV1::kValid;
  bool blackout_applied = false;
  float requested_current_ma = 0.0F;
  float limiter_scale = 1.0F;
};

EndpointReportV1 resolveWideEndpointV1(
    const WorkingFrame& channel_a, const WorkingFrame& channel_b,
    const EndpointChannelIntentV1& intent_a,
    const EndpointChannelIntentV1& intent_b, const EndpointConfigV1& config,
    EndpointWorkspaceV1& workspace, DeviceRgb16Frame& out_a,
    DeviceRgb16Frame& out_b) noexcept;

// ---------------------------------------------------------------------------
// WS2816 native words. Semantics aligned with Titan
// src/k1/core/visual/ws2816_pack.h (blob bd2bfa6f): per pixel the wire bytes
// are G_hi, G_lo, R_hi, R_lo, B_hi, B_lo; logical pixels 0..79 fill the low
// half-lane and 80..159 the high half-lane, each in ascending index order.
// Physical lane direction/crop remains a platform mapping.
inline constexpr std::size_t kWs2816BytesPerPixel = 6U;
inline constexpr std::size_t kWs2816BytesPerHalfLane =
    kPixelsPerHalf * kWs2816BytesPerPixel;

void packWs2816PixelV1(const DeviceRgb16V1& pixel,
                       std::uint8_t (&out)[kWs2816BytesPerPixel]) noexcept;

[[nodiscard]] bool packWs2816ChannelV1(const DeviceRgb16Frame& frame,
                                       std::uint8_t* lane_low,
                                       std::uint8_t* lane_high) noexcept;

// ---------------------------------------------------------------------------
// RGB8 evaluation / compatibility adapters (never part of the native path).
//
// Rounded:  code8 = floor(v * 255 / 65535 + 1/2), exact integer arithmetic.
// Dithered: four-phase ordered temporal dither with offsets 1/8, 3/8, 5/8,
//           7/8 of one RGB8 step; phase index (pixel + phase) & 3. The mean
//           over the four phases is within 1/8 step of v * 255 / 65535.
// Replicate: legacy lift code8 * 257 (REPLICATE8). It can only express 256
//           distinct words and is kept solely as a named compatibility lift.
[[nodiscard]] std::uint8_t code8RoundedFromUnorm16V1(std::uint16_t value) noexcept;
[[nodiscard]] std::uint8_t code8DitheredFromUnorm16V1(
    std::uint16_t value, std::size_t pixel_index, std::uint8_t phase) noexcept;
[[nodiscard]] std::uint16_t unorm16FromCode8ReplicateV1(std::uint8_t code) noexcept;

// Frame emitters require a span of exactly kPixelsPerChannel (160) logical
// pixels and write nothing (returning false) for any other length: a
// physical crop or topology mapping belongs to the platform adapter, never
// to the portable endpoint.
//
// The dither phase advances only when the output owner reports that a frame
// was actually presented; a skipped or busy submission must not advance it.
struct Rgb8DitherStateV1 final {
  std::uint8_t phase = 0U;
  std::uint32_t presented_frames = 0U;
};

[[nodiscard]] bool emitRgb8RoundedV1(const DeviceRgb16Frame& frame,
                                     PixelSpan out) noexcept;
[[nodiscard]] bool emitRgb8DitheredV1(const DeviceRgb16Frame& frame,
                                      const Rgb8DitherStateV1& state,
                                      PixelSpan out) noexcept;
void notePresentedRgb8FrameV1(Rgb8DitherStateV1& state) noexcept;

// Compatibility emission straight from the working domain into a legacy
// Pixel8 frame: artistic boundary clamp to [0, 1] then exact rounding to
// 8 bits. Used when a wide-rendered mode must feed the unchanged Pixel8
// treatment/gain/limit chain. Non-finite components become 0 and are
// counted; the caller decides whether that frame may be submitted.
[[nodiscard]] bool emitWorkingCompatRgb8RoundedV1(
    const WorkingFrame& frame, PixelSpan out,
    StageBoundaryCountersV1& counters) noexcept;

// VP-05 capture hook: the rounded-8-expanded and dithered-8-expanded words for
// the same wide scene as the native words, for matched optical comparison.
void buildVp05ComparisonFramesV1(const DeviceRgb16Frame& native,
                                 std::uint8_t dither_phase,
                                 DeviceRgb16Frame& rounded8_expanded,
                                 DeviceRgb16Frame& dithered8_expanded) noexcept;

}  // namespace k1::core::visual::wide
