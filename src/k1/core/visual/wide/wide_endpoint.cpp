#include "core/visual/wide/wide_endpoint.h"

#include <cmath>
#include <cstring>

namespace k1::core::visual::wide {
namespace {

// floor(value * full_scale + 0.5) for value in (0, 1), computed exactly.
// value = significand * 2^-shift with a 24-bit significand, so
// floor(significand * full_scale / 2^shift + 1/2) is one integer
// multiply-add and shift. No float rounding participates.
std::uint32_t quantiseExact(const float value,
                            const std::uint32_t full_scale) noexcept {
  if (!(value > 0.0F)) {
    return 0U;  // NaN, -0, +0, negative
  }
  if (!(value < 1.0F)) {
    return full_scale;  // >= 1, +Inf
  }
  std::uint32_t bits = 0U;
  std::memcpy(&bits, &value, sizeof(bits));
  const std::uint32_t biased_exponent = (bits >> 23U) & 0xFFU;
  if (biased_exponent == 0U) {
    return 0U;  // subnormal: value * full_scale < 0.5
  }
  const std::uint64_t significand =
      static_cast<std::uint64_t>((bits & 0x007FFFFFU) | 0x00800000U);
  const std::uint32_t shift = 150U - biased_exponent;  // >= 24 for value < 1
  if (shift >= 63U) {
    return 0U;  // value < 2^-39, so value * full_scale < 0.5
  }
  const std::uint64_t scaled = significand * full_scale;
  const std::uint64_t half = std::uint64_t{1U} << (shift - 1U);
  return static_cast<std::uint32_t>((scaled + half) >> shift);
}

bool finiteIn(const float value, const float low, const float high) noexcept {
  return std::isfinite(value) && value >= low && value <= high;
}

void fillBlack(DeviceRgb16Frame& frame) noexcept {
  for (DeviceRgb16V1& pixel : frame) {
    pixel = DeviceRgb16V1{};
  }
}

void fillBlack(DeviceDriveFrame& frame) noexcept {
  for (DeviceDriveRgbF32V1& pixel : frame) {
    pixel = DeviceDriveRgbF32V1{};
  }
}

bool channelFinite(const WorkingFrame& frame,
                   EndpointChannelReportV1& report) noexcept {
  for (const WorkingRgbF32V1& pixel : frame) {
    if (!std::isfinite(pixel.red)) {
      ++report.nonfinite;
    }
    if (!std::isfinite(pixel.green)) {
      ++report.nonfinite;
    }
    if (!std::isfinite(pixel.blue)) {
      ++report.nonfinite;
    }
  }
  return report.nonfinite == 0U;
}

// Artistic boundary clamp: the only place working intent is bounded to the
// reference range before channel/master intent is applied. Finite input only
// (checked per channel first). Signed zeros are classified on the bit
// pattern because -ffast-math (-fno-signed-zeros) may fold a float-level
// collapse; every zero leaves as +0.0.
float boundedIntent(const float value, EndpointChannelReportV1& report) noexcept {
  std::uint32_t bits = 0U;
  std::memcpy(&bits, &value, sizeof(bits));
  if ((bits & 0x7FFFFFFFU) == 0U) {
    return 0.0F;
  }
  if ((bits & 0x80000000U) != 0U) {
    ++report.negative;
    return 0.0F;
  }
  if (value > 1.0F) {
    ++report.overrange;
    return 1.0F;
  }
  return value;
}

float driveComponent(const float working, const float gain,
                     const DeviceTransferV1& transfer,
                     EndpointChannelReportV1& report) noexcept {
  const float bounded = boundedIntent(working, report);
  return applyDeviceTransferV1(transfer, bounded * gain);
}

// E3 for one channel. Returns the requested current in mA.
float resolveChannelDrive(const WorkingFrame& working,
                          const EndpointChannelIntentV1& intent,
                          const EndpointConfigV1& config,
                          DeviceDriveFrame& drive,
                          EndpointChannelReportV1& report) noexcept {
  if (!intent.enabled) {
    fillBlack(drive);
    return 0.0F;
  }
  if (!channelFinite(working, report)) {
    report.frame_valid = false;
    fillBlack(drive);
    return 0.0F;
  }
  const float gain = intent.intensity * config.master;
  float component_sum = 0.0F;
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    const WorkingRgbF32V1& source = working[index];
    DeviceDriveRgbF32V1& target = drive[index];
    target.red = driveComponent(source.red, gain, config.transfer, report);
    target.green = driveComponent(source.green, gain, config.transfer, report);
    target.blue = driveComponent(source.blue, gain, config.transfer, report);
    component_sum += target.red + target.green + target.blue;
  }
  return component_sum * config.full_component_current_ma;
}

void quantiseChannel(const DeviceDriveFrame& drive, const float scale,
                     DeviceRgb16Frame& out) noexcept {
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    const DeviceDriveRgbF32V1& source = drive[index];
    out[index] = {quantiseUnorm16(source.red * scale),
                  quantiseUnorm16(source.green * scale),
                  quantiseUnorm16(source.blue * scale)};
  }
}

}  // namespace

