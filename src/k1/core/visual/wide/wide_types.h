#pragma once

// Wide visual path: declared colour domains, fixed-capacity frames and
// stage-boundary numeric policy. Portable core only: no board, LED-driver or
// allocation dependency. The 8-bit Pixel8 path is untouched by these types
// and remains the compatibility path.
//
// Colour domains (contract version kWideContractVersion):
//
//   LegacyPaletteCodeV1  Normalised legacy palette code relationships
//                        (byte / 255). A code, not measured light. The
//                        existing PaletteLinearRgb values belong here in spite
//                        of their name.
//   WorkingRgbF32V1      Reference light intent in device primaries,
//                        display-referred, non-negative, additive. 1.0 is the
//                        documented reference channel level; per-role
//                        headroom up to kLayerComponentMax and an accumulator
//                        bound of kAccumulatorComponentMax. Until measured,
//                        linearity against LED drive is a declared modelling
//                        assumption, not photometric truth.
//   DeviceDriveRgbF32V1  Bounded [0, 1] drive after artistic treatment,
//                        channel/master intent and the single device transfer
//                        (identity by default).
//   DeviceRgb16V1        Native UNORM16 command: three uint16 components,
//                        never RGB565. Produced once, at the endpoint.
//   CompatRgb8V1         Explicit RGB8 evaluation/compatibility code.
//
// The only conversion from LegacyPaletteCodeV1 to WorkingRgbF32V1 is the
// declared legacyCodeAsIntentV1 adapter (identity numerics, no transfer).
// The only gamma/device mapping boundary is the endpoint transfer stage.

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "core/visual/pixel_topology.h"

namespace k1::core::visual::wide {

inline constexpr std::uint16_t kWideContractVersion = 1U;

enum class ColourDomain : std::uint8_t {
  kNone = 0U,  // no colour input (scalar envelope or audio evidence)
  kLegacyPaletteCodeV1 = 1U,
  kWorkingRgbF32V1 = 2U,
  kDeviceDriveRgbF32V1 = 3U,
  kDeviceRgb16V1 = 4U,
  kCompatRgb8V1 = 5U,
};

struct LegacyPaletteCodeV1 final {
  static constexpr ColourDomain kDomain = ColourDomain::kLegacyPaletteCodeV1;
  float red = 0.0F;
  float green = 0.0F;
  float blue = 0.0F;
};

struct WorkingRgbF32V1 final {
  static constexpr ColourDomain kDomain = ColourDomain::kWorkingRgbF32V1;
  float red = 0.0F;
  float green = 0.0F;
  float blue = 0.0F;
};

struct DeviceDriveRgbF32V1 final {
  static constexpr ColourDomain kDomain = ColourDomain::kDeviceDriveRgbF32V1;
  float red = 0.0F;
  float green = 0.0F;
  float blue = 0.0F;
};

struct DeviceRgb16V1 final {
  static constexpr ColourDomain kDomain = ColourDomain::kDeviceRgb16V1;
  std::uint16_t red = 0U;
  std::uint16_t green = 0U;
  std::uint16_t blue = 0U;
};

static_assert(sizeof(LegacyPaletteCodeV1) == 12U);
static_assert(sizeof(WorkingRgbF32V1) == 12U);
static_assert(sizeof(DeviceDriveRgbF32V1) == 12U);
static_assert(sizeof(DeviceRgb16V1) == 6U);

// One logical channel: 160 pixels, centre pair 79/80. Channels A and B are
// always separate objects; no combined two-channel frame type exists.
using WorkingFrame = std::array<WorkingRgbF32V1, kPixelsPerChannel>;
using ScalarFrame = std::array<float, kPixelsPerChannel>;
using DeviceDriveFrame = std::array<DeviceDriveRgbF32V1, kPixelsPerChannel>;
using DeviceRgb16Frame = std::array<DeviceRgb16V1, kPixelsPerChannel>;

static_assert(sizeof(WorkingFrame) == 1920U);
static_assert(sizeof(DeviceRgb16Frame) == 960U);

// Declared headroom. A single layer role may carry up to four times the
// reference level per component; the pre-tone accumulator is bounded at
// twelve. Values above these are clamped at the stage boundary and counted.
inline constexpr float kLayerComponentMax = 4.0F;
inline constexpr float kAccumulatorComponentMax = 12.0F;

// Stage-boundary numeric policy for values entering a stage from outside the
// stage's own arithmetic (mode adapters, audio-derived amplitudes, controls).
// Non-finite input becomes exact zero, negative input becomes exact zero,
// input above the bound is clamped; every event is counted. The endpoint is
// stricter: a non-finite component invalidates the whole channel frame.
struct StageBoundaryCountersV1 final {
  std::uint32_t nonfinite = 0U;
  std::uint32_t negative = 0U;
  std::uint32_t overrange = 0U;

  [[nodiscard]] std::uint32_t total() const noexcept {
    return nonfinite + negative + overrange;
  }
};

// The classification uses the IEEE-754 bit pattern, not float comparisons:
// the mandated -ffast-math implies -fno-signed-zeros, under which the
// compiler may fold a float-level `-0.0 -> +0.0` collapse away. Every zero
// (either sign) leaves as +0.0 bits, so byte digests over wide state are
// stable.
[[nodiscard]] inline float sanitiseComponent(
    const float value, const float upper_bound,
    StageBoundaryCountersV1& counters) noexcept {
  std::uint32_t bits = 0U;
  std::memcpy(&bits, &value, sizeof(bits));
  if ((bits & 0x7F800000U) == 0x7F800000U) {
    ++counters.nonfinite;  // NaN or +/-Inf
    return 0.0F;
  }
  if ((bits & 0x7FFFFFFFU) == 0U) {
    return 0.0F;  // +0.0 or -0.0 -> +0.0
  }
  if ((bits & 0x80000000U) != 0U) {
    ++counters.negative;
    return 0.0F;
  }
  if (value > upper_bound) {
    ++counters.overrange;
    return upper_bound;
  }
  return value;
}

[[nodiscard]] inline WorkingRgbF32V1 sanitiseWorking(
    const WorkingRgbF32V1& value, const float upper_bound,
    StageBoundaryCountersV1& counters) noexcept {
  return {sanitiseComponent(value.red, upper_bound, counters),
          sanitiseComponent(value.green, upper_bound, counters),
          sanitiseComponent(value.blue, upper_bound, counters)};
}

// Declared adapter: legacy palette code relationships are taken as the
// reference light intent with identity numerics (no sRGB decode, no gamma).
// A calibrated profile would be a separately versioned adapter.
[[nodiscard]] constexpr WorkingRgbF32V1 legacyCodeAsIntentV1(
    const LegacyPaletteCodeV1& code) noexcept {
  return {code.red, code.green, code.blue};
}

[[nodiscard]] constexpr bool isExactBlack(const WorkingRgbF32V1& value) noexcept {
  return value.red == 0.0F && value.green == 0.0F && value.blue == 0.0F;
}

[[nodiscard]] constexpr bool isExactBlack(const DeviceRgb16V1& value) noexcept {
  return value.red == 0U && value.green == 0U && value.blue == 0U;
}

[[nodiscard]] constexpr bool operator==(const DeviceRgb16V1& left,
                                        const DeviceRgb16V1& right) noexcept {
  return left.red == right.red && left.green == right.green &&
         left.blue == right.blue;
}

[[nodiscard]] constexpr bool operator!=(const DeviceRgb16V1& left,
                                        const DeviceRgb16V1& right) noexcept {
  return !(left == right);
}

}  // namespace k1::core::visual::wide
