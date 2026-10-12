#include "core/visual/inhouse_waveform.h"

#include <algorithm>
#include <cmath>

namespace k1::core::visual {
namespace {
constexpr std::int32_t one = 65536;
constexpr float tau = 6.2831853071795864769F;
float clamp01(float x) noexcept {
  return std::isfinite(x) ? std::max(0.0F, std::min(1.0F, x)) : 0.0F;
}
float wrap(float x) noexcept {
  return std::isfinite(x) ? x - ::floorf(x) : 0.0F;
}
std::int32_t q(float x) noexcept {
  return static_cast<std::int32_t>(clamp01(x) * 65536.0F);
}
std::int32_t positiveQ(float x) noexcept {
  // Source SQ15x16 spectrum is not unit-clamped until chroma folding.
  if (!std::isfinite(x) || x <= 0.0F) return 0;
  return static_cast<std::int32_t>(std::min(32767.0F, x) * 65536.0F);
}
float f(std::int32_t x) noexcept { return static_cast<float>(x) / 65536.0F; }
std::int32_t mul(std::int32_t a, std::int32_t b) noexcept {
  return static_cast<std::int32_t>((static_cast<std::int64_t>(a) * b) / one);
}
std::int32_t divide(std::int32_t a, std::int32_t b) noexcept {
  return b > 0 ? static_cast<std::int32_t>((static_cast<std::int64_t>(a) * one) / b) : 0;
}
float ema(float value, float target, float dt, float time) noexcept {
  return value + (target - value) * (1.0F - ::expf(-dt / time));
}
float follow(float value, float target, float dt, float up, float down) noexcept {
  return ema(value, target, dt, target > value ? up : down);
}
std::uint16_t outputChannel(std::int32_t channel) noexcept {
  return static_cast<std::uint16_t>((static_cast<std::uint64_t>(
      std::max<std::int32_t>(0, std::min(one, channel))) * 65535U) / 65536U);
}
PaletteLinearRgb sample(std::uint16_t id, float coordinate, float brightness) noexcept {
  // Match palette_manual_colour's SQ15x16 coordinate/colour/level boundaries.
  coordinate = f(q(wrap(coordinate)));
  auto colour = sampleProductPaletteHd(id, coordinate);
  const auto level = q(brightness);
  return {f(mul(q(colour.red), level)), f(mul(q(colour.green), level)),
          f(mul(q(colour.blue), level))};
}
}  // namespace

void InhouseWaveformRenderer::reset() noexcept { *this = InhouseWaveformRenderer{}; }

bool InhouseWaveformRenderer::render(const InhouseWaveformAudio& audio,
    const InhouseWaveformControls& controls, std::uint64_t render_us,
    Pixel16* output, std::size_t count) noexcept {
  if (output == nullptr || count != 160U) return false;
  if (started_ && render_us <= last_render_us_) return false;
  // Preserve the source millis() arithmetic, including its fractional-scroll
  // boundary behaviour; the reconstructed first frame is one 100 Hz interval.
  float dt = started_ ? static_cast<float>(render_us / 1000U - last_render_us_ / 1000U) * 0.001F
                      : 10.0F * 0.001F;
  dt = std::max(0.001F, std::min(0.05F, dt));
  started_ = true;
  last_render_us_ = render_us;
  // Invalid, regressive or stale publication must drain naturally, never replay
  // the last loud sample indefinitely. Epoch changes permit a new generation.
  const bool ordered = audio.epoch > epoch_ ||
      (audio.epoch == epoch_ && audio.generation >= generation_ &&
       audio.capture_us >= newest_capture_us_);
  const bool fresh = audio.valid && audio.generation != 0U && ordered &&
      audio.analysis_end_sample_exclusive > audio.analysis_begin_sample &&
      audio.capture_us <= audio.publish_us && audio.publish_us <= render_us &&
      render_us - audio.capture_us <= 30000U;
  if (fresh) {
    epoch_ = audio.epoch;
    generation_ = audio.generation;
    newest_capture_us_ = audio.capture_us;
  }
  observation_.input_fresh = fresh;
  const float peak = fresh ? std::max(clamp01(audio.peak_scaled),
      audio.peak_summary_valid ? clamp01(audio.observed_peak_scaled) : 0.0F) : 0.0F;
  const float vu = fresh ? clamp01(audio.vu_level) : 0.0F;
  const bool silent = !fresh || (audio.silence && peak < 0.02F);

  // Reference get_smooth_spectrogram has symmetric .75 per-render coefficients.
  // The approved reconstruction cadence is 100 Hz. This explicit basis converts
  // frame coefficients to elapsed time; it does not date the historical binary.
  const auto spectrum_mix = q(1.0F - ::powf(0.25F, dt * 100.0F));
  for (std::size_t i = 0; i < spectrum_.size(); ++i) {
    const auto target = fresh ? positiveQ(audio.spectrum[i]) : 0;
    const auto distance = std::abs(target - spectrum_[i]);
    const auto step = mul(distance, spectrum_mix);
    spectrum_[i] += target > spectrum_[i] ? step : -step;
  }
  std::array<std::int32_t, 12> chroma{};
  const unsigned range = std::max(1U, std::min(80U, unsigned(controls.chroma_range)));
  const auto divisor = static_cast<std::int32_t>(static_cast<float>(range) / 12.0F * 65536.0F);
  for (unsigned i = 0; i < range; ++i) chroma[i % 12U] += divide(std::min(one, spectrum_[i]), divisor);
  chroma_max_ = std::max(q(0.01F), mul(chroma_max_, q(::powf(0.999F, dt * 100.0F))));
  const auto rise = q(1.0F - ::powf(0.95F, dt * 100.0F));
  for (auto value : chroma) if (value > chroma_max_) chroma_max_ += mul(value - chroma_max_, rise);
  const auto multiplier = divide(one, chroma_max_);
  std::int32_t maximum = 0, sum = 0;
  for (auto& value : chroma) { value = mul(value, multiplier); maximum = std::max(maximum, value); sum += value; }
  const auto flatness = maximum - sum / 12;
  const auto gate = maximum <= q(0.08F) || flatness <= q(0.08F) ? 0 :
      flatness >= q(0.28F) ? one : divide(flatness - q(0.08F), q(0.28F) - q(0.08F));
  float x = 0.0F, y = 0.0F, total_weight = 0.0F;
  for (unsigned i = 0; i < 12U; ++i) {
    observation_.chroma_pregate[i] = maximum > q(0.08F) ? f(chroma[i]) : 0.0F;
    observation_.chroma[i] = f(std::min(one, mul(std::max<std::int32_t>(0, chroma[i] - q(0.1F)), gate)));
    const float weight = clamp01(observation_.chroma_pregate[i]);
    x += ::cosf(static_cast<float>(i) / 12.0F * tau) * weight;
    y += ::sinf(static_cast<float>(i) / 12.0F * tau) * weight;
    total_weight += weight;
  }
  const float centroid = (x == 0.0F && y == 0.0F) ? 0.0F : wrap(::atan2f(y, x) / tau);
  // The source held-coordinate helper only updates a strong musical centroid.
  // It is local here; no primary/secondary global can overwrite a peer anchor.
  if (total_weight > 0.0001F && ::sqrtf(x*x + y*y) / total_weight >= 0.08F) {
    observation_.held_coordinate = centroid;
    held_valid_ = true;
  }
  observation_.held_coordinate_valid = held_valid_;

  hold_ = follow(hold_, peak > 0.02F || vu > 0.02F ? 1.0F : 0.0F, dt, 0.02F, 0.50F);
  silence_scale_ = follow(silence_scale_, silent ? 0.0F : 1.0F, dt, 0.05F, 0.30F);
  observation_.peak_stage1 = ema(observation_.peak_stage1, peak, dt, 0.016F);
  observation_.peak_stage2 = ema(observation_.peak_stage2, observation_.peak_stage1, dt, 0.023F);
  float total_mag = 0.0F, energy = 0.0F;
  for (float value : observation_.chroma) { energy += value; total_mag += std::min(1.0F, value*value*1.5F); }
  energy /= 12.0F;
  const float bright = clamp01(1.0F - ::expf(-total_mag * 0.25F));
  float blend = clamp01(energy * 2.0F);
  if (energy < 0.15F && peak > 0.02F) blend = std::min(blend, 0.25F);
  const float seed_level = std::max(peak, std::max(vu, observation_.peak_stage2));
  const bool active = peak > 0.02F || vu > 0.02F;
  const float fallback_bright = active ? seed_level : 0.0F;
  const float walk = observation_.peak_stage2 * 0.25F;
  // Source manual colour offset belongs to fallback; particle colour is the
  // chromagram centroid + amplitude walk, not a generic palette attractor.
  observation_.palette_coordinate = wrap(centroid + walk);
  auto chroma_colour = sample(controls.palette_id, centroid + walk, bright);
  auto fallback = sample(controls.palette_id,
      (held_valid_ ? observation_.held_coordinate : controls.colour_offset) + walk, fallback_bright);
  if (fallback_bright > 0.0F && std::max(fallback.red, std::max(fallback.green, fallback.blue)) < 0.02F) {
    // Current source k1_emo_palette_fallback: brightest compiled palette entry.
    // It remains a colour of this authored palette, never a single-hue HSV path.
    const auto& palette = productPalette(controls.palette_id);
    unsigned best = 0, level = 0;
    for (unsigned i = 0; i < 16U; ++i) {
      const auto c = palette.compiled_fastled16[i];
      const unsigned n = unsigned(c.red) + c.green + c.blue;
      if (n > level) { level = n; best = i; }
    }
    const auto c = palette.compiled_fastled16[best];
    fallback = {f(q(float(c.red)/255.0F*fallback_bright)), f(q(float(c.green)/255.0F*fallback_bright)), f(q(float(c.blue)/255.0F*fallback_bright))};
  }
  const auto b = q(blend), ib = q(1.0F - blend);
  PaletteLinearRgb raw{f(std::min(one, mul(q(chroma_colour.red), b) + mul(q(fallback.red), ib))),
      f(std::min(one, mul(q(chroma_colour.green), b) + mul(q(fallback.green), ib))),
      f(std::min(one, mul(q(chroma_colour.blue), b) + mul(q(fallback.blue), ib)))};
  dot_.red = ema(dot_.red, raw.red, dt, 0.080F);
  dot_.green = ema(dot_.green, raw.green, dt, 0.080F);
  dot_.blue = ema(dot_.blue, raw.blue, dt, 0.080F);
  const float raw_max = std::max(raw.red, std::max(raw.green, raw.blue));
  const float raw_min = std::min(raw.red, std::min(raw.green, raw.blue));
  saturation_ema_ = ema(saturation_ema_, raw_max > 0.001F ? 1.0F-raw_min/raw_max : 0.0F, dt, 0.080F);
  const float hi = std::max(dot_.red, std::max(dot_.green, dot_.blue));
  const float lo = std::min(dot_.red, std::min(dot_.green, dot_.blue));
  if (hi > 0.001F && hi > lo && saturation_ema_ > 1.0F-lo/hi+0.01F) {
    const float k = hi*saturation_ema_/(hi-lo);
    dot_.red = std::max(0.0F, hi-(hi-dot_.red)*k);
    dot_.green = std::max(0.0F, hi-(hi-dot_.green)*k);
    dot_.blue = std::max(0.0F, hi-(hi-dot_.blue)*k);
  }
  const float quiet = std::max(std::min(hold_, silence_scale_), active ? seed_level : 0.0F);
  observation_.smoothed_colour = dot_;
  const float gain = std::max(hold_*silence_scale_, active ? seed_level : 0.0F);
  observation_.decay_rate = 0.8F + 3.5F*peak + (quiet < 0.9F ? 10.0F*(1.0F-quiet) : 0.0F);
  const auto fade = q(::expf(-observation_.decay_rate*dt));
  for (auto& pixel : history_) { pixel.red = mul(pixel.red, fade); pixel.green = mul(pixel.green, fade); pixel.blue = mul(pixel.blue, fade); }
  for (unsigned i = 0; i < 80U; ++i) history_[i] = {};
  const float motion = std::isfinite(controls.motion_scale) ? std::max(0.0F, std::min(4.0F, controls.motion_scale)) : 1.0F;
  observation_.scroll_remainder += 405.0F*dt*motion;
  unsigned steps = static_cast<unsigned>(observation_.scroll_remainder);
  observation_.scroll_remainder -= static_cast<float>(steps);
  steps = std::min(8U, steps);
  observation_.scroll_steps = steps;
  for (unsigned s = 0; s < steps; ++s) {
    for (unsigned i = 159U; i > 80U; --i) history_[i] = history_[i-1U];
    history_[80] = {};
  }
  observation_.position = std::min(159.0F, 80.0F + observation_.peak_stage2*0.7F*80.0F);
  const unsigned position = static_cast<unsigned>(observation_.position);
  const float fraction = observation_.position-static_cast<float>(position);
  const FixedRgb deposit = fresh ? FixedRgb{q(dot_.red*gain), q(dot_.green*gain), q(dot_.blue*gain)} : FixedRgb{};
  observation_.deposit_strength = f(deposit.red + deposit.green + deposit.blue);
  for (unsigned i = 0; i < 2U && position+i < 160U; ++i) {
    const auto weight = q(i == 0 ? 1.0F-fraction : fraction);
    auto& pixel = history_[position+i];
    const auto red = mul(deposit.red, weight), green = mul(deposit.green, weight), blue = mul(deposit.blue, weight);
    pixel.red += red; pixel.green += green; pixel.blue += blue;
    if (red != 0 || green != 0 || blue != 0) ++observation_.injection_count;
  }
  for (auto& pixel : history_) {
    const auto maximum_channel = std::max(pixel.red, std::max(pixel.green, pixel.blue));
    if (maximum_channel > one) {
      const auto scale = divide(one, maximum_channel);
      pixel.red = mul(pixel.red, scale); pixel.green = mul(pixel.green, scale); pixel.blue = mul(pixel.blue, scale);
    }
  }
  // Store source history before mirroring/display saturation/output attenuation.
  const float saturation = clamp01(controls.saturation);
  for (unsigned i = 0; i < 160U; ++i) {
    const auto& pixel = history_[controls.mirror && i < 80U ? 159U-i : i];
    const auto top = std::max(pixel.red, std::max(pixel.green, pixel.blue));
    output[i] = {outputChannel(top + static_cast<std::int32_t>(static_cast<float>(pixel.red-top)*saturation)),
                 outputChannel(top + static_cast<std::int32_t>(static_cast<float>(pixel.green-top)*saturation)),
                 outputChannel(top + static_cast<std::int32_t>(static_cast<float>(pixel.blue-top)*saturation))};
  }
  return true;
}
}  // namespace k1::core::visual
