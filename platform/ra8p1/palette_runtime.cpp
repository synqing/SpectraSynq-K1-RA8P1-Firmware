#include "palette_runtime.h"
#include "core/visual/product_effect_renderer.h"
#include "core/visual/product_output_treatment.h"
#include "core/visual/product_palette.h"
#include "core/visual/ws2816_pack.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace k1::titan {
using namespace core;
using namespace core::visual;
namespace {
bool supportedMode(std::uint32_t mode) noexcept {
  if (mode == 0U || mode == kDiagnosticBounceMode) return true;
#ifdef K1_PALETTE_MORPH
  if (isCentreEffect(mode)) return true;
#endif
  constexpr unsigned modes[]{3,7,8,9,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,28,29,32};
  for (const auto item : modes) if (item == mode) return true;
  return false;
}
std::uint32_t frameCrc(ConstPixelSpan frame) noexcept {
  std::uint32_t crc = 0xffffffffU;
  for (std::size_t i = 0; i < frame.size(); ++i) {
    const std::uint8_t rgb[]{frame[i].red, frame[i].green, frame[i].blue};
    for (auto value : rgb) {
      crc ^= value;
      for (unsigned bit = 0; bit < 8; ++bit)
        crc = (crc >> 1U) ^ ((crc & 1U) ? 0xedb88320U : 0U);
    }
  }
  return ~crc;
}
void preview(ChannelRenderState& channel, std::uint64_t now_us, bool inward) noexcept {
  const unsigned offset = static_cast<unsigned>((now_us / 20000U) & 255U);
  for (unsigned i = 0; i < kPixelsPerChannel; ++i) {
    const unsigned radial = i < 80U ? 79U-i : i-80U;
    const unsigned position = (inward ? 79U-radial : radial) * 255U / 79U;
#ifdef K1_PALETTE_MORPH
    channel.frame()[i] = channel.controls().palette_transition->fast(
        static_cast<std::uint8_t>(position + 256U - offset));
#else
    channel.frame()[i] = sampleProductPaletteFastLed16(
        channel.controls().palette_id, static_cast<std::uint8_t>(position + 256U - offset));
#endif
  }
}
unsigned bounceRadius(std::uint64_t now_us) noexcept {
  const unsigned step =
      static_cast<unsigned>((now_us / kDiagnosticBounceFrameUs) % kDiagnosticBounceSteps);
  return step < 64U ? step : 126U - step;
}
unsigned benchToNative(unsigned i) noexcept { return (i * 159U + 63U) / 127U; }

void ageAuthoredHistory(PixelSpan dest, PixelSpan history,
                        std::uint16_t q8[][3], bool& armed, float dt,
                        bool mirror) noexcept {
  if (!armed) {
    for (unsigned i = 0U; i < 160U; ++i) {
      q8[i][0] = std::uint16_t(history[i].red) << 8;
      q8[i][1] = std::uint16_t(history[i].green) << 8;
      q8[i][2] = std::uint16_t(history[i].blue) << 8;
    }
    armed = true;
  }
  float gain = dt <= 0.0F ? 1.0F : std::exp(-dt / kTitanDwellTauS);
  if (gain < 0.0F) gain = 0.0F;
  if (gain > 1.0F) gain = 1.0F;
  const std::uint32_t k = static_cast<std::uint32_t>(gain * 65536.0F + 0.5F);
  for (unsigned i = 0U; i < 160U; ++i) {
    q8[i][0] = static_cast<std::uint16_t>((std::uint32_t(q8[i][0]) * k) >> 16);
    q8[i][1] = static_cast<std::uint16_t>((std::uint32_t(q8[i][1]) * k) >> 16);
    q8[i][2] = static_cast<std::uint16_t>((std::uint32_t(q8[i][2]) * k) >> 16);
    const Pixel8 pixel{static_cast<std::uint8_t>(q8[i][0] >> 8),
                       static_cast<std::uint8_t>(q8[i][1] >> 8),
                       static_cast<std::uint8_t>(q8[i][2] >> 8)};
    history[i] = pixel;
    dest[i] = pixel;
  }
  if (mirror) {
    for (unsigned distance = 0U; distance < 80U; ++distance)
      dest[79U - distance] = dest[80U + distance];
  }
}
void diagnosticBounce(ChannelRenderState& channel, std::uint64_t now_us) noexcept {
  channel.clearFrame();
  const unsigned radius = bounceRadius(now_us);
  auto frame = channel.frame();
  for (unsigned tail = 0; tail < 6U && tail <= radius; ++tail) {
    const std::uint8_t falloff = static_cast<std::uint8_t>(255U >> tail);
    if (!falloff) break;
    const unsigned left = 63U - radius + tail;
    const unsigned right = 64U + radius - tail;
    const unsigned ends[2]{left, right};
    for (unsigned index : ends) {
      if (index >= 128U) continue;
      const unsigned radial = index < 64U ? 63U - index : index - 64U;
      const auto pal = static_cast<std::uint8_t>(radial * 255U / 63U);
#ifdef K1_PALETTE_MORPH
      const auto colour = channel.controls().palette_transition
                              ? channel.controls().palette_transition->fast(pal)
                              : sampleProductPaletteFastLed16(channel.controls().palette_id, pal);
#else
      const auto colour = sampleProductPaletteFastLed16(channel.controls().palette_id, pal);
#endif
      Pixel8 out{static_cast<std::uint8_t>(std::uint32_t(colour.red) * falloff / 255U),
                 static_cast<std::uint8_t>(std::uint32_t(colour.green) * falloff / 255U),
                 static_cast<std::uint8_t>(std::uint32_t(colour.blue) * falloff / 255U)};
      auto& dst = frame[benchToNative(index)];
      if (out.red > dst.red) dst.red = out.red;
      if (out.green > dst.green) dst.green = out.green;
      if (out.blue > dst.blue) dst.blue = out.blue;
    }
  }
}
}
bool PaletteRuntime::configure(const PaletteConfig& config, std::uint64_t now_us) noexcept {
#ifdef K1_PALETTE_MORPH
  const bool version_ok = ((config.version == 1U && config.transition_ms == 0U) ||
      (config.version == 2U && config.transition_ms <= PaletteTransition::kMaximumDurationMs))
      ? config.travel_ms == 4000U
      : config.version == 3U && config.transition_ms <= PaletteTransition::kMaximumDurationMs &&
        config.travel_ms >= 500U && config.travel_ms <= 30000U;
  if ((isCentreEffect(config.mode_a) || isCentreEffect(config.mode_b)) && config.version != 3U)
    return false;
  if ((config.flags & 8U) &&
      ((config.mode_a && !isCentreEffect(config.mode_a)) ||
       (config.mode_b && !isCentreEffect(config.mode_b)))) return false;
  if ((config.flags & 16U) &&
      (!isCentreEffect(config.mode_a) || !isCentreEffect(config.mode_b))) return false;
#else
  const bool version_ok = config.version == 1U && config.transition_ms == 0U && config.travel_ms == 4000U;
#endif
  if (!version_ok || config.palette_a >= kProductPaletteCount ||
      config.palette_b >= kProductPaletteCount || !supportedMode(config.mode_a) ||
      !supportedMode(config.mode_b) || (config.flags & ~(config.version == 3U ? 31U : 7U)) ||
      config.brightness > 255U || config.output_channel > 1U)
    return false;
  const bool cut = config.mode_a != config_.mode_a || config.mode_b != config_.mode_b;
  if (cut) {
    a_ = ChannelRenderState{PixelChannelId::kChannelA};
    b_ = ChannelRenderState{PixelChannelId::kChannelB};
  }
  config_ = config;
  auto& ca = a_.controls(); auto& cb = b_.controls();
  ca.palette_mode_enabled = cb.palette_mode_enabled = true;
  ca.palette_id = static_cast<std::uint16_t>(config.palette_a);
  cb.palette_id = static_cast<std::uint16_t>(config.palette_b);
  ca.mode_id = static_cast<std::uint16_t>(config.mode_a);
  cb.mode_id = static_cast<std::uint16_t>(config.mode_b);
  // Physical brightness is applied once by the bench adapter, after treatment.
  ca.brightness = cb.brightness = 255U;
  ca.photons_id = cb.photons_id = 65535U;
#ifdef K1_PALETTE_MORPH
  ca.palette_transition = &transitions_[0];
  cb.palette_transition = &transitions_[1];
  transitions_[0].select(ca.palette_id, config.transition_ms, now_us);
  transitions_[1].select(cb.palette_id, config.transition_ms, now_us);
#endif
  next_us_ = now_us; last_us_ = now_us; cycle_start_us_ = now_us;
  last_live_us_ = 0U;
  waiting_for_audio_ = false;
  dwell_armed_[0] = dwell_armed_[1] = false;
  visual_path_ = "none";
  last_musical_ = last_in_dwell_ = last_dwell_reinit_ = false;
  last_live_age_us_ = 0U;
  last_peak_milli_ = last_vu_milli_ = last_chroma_milli_ = last_wave_milli_ = 0U;
  effect_frames_ = dwell_frames_ = dwell_reinits_ = 0U;
  return true;
}
bool PaletteRuntime::step(std::uint64_t now_us,
                          const VisualAudioFrameView* audio) noexcept {
  if (!active() || now_us < next_us_) return false;
  const std::uint64_t late = now_us - next_us_;
  skipped_ += late / kPalettePeriodUs;
  next_us_ += (late / kPalettePeriodUs + 1U) * kPalettePeriodUs;
  const auto cycle = (config_.flags & 2U)
      ? static_cast<unsigned>((now_us - cycle_start_us_) / 4000000U % kProductPaletteCount)
      : 0U;
  a_.controls().palette_id = static_cast<std::uint16_t>((config_.palette_a + cycle) % kProductPaletteCount);
  b_.controls().palette_id = static_cast<std::uint16_t>((config_.palette_b + cycle) % kProductPaletteCount);
#ifdef K1_PALETTE_MORPH
  if (config_.flags & 16U) {
    const auto stage = unsigned((now_us-cycle_start_us_)/12000000U % kCentreEffectCount);
    a_.controls().mode_id = kCentreEffectFirst + (config_.mode_a-kCentreEffectFirst+stage)%kCentreEffectCount;
    b_.controls().mode_id = kCentreEffectFirst + (config_.mode_b-kCentreEffectFirst+stage)%kCentreEffectCount;
  }
  transitions_[0].select(a_.controls().palette_id, config_.transition_ms, now_us);
  transitions_[1].select(b_.controls().palette_id, config_.transition_ms, now_us);
#endif
  const float dt = last_us_ == now_us ? 1.0F / 120.0F
      : static_cast<float>(now_us - last_us_) / 1000000.0F;
  last_us_ = now_us;
  waiting_for_audio_ = false;
  unsigned channel_index = 0U;
  for (auto* channel : {&a_, &b_}) {
    if (channel->controls().mode_id == 0U) {
      preview(*channel, now_us, config_.flags & 8U);
      if (channel_index == 0U) visual_path_ = "preview";
    } else if (channel->controls().mode_id == kDiagnosticBounceMode) {
      diagnosticBounce(*channel, now_us);
      if (channel_index == 0U) visual_path_ = "bounce";
    }
#ifdef K1_PALETTE_MORPH
    else if (isCentreEffect(channel->controls().mode_id)) {
      const auto age = now_us-cycle_start_us_;
      const float t = float(age % 12000000U)/800000.0F;
      const bool blending = (config_.flags & 16U) && age >= 12000000U && t < 1.0F;
      const unsigned previous = kCentreEffectFirst +
          (channel->controls().mode_id-kCentreEffectFirst+kCentreEffectCount-1U)%kCentreEffectCount;
      renderCentreEffect(*channel, *channel->controls().palette_transition,
                         now_us, config_.travel_ms, config_.flags & 8U,
                         blending ? previous : 0U, blending ? t*t*(3.0F-2.0F*t) : 1.0F);
    }
#endif
    else if (audio) {
      contract::AudioFeaturesV1 governed_audio = audio->audio;
      applyK1PresencePolicy(governed_audio, now_us, last_live_us_);
      const bool musical = k1MusicalPresence(governed_audio);
      const bool in_dwell =
          last_live_us_ != 0U && now_us - last_live_us_ < kTitanSilenceDwellUs;
      if (channel_index == 0U) {
        last_musical_ = musical;
        last_in_dwell_ = in_dwell;
        last_live_age_us_ = last_live_us_ == 0U ? 0U : now_us - last_live_us_;
        auto milli = [](float value) -> std::uint32_t {
          if (value <= 0.0F) return 0U;
          if (value >= 1000.0F) return 10000000U;
          return static_cast<std::uint32_t>(value * 10000.0F + 0.5F);
        };
        last_peak_milli_ = milli(governed_audio.peak_scaled);
        last_vu_milli_ = milli(governed_audio.vu_level);
        last_chroma_milli_ = milli(governed_audio.chroma_strength);
        last_wave_milli_ = milli(audio->waveform.peak_scaled);
      }
      if (!musical) {
        // Age authored history in Q8.8 with elapsed time. Do not clamp to 1.
        // Mirror and output treatment stay display-only.
        // Not-musical DualMCU after dwell expiry redrew the plate (run-29).
        const bool reinit = !dwell_armed_[channel_index];
        ageAuthoredHistory(channel->frame(), channel->previousFrame(),
                           dwell_q8_[channel_index], dwell_armed_[channel_index],
                           dt, channel->controls().mirror_enabled);
        if (channel_index == 0U) {
          visual_path_ = "dwell";
          last_dwell_reinit_ = reinit;
          ++dwell_frames_;
          if (reinit) ++dwell_reinits_;
        }
      } else {
        dwell_armed_[channel_index] = false;
        const VisualAudioFrameView governed{governed_audio, audio->tempo,
                                           audio->waveform,
                                           audio->source_publication_us};
        channel->prepareAudio(governed_audio);
        // Live/silence path: clear then DualMCU previousFrame seed.
        // Skipping clear here saturates (snap-freeze).
        channel->clearFrame();
        (void)renderProductChannel(*channel, governed, dt);
        if (channel_index == 0U) {
          visual_path_ = "effect";
          last_dwell_reinit_ = false;
          ++effect_frames_;
        }
      }
    } else {
      channel->clearFrame();
      waiting_for_audio_ = true;
      if (channel_index == 0U) visual_path_ = "no_audio";
    }
    applyProductOutputTreatment(channel->frame(), channel->controls(),
                                channel->outputTreatmentState());
    ++channel_index;
  }
  ++frames_;
  return true;
}
std::size_t PaletteRuntime::packBenchGrb(std::uint8_t* out, std::size_t capacity,
                                        unsigned pixels) const noexcept {
  if (!out || pixels < 2U || pixels > kPixelsPerChannel ||
      capacity < std::size_t(pixels) * 3U) return 0U;
  const auto source = channel(config_.output_channel).frame();
  for (unsigned i = 0; i < pixels; ++i) {
    // Bench-only resampling: preserve both endpoints and the centre pair.
    const unsigned index = (i * (kPixelsPerChannel - 1U) + (pixels - 1U) / 2U) / (pixels - 1U);
    const auto pixel = source[index];
    const auto scale = [this](std::uint8_t value) {
      return static_cast<std::uint8_t>((std::uint32_t(value) * config_.brightness) / 255U);
    };
    out[i * 3U] = scale(pixel.green);
    out[i * 3U + 1U] = scale(pixel.red);
    out[i * 3U + 2U] = scale(pixel.blue);
  }
  return std::size_t(pixels) * 3U;
}
std::size_t PaletteRuntime::catalogueJson(char* out, std::size_t capacity) const noexcept {
  if (!out || capacity == 0U) return 0U;
  const int head = std::snprintf(out, capacity, "{\"version\":1,\"count\":%u,\"palettes\":[", unsigned(kProductPaletteCount));
  if (head < 0 || std::size_t(head) >= capacity) return 0U;
  std::size_t used = std::size_t(head);
  for (const auto& palette : productPaletteCatalogue()) {
    const int n = std::snprintf(out + used, capacity - used,
        "%s{\"id\":%u,\"name\":\"%s\",\"stops\":%u}",
        palette.id ? "," : "", unsigned(palette.id), palette.name, unsigned(palette.stop_count));
    if (n < 0 || std::size_t(n) >= capacity - used) return 0U;
    used += std::size_t(n);
  }
  if (capacity - used < 3U) return 0U;
  std::memcpy(out + used, "]}", 3U);
  return used + 2U;
}
std::size_t PaletteRuntime::packBenchGrb48Lane(std::uint8_t* out,
                                             std::size_t capacity,
                                             unsigned lane) const noexcept {
  if (!out || capacity < kPackedBytesPerLane || lane > 1U) return 0U;
  const auto source = channel(config_.output_channel).frame();
  for (unsigned i = 0; i < kPixelsPerHalf; ++i) {
    const auto pixel = source[lane * kPixelsPerHalf + i];
    // The existing renderer is Pixel8. Expand its output once into the 16-bit
    // wire domain; this does not claim additional renderer colour precision.
    const auto scale = [this](std::uint8_t value) {
      return static_cast<std::uint16_t>(
          (std::uint32_t(value) * 257U * config_.brightness) / 255U);
    };
    packPixel(Pixel16{scale(pixel.red), scale(pixel.green), scale(pixel.blue)},
              out + i * kPackedBytesPerPixel);
  }
  return kPackedBytesPerLane;
}
std::size_t PaletteRuntime::statusJson(char* out, std::size_t capacity) const noexcept {
  if (!out || capacity == 0U) return 0U;
  const int n = std::snprintf(out, capacity,
      "{\"version\":1,\"palette_count\":%u,\"active\":%s,\"automatic_cycle\":%s,"
      "\"emit_enabled\":%s,\"output_backend\":\"gpio_diagnostic\","
      "\"waiting_for_audio\":%s,\"palette_a\":%u,\"palette_b\":%u,"
      "\"name_a\":\"%s\",\"name_b\":\"%s\",\"mode_a\":%lu,\"mode_b\":%lu,"
      "\"brightness\":%lu,\"output_channel\":%lu,\"period_us\":%lu,"
      "\"frames\":%llu,\"skipped_releases\":%llu,\"emitted\":%llu,\"emit_errors\":%llu,"
      "\"last_emit_cycles\":%lu,\"maximum_emit_cycles\":%lu,"
      "\"frame_a_crc\":%lu,\"frame_b_crc\":%lu,\"native_pixels_per_channel\":160,"
#ifdef K1_PALETTE_WS2816
      "\"bench_pixels\":160,\"wire_profile\":4,\"wire_bits_per_pixel\":48,"
      "\"din_a\":\"P601\",\"din_b\":\"P004\",\"pixels_per_din\":80,"
#else
      "\"bench_pixels\":128,\"wire_profile\":1,\"wire_bits_per_pixel\":24,"
#endif
      "\"host_pixel_stream_required\":false"
#ifdef K1_PALETTE_MORPH
      ",\"morph_supported\":true,\"transition_ms\":%lu,\"transition_a_q16\":%u,"
      "\"transition_b_q16\":%u,\"contributors_a\":%u,\"contributors_b\":%u,"
      "\"centre_effects_supported\":true,\"direction\":\"%s\",\"travel_ms\":%lu,"
      "\"showcase\":%s,\"effect_a\":\"%s\",\"effect_b\":\"%s\""
#else
      ",\"morph_supported\":false"
#endif
      "}",
      unsigned(kProductPaletteCount), active() ? "true" : "false",
      (config_.flags & 2U) ? "true" : "false", emitEnabled() ? "true" : "false",
      waiting_for_audio_ ? "true" : "false", unsigned(a_.controls().palette_id),
      unsigned(b_.controls().palette_id), productPalette(a_.controls().palette_id).name,
      productPalette(b_.controls().palette_id).name,
      (unsigned long)a_.controls().mode_id, (unsigned long)b_.controls().mode_id,
      (unsigned long)config_.brightness, (unsigned long)config_.output_channel,
      (unsigned long)kPalettePeriodUs, (unsigned long long)frames_, (unsigned long long)skipped_,
      (unsigned long long)emitted_, (unsigned long long)emit_errors_,
      (unsigned long)last_emit_cycles_, (unsigned long)maximum_emit_cycles_,
      (unsigned long)frameCrc(a_.frame()), (unsigned long)frameCrc(b_.frame())
#ifdef K1_PALETTE_MORPH
      , (unsigned long)config_.transition_ms, transitions_[0].progress(),
      transitions_[1].progress(), transitions_[0].contributors(), transitions_[1].contributors(),
      (config_.flags & 8U) ? "edges_in" : "centre_out", (unsigned long)config_.travel_ms,
      (config_.flags & 16U) ? "true" : "false",
      a_.controls().mode_id == kDiagnosticBounceMode ? "PALETTE_BOUNCE"
                                                     : centreEffectName(a_.controls().mode_id),
      b_.controls().mode_id == kDiagnosticBounceMode ? "PALETTE_BOUNCE"
                                                     : centreEffectName(b_.controls().mode_id)
#endif
      );
  if (n <= 0 || std::size_t(n) >= capacity || out[n - 1] != '}') return 0U;
  out[n - 1] = '\0';
  const int extra = std::snprintf(
      out + n - 1, capacity - std::size_t(n) + 1,
      ",\"visual_path\":\"%s\",\"musical\":%s,\"in_dwell\":%s,"
      "\"live_age_us\":%llu,\"peak_milli\":%lu,\"vu_milli\":%lu,"
      "\"chroma_milli\":%lu,\"wave_milli\":%lu,\"dwell_armed\":%s,"
      "\"dwell_reinit\":%s,\"effect_frames\":%llu,\"dwell_frames\":%llu,"
      "\"dwell_reinits\":%llu}",
      visual_path_ ? visual_path_ : "none",
      last_musical_ ? "true" : "false", last_in_dwell_ ? "true" : "false",
      (unsigned long long)last_live_age_us_,
      (unsigned long)last_peak_milli_, (unsigned long)last_vu_milli_,
      (unsigned long)last_chroma_milli_, (unsigned long)last_wave_milli_,
      dwell_armed_[0] ? "true" : "false",
      last_dwell_reinit_ ? "true" : "false",
      (unsigned long long)effect_frames_, (unsigned long long)dwell_frames_,
      (unsigned long long)dwell_reinits_);
  if (extra <= 0 || std::size_t(n - 1 + extra) >= capacity) return 0U;
  return std::size_t(n - 1 + extra);
}
void PaletteRuntime::recordEmit(int result, std::uint32_t cycles) noexcept {
  if (result) ++emit_errors_; else ++emitted_;
  last_emit_cycles_ = cycles;
  if (cycles > maximum_emit_cycles_) maximum_emit_cycles_ = cycles;
}
} // namespace k1::titan
