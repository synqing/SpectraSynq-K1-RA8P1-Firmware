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
}  // namespace k1::core::visual