std::uint16_t quantiseUnorm16(const float value) noexcept {
  return static_cast<std::uint16_t>(quantiseExact(value, 65535U));
}

std::uint8_t quantiseUnorm8(const float value) noexcept {
  return static_cast<std::uint8_t>(quantiseExact(value, 255U));
}

LutValidationV1 validateDeviceLutV1(const float* const knots) noexcept {
  if (knots == nullptr) {
    return LutValidationV1::kMissing;
  }
  for (std::size_t index = 0U; index < kDeviceLutKnots; ++index) {
    if (!std::isfinite(knots[index])) {
      return LutValidationV1::kNonFinite;
    }
    if (knots[index] < 0.0F || knots[index] > 1.0F) {
      return LutValidationV1::kOutOfRange;
    }
  }
  if (knots[0] != 0.0F || knots[kDeviceLutKnots - 1U] != 1.0F) {
    return LutValidationV1::kEndpoints;
  }
  for (std::size_t index = 1U; index < kDeviceLutKnots; ++index) {
    if (knots[index] < knots[index - 1U]) {
      return LutValidationV1::kNonMonotonic;
    }
  }
  return LutValidationV1::kValid;
}

float applyDeviceTransferV1(const DeviceTransferV1& transfer,
                            const float x) noexcept {
  if (transfer.mode == DeviceTransferModeV1::kIdentity ||
      transfer.knots == nullptr) {
    return x;
  }
  constexpr float kLastSegment = static_cast<float>(kDeviceLutKnots - 1U);
  const float position = x * kLastSegment;
  if (!(position < kLastSegment)) {
    return transfer.knots[kDeviceLutKnots - 1U];
  }
  if (!(position > 0.0F)) {
    return transfer.knots[0];
  }
  const std::size_t lower = static_cast<std::size_t>(position);
  const float fraction = position - static_cast<float>(lower);
  if (!transfer.interpolate) {
    return transfer.knots[fraction < 0.5F ? lower : lower + 1U];
  }
  const float low = transfer.knots[lower];
  const float high = transfer.knots[lower + 1U];
  return low + fraction * (high - low);
}

EndpointValidationV1 validateEndpointConfigV1(
    const EndpointConfigV1& config, const EndpointChannelIntentV1& intent_a,
    const EndpointChannelIntentV1& intent_b) noexcept {
  if (!finiteIn(config.master, 0.0F, 1.0F)) {
    return EndpointValidationV1::kMaster;
  }
  if (!finiteIn(intent_a.intensity, 0.0F, 1.0F) ||
      !finiteIn(intent_b.intensity, 0.0F, 1.0F)) {
    return EndpointValidationV1::kIntensity;
  }
  if (!finiteIn(config.max_current_ma, 100.0F, 1.0e6F)) {
    return EndpointValidationV1::kCurrentLimit;
  }
  if (!finiteIn(config.full_component_current_ma, 0.001F, 1000.0F)) {
    return EndpointValidationV1::kCurrentModel;
  }
  if (config.transfer.mode == DeviceTransferModeV1::kLut257 &&
      validateDeviceLutV1(config.transfer.knots) != LutValidationV1::kValid) {
    return EndpointValidationV1::kTransfer;
  }
  if (config.transfer.mode != DeviceTransferModeV1::kIdentity &&
      config.transfer.mode != DeviceTransferModeV1::kLut257) {
    return EndpointValidationV1::kTransfer;
  }
  return EndpointValidationV1::kValid;
}

