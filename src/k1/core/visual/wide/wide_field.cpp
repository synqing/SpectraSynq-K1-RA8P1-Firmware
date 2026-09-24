#include "core/visual/wide/wide_field.h"

#include <cmath>

namespace k1::core::visual::wide {
namespace {

bool finiteIn(const float value, const float low, const float high) noexcept {
  return std::isfinite(value) && value >= low && value <= high;
}

float maxOf(const float left, const float right) noexcept {
  return left > right ? left : right;
}

std::size_t distanceOf(const std::size_t pixel) noexcept {
  return pixel >= kCentreRight ? pixel - kCentreRight : kCentreLeft - pixel;
}

// Outward target coordinate of pixel `index` after `shift_px`.
float outwardTarget(const std::size_t index, const float shift_px) noexcept {
  return index <= kCentreLeft ? static_cast<float>(index) - shift_px
                              : static_cast<float>(index) + shift_px;
}

void splatScalar(float* destination, const std::size_t size,
                 const float target, const float value,
                 const float retention) noexcept {
  const float lower_f = std::floor(target);
  const int lower = static_cast<int>(lower_f);
  const float fraction = target - lower_f;
  const int upper = lower + 1;
  if (lower >= 0 && lower < static_cast<int>(size)) {
    destination[lower] += value * ((1.0F - fraction) * retention);
  }
  if (upper >= 0 && upper < static_cast<int>(size)) {
    destination[upper] += value * (fraction * retention);
  }
}

}  // namespace

void composeAddV1(WorkingFrame& accumulator, const WorkingFrame& layer,
                  const float gain) noexcept {
  if (!(gain > 0.0F) || !std::isfinite(gain)) {
    return;  // zero (or invalid) gain contributes nothing
  }
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    accumulator[index].red += gain * layer[index].red;
    accumulator[index].green += gain * layer[index].green;
    accumulator[index].blue += gain * layer[index].blue;
  }
}

void composeWeightedMixV1(WorkingFrame& out, const WorkingFrame& a,
                          const WorkingFrame& b, float weight_b) noexcept {
  if (!finiteIn(weight_b, 0.0F, 1.0F)) {
    weight_b = weight_b > 1.0F ? 1.0F : 0.0F;
  }
  const float weight_a = 1.0F - weight_b;
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    out[index] = {weight_a * a[index].red + weight_b * b[index].red,
                  weight_a * a[index].green + weight_b * b[index].green,
                  weight_a * a[index].blue + weight_b * b[index].blue};
  }
}

void composePremultipliedOverV1(WorkingFrame& destination,
                                const WorkingFrame& source_premultiplied,
                                const ScalarFrame& source_alpha) noexcept {
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    float alpha = source_alpha[index];
    if (!finiteIn(alpha, 0.0F, 1.0F)) {
      alpha = alpha > 1.0F ? 1.0F : 0.0F;
    }
    const float keep = 1.0F - alpha;
    destination[index] = {
        source_premultiplied[index].red + keep * destination[index].red,
        source_premultiplied[index].green + keep * destination[index].green,
        source_premultiplied[index].blue + keep * destination[index].blue};
  }
}

void composeMaxV1(WorkingFrame& accumulator, const WorkingFrame& layer) noexcept {
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    accumulator[index] = {maxOf(accumulator[index].red, layer[index].red),
                          maxOf(accumulator[index].green, layer[index].green),
                          maxOf(accumulator[index].blue, layer[index].blue)};
  }
}

void boundAccumulatorV1(WorkingFrame& accumulator,
                        StageBoundaryCountersV1& counters) noexcept {
  for (WorkingRgbF32V1& pixel : accumulator) {
    pixel = sanitiseWorking(pixel, kAccumulatorComponentMax, counters);
  }
}

void clearFrameV1(WorkingFrame& frame) noexcept {
  for (WorkingRgbF32V1& pixel : frame) {
    pixel = WorkingRgbF32V1{};
  }
}

bool shiftOutwardV1(const WorkingFrame& source, WorkingFrame& destination,
                    const float shift_px, const float retention) noexcept {
  clearFrameV1(destination);
  if (!std::isfinite(shift_px) || shift_px < 0.0F ||
      !finiteIn(retention, 0.0F, 1.0F)) {
    return false;
  }
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    const WorkingRgbF32V1 value = source[index];
    if (isExactBlack(value)) {
      continue;
    }
    const float target = outwardTarget(index, shift_px);
    const float lower_f = std::floor(target);
    const int lower = static_cast<int>(lower_f);
    const float fraction = target - lower_f;
    const float lower_weight = (1.0F - fraction) * retention;
    const float upper_weight = fraction * retention;
    if (lower >= 0 && lower < static_cast<int>(kPixelsPerChannel)) {
      WorkingRgbF32V1& cell = destination[static_cast<std::size_t>(lower)];
      cell.red += value.red * lower_weight;
      cell.green += value.green * lower_weight;
      cell.blue += value.blue * lower_weight;
    }
    const int upper = lower + 1;
    if (upper >= 0 && upper < static_cast<int>(kPixelsPerChannel)) {
      WorkingRgbF32V1& cell = destination[static_cast<std::size_t>(upper)];
      cell.red += value.red * upper_weight;
      cell.green += value.green * upper_weight;
      cell.blue += value.blue * upper_weight;
    }
  }
  return true;
}

