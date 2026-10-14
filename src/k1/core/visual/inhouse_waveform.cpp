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
  // Source normalises stop channels BEFORE interpolation. The general product
  // sampler normalises afterwards; algebraically equal but not bit-identical
  // at Q16 boundaries (mode 11's offset colour exposes that distinction).
  const auto& palette = productPalette(id);
  if (palette.stop_count == 0U || palette.stops == nullptr) return {};
  unsigned i = 0;
  while (i+1U < palette.stop_count &&
         float(palette.stops[i+1U].position)/255.0F < coordinate) ++i;
  const auto convert=[](const ProductPaletteStop& stop) {
    return PaletteLinearRgb{float(stop.colour.red)/255.0F,
        float(stop.colour.green)/255.0F,float(stop.colour.blue)/255.0F};
  };
  PaletteLinearRgb colour{};
  if (i+1U >= palette.stop_count || coordinate <= float(palette.stops[0].position)/255.0F) {
    colour = convert(palette.stops[coordinate <= float(palette.stops[0].position)/255.0F ? 0U : palette.stop_count-1U]);
  } else {
    const float position=float(palette.stops[i].position)/255.0F;
    const float span=float(palette.stops[i+1U].position)/255.0F-position;
    const float fraction=span>1.0e-6F ? clamp01((coordinate-position)/span) : 0.0F;
    const auto first=convert(palette.stops[i]), next=convert(palette.stops[i+1U]);
    colour={first.red+(next.red-first.red)*fraction,
            first.green+(next.green-first.green)*fraction,
            first.blue+(next.blue-first.blue)*fraction};
  }
  const auto level = q(brightness);
  return {f(mul(q(colour.red), level)), f(mul(q(colour.green), level)),
          f(mul(q(colour.blue), level))};
}
}  // namespace

void InhouseWaveformRenderer::reset() noexcept {
  // Do not put an entire 160-pixel renderer temporary on the render-thread stack.
  history_.fill({});
  spectrum_.fill(0);
  chroma_max_ = 65;
  hold_ = silence_scale_ = saturation_ema_ = 0.0F;
  dot_ = {};
  held_valid_ = started_ = false;
  last_render_us_ = newest_capture_us_ = epoch_ = generation_ = 0;
  observation_ = {};
  mode_ = 32;
#if defined(K1_INHOUSE_MAPPING_V9)
  intensity_.fill(0);
  colour_.fill({});
  offset_.fill(0.0F);
  deposit_offset_ = 0.0F;
  colour_position_ = colour_velocity_ = colour_anchor_ = centroid_ = centroid_strength_ = 0.0F;
  low_previous_.fill(0.0F);
  density_fast_ = density_slow_ = accent_ = 0.0F;
  luma_ref_ = -1.0F;
  release_fast_ = release_slow_ = release_ = 0.0F;
  release_peak_db_ = -120.0F;
  dip_ring_.fill(0.0F);
  dip_count_ = dip_index_ = 0;
  dip_acc_ = dip_acc_s_ = 0.0F;
  colour_valid_ = false;
  tonality_gate_ = 0;
  shape_.fill(0.0F);
#endif
}