EndpointReportV1 resolveWideEndpointV1(
    const WorkingFrame& channel_a, const WorkingFrame& channel_b,
    const EndpointChannelIntentV1& intent_a,
    const EndpointChannelIntentV1& intent_b, const EndpointConfigV1& config,
    EndpointWorkspaceV1& workspace, DeviceRgb16Frame& out_a,
    DeviceRgb16Frame& out_b) noexcept {
  EndpointReportV1 report{};
  // E1: a configuration that escaped transaction-time validation fails safe.
  report.validation = validateEndpointConfigV1(config, intent_a, intent_b);
  if (report.validation != EndpointValidationV1::kValid) {
    fillBlack(workspace.drive[0]);
    fillBlack(workspace.drive[1]);
    fillBlack(out_a);
    fillBlack(out_b);
    report.limiter_scale = 0.0F;
    return report;
  }
  // E2: blackout dominates every artistic tail, persistence and background.
  if (config.blackout) {
    fillBlack(workspace.drive[0]);
    fillBlack(workspace.drive[1]);
    fillBlack(out_a);
    fillBlack(out_b);
    report.blackout_applied = true;
    report.limiter_scale = 0.0F;
    return report;
  }
  // E3: bounded intent -> channel/master intent -> device transfer.
  report.channel[0].requested_current_ma = resolveChannelDrive(
      channel_a, intent_a, config, workspace.drive[0], report.channel[0]);
  report.channel[1].requested_current_ma = resolveChannelDrive(
      channel_b, intent_b, config, workspace.drive[1], report.channel[1]);
  report.requested_current_ma = report.channel[0].requested_current_ma +
                                report.channel[1].requested_current_ma;
  // E4: one shared scale, computed in the drive domain.
  if (report.requested_current_ma > config.max_current_ma) {
    report.limiter_scale = config.max_current_ma / report.requested_current_ma;
  }
  // E5: quantise once.
  quantiseChannel(workspace.drive[0], report.limiter_scale, out_a);
  quantiseChannel(workspace.drive[1], report.limiter_scale, out_b);
  return report;
}

void packWs2816PixelV1(const DeviceRgb16V1& pixel,
                       std::uint8_t (&out)[kWs2816BytesPerPixel]) noexcept {
  out[0] = static_cast<std::uint8_t>(pixel.green >> 8U);
  out[1] = static_cast<std::uint8_t>(pixel.green & 0xFFU);
  out[2] = static_cast<std::uint8_t>(pixel.red >> 8U);
  out[3] = static_cast<std::uint8_t>(pixel.red & 0xFFU);
  out[4] = static_cast<std::uint8_t>(pixel.blue >> 8U);
  out[5] = static_cast<std::uint8_t>(pixel.blue & 0xFFU);
}

bool packWs2816ChannelV1(const DeviceRgb16Frame& frame,
                         std::uint8_t* const lane_low,
                         std::uint8_t* const lane_high) noexcept {
  if (lane_low == nullptr || lane_high == nullptr || lane_low == lane_high) {
    return false;
  }
  for (std::size_t index = 0U; index < kPixelsPerHalf; ++index) {
    std::uint8_t low_bytes[kWs2816BytesPerPixel]{};
    std::uint8_t high_bytes[kWs2816BytesPerPixel]{};
    packWs2816PixelV1(frame[index], low_bytes);
    packWs2816PixelV1(frame[kPixelsPerHalf + index], high_bytes);
    std::memcpy(lane_low + index * kWs2816BytesPerPixel, low_bytes,
                kWs2816BytesPerPixel);
    std::memcpy(lane_high + index * kWs2816BytesPerPixel, high_bytes,
                kWs2816BytesPerPixel);
  }
  return true;
}

std::uint8_t code8RoundedFromUnorm16V1(const std::uint16_t value) noexcept {
  // 65535 is odd, so v * 255 / 65535 is never exactly half-way between codes.
  return static_cast<std::uint8_t>(
      (static_cast<std::uint32_t>(value) * 255U + 32767U) / 65535U);
}