void applyMirrorV1(WorkingFrame& frame, const MirrorPolicyV1 policy) noexcept {
  if (policy == MirrorPolicyV1::kNone) {
    return;
  }
  for (std::size_t distance = 0U; distance < kPixelsPerHalf; ++distance) {
    const MirroredPair pair = mirroredPair(distance);
    if (policy == MirrorPolicyV1::kRightToLeft) {
      frame[pair.left] = frame[pair.right];
    } else if (policy == MirrorPolicyV1::kLeftToRight) {
      frame[pair.right] = frame[pair.left];
    }
  }
}

void clearHistoryV1(HistoryFieldV1& field) noexcept {
  for (float& value : field.storage) {
    value = 0.0F;
  }
}

void setHistoryMeaningV1(HistoryFieldV1& field,
                         const HistoryMeaningV1 meaning) noexcept {
  if (field.meaning == meaning) {
    return;
  }
  clearHistoryV1(field);
  field.meaning = meaning;
  ++field.generation;
  ++field.cuts;
}

bool transportHistoryOutwardV1(HistoryFieldV1& field, const float shift_px,
                               const float retention,
                               WorkingFrame& scratch) noexcept {
  if (!std::isfinite(shift_px) || shift_px < 0.0F ||
      !finiteIn(retention, 0.0F, 1.0F)) {
    return false;
  }
  if (field.meaning == HistoryMeaningV1::kScalarRecolouredNow) {
    float* const data = field.storage.data();
    float* const spare = field.storage.data() + kPixelsPerChannel;
    for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
      spare[index] = 0.0F;
    }
    for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
      const float value = data[index];
      if (value == 0.0F) {
        continue;
      }
      splatScalar(spare, kPixelsPerChannel, outwardTarget(index, shift_px),
                  value, retention);
    }
    for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
      data[index] = spare[index];
      spare[index] = 0.0F;
    }
    return true;
  }
  // RGB meaning: copy the retained colours into the caller's scratch, then
  // splat them back into the (cleared) history storage.
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    scratch[index] = historyRgbAtV1(field, index);
  }
  clearHistoryV1(field);
  float* const red = field.storage.data();
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    const WorkingRgbF32V1 value = scratch[index];
    if (isExactBlack(value)) {
      continue;
    }
    const float target = outwardTarget(index, shift_px);
    const float lower_f = std::floor(target);
    const int lower = static_cast<int>(lower_f);
    const float fraction = target - lower_f;
    const float lower_weight = (1.0F - fraction) * retention;
    const float upper_weight = fraction * retention;
    if (lower >= 0 && lower < static_cast<int>(kPixelsPerChannel)) {
      float* const cell = red + 3U * static_cast<std::size_t>(lower);
      cell[0] += value.red * lower_weight;
      cell[1] += value.green * lower_weight;
      cell[2] += value.blue * lower_weight;
    }
    const int upper = lower + 1;
    if (upper >= 0 && upper < static_cast<int>(kPixelsPerChannel)) {
      float* const cell = red + 3U * static_cast<std::size_t>(upper);
      cell[0] += value.red * upper_weight;
      cell[1] += value.green * upper_weight;
      cell[2] += value.blue * upper_weight;
    }
  }
  return true;
}