bool InhouseWaveformRenderer::render(const InhouseWaveformAudio& audio,
    const InhouseWaveformControls& controls, std::uint64_t render_us,
    Pixel16* output, std::size_t count) noexcept {
  if (output == nullptr || count != 160U) return false;
  if (controls.mode != 32U && controls.mode != 18U && controls.mode != 8U &&
      controls.mode != 7U && controls.mode != 11U) return false;
  if (mode_ != controls.mode) { reset(); mode_ = controls.mode; }
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
  // Coherent Titan publications make the producer's confirmed quiet decisive.
  // The legacy peak veto defended torn snapshots; AGC-normalised room noise
  // must not override this flag or keep replenishing the trail.
  const bool silent = !fresh || audio.silence;
  const bool active = !silent && (peak > 0.02F || vu > 0.02F);

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

#if defined(K1_INHOUSE_MAPPING_V9)
  // v9 reads levelling stage 4's outputs (tonality gate, centroid); the stage itself
  // above is unchanged and shared with v8. Path P replaces only its own centroid,
  // from the pre-knee shape, because the knee'd vector collapses when the input is loud.
  if (controls.mode == 32U || controls.mode == 18U) {
    tonality_gate_ = gate;
    if (perPixelMapped()) applyShapeCentroidP(audio, controls, dt, fresh);
    else {
      centroid_ = centroid;
      centroid_strength_ = total_weight > 0.0001F ? ::sqrtf(x*x + y*y) / total_weight : 0.0F;
    }
    return renderV9(audio, controls, dt, fresh, silent, output);
  }
#endif
  if (controls.mode != 32U) return renderFamily(audio, controls, dt, fresh, output);

  hold_ = follow(hold_, active ? 1.0F : 0.0F, dt, 0.02F, 0.50F);
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
  const FixedRgb deposit = !silent ? FixedRgb{q(dot_.red*gain), q(dot_.green*gain), q(dot_.blue*gain)} : FixedRgb{};
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

// The other four source modes share chroma preparation, but not Hybrid K1's
// particle colour, additive deposit, transport or smoothing. Palette state is
// owned here instead of the reference helper's function-local static anchor.
PaletteLinearRgb InhouseWaveformRenderer::familyColour(
    const InhouseWaveformControls& controls, float brightness, float offset) noexcept {
  float x = 0.0F, y = 0.0F, total = 0.0F, maximum = 0.0F;
  unsigned dominant = 0;
  for (unsigned i = 0; i < 12U; ++i) {
    const float weight = clamp01(observation_.chroma_pregate[i]);
    const float angle = (static_cast<float>(i) / 12.0F) * tau;
    x += ::cosf(angle) * weight; y += ::sinf(angle) * weight; total += weight;
    if (weight > maximum) { maximum = weight; dominant = i; }
  }
  if (total <= 0.0001F) {
    observation_.palette_coordinate = observation_.held_coordinate;
    return sample(controls.palette_id, observation_.held_coordinate, brightness);
  }
  float coordinate = ::atan2f(y, x) / tau;
  if (coordinate < 0.0F) coordinate += 1.0F;
  const float strength = ::sqrtf(x*x + y*y) / total;
  if (strength < 0.08F) {
    if (held_valid_) {
      float delta = coordinate - observation_.held_coordinate;
      if (delta > 0.5F) delta -= 1.0F;
      if (delta < -0.5F) delta += 1.0F;
      coordinate = wrap(observation_.held_coordinate + delta * clamp01(strength / 0.08F));
    } else coordinate = static_cast<float>(dominant) / 12.0F;
  } else {
    observation_.held_coordinate = coordinate;
    held_valid_ = true;
  }
  observation_.held_coordinate_valid = held_valid_;
  const float average = total / 12.0F;
  const float contrast = std::max(0.0F, maximum-average);
  const float energy = clamp01(maximum*0.62F + contrast*0.30F + average*0.08F);
  observation_.palette_coordinate = wrap(coordinate + offset);
  return sample(controls.palette_id, observation_.palette_coordinate,
                f(std::max(q(brightness), q(energy))));
}

bool InhouseWaveformRenderer::renderFamily(const InhouseWaveformAudio& audio,
    const InhouseWaveformControls& controls, float dt, bool fresh,
    Pixel16* output) noexcept {
  const auto mode = controls.mode;
  // Preserve unbounded source peak for the 7/8/11 follower and clamp only at
  // each original consumer. Confirmed Titan admission still vetoes new birth.
  const float peak = fresh && std::isfinite(audio.peak_scaled)
      ? std::max(0.0F,std::min(32767.0F,audio.peak_scaled)) : 0.0F;
  const float absolute_peak = clamp01(peak);
  const bool admitted = fresh && !audio.silence;
  const float motion = std::isfinite(controls.motion_scale)
      ? std::max(0.0F, std::min(4.0F, controls.motion_scale)) : 1.0F;
  const float contrast = std::isfinite(controls.square_iter)
      ? std::max(0.0F, std::min(8.0F, controls.square_iter)) : 1.0F;
  const float raw_floor = std::isfinite(audio.raw_floor_pcm)
      ? std::max(0.0F, audio.raw_floor_pcm) : 350.0F;
  const bool raw_active = fresh && std::isfinite(audio.raw_peak_pcm) &&
      audio.raw_peak_pcm > raw_floor*1.10F;
  const float vu = fresh ? clamp01(audio.vu_average) : 0.0F;
  float seed = absolute_peak;
  bool inject = admitted;
  float target_fade = 1.0F;
  float displacement = peak;
  float rate = 0.0F;
  unsigned maximum_steps = 8U;
  PaletteLinearRgb colour{};

  if (mode == 18U) {
    const float source_peak = fresh ? std::max(absolute_peak,
        audio.peak_summary_valid ? clamp01(audio.observed_peak_scaled) : 0.0F) : 0.0F;
    const float level = std::max(source_peak, fresh ? clamp01(audio.vu_level) : 0.0F);
    const bool presence = level >= 0.02F &&
        !(audio.spectral_energy < 0.08F && audio.novelty < 0.08F && level < 0.08F);
    inject = admitted && presence;
    target_fade = inject ? 1.0F-0.10F*source_peak : 0.82F;
    const bool tempo_fresh = fresh && audio.tempo_valid &&
        audio.tempo_capture_us <= last_render_us_ &&
        last_render_us_-audio.tempo_capture_us <= 30000U+kInhouseTempoUpdatePeriodUs &&
        std::isfinite(audio.tempo_bpm) && audio.tempo_bpm >= 60.0F &&
        audio.tempo_bpm <= 156.0F && std::isfinite(audio.tempo_phase01);
    float gate = tempo_fresh ? clamp01((audio.tempo_confidence-0.30F)/0.30F) : 0.0F;
    gate = gate*gate*(3.0F-2.0F*gate);
    const float tempo_rate = tempo_fresh ? 24.0F*(audio.tempo_bpm/60.0F)*
        (1.0F+0.5F*::cosf(6.28318530718F*audio.tempo_phase01)) : 0.0F;
    const float idle_rate = admitted ? 30.0F : 0.0F;
    rate = (idle_rate+(tempo_rate-idle_rate)*gate)*motion;
    maximum_steps = 30U;
    displacement = source_peak;
    if (inject) {
      colour = familyColour(controls, 0.0F, 0.0F);
      if (std::max(colour.red, std::max(colour.green, colour.blue)) < 0.02F && source_peak >= 0.02F) {
        colour = sample(controls.palette_id,
            held_valid_ ? observation_.held_coordinate : controls.colour_offset, source_peak);
        if (std::max(colour.red, std::max(colour.green, colour.blue)) < 0.02F) {
          const auto& palette = productPalette(controls.palette_id);
          unsigned best = 0, maximum = 0;
          for (unsigned i=0; i<16U; ++i) {
            const auto c=palette.compiled_fastled16[i];
            const unsigned sum=unsigned(c.red)+c.green+c.blue;
            if (sum>maximum) { maximum=sum; best=i; }
          }
          const auto c=palette.compiled_fastled16[best];
          colour={f(q(float(c.red)/255.0F*source_peak)),
                  f(q(float(c.green)/255.0F*source_peak)),
                  f(q(float(c.blue)/255.0F*source_peak))};
        }
      }
    }
  } else {
    // At the declared 100 Hz reconstruction this is bit-identical to source
    // SQ15x16(peak)*alpha + SQ15x16(previous)*(1-alpha).
    const float alpha = mode == 7U ? 0.05F : 0.08F;
    const float mix = dt == 10.0F*0.001F ? alpha : 1.0F-::powf(1.0F-alpha, dt*100.0F);
    observation_.peak_stage1 = f(mul(positiveQ(peak), q(mix)) +
        mul(positiveQ(observation_.peak_stage1), q(1.0F-mix)));
    observation_.peak_stage2 = observation_.peak_stage1;
    seed = std::max(absolute_peak, std::max(clamp01(observation_.peak_stage1), vu));
    float brightness = 0.0F;
    std::int32_t magnitude = 0;
    for (float bin : observation_.chroma) {
      float bright = bin;
      for (unsigned i=0; i<static_cast<unsigned>(contrast); ++i) bright *= bright;
      const float fraction = contrast-::floorf(contrast);
      if (fraction > 0.01F) bright = bright*(1.0F-fraction)+bright*bright*fraction;
      if (mode == 8U) { if (bright > 0.05F) magnitude += positiveQ(bright); }
      else brightness += std::min(1.0F, bright*1.5F)*(1.0F/12.0F);
    }
    if (mode == 8U) {
      if (magnitude < q(0.01F) && audio.vu_level > 0.02F) magnitude = q(audio.vu_level);
      if (magnitude > q(0.01F)) colour = familyColour(controls, f(std::min(one,magnitude)), 0.0F);
      const auto photons = q(std::isfinite(controls.photons)
          ? std::max(0.05F, std::min(1.0F,controls.photons)) : 1.0F);
      colour={f(mul(q(colour.red),photons)),f(mul(q(colour.green),photons)),f(mul(q(colour.blue),photons))};
      displacement = observation_.peak_stage1;
      if (std::fabs(displacement)<0.05F) displacement=0.0F;
      const float sensitivity = std::isfinite(controls.sensitivity)
          ? std::max(0.10F,std::min(20.0F,controls.sensitivity)) : 2.4F;
      displacement *= 0.7F/sensitivity;
      rate=100.0F; // Source ignores shift/speed arguments: one pixel/reference render.
      target_fade=1.0F-0.10F*absolute_peak;
    } else {
      inject = admitted && raw_active && (mode == 7U ? absolute_peak >= 0.08F
          : (absolute_peak >= 0.08F || vu >= 0.02F));
      // A missing PCM history is not permission to invent mode 11's shape.
      if (mode == 11U && !audio.pcm_history_valid) inject=false;
      colour = familyColour(controls, f(q(brightness)), 0.0F);
      float energy=0.0F; for (float bin : observation_.chroma) energy+=bin;
      energy/=12.0F;
      const auto blend=q(energy*2.0F), inverse=one-blend;
      const auto fallback=familyColour(controls, inject ? (mode == 7U ? absolute_peak : seed) : 0.0F,
                                      mode == 7U ? 0.18F : 0.22F);
      colour={f(std::min(one,mul(q(colour.red),blend)+mul(q(fallback.red),inverse))),
              f(std::min(one,mul(q(colour.green),blend)+mul(q(fallback.green),inverse))),
              f(std::min(one,mul(q(colour.blue),blend)+mul(q(fallback.blue),inverse)))};
      rate=120.0F*motion;
      target_fade=mode == 7U ? (inject ? 1.0F-0.10F*absolute_peak : 0.85F)
          : (inject ? std::max(0.985F,std::min(0.999F,1.0F-0.04F*(1.0F-seed))) : 0.985F);
    }
  }
  // Preserve mode 11's explicitly elapsed-time 120-basis linear decay. Other
  // per-render fades are reconstructed at 100 Hz and converted to elapsed time.
  auto fade = mode == 11U ? q(std::min(0.999F,1.0F-(1.0F-target_fade)*(dt*120.0F)))
      : q(dt == 10.0F*0.001F ? target_fade : ::powf(target_fade,dt*100.0F));
  if (mode == 7U && inject && dt == 10.0F*0.001F) {
    // Fast's original expression uses double 1.0 minus a float product before
    // SQ conversion. Preserve its truncation without a software-double call.
    fade = one-static_cast<std::int32_t>(::ceilf((0.10F*absolute_peak)*65536.0F));
  }
  for (auto& p : history_) { p.red=mul(p.red,fade);p.green=mul(p.green,fade);p.blue=mul(p.blue,fade); }
  observation_.scroll_remainder += rate*dt;
  unsigned steps=std::min(maximum_steps,static_cast<unsigned>(observation_.scroll_remainder));
  observation_.scroll_remainder-=static_cast<float>(steps);
  observation_.scroll_steps=steps;
  for (unsigned s=0;s<steps;++s) {
    if (mode == 11U) {
      for (unsigned i=0;i<79U;++i) history_[i]=history_[i+1U];
      history_[79]={};
    }
    const unsigned start=mode == 11U || controls.mirror ? 80U : 0U;
    for (unsigned i=159U;i>start;--i) history_[i]=history_[i-1U];
    history_[start]={};
  }
  observation_.deposit_strength=0.0F;
  observation_.smoothed_colour=colour;
  observation_.decay_rate=target_fade>0.0F ? -::logf(target_fade)*100.0F : 0.0F;
  const auto assign=[&](unsigned i, FixedRgb c) {
    history_[i]=c;
    if(c.red||c.green||c.blue) ++observation_.injection_count;
    observation_.deposit_strength+=f(c.red+c.green+c.blue);
  };
  if (inject && mode == 11U) {
    const auto gain=q(0.30F+0.70F*seed);
    const unsigned radius=std::min(10U,3U+static_cast<unsigned>(seed*7.0F));
    const float norm=std::max(1.0F,audio.raw_peak_pcm);
    for (unsigned r=0;r<=radius;++r) {
      const unsigned centre_sample=r*95U/radius;
      std::uint32_t sum=0;
      for (const auto& block:audio.pcm_history) for (int tap=-2;tap<=2;++tap) {
        const auto index=std::max(0,std::min(95,int(centre_sample)+tap));
        const std::int32_t value=block[static_cast<unsigned>(index)];
        sum+=static_cast<std::uint32_t>(value<0 ? -value : value);
      }
      const auto shape=q(::sqrtf(clamp01((static_cast<float>(sum)/20.0F)/norm)));
      const auto width=q(0.25F)+mul(q(1.0F-static_cast<float>(r)/static_cast<float>(radius+1U)),q(0.75F));
      const FixedRgb c{mul(mul(mul(q(colour.red),gain),width),shape),
                       mul(mul(mul(q(colour.green),gain),width),shape),
                       mul(mul(mul(q(colour.blue),gain),width),shape)};
      assign(79U-r,c);assign(80U+r,c);
    }
    observation_.position=80.0F;
  } else if (inject) {
    const float position=80.0F+std::max(-1.0F,std::min(1.0F,displacement))*80.0F;
    unsigned index=static_cast<unsigned>(std::max(0.0F,std::min(159.0F,position+0.5F)));
    if(controls.mirror) index=std::max(80U,index);
    observation_.position=static_cast<float>(index);
    assign(index,{q(colour.red),q(colour.green),q(colour.blue)});
  }
  // Source Tempo returns before mirroring on a dark frame; retain this precise
  // history rule instead of silently manufacturing a mirror on its drain path.
  if (controls.mirror && (mode != 18U || inject))
    for (unsigned i=0;i<80U;++i) history_[i]=history_[159U-i];
  const float saturation=clamp01(controls.saturation);
  for (unsigned i=0;i<160U;++i) {
    const auto& p=history_[i]; const auto top=std::max(p.red,std::max(p.green,p.blue));
    output[i]={outputChannel(top+static_cast<std::int32_t>(static_cast<float>(p.red-top)*saturation)),
               outputChannel(top+static_cast<std::int32_t>(static_cast<float>(p.green-top)*saturation)),
               outputChannel(top+static_cast<std::int32_t>(static_cast<float>(p.blue-top)*saturation))};
  }
  return true;
}

#if defined(K1_INHOUSE_MAPPING_V9)
namespace {
// Iteration 4 variant A stops (P6-IT4-PALETTE-VARIANTS-FROZEN.json): same positions and
// hue per stop as product palettes 32/39, with the dark low end lifted.
constexpr ProductPaletteStop kLifted32[] = {{0, {150, 0, 0}}, {42, {200, 0, 0}}, {84, {255, 0, 0}},
    {127, {255, 0, 45}}, {170, {255, 0, 255}}, {212, {255, 55, 45}}, {255, {255, 255, 0}}};
constexpr ProductPaletteStop kLifted39[] = {{0, {150, 0, 125}}, {32, {190, 0, 131}}, {64, {220, 0, 121}},
    {96, {220, 0, 70}}, {128, {255, 36, 24}}, {160, {255, 92, 0}}, {192, {255, 145, 0}},
    {218, {255, 190, 40}}, {240, {255, 70, 120}}, {255, {150, 0, 125}}};

PaletteLinearRgb sampleStops(const ProductPaletteStop* stops, unsigned count, float coordinate) noexcept {
  coordinate = f(q(wrap(coordinate)));  // same coordinate quantisation as sample()
  unsigned i = 0;
  while (i + 1U < count && float(stops[i + 1U].position) / 255.0F < coordinate) ++i;
  const auto convert = [](const ProductPaletteStop& stop) {
    return PaletteLinearRgb{float(stop.colour.red) / 255.0F, float(stop.colour.green) / 255.0F,
                            float(stop.colour.blue) / 255.0F};
  };
  PaletteLinearRgb colour{};
  if (i + 1U >= count || coordinate <= float(stops[0].position) / 255.0F) {
    colour = convert(stops[coordinate <= float(stops[0].position) / 255.0F ? 0U : count - 1U]);
  } else {
    const float position = float(stops[i].position) / 255.0F;
    const float span = float(stops[i + 1U].position) / 255.0F - position;
    const float fraction = span > 1.0e-6F ? clamp01((coordinate - position) / span) : 0.0F;
    const auto first = convert(stops[i]), next = convert(stops[i + 1U]);
    colour = {first.red + (next.red - first.red) * fraction, first.green + (next.green - first.green) * fraction,
              first.blue + (next.blue - first.blue) * fraction};
  }
  return {f(q(colour.red)), f(q(colour.green)), f(q(colour.blue))};
}
}  // namespace

// Palette colour for the v9 colour coordinate under the selected test variant.
PaletteLinearRgb InhouseWaveformRenderer::colourV9(std::uint16_t palette_id, float coordinate) const noexcept {
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  const InhouseV9Palette variant = v9_palette_;
#else
  constexpr InhouseV9Palette variant = kInhouseV9Palette;
#endif
  if (variant == InhouseV9Palette::kLiftedLowEnd && (palette_id == 31U || palette_id == 38U)) {
    return palette_id == 31U ? sampleStops(kLifted32, sizeof(kLifted32) / sizeof(kLifted32[0]), coordinate)
                             : sampleStops(kLifted39, sizeof(kLifted39) / sizeof(kLifted39[0]), coordinate);
  }
  if (variant == InhouseV9Palette::kRestrictedRange && (palette_id == 31U || palette_id == 38U)) {
    // Continuous fold of the colour circle onto [lo, hi], excluding the near-black ends.
    const float lo = palette_id == 31U ? 0.3294F : 0.2510F;
    const float hi = palette_id == 31U ? 1.0F : 0.9412F;
    const float c = wrap(coordinate);
    const float p = lo + (hi - lo) * (1.0F - std::fabs(2.0F * c - 1.0F));
    return sample(palette_id, std::min(p, 65535.0F / 65536.0F), 1.0F);
  }
  return sample(palette_id, coordinate, 1.0F);
}

bool InhouseWaveformRenderer::perPixelMapped() const noexcept {
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  return v9_palette_ == InhouseV9Palette::kPerPixelMapped;
#else
  return kInhouseV9Palette == InhouseV9Palette::kPerPixelMapped;
#endif
}

// Path P centroid. The knee'd stage-4 vector is left in place for the tonality gate and for
// every palette that is not per-pixel mapped. This one max-normalises the pre-knee shape so
// a louder input cannot flatten the harmony, then uses stage-4's spectrum_mix smoother.
void InhouseWaveformRenderer::applyShapeCentroidP(const InhouseWaveformAudio& audio,
    const InhouseWaveformControls& controls, float dt, bool fresh) noexcept {
  const float mix = 1.0F - ::powf(0.25F, dt * 100.0F);
  float incoming_peak = 0.0F;
  if (fresh) {
    for (const float value : audio.linear_shape) incoming_peak = std::max(incoming_peak, value);
  }
  for (std::size_t i = 0; i < shape_.size(); ++i) {
    const float target = incoming_peak > 1.0e-8F
        ? std::max(0.0F, audio.linear_shape[i]) / incoming_peak : 0.0F;
    const float distance = std::fabs(target - shape_[i]);
    shape_[i] += target > shape_[i] ? distance * mix : -distance * mix;
  }
  const unsigned range = std::max(1U, std::min(80U, static_cast<unsigned>(controls.chroma_range)));
  std::array<float, 12> notes{};
  for (unsigned i = 0; i < range; ++i) notes[i % 12U] += shape_[i];
  float maximum = 0.0F;
  for (const float note : notes) maximum = std::max(maximum, note);
  float x = 0.0F, y = 0.0F, total = 0.0F;
  if (maximum > 1.0e-8F) {
    for (unsigned i = 0; i < 12U; ++i) {
      const float weight = std::min(1.0F, std::max(0.0F, notes[i] / maximum));
      x += ::cosf(static_cast<float>(i) / 12.0F * tau) * weight;
      y += ::sinf(static_cast<float>(i) / 12.0F * tau) * weight;
      total += weight;
    }
  }
  centroid_ = (x == 0.0F && y == 0.0F) ? 0.0F : wrap(::atan2f(y, x) / tau);
  centroid_strength_ = total > 0.0001F ? ::sqrtf(x * x + y * y) / total : 0.0F;
}

// Path P colour offset, the K1 ae5d90a rule (v8 familyColour): the PRE-gate chroma centroid
// sets the offset every frame; a weak centroid (strength < 0.08) eases from the last strong
// one by strength / 0.08. No tonality gate, no speed cap, no smoothing.
void InhouseWaveformRenderer::updateOffsetP(const InhouseWaveformControls& controls, bool silent) noexcept {
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  if (v9_fault_ == InhouseV9Fault::kFrozenPosition) {
    colour_position_ = wrap(controls.colour_offset);
    observation_.palette_coordinate = colour_position_;
    return;
  }
#endif
  constexpr float kStrong = 0.08F;
  if (!silent && centroid_strength_ > 0.0F) {
    if (centroid_strength_ < kStrong && colour_valid_) {
      float delta = centroid_ - colour_anchor_;
      if (delta > 0.5F) delta -= 1.0F;
      if (delta < -0.5F) delta += 1.0F;
      colour_position_ = wrap(colour_anchor_ + delta * clamp01(centroid_strength_ / kStrong));
    } else {
      colour_position_ = colour_anchor_ = centroid_;
      colour_valid_ = true;
    }
  } else if (!colour_valid_) {
    colour_position_ = wrap(controls.colour_offset);
  }
  observation_.palette_coordinate = colour_position_;
}

// Colour is the slow layer. Confident harmony (tonality gate open, strong centroid, and
// audible light being deposited) pulls the palette coordinate with capped momentum. When
// confidence is low the colour holds, drifting only toward its own long-term confident
// anchor, never toward a weak centroid or a palette end.
void InhouseWaveformRenderer::updateColourV9(const InhouseWaveformControls& controls,
    float dt, bool silent, float level) noexcept {
  // P6 iteration 2: confidence is graded, not a hard threshold. On the replay corpus the
  // gate reached 0.5 in only 3-23% of frames and strength 0.08 in 1-15%, so a hard
  // threshold left colour parked at colour_offset, near-black on both v8 palettes.
  // P6 iteration 3: confidence also scales with presence (level / 0.5, full speed from
  // -21 dBFS, 2.5 dB below the corpus music median). In BOOM's near-silent tonal cut
  // (level 0.01-0.07) the gate opened fully and moved colour while almost no light was
  // drawn; the drop then inherited it. Low confidence now drifts toward the anchor.
  constexpr float kConfidentStrength = 0.08F, kWeakStrength = 0.02F, kFullPresence = 0.5F;
  constexpr float kFollowS = 0.6F;    // ~1/3 bar at 120-140 BPM: harmony, not beats
  constexpr float kMomentumS = 0.25F;
  constexpr float kAnchorS = 15.0F;   // ~8 bars at 128 BPM: one phrase of held colour
  constexpr float kDriftS = 8.0F;
  constexpr float kFollowCap = 0.25F, kDriftCap = 0.03F;  // palette units per second
  const float confidence = silent ? 0.0F
      : f(tonality_gate_) * clamp01(centroid_strength_ / kConfidentStrength) *
        clamp01(level / kFullPresence);
  const bool confident = confidence > 0.0F && centroid_strength_ >= kWeakStrength;
  const auto shortest = [](float to, float from) {
    float delta = to - from;
    if (delta > 0.5F) delta -= 1.0F;
    if (delta < -0.5F) delta += 1.0F;
    return delta;
  };
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  const bool luma_hold = v9_luma_hold_;
#else
  constexpr bool luma_hold = kInhouseV9LumaHold;
#endif
  // Iteration 6 luma hold: below strong confidence the colour may move only to a palette
  // position whose luma is not below that of the last strongly confident colour.
  constexpr float kStrongConfidence = 0.5F;
  const auto luma = [&](float position) {
    const auto c = colourV9(controls.palette_id, position);
    return 0.2126F * c.red + 0.7152F * c.green + 0.0722F * c.blue;
  };
  if (!colour_valid_) {
    colour_position_ = colour_anchor_ = confident ? centroid_ : wrap(controls.colour_offset);
    colour_valid_ = confident;
    colour_velocity_ = 0.0F;
    if (luma_hold) luma_ref_ = confident ? luma(colour_position_) : -1.0F;
  } else {
    float target = 0.0F, cap = kFollowCap;
    if (confident) {
      target = confidence * shortest(centroid_, colour_position_) / kFollowS;
    } else {
      cap = kDriftCap;
      target = shortest(colour_anchor_, colour_position_) / kDriftS;
    }
    colour_velocity_ = std::max(-cap, std::min(cap, ema(colour_velocity_, target, dt, kMomentumS)));
    const float proposed = wrap(colour_position_ + colour_velocity_ * dt);
    if (luma_hold && confidence < kStrongConfidence && luma_ref_ >= 0.0F && luma(proposed) < luma_ref_) {
      colour_velocity_ = 0.0F;  // hold: no move to a darker colour without strong confidence
    } else {
      colour_position_ = proposed;
      if (luma_hold && confidence >= kStrongConfidence) luma_ref_ = luma(colour_position_);
    }
    // The anchor learns only from confident colour, weighted by that confidence.
    if (confident) colour_anchor_ = wrap(colour_anchor_ + shortest(colour_position_, colour_anchor_) *
        confidence * (1.0F - ::expf(-dt / kAnchorS)));
  }
  observation_.palette_coordinate = colour_position_;
  observation_.luma_ref = luma_ref_;
}

// Iteration 6 drop accent: positive flux of the lowest 12 note bins (110-208 Hz, kick and
// bass harmonics) feeds a fast and a slow density; the accent is how far the fast density
// exceeds the slow baseline. A cut empties both, so the drop's return reads as a rise.
void InhouseWaveformRenderer::updateAccentV9(const InhouseWaveformAudio& audio, bool fresh,
    bool silent, float dt) noexcept {
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  const InhouseV9Accent& a = v9_accent_;
#else
  constexpr InhouseV9Accent a = kInhouseV9AccentConfig;
#endif
  float flux = 0.0F;
  if (fresh) {
    for (unsigned i = 0; i < 12U; ++i) {
      const float v = std::isfinite(audio.spectrum[i]) ? std::max(0.0F, audio.spectrum[i]) : 0.0F;
      flux += std::max(0.0F, v - low_previous_[i]);
      low_previous_[i] = v;
    }
  }
  if (silent) flux = 0.0F;
  density_fast_ = ema(density_fast_, flux, dt, a.fast_s);
  density_slow_ = ema(density_slow_, flux, dt, a.slow_s);
  const float ratio = (density_fast_ + a.epsilon) / (density_slow_ + a.epsilon + 1.0e-12F);
  accent_ = a.boost > 0.0F || a.widen_px > 0.0F ? clamp01((ratio - 1.0F) / (a.ratio_full - 1.0F)) : 0.0F;
  observation_.low_flux = flux;
  observation_.accent = accent_;
}

// Extent release: the waveform's length is the release driver. A pre-AGC level surge over a
// slow baseline pulls the deposit inward (edge-to-centre extension) with instant attack and a
// fixed release; between surges the level-driven contraction is untouched.
void InhouseWaveformRenderer::updateReleaseV9(const InhouseWaveformAudio& audio, bool fresh,
    bool silent, float dt) noexcept {
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  const InhouseV9Release& r = v9_release_;
#else
  constexpr InhouseV9Release r = kInhouseV9ReleaseConfig;
#endif
  if (!r.enabled) { release_ = 0.0F; observation_.release = 0.0F; return; }
  const float vu = fresh && !silent && std::isfinite(audio.vu_level) ? std::max(0.0F, audio.vu_level) : 0.0F;
  const float sub = fresh && !silent && std::isfinite(audio.sub_rms) ? std::max(0.0F, audio.sub_rms) : 0.0F;
  const float power = r.trigger_sub ? sub * sub : vu * vu;
  // Dip gate memory: 0.1 s broadband means; a cut or breakdown must precede the surge.
  dip_acc_ += vu * vu * dt;
  dip_acc_s_ += dt;
  if (dip_acc_s_ >= 0.1F) {
    dip_ring_[dip_index_] = 10.0F * ::log10f(dip_acc_ / dip_acc_s_ + 1.0e-12F);
    dip_index_ = (dip_index_ + 1U) % dip_ring_.size();
    dip_count_ = std::min<unsigned>(dip_count_ + 1U, dip_ring_.size());
    dip_acc_ = dip_acc_s_ = 0.0F;
  }
  release_fast_ = ema(release_fast_, power, dt, r.fast_s);
  release_slow_ = ema(release_slow_, power, dt, r.slow_s);
  const float fast_db = 10.0F * ::log10f(release_fast_ + 1.0e-12F);
  const float surge = fast_db - 10.0F * ::log10f(release_slow_ + 1.0e-12F);
  float x = clamp01((surge - r.lo_db) / r.span_db);
  if (r.newpeak_db_s > 0.0F) {
    if (fast_db < release_peak_db_ - 1.0F) x = 0.0F;
    release_peak_db_ = fast_db > release_peak_db_ ? fast_db : release_peak_db_ - r.newpeak_db_s * dt;
  }
  if (r.dip_db > 0.0F) {
    // Span over [t - 6 s, t - 0.5 s]: skip the newest 5 entries (the surge itself).
    float lo = 1.0e9F, hi = -1.0e9F;
    for (unsigned k = 5; k < dip_count_; ++k) {
      const float v = dip_ring_[(dip_index_ + dip_ring_.size() - 1U - k) % dip_ring_.size()];
      lo = std::min(lo, v);
      hi = std::max(hi, v);
    }
    if (dip_count_ <= 10U || hi - lo < r.dip_db) x = 0.0F;
  }
  release_ = silent ? 0.0F : std::max(x, release_ * ::expf(-dt / r.release_s));
  observation_.release = release_;
}

// Separate attack and release on the intensity field: constant release in seconds,
// independent of level and frame rate. Confirmed quiet keeps v8's extra drain.
void InhouseWaveformRenderer::fadeV9(bool mode32, bool silent, float dt) noexcept {
  observation_.decay_rate = 1.0F / (mode32 ? 0.29F : 0.125F) + (silent ? 10.0F : 0.0F);
  const auto fade = q(::expf(-observation_.decay_rate * dt));
  for (auto& value : intensity_) value = mul(value, fade);
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  for (auto& p : history_) { p.red = mul(p.red, fade); p.green = mul(p.green, fade); p.blue = mul(p.blue, fade); }
#endif
}

// Deposit: level drives amplitude and a bounded triangular width. A pixel takes the
// new intensity AND its colour only where the deposit exceeds the decayed value, so
// trails keep their deposited colour and no hue is mixed per channel.
void InhouseWaveformRenderer::depositV9(bool mode32, unsigned start, float position,
    float level, const FixedRgb& colour) noexcept {
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  const InhouseV9Variant variant = v9_variant_;
  const float gain = v9_gain_[mode32 ? 0 : 1];
#else
  constexpr InhouseV9Variant variant = kInhouseV9Variant;
  const float gain = mode32 ? kInhouseV9Gain32 : kInhouseV9Gain18;
#endif
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  const InhouseV9Accent& accent_config = v9_accent_;
#else
  constexpr InhouseV9Accent accent_config = kInhouseV9AccentConfig;
#endif
  float width = 0.5F + 3.5F * level;
  if (accent_ > 0.0F) width += accent_config.widen_px * accent_;
  float amplitude = gain;
  if (variant == InhouseV9Variant::kConstantEnergy) amplitude = gain * 2.0F / (width + 1.0F);
  else if (variant == InhouseV9Variant::kLevelEmphasis) {
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
    const float emphasis = v9_emphasis_;
#else
    constexpr float emphasis = kInhouseV9Emphasis;
#endif
    // 1.5 keeps the iteration-4 arithmetic exactly (level * sqrt(level)).
    amplitude = emphasis == 1.5F ? gain * level * ::sqrtf(level) : gain * ::powf(level, emphasis);
  }
  if (accent_ > 0.0F) amplitude *= 1.0F + accent_config.boost * accent_;
  amplitude = clamp01(amplitude);
  const float reach = width + 1.0F;
  const int first = std::max(static_cast<int>(start), static_cast<int>(std::ceil(position - reach)));
  const int last = std::min(159, static_cast<int>(std::floor(position + reach)));
  for (int i = first; i <= last; ++i) {
    const auto x = q(amplitude * clamp01(1.0F - std::fabs(static_cast<float>(i) - position) / reach));
    if (x == 0) continue;
    observation_.deposit_strength += f(x);
    ++observation_.injection_count;
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
    auto& p = history_[static_cast<unsigned>(i)];
    if (v9_fault_ == InhouseV9Fault::kPerChannelMax) {
      p.red = std::max(p.red, mul(colour.red, x));
      p.green = std::max(p.green, mul(colour.green, x));
      p.blue = std::max(p.blue, mul(colour.blue, x));
      continue;
    }
    if (v9_fault_ == InhouseV9Fault::kRgbBlend) {
      p.red += (mul(colour.red, x) - p.red) / 2;
      p.green += (mul(colour.green, x) - p.green) / 2;
      p.blue += (mul(colour.blue, x) - p.blue) / 2;
      continue;
    }
#endif
    if (x > intensity_[static_cast<unsigned>(i)]) {
      intensity_[static_cast<unsigned>(i)] = x;
      colour_[static_cast<unsigned>(i)] = colour;
      offset_[static_cast<unsigned>(i)] = deposit_offset_;
    }
  }
}

void InhouseWaveformRenderer::outputV9(const InhouseWaveformControls& controls,
    const FixedRgb& colour, Pixel16* output) noexcept {
  (void)colour;  // used only by the host brightness-floor fault
  std::int64_t intensity_sum = 0;
  for (unsigned i = 0; i < 160U; ++i) intensity_sum += intensity_[controls.mirror && i < 80U ? 159U - i : i];
  observation_.intensity_mean = static_cast<float>(intensity_sum) / (160.0F * 65536.0F);
  const float saturation = clamp01(controls.saturation);
  const bool mapped = perPixelMapped();
  for (unsigned i = 0; i < 160U; ++i) {
    const unsigned source = controls.mirror && i < 80U ? 159U - i : i;
    auto level_q = intensity_[source];
    FixedRgb p{mul(colour_[source].red, level_q), mul(colour_[source].green, level_q),
               mul(colour_[source].blue, level_q)};
    if (mapped && level_q > 0) {
      // Path P: HD palette sample per pixel at its deposited offset plus its mapped place
      // across the half-strip (0 at the centre, 79/80 at the edge, K1 GDFT freq_prog).
      const float d = static_cast<float>(source >= 80U ? source - 80U : 79U - source) / 80.0F;
      const float position = wrap(offset_[source] + d);
      auto c = sampleProductPaletteHd(controls.palette_id, position, 1.0F);
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
      if (v9_fault_ == InhouseV9Fault::kRgbMixPalette) {
        const auto e = sampleProductPaletteHd(controls.palette_id, wrap(position + 0.25F), 1.0F);
        c = {0.5F * (c.red + e.red), 0.5F * (c.green + e.green), 0.5F * (c.blue + e.blue)};
      } else if (v9_fault_ == InhouseV9Fault::kSlot16) {
        const auto s16 = sampleProductPaletteFastLed16(controls.palette_id,
            static_cast<std::uint8_t>(std::min(255.0F, position * 255.0F)));
        c = {s16.red / 255.0F, s16.green / 255.0F, s16.blue / 255.0F};
      }
#endif
      p = {mul(q(c.red), level_q), mul(q(c.green), level_q), mul(q(c.blue), level_q)};
    }
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
    if (v9_fault_ == InhouseV9Fault::kPerChannelMax || v9_fault_ == InhouseV9Fault::kRgbBlend) p = history_[source];
    if (v9_fault_ == InhouseV9Fault::kBrightnessFloor) {
      level_q = std::max<std::int32_t>(level_q, one / 1024);
      const FixedRgb floor_colour = colour_[source].red || colour_[source].green || colour_[source].blue
          ? colour_[source] : colour;
      p = {mul(floor_colour.red, level_q), mul(floor_colour.green, level_q), mul(floor_colour.blue, level_q)};
    }
#endif
    const auto top = std::max(p.red, std::max(p.green, p.blue));
    output[i] = {outputChannel(top + static_cast<std::int32_t>(static_cast<float>(p.red - top) * saturation)),
                 outputChannel(top + static_cast<std::int32_t>(static_cast<float>(p.green - top) * saturation)),
                 outputChannel(top + static_cast<std::int32_t>(static_cast<float>(p.blue - top) * saturation))};
  }
}

#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
bool InhouseWaveformRenderer::probeV9Deposit(const InhouseWaveformControls& controls,
    float palette_position, float level, float position, float dt, Pixel16* output) noexcept {
  if (output == nullptr || (controls.mode != 32U && controls.mode != 18U)) return false;
  const bool mode32 = controls.mode == 32U;
  const auto colour_f = colourV9(controls.palette_id, palette_position);
  const FixedRgb colour{q(colour_f.red), q(colour_f.green), q(colour_f.blue)};
  if (dt > 0.0F) fadeV9(mode32, false, dt);
  deposit_offset_ = wrap(palette_position);
  depositV9(mode32, mode32 || controls.mirror ? 80U : 0U, position, clamp01(level), colour);
  outputV9(controls, colour, output);
  return true;
}
#endif

#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
bool InhouseWaveformRenderer::setV9HostFront(const InhouseV9Front& front) noexcept {
  if (!(front.travel_s >= 0.100F && front.travel_s <= 0.180F)) return false;
  if (!(front.hold_lead_s >= 0.0F && front.hold_lead_s <= 8.0F)) return false;
  if (!(front.hold_s >= 0.0F && front.hold_s <= 2.0F)) return false;
  if (!(front.decay_s >= 0.05F && front.decay_s <= 2.0F)) return false;
  if (front.drop_count > front.drop_s.size()) return false;
  for (unsigned i = 0; i < front.drop_count; ++i)
    if (!std::isfinite(front.drop_s[i])) return false;
  v9_front_ = front;
  return true;
}

// Minimum extent for the host front. 0 is the edge, 1 is the centre. During
// travel the reach eases inward. After it arrives it stays at 1 for hold_s,
// then falls toward the music-driven extent with time constant decay_s.
// The legacy receding plant does not use this; it still paints its own front.
bool InhouseWaveformRenderer::v9HoldReach(float music_position, float* reach,
                                          bool* clear_inner) const noexcept {
  *reach = 0.0F;
  *clear_inner = false;
  if (!v9_front_.enabled || v9_front_.legacy_recede || v9_front_.drop_count == 0U ||
      !std::isfinite(v9_clock_s_))
    return false;
  const float travel = v9_front_.travel_s;
  const float tail = travel + v9_front_.hold_s + 4.0F * v9_front_.decay_s;
  float release_age = -1.0F;
  for (unsigned n = 0; n < v9_front_.drop_count; ++n) {
    const float age = v9_clock_s_ - v9_front_.drop_s[n];
    if (age >= 0.0F && age <= tail) release_age = age;
  }
  if (!(release_age >= 0.0F)) return false;
  if (release_age <= travel) {
    // Ease from the music's own extent toward the centre. Starting the ease
    // at the edge leaves an already-inward deposit sitting still until the
    // envelope catches it, and Sonic then crosses the old 50 ms line late.
    const float u = clamp01(release_age / travel);
    const float eased = 1.0F - (1.0F - u) * (1.0F - u) * (1.0F - u);
    const float music_extent = clamp01((160.0F - music_position) / 80.0F);
    *reach = music_extent + (1.0F - music_extent) * eased;
    return true;
  }
  float sustain = 1.0F;
  const float held_until = travel + v9_front_.hold_s;
  if (release_age > held_until)
    sustain = ::expf(-(release_age - held_until) / v9_front_.decay_s);
  if (sustain < 0.02F) return false;
  const float music_extent = clamp01((160.0F - music_position) / 80.0F);
  *reach = music_extent + (1.0F - music_extent) * sustain;
  *clear_inner = release_age > held_until;
  return true;
}

// Inward release front. The band maths is composed_motion::renderRibbon with
// inward = true: path = 1 - x, smoothstep feather, three-tap trail (0.66/0.17/0.17).
// Geometry is 160 px, 80 radial pixels per edge field. Travel is eased and kept
// inside 100-180 ms, faster than the donor's 200 ms floor. Colour is path P:
// offset_ keeps the deposited position and outputV9 samples wrap(offset + d).
// The live hold does not paint that ribbon. It only limits how far out the
// music deposit may sit, then lets the deposit, the trail and the per-pixel
// palette keep running. The ribbon remains on the planted receding front.
void InhouseWaveformRenderer::applyOracleFront(bool mode32, unsigned start,
                                              float music_position,
                                              std::uint16_t palette_id) noexcept {
  (void)palette_id;
  if (!v9_front_.enabled || v9_front_.drop_count == 0U || !std::isfinite(v9_clock_s_)) return;
  const float t = v9_clock_s_;
  bool hold = false;
  float release_age = -1.0F;
  // legacy_recede paints only while the front is travelling. The hold keeps the
  // full reach, then the fall follows the music deposit for about four time constants.
  const float tail = v9_front_.legacy_recede
      ? v9_front_.travel_s
      : v9_front_.travel_s + v9_front_.hold_s + 4.0F * v9_front_.decay_s;
  for (unsigned n = 0; n < v9_front_.drop_count; ++n) {
    const float td = v9_front_.drop_s[n];
    if (v9_front_.hold_contraction && t >= td - v9_front_.hold_lead_s && t < td) hold = true;
    const float age = t - td;
    if (age >= 0.0F && age <= tail) release_age = age;
  }
  if (hold) {
    // Drop the inner pixels. Leave the outer band lit so the riser is not blacked out.
    const unsigned cut = mode32 ? 128U : 148U;
    for (unsigned i = start; i < cut && i < 160U; ++i) {
      intensity_[i] = 0;
      colour_[i] = {};
      offset_[i] = 0.0F;
    }
  }
  if (!(release_age >= 0.0F)) return;
  const float travel = v9_front_.travel_s;
  // Reach in path units: 0 is the edge, 1 is the centre. During travel the reach
  // is the eased front. After it arrives, the reach stays at the centre for hold_s,
  // then eases toward the music deposit (loud still parks near the edge).
  float centre = 0.0F;
  // The Experiment 1 front cleared every pixel ahead of a reach that starts at the
  // edge, so the drop frame kept only the outer sparks. That clear stays on the
  // planted receding front. The hold path leaves existing light alone while the
  // front travels, and clears only while the reach is retreating after the hold.
  bool clear_beyond = v9_front_.legacy_recede;
  if (release_age <= travel) {
    const float u = clamp01(release_age / travel);
    centre = 1.0F - (1.0F - u) * (1.0F - u) * (1.0F - u);
  } else {
    float sustain = 1.0F;
    const float held_until = travel + v9_front_.hold_s;
    if (release_age > held_until)
      sustain = ::expf(-(release_age - held_until) / v9_front_.decay_s);
    if (sustain < 0.02F) return;
    const float music_extent = clamp01((160.0F - music_position) / 80.0F);
    centre = music_extent + (1.0F - music_extent) * sustain;
    clear_beyond = release_age > held_until;
  }
  if (!v9_front_.legacy_recede) {
    // The deposit position was already limited to this reach. Leave the music
    // in place. After the hold, drop only the pixels further in than the
    // falling reach, so the length eases back without a painted bar.
    if (clear_beyond) {
      for (unsigned pix = 80U; pix < 160U; ++pix) {
        const float path = 1.0F - static_cast<float>(pix - 80U) / 79.0F;
        if (path > centre) {
          intensity_[pix] = 0;
          colour_[pix] = {};
          offset_[pix] = 0.0F;
        }
      }
    }
    return;
  }
  const float width = 0.10F;
  const float feather = 0.5F * width;
  const float trail_s = 0.016F;
  const auto envelope = [](float delta, float band, float feather_px) {
    const float a = std::fabs(delta);
    const float half = band * 0.5F;
    if (a >= half) return 0.0F;
    if (feather_px <= 0.0F) return 1.0F;
    const float e = clamp01((half - a) / feather_px);
    return e * e * (3.0F - 2.0F * e);
  };
  for (unsigned pix = 80U; pix < 160U; ++pix) {
    const float x = static_cast<float>(pix - 80U) / 79.0F;
    const float path = 1.0F - x;
    float shape = 0.0F;
    for (unsigned trail = 0; trail < 3U; ++trail) {
      const float lag = trail_s * static_cast<float>(trail) * 0.5F;
      if (release_age < lag) continue;
      const float delta = path - (centre - lag / travel);
      shape += envelope(delta, width, feather) * (trail == 0U ? 0.66F : 0.17F);
    }
    if (path > centre + width * 0.5F) {
      if (clear_beyond) {
        intensity_[pix] = 0;
        colour_[pix] = {};
        offset_[pix] = 0.0F;
      }
      continue;
    }
    const float body = path <= centre ? 0.55F : 0.0F;
    const auto level_q = q(std::max(body, shape));
    if (level_q > intensity_[pix]) {
      intensity_[pix] = level_q;
      offset_[pix] = deposit_offset_;
    }
  }
}
#endif

bool InhouseWaveformRenderer::renderV9(const InhouseWaveformAudio& audio,
    const InhouseWaveformControls& controls, float dt, bool fresh, bool silent,
    Pixel16* output) noexcept {
  const bool mode32 = controls.mode == 32U;
  const float motion = std::isfinite(controls.motion_scale)
      ? std::max(0.0F, std::min(4.0F, controls.motion_scale)) : 1.0F;
  // Level: fixed dB transfer of the pre-AGC RMS (-33 dBFS -> 0, ceiling -> 1). It sets
  // deposit amplitude and width only; it never changes the release.
  const float vu = fresh ? clamp01(audio.vu_level) : 0.0F;
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  const float ceiling_db = v9_ceiling_db_;
#else
  constexpr float ceiling_db = kInhouseV9LevelCeilingDb;
#endif
  const float level = silent || vu <= 0.0F ? 0.0F
      : clamp01((20.0F * ::log10f(vu) + 33.0F) / (ceiling_db + 33.0F));
  observation_.level = level;
  observation_.tonality_gate = f(tonality_gate_);
  observation_.centroid_strength = centroid_strength_;
  updateAccentV9(audio, fresh, silent, dt);
  updateReleaseV9(audio, fresh, silent, dt);
  if (perPixelMapped()) updateOffsetP(controls, silent);
  else updateColourV9(controls, dt, silent, level);
  deposit_offset_ = colour_position_;
  const auto colour_f = colourV9(controls.palette_id, colour_position_);
  const FixedRgb colour{q(colour_f.red), q(colour_f.green), q(colour_f.blue)};
  observation_.smoothed_colour = colour_f;
  fadeV9(mode32, silent, dt);

  // Transport. Mode 32: v8's outward scroll from the centre. Mode 18: tempo motion
  // weighted by tempo confidence (never above 0.7), with an onset-driven energy layer
  // carrying the rest, so no motion depends solely on the single tempo winner.
  const unsigned start = mode32 || controls.mirror ? 80U : 0U;
  float rate = 405.0F * motion;
  unsigned maximum_steps = 8U;
  float position = 80.0F;
  const float peak = fresh ? std::max(clamp01(audio.peak_scaled),
      audio.peak_summary_valid ? clamp01(audio.observed_peak_scaled) : 0.0F) : 0.0F;
  if (mode32) {
    for (unsigned i = 0; i < 80U; ++i) { intensity_[i] = 0; colour_[i] = {}; offset_[i] = 0.0F; }
    observation_.peak_stage1 = ema(observation_.peak_stage1, peak, dt, 0.016F);
    observation_.peak_stage2 = ema(observation_.peak_stage2, observation_.peak_stage1, dt, 0.023F);
    position = std::min(159.0F, 80.0F + observation_.peak_stage2 * 0.7F * 80.0F);
    if (release_ > 0.0F) position = 80.0F + (position - 80.0F) * (1.0F - release_);
    observation_.tempo_weight = 0.0F;
  } else {
    const bool tempo_fresh = fresh && audio.tempo_valid &&
        audio.tempo_capture_us <= last_render_us_ &&
        last_render_us_ - audio.tempo_capture_us <= 30000U + kInhouseTempoUpdatePeriodUs &&
        std::isfinite(audio.tempo_bpm) && audio.tempo_bpm >= 60.0F &&
        audio.tempo_bpm <= 156.0F && std::isfinite(audio.tempo_phase01) &&
        std::isfinite(audio.tempo_confidence);
    float w = tempo_fresh ? clamp01((audio.tempo_confidence - 0.30F) / 0.30F) : 0.0F;
    w = 0.7F * w * w * (3.0F - 2.0F * w);
    const float tempo_rate = tempo_fresh ? 24.0F * (audio.tempo_bpm / 60.0F) *
        (1.0F + 0.5F * ::cosf(tau * audio.tempo_phase01)) : 0.0F;
    const float onset = std::isfinite(audio.novelty) ? clamp01(audio.novelty / 0.12F) : 0.0F;
    const float energy_rate = !silent ? 30.0F + 90.0F * onset : 0.0F;
    rate = (w * tempo_rate + (1.0F - w) * energy_rate) * motion;
    maximum_steps = 30U;
    observation_.tempo_weight = w;
    const float index = std::max(0.0F, std::min(159.0F, 80.0F + peak * 80.0F + 0.5F));
    position = std::floor(index);
    if (controls.mirror) position = std::max(80.0F, position);
    if (release_ > 0.0F) position = std::floor(80.0F + (position - 80.0F) * (1.0F - release_));
  }
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  // R03: through the riser, keep the deposit on the contracted outer band.
  if (v9_front_.enabled && v9_front_.hold_contraction) {
    for (unsigned n = 0; n < v9_front_.drop_count; ++n) {
      const float td = v9_front_.drop_s[n];
      if (v9_clock_s_ >= td - v9_front_.hold_lead_s && v9_clock_s_ < td) {
        position = mode32 ? (80.0F + 0.7F * 80.0F) : 159.0F;
        break;
      }
    }
  }
  // The live hold is a floor on reach. Pull the music deposit inward to the
  // envelope. The planted receding front is left on its own painted path.
  const float music_position = position;
  float reach = 0.0F;
  bool clear_inner = false;
  if (v9HoldReach(music_position, &reach, &clear_inner))
    position = std::min(position, 160.0F - 80.0F * reach);
  (void)clear_inner;
#endif
  observation_.scroll_remainder += rate * dt;
  unsigned steps = std::min(maximum_steps, static_cast<unsigned>(observation_.scroll_remainder));
  observation_.scroll_remainder -= static_cast<float>(steps);
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  // Full hold. Mode 32's natural four pixels a frame walks a dark palette
  // moment past the 0.90 line, so it scrolls two. Mode 18's natural crawl
  // smears into one colour, so it scrolls four and the trail stays separated.
  if (reach >= 0.999F && !v9_front_.legacy_recede)
    steps = mode32 ? 2U : 4U;
#endif
  observation_.scroll_steps = steps;
  for (unsigned s = 0; s < steps; ++s) {
    for (unsigned i = 159U; i > start; --i) {
      intensity_[i] = intensity_[i - 1U];
      colour_[i] = colour_[i - 1U];
      offset_[i] = offset_[i - 1U];
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
      history_[i] = history_[i - 1U];
#endif
    }
    intensity_[start] = 0;
    colour_[start] = {};
    offset_[start] = 0.0F;
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
    history_[start] = {};
#endif
  }
  observation_.position = position;
  observation_.deposit_strength = 0.0F;
  if (!silent && level > 0.0F) depositV9(mode32, start, position, level, colour);
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  // A dark stop at the centre is invisible to the extent line, which reads
  // display luma. When the inner eight pixels fall under that line, bring the
  // nearest lit run of the music's own trail back to the centre. The colour
  // stays the trail's colour. Nothing is filled with one stop.
  if (reach >= 0.999F && !v9_front_.legacy_recede && perPixelMapped()) {
    const auto y_of = [&](unsigned p) {
      if (intensity_[p] <= 0) return 0.0F;
      const float d = static_cast<float>(p - 80U) / 80.0F;
      const auto c = sampleProductPaletteHd(controls.palette_id, wrap(offset_[p] + d), 1.0F);
      return f(intensity_[p]) * (0.2126F * c.red + 0.7152F * c.green + 0.0722F * c.blue);
    };
    float peak = 0.0F;
    for (unsigned p = 80U; p < 160U; ++p) peak = std::max(peak, y_of(p));
    float inner = 0.0F;
    for (unsigned p = 80U; p < 88U; ++p) inner = std::max(inner, y_of(p));
    if (peak > 1.0e-8F && inner < 0.10F * peak) {
      unsigned src = 160U;
      for (unsigned p = 88U; p < 160U; ++p) {
        if (y_of(p) >= 0.10F * peak) { src = p; break; }
      }
      if (src + 8U <= 160U) {
        const float shift = static_cast<float>(src - 80U) / 80.0F;
        for (unsigned k = 0; k < 8U; ++k) {
          intensity_[80U + k] = intensity_[src + k];
          colour_[80U + k] = colour_[src + k];
          offset_[80U + k] = wrap(offset_[src + k] + shift);
          history_[80U + k] = history_[src + k];
        }
      }
    }
  }
  applyOracleFront(mode32, start, music_position, controls.palette_id);
#endif
  outputV9(controls, colour, output);
  return true;
}
#endif
}  // namespace k1::core::visual