std::uint8_t code8DitheredFromUnorm16V1(const std::uint16_t value,
                                        const std::size_t pixel_index,
                                        const std::uint8_t phase) noexcept {
  const std::uint32_t scaled = static_cast<std::uint32_t>(value) * 255U;
  const std::uint32_t base = scaled / 65535U;
  const std::uint32_t remainder = scaled % 65535U;
  const std::uint32_t slot =
      static_cast<std::uint32_t>((pixel_index + phase) & 0x03U);
  // floor(scaled / 65535 + (2 * slot + 1) / 8)
  //   = base + [8 * remainder >= (7 - 2 * slot) * 65535].
  // base == 255 implies remainder == 0, so the result never exceeds 255.
  const bool step_up = 8U * remainder >= (7U - 2U * slot) * 65535U;
  return static_cast<std::uint8_t>(base + (step_up ? 1U : 0U));
}

std::uint16_t unorm16FromCode8ReplicateV1(const std::uint8_t code) noexcept {
  return static_cast<std::uint16_t>(static_cast<std::uint16_t>(code) * 257U);
}

bool emitRgb8RoundedV1(const DeviceRgb16Frame& frame,
                       const PixelSpan out) noexcept {
  if (out.data() == nullptr || out.size() != kPixelsPerChannel) {
    return false;
  }
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    out[index] = {code8RoundedFromUnorm16V1(frame[index].red),
                  code8RoundedFromUnorm16V1(frame[index].green),
                  code8RoundedFromUnorm16V1(frame[index].blue)};
  }
  return true;
}

bool emitRgb8DitheredV1(const DeviceRgb16Frame& frame,
                        const Rgb8DitherStateV1& state,
                        const PixelSpan out) noexcept {
  if (out.data() == nullptr || out.size() != kPixelsPerChannel) {
    return false;
  }
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    out[index] = {
        code8DitheredFromUnorm16V1(frame[index].red, index, state.phase),
        code8DitheredFromUnorm16V1(frame[index].green, index, state.phase),
        code8DitheredFromUnorm16V1(frame[index].blue, index, state.phase)};
  }
  return true;
}

void notePresentedRgb8FrameV1(Rgb8DitherStateV1& state) noexcept {
  state.phase = static_cast<std::uint8_t>((state.phase + 1U) & 0x03U);
  ++state.presented_frames;
}

bool emitWorkingCompatRgb8RoundedV1(const WorkingFrame& frame,
                                    const PixelSpan out,
                                    StageBoundaryCountersV1& counters) noexcept {
  if (out.data() == nullptr || out.size() != kPixelsPerChannel) {
    return false;
  }
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    const WorkingRgbF32V1 bounded =
        sanitiseWorking(frame[index], 1.0F, counters);
    out[index] = {quantiseUnorm8(bounded.red), quantiseUnorm8(bounded.green),
                  quantiseUnorm8(bounded.blue)};
  }
  return true;
}

void buildVp05ComparisonFramesV1(const DeviceRgb16Frame& native,
                                 const std::uint8_t dither_phase,
                                 DeviceRgb16Frame& rounded8_expanded,
                                 DeviceRgb16Frame& dithered8_expanded) noexcept {
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    const DeviceRgb16V1& source = native[index];
    rounded8_expanded[index] = {
        unorm16FromCode8ReplicateV1(code8RoundedFromUnorm16V1(source.red)),
        unorm16FromCode8ReplicateV1(code8RoundedFromUnorm16V1(source.green)),
        unorm16FromCode8ReplicateV1(code8RoundedFromUnorm16V1(source.blue))};
    dithered8_expanded[index] = {
        unorm16FromCode8ReplicateV1(
            code8DitheredFromUnorm16V1(source.red, index, dither_phase)),
        unorm16FromCode8ReplicateV1(
            code8DitheredFromUnorm16V1(source.green, index, dither_phase)),
        unorm16FromCode8ReplicateV1(
            code8DitheredFromUnorm16V1(source.blue, index, dither_phase))};
  }
}

}  // namespace k1::core::visual::wide