bool advectBodyOutwardV1(HistoryFieldV1& field, const float shift_px,
                         const float retention,
                         const WorkingRgbF32V1& emission_now,
                         const float amplitude_now,
                         const WorkingRgbF32V1& emission_previous,
                         const float amplitude_previous,
                         WorkingFrame& scratch) noexcept {
  if (!std::isfinite(shift_px) || shift_px < 0.0F ||
      !finiteIn(retention, 0.0F, 1.0F)) {
    return false;
  }
  const bool scalar = field.meaning == HistoryMeaningV1::kScalarRecolouredNow;
  // Snapshot the previous field (distance-indexed per half).
  float* const spare = field.storage.data() + kPixelsPerChannel;
  for (std::size_t pixel = 0U; pixel < kPixelsPerChannel; ++pixel) {
    if (scalar) {
      spare[pixel] = field.storage[pixel];
    } else {
      scratch[pixel] = historyRgbAtV1(field, pixel);
    }
  }
  const float log2_retention =
      retention > 0.0F ? std::log2(retention) : -1000.0F;
  for (std::size_t distance = 0U; distance < kPixelsPerHalf; ++distance) {
    const float d = static_cast<float>(distance);
    float weight_now = 0.0F;
    float weight_previous = 0.0F;
    float weight_lower = 0.0F;
    float weight_upper = 0.0F;
    std::size_t lower = 0U;
    bool emitted = false;
    if (shift_px == 0.0F) {
      if (distance == 0U) {
        weight_now = 1.0F;
        emitted = true;
      } else {
        lower = distance;
        weight_lower = retention;
      }
    } else if (d < shift_px) {
      const float w = d / shift_px;
      const float decay = std::exp2(w * log2_retention);
      weight_now = (1.0F - w) * decay;
      weight_previous = w * decay;
      emitted = true;
    } else {
      const float x = d - shift_px;
      const float lower_f = std::floor(x);
      lower = static_cast<std::size_t>(lower_f);
      const float fraction = x - lower_f;
      weight_lower = (1.0F - fraction) * retention;
      weight_upper = fraction * retention;
    }
    const std::size_t upper = lower + 1U;
    for (const bool right : {true, false}) {
      const std::size_t pixel = right ? kCentreRight + distance
                                      : kCentreLeft - distance;
      const std::size_t source_lower =
          right ? kCentreRight + lower : kCentreLeft - lower;
      const bool upper_on_strip = upper < kPixelsPerHalf;
      const std::size_t source_upper =
          upper_on_strip ? (right ? kCentreRight + upper : kCentreLeft - upper)
                         : source_lower;
      if (scalar) {
        float value = 0.0F;
        if (emitted) {
          value = weight_now * amplitude_now +
                  weight_previous * amplitude_previous;
        } else {
          value = weight_lower * spare[source_lower] +
                  (upper_on_strip ? weight_upper * spare[source_upper] : 0.0F);
        }
        field.storage[pixel] = value;
      } else {
        WorkingRgbF32V1 value{};
        if (emitted) {
          value = {weight_now * emission_now.red +
                       weight_previous * emission_previous.red,
                   weight_now * emission_now.green +
                       weight_previous * emission_previous.green,
                   weight_now * emission_now.blue +
                       weight_previous * emission_previous.blue};
        } else {
          const WorkingRgbF32V1& a = scratch[source_lower];
          const WorkingRgbF32V1 b =
              upper_on_strip ? scratch[source_upper] : WorkingRgbF32V1{};
          value = {weight_lower * a.red + weight_upper * b.red,
                   weight_lower * a.green + weight_upper * b.green,
                   weight_lower * a.blue + weight_upper * b.blue};
        }
        field.storage[3U * pixel] = value.red;
        field.storage[3U * pixel + 1U] = value.green;
        field.storage[3U * pixel + 2U] = value.blue;
      }
    }
  }
  if (scalar) {
    for (std::size_t pixel = 0U; pixel < kPixelsPerChannel; ++pixel) {
      spare[pixel] = 0.0F;
    }
  }
  return true;
}

void injectHistoryCentreV1(HistoryFieldV1& field,
                           const WorkingRgbF32V1& colour,
                           const float amplitude) noexcept {
  if (field.meaning == HistoryMeaningV1::kScalarRecolouredNow) {
    field.storage[kCentreLeft] = amplitude;
    field.storage[kCentreRight] = amplitude;
    return;
  }
  for (const std::size_t pixel : {kCentreLeft, kCentreRight}) {
    field.storage[3U * pixel] = colour.red;
    field.storage[3U * pixel + 1U] = colour.green;
    field.storage[3U * pixel + 2U] = colour.blue;
  }
}

void renderHistoryLayerV1(const HistoryFieldV1& field,
                          const DistanceColourTable& colour_by_distance,
                          WorkingFrame& layer) noexcept {
  for (std::size_t pixel = 0U; pixel < kPixelsPerChannel; ++pixel) {
    if (field.meaning == HistoryMeaningV1::kScalarRecolouredNow) {
      const float amplitude = field.storage[pixel];
      const WorkingRgbF32V1& colour = colour_by_distance[distanceOf(pixel)];
      layer[pixel] = {amplitude * colour.red, amplitude * colour.green,
                      amplitude * colour.blue};
    } else {
      layer[pixel] = historyRgbAtV1(field, pixel);
    }
  }
}

float historyScalarAtV1(const HistoryFieldV1& field,
                        const std::size_t pixel) noexcept {
  return pixel < kPixelsPerChannel ? field.storage[pixel] : 0.0F;
}

WorkingRgbF32V1 historyRgbAtV1(const HistoryFieldV1& field,
                               const std::size_t pixel) noexcept {
  if (pixel >= kPixelsPerChannel) {
    return {};
  }
  return {field.storage[3U * pixel], field.storage[3U * pixel + 1U],
          field.storage[3U * pixel + 2U]};
}

}  // namespace k1::core::visual::wide
