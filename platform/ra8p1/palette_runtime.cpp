#include <algorithm>
#include "palette_runtime.h"
#ifdef K1_EMOTISCOPE_PORT
#include "emotiscope_port.h"
#ifdef K1_TITAN_TOP_OUTPUT
#include "emotiscope_secondary.h"
#endif
#include "emotiscope_output.h"
#endif
#include "core/visual/product_effect_renderer.h"
#ifdef K1_DUAL_EDGE_DONOR
#include "k1_dual_edge_donor.h"
#endif
#include "core/visual/product_output_treatment.h"
#include "core/visual/product_palette.h"
#include "core/visual/ws2816_pack.h"
#include "hd_pixel16.h"
#ifdef K1_PALETTE_WS2816_GPT_PAIR
#include "ws281x_gpt_dma_hw_pair.h"
#include "ws281x_p601_loopback.h"
#endif
#include <cmath>
#include <cstdio>
#include <cstring>

#ifdef K1_EMOTISCOPE_PORT
static_assert(K1_EMOTISCOPE_PALETTES == k1::core::visual::kProductPaletteCount,
              "Emotiscope palette range must match the K1 catalogue");
static void emotiscopePaletteSample(std::uint32_t id, float phase, float rgb[3]) {
  const auto c = k1::core::visual::sampleProductPaletteHd(static_cast<std::uint16_t>(id), phase, 1.0F);
  rgb[0] = c.red; rgb[1] = c.green; rgb[2] = c.blue;
}
#endif
namespace k1::titan {
using namespace core;
using namespace core::visual;
namespace {
bool supportedMode(std::uint32_t mode) noexcept {
#ifdef K1_EMOTISCOPE_PORT
  return mode <= 6U
#ifdef K1_TITAN_TOP_OUTPUT
      || k1_emotiscope_is_inhouse_mode(static_cast<std::int32_t>(mode))
#endif
      ; // Atomic family/mode selector; only admitted effects.
#endif
  if (mode == 0U || mode == kDiagnosticBounceMode) return true;
#if defined(K1_PALETTE_MORPH) && !defined(K1_EMOTISCOPE_PORT)
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
#if defined(K1_PALETTE_MORPH) && !defined(K1_EMOTISCOPE_PORT)
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
#if defined(K1_PALETTE_MORPH) && !defined(K1_EMOTISCOPE_PORT)
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
#ifdef K1_EMOTISCOPE_PORT
  // This profile owns one complete donor scene. Old Titan palette/performance
  // fields are not silently interpreted as donor controls.
  const bool version_ok = config.version == 1U && config.transition_ms == 0U &&
      config.travel_ms == 4000U && config.palette_a == 0U && config.palette_b == 0U &&
      config.mode_a == config.mode_b && config.mode_a <= 6U &&
      config.output_channel == 0U && (config.flags & ~5U) == 0U;
#elif defined(K1_PALETTE_MORPH)
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
#ifdef K1_EMOTISCOPE_PORT
  K1EmotiscopeConfig donor{};
  k1_emotiscope_get_config(&donor);
  donor.current_mode = static_cast<std::int32_t>(config.mode_a);
  if (!k1_emotiscope_set_config(&donor)) return false;
#endif
  const bool cut = config.mode_a != config_.mode_a || config.mode_b != config_.mode_b;
  const bool palette_cut =
      config.palette_a != config_.palette_a || config.palette_b != config_.palette_b;
  if (cut) {
    a_ = ChannelRenderState{PixelChannelId::kChannelA};
    b_ = ChannelRenderState{PixelChannelId::kChannelB};
  } else if (palette_cut) {
    // WaveformK1 colour EMA otherwise keeps the previous palette on the plate.
    a_.effectState().waveform_k1_dot = {};
    b_.effectState().waveform_k1_dot = {};
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
#if defined(K1_PALETTE_MORPH) && !defined(K1_EMOTISCOPE_PORT)
  ca.palette_transition = &transitions_[0];
  cb.palette_transition = &transitions_[1];
  transitions_[0].select(ca.palette_id, config.transition_ms, now_us);
  transitions_[1].select(cb.palette_id, config.transition_ms, now_us);
#endif
  next_us_ = now_us; last_us_ = now_us; cycle_start_us_ = now_us;
  last_live_us_ = 0U;
  last_live_valid_ = false;
  last_publication_us_ = 0U;
  last_presence_sequence_ = 0U;
  waiting_for_audio_ = false;
  dwell_armed_[0] = dwell_armed_[1] = false;
  visual_path_ = "none";
  last_musical_ = last_in_dwell_ = last_dwell_reinit_ = false;
  last_live_age_us_ = 0U;
  last_peak_milli_ = last_vu_milli_ = last_chroma_milli_ = last_wave_milli_ = 0U;
  effect_frames_ = dwell_frames_ = dwell_reinits_ = 0U;
  wide_valid_[0] = wide_valid_[1] = false;
  return true;
}
void PaletteRuntime::setHardwareBrightness(std::uint32_t brightness) noexcept {
  if (brightness <= 255U) config_.brightness = brightness;
}
bool PaletteRuntime::setEmissionEnabled(bool enabled) noexcept {
  const bool was = emitEnabled();
  if (enabled) config_.flags |= 4U;
  else config_.flags &= ~4U;
  return was && !enabled;
}
#if defined(K1_TITAN_TOP_OUTPUT) && defined(K1_EMOTISCOPE_PORT)
bool PaletteRuntime::setInhouseParameters(unsigned board, float sensitivity, float square_iter, float photons) noexcept {
  if(board>1U || !std::isfinite(sensitivity) || sensitivity<.10f || sensitivity>20.0f ||
      !std::isfinite(square_iter) || square_iter<0.0f || square_iter>8.0f ||
      !std::isfinite(photons) || photons<.05f || photons>1.0f)return false;
  auto& c=inhouse_controls_[board];c.sensitivity=sensitivity;c.square_iter=square_iter;c.photons=photons;
  return true;
}
bool PaletteRuntime::renderInhouse(unsigned board,std::uint64_t now_us) noexcept {
  K1EmotiscopeConfig config{};
  if(board)k1_secondary_emotiscope_get_config(&config);else k1_emotiscope_get_config(&config);
  auto controls=inhouse_controls_[board];
  controls.mode=static_cast<std::uint16_t>(config.current_mode);
  const auto palette=board?k1_secondary_emotiscope_palette():k1_emotiscope_palette();
  controls.palette_id=static_cast<std::uint16_t>(palette-1u);
  controls.motion_scale=config.speed*2.0f;
  controls.mirror=config.mirror_mode!=0;
  auto input=inhouse_audio_[board];
  if(input.epoch!=inhouse_observed_audio_[board].epoch)inhouse_admission_[board]={};
  // Intentional Titan acoustic boundary: reuse the existing measured-room Bloom
  // admission on this prepared donor publication. The source RMS silence remains
  // in the producer/MIR; only this local render view uses Titan admission. Keep
  // legacy feature values and travelling history intact.
  const bool bloom_admitted=inhouse_admission_[board].update(donor_publication_,static_cast<std::uint32_t>(now_us));
  // Once the source has confirmed quiet through its existing dwell, isolated
  // room peaks cannot reopen births below that same quiet-entry RMS. A weak
  // attack above entry still wakes locally without needing the old .08 exit;
  // brief musical gaps retain the source's unmodified quiet dwell.
  const bool confirmed_quiet=input.silence && input.raw_rms<input.quiet_entry_rms;
  input.silence=!bloom_admitted || confirmed_quiet;
  const float response=board?k1_secondary_emotiscope_response_gain():k1_emotiscope_response_gain();
  // The source 7/8/11 peak follower carries ratios above unity; its
  // consumers clamp light level independently from displacement/history.
  const float peak_limit=(config.current_mode==7 || config.current_mode==8 || config.current_mode==11)?32767.0f:1.0f;
  input.peak_scaled=std::clamp(input.peak_scaled*response,0.0f,peak_limit);
  input.vu_level=std::clamp(input.vu_level*response,0.0f,1.0f);
  input.vu_average=std::clamp(input.vu_average*response,0.0f,1.0f);
  input.observed_peak_scaled=std::clamp(input.observed_peak_scaled*response,0.0f,1.0f);
  if(!inhouse_renderer_[board].render(input,controls,now_us,wide_[board],kPixelsPerChannel))return false;
  // The core renderer remains authoritative for Inhouse chronology/validity.
  // Invalid, stale or regressive input cannot retain an open hysteresis state.
  if(!inhouse_renderer_[board].observation().input_fresh){
    inhouse_admission_[board]={};input.silence=true;
  }
  inhouse_source_silence_[board]=inhouse_audio_[board].silence;
  inhouse_observed_audio_[board]=input;inhouse_render_us_[board]=now_us;
  // Existing display-only brightness, hue-preserving shoulder, warmth and
  // white balance. Master/edge gains remain in the unchanged wire packer.
  const float brightness=.3f+.7f*config.brightness;
  const float warm[3]={1.0f,1.0f-config.warmth+config.warmth*std::sqrt(.1982f),
      1.0f-config.warmth+config.warmth*std::sqrt(.0244f)};
  const float white[3]={1.0f,.9375f,.84f};
  auto frame=board?b_.frame():a_.frame();
  for(unsigned i=0;i<kPixelsPerChannel;++i){
    auto& p=wide_[board][i];
    float c[3]={p.red/65535.0f*brightness,p.green/65535.0f*brightness,p.blue/65535.0f*brightness};
    const float maximum=std::max(c[0],std::max(c[1],c[2]));
    const float shoulder=maximum>.75f?(.75f+.25f*std::tanh((maximum-.75f)*4.0f))/maximum:1.0f;
    std::uint16_t q[3];
    for(unsigned k=0;k<3;++k)q[k]=static_cast<std::uint16_t>(std::clamp(c[k]*shoulder*warm[k]*white[k],0.0f,1.0f)*65535.0f+.5f);
    p={q[0],q[1],q[2]};
    // Preview only. History and actual wire output never pass through Pixel8.
    frame[i]={std::uint8_t(q[0]>>8),std::uint8_t(q[1]>>8),std::uint8_t(q[2]>>8)};
  }
  return true;
}
#endif
void PaletteRuntime::noteDonorMode(std::uint32_t mode, unsigned board) noexcept {
  if (!supportedMode(mode) || board > 1U) return;
#if defined(K1_TITAN_TOP_OUTPUT) && defined(K1_EMOTISCOPE_PORT)
  const auto previous=board?config_.mode_b:config_.mode_a;
  if(previous!=mode){inhouse_renderer_[board].reset();inhouse_admission_[board]={};}
#endif
  if(board==0U){config_.mode_a=mode;a_.controls().mode_id=static_cast<std::uint16_t>(mode);}
  else{config_.mode_b=mode;b_.controls().mode_id=static_cast<std::uint16_t>(mode);}
}
void PaletteRuntime::selectPerformancePalette(std::uint32_t id, std::uint64_t now_us) noexcept {
  // Caller validates the curated ID; preserve audio and owner timing.
  if (id >= kProductPaletteCount || config_.output_channel != 0U) return;
  if (id != config_.palette_a && config_.mode_a == 32U)
    a_.effectState().waveform_k1_dot = {};
  config_.palette_a = id;
  a_.controls().palette_id = static_cast<std::uint16_t>(id);
#if defined(K1_PALETTE_MORPH) && !defined(K1_EMOTISCOPE_PORT)
  transitions_[0].select(a_.controls().palette_id, config_.transition_ms, now_us);
#else
  (void)now_us;
#endif
}
void PaletteRuntime::selectPerformanceMode(std::uint32_t mode) noexcept {
  // Validated by the finite selection command. Called only by the idle RT owner.
  // Keep audio publication, timing, focus and scalar controls; reset effect history.
  if (mode == config_.mode_a && mode == config_.mode_b) return;
  auto ca = a_.controls(); auto cb = b_.controls();
  auto fa = a_.audioFocus(); auto fb = b_.audioFocus();
  a_ = ChannelRenderState{PixelChannelId::kChannelA};
  b_ = ChannelRenderState{PixelChannelId::kChannelB};
  a_.controls() = ca; b_.controls() = cb;
  a_.audioFocus() = fa; b_.audioFocus() = fb;
  config_.mode_a = config_.mode_b = mode;
  a_.controls().mode_id = b_.controls().mode_id = static_cast<std::uint16_t>(mode);
  dwell_armed_[0] = dwell_armed_[1] = false;
  wide_valid_[0] = wide_valid_[1] = false;
}
void PaletteRuntime::resetAudioState() noexcept {
#if defined(K1_TITAN_TOP_OUTPUT) && defined(K1_EMOTISCOPE_PORT)
  inhouse_admission_[0]=inhouse_admission_[1]={};
#endif
#ifdef K1_DUAL_EDGE_DONOR
  invalidateDualEdgeDonorAudio();
#endif
  last_live_us_ = 0U;
  last_live_valid_ = false;
  last_publication_us_ = 0U;
  last_presence_sequence_ = 0U;
  waiting_for_audio_ = true;
  dwell_armed_[0] = dwell_armed_[1] = false;
  visual_path_ = "none";
  last_musical_ = last_in_dwell_ = last_dwell_reinit_ = false;
  last_live_age_us_ = 0U;
}
bool PaletteRuntime::step(std::uint64_t now_us,
                          const VisualAudioFrameView* audio) noexcept {
  if (!frameDue(now_us)) return false;
  const std::uint64_t late = now_us - next_us_;
  skipped_ += late / kPalettePeriodUs;
  next_us_ += (late / kPalettePeriodUs + 1U) * kPalettePeriodUs;
  const auto cycle = (config_.flags & 2U)
      ? static_cast<unsigned>((now_us - cycle_start_us_) / 4000000U % kProductPaletteCount)
      : 0U;
  a_.controls().palette_id = static_cast<std::uint16_t>((config_.palette_a + cycle) % kProductPaletteCount);
  b_.controls().palette_id = static_cast<std::uint16_t>((config_.palette_b + cycle) % kProductPaletteCount);
#if defined(K1_PALETTE_MORPH) && !defined(K1_EMOTISCOPE_PORT)
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
#ifdef K1_EMOTISCOPE_PORT
  // Palette colour source for the donor's hsv(); registered here because this
  // unit links the K1 product palette catalogue. Idempotent.
  k1_emotiscope_set_palette_sampler(&emotiscopePaletteSample);
  std::uint8_t rgb[K1_EMOTISCOPE_PIXELS * 3U]{};
  std::uint16_t rgb16[K1_EMOTISCOPE_PIXELS * 3U]{};
#ifdef K1_TITAN_TOP_OUTPUT
  board_frame_valid_[0]=board_frame_valid_[1]=false;
#endif
  if(!k1_emotiscope_prepare_frame(static_cast<std::uint32_t>(now_us)))return false;
#ifdef K1_TITAN_TOP_OUTPUT
  K1EmotiscopeEdgePairConfig pair{};k1_emotiscope_get_edge_pair(&pair);
  k1_secondary_emotiscope_set_edge_pair(&pair);
  k1_emotiscope_audio_publication(&donor_publication_);
  if(!k1_secondary_emotiscope_accept_audio(&donor_publication_))return false;
  K1EmotiscopeMlResult model{};k1_emotiscope_get_ml_result(&model);
  k1_secondary_emotiscope_accept_ml(&model);
  k1_emotiscope_scene_pair_frame(static_cast<std::uint32_t>(now_us));
#endif
#ifdef K1_TITAN_TOP_OUTPUT
  K1EmotiscopeConfig primary_config{},secondary_config{};
  k1_emotiscope_get_config(&primary_config);k1_secondary_emotiscope_get_config(&secondary_config);
  const bool primary_inhouse=k1_emotiscope_is_inhouse_mode(primary_config.current_mode);
  const bool secondary_inhouse=k1_emotiscope_is_inhouse_mode(secondary_config.current_mode);
  if(!primary_inhouse)inhouse_admission_[0]={};
  if(!secondary_inhouse)inhouse_admission_[1]={};
  const bool primary_valid=primary_inhouse?renderInhouse(0,now_us):
      k1_emotiscope_render(static_cast<std::uint32_t>(now_us),rgb);
#else
  const bool primary_valid=k1_emotiscope_render(static_cast<std::uint32_t>(now_us),rgb);
#endif
  const bool candidate=output_precision_!=0U;
  if(primary_valid){
#ifdef K1_TITAN_TOP_OUTPUT
    if(!primary_inhouse){
#endif
    k1_emotiscope_last_rgb16(rgb16);
    for(unsigned i=0;i<kPixelsPerChannel;++i){
      const unsigned p=emotiscopeSourceIndex(i)*3U;
      a_.frame()[i]={rgb[p],rgb[p+1U],rgb[p+2U]};
      wide_[0][i]=candidate?emotiscopeWirePixel16(rgb16,i):emotiscopeWirePixel(rgb,i);
    }
#ifdef K1_TITAN_TOP_OUTPUT
    }
#endif
    wide_valid_[0]=true;
#ifdef K1_TITAN_TOP_OUTPUT
    board_frame_valid_[0]=true;board_frame_us_[0]=now_us;
#endif
  }
#ifdef K1_TITAN_TOP_OUTPUT
  // A failed visual frame retains only that board's previous output. The
  // musical publication still advances and the other VP is independent.
  const bool secondary_valid=secondary_inhouse?renderInhouse(1,now_us):k1_secondary_emotiscope_render(static_cast<std::uint32_t>(now_us),rgb);
  if(secondary_valid){
    if(!secondary_inhouse){
    k1_secondary_emotiscope_last_rgb16(rgb16);
    for(unsigned i=0;i<kPixelsPerChannel;++i){
      const unsigned p=emotiscopeSourceIndex(i)*3U;
      b_.frame()[i]={rgb[p],rgb[p+1U],rgb[p+2U]};
      wide_[1][i]=candidate?emotiscopeWirePixel16(rgb16,i):emotiscopeWirePixel(rgb,i);
    }
    }
    wide_valid_[1]=true;board_frame_valid_[1]=true;board_frame_us_[1]=now_us;
  }
  if(!primary_valid&&!secondary_valid)return false;
#else
  if(!primary_valid)return false;
  for(unsigned i=0;i<kPixelsPerChannel;++i){b_.frame()[i]=a_.frame()[i];wide_[1][i]=wide_[0][i];}
  wide_valid_[1]=true;
#endif
  last_publication_us_ = audio ? audio->source_publication_us : 0U;
  last_musical_ = audio != nullptr;
  waiting_for_audio_ = audio == nullptr;
  visual_path_ = audio ? "emotiscope" : "emotiscope_history";
#ifdef K1_TITAN_TOP_OUTPUT
  if(primary_inhouse||secondary_inhouse)visual_path_=primary_inhouse&&secondary_inhouse?"inhouse_waveform":"mixed_waveform_donor";
#endif
  if (audio) {
    last_peak_milli_ = std::uint32_t(std::clamp(audio->audio.peak_scaled,0.0F,1.0F)*10000);
    last_vu_milli_ = std::uint32_t(std::clamp(audio->audio.vu_level,0.0F,1.0F)*10000);
  }
  ++effect_frames_; ++frames_;
  return true;
#endif
#ifdef K1_DUAL_EDGE_DONOR
  // The dedicated mode8 physical audition consumes the actual live AP. The
  // retained packet scene and HTML reconstruction are not in this image.
  if(config_.mode_a == 8U && config_.mode_b == 8U) {
    renderDualEdgeDonor(now_us,audio,a_,b_,wide_[0],wide_[1]);
    wide_valid_[0]=wide_valid_[1]=true;
    last_publication_us_=audio?audio->source_publication_us:0U;
    last_musical_=audio&&k1MusicalPresence(audio->audio);
    visual_path_=audio?"effect":"no_audio";
    waiting_for_audio_=audio==nullptr;
    if(audio) { last_peak_milli_=std::uint32_t(std::clamp(audio->audio.peak_scaled,0.0F,1.0F)*10000);
      last_vu_milli_=std::uint32_t(std::clamp(audio->audio.vu_level,0.0F,1.0F)*10000); }
    ++effect_frames_;++frames_;return true;
  }
#endif
  unsigned channel_index = 0U;
  for (auto* channel : {&a_, &b_}) {
    if (channel->controls().mode_id == 0U) {
      preview(*channel, now_us, config_.flags & 8U);
      if (channel_index == 0U) visual_path_ = "preview";
    } else if (channel->controls().mode_id == kDiagnosticBounceMode) {
      diagnosticBounce(*channel, now_us);
      if (channel_index == 0U) visual_path_ = "bounce";
    }
#if defined(K1_PALETTE_MORPH) && !defined(K1_EMOTISCOPE_PORT)
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
      const bool generation_tracked = governed_audio.sequence != 0U;
      const bool new_publication =
          !generation_tracked || governed_audio.sequence != last_presence_sequence_;
      if (new_publication) {
        last_presence_sequence_ = governed_audio.sequence;
        last_publication_us_ = audio->source_publication_us;
        applyK1PresencePolicy(governed_audio, now_us, last_live_us_,
                              last_live_valid_);
      }
      const bool musical = k1MusicalPresence(governed_audio) &&
                           (!generation_tracked || new_publication);
      const bool in_dwell =
          last_live_valid_ && now_us - last_live_us_ < kTitanSilenceDwellUs;
      // DualMCU keeps the product renderer running for SILENCE_DWELL_MS after
      // the last musical peak. The outer 0.25 gate only *arms* that window;
      // chopping to dwell-age on every dip is the staggered-frame stutter.
      const bool keep_live = musical || in_dwell;
      if (channel_index == 0U) {
        last_musical_ = musical;
        last_in_dwell_ = in_dwell;
        last_live_age_us_ = last_live_valid_ ? now_us - last_live_us_ : 0U;
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
      if (!keep_live) {
#ifdef K1_LIVE_RUNTIME
        if (!last_live_valid_) {
          preview(*channel, now_us, config_.flags & 8U);
          if (channel_index == 0U) visual_path_ = "preview";
        } else {
#endif
        // Age authored history in Q8.8 with elapsed time. Do not clamp to 1.
        // Mirror and output treatment stay display-only.
        // Not-musical DualMCU after dwell expiry redrew the plate (run-29).
        // Do NOT switch to preview on black here: that erased the true-black
        // and dwell-reinit isolation contract (V-MODE32-WAKE-ISOLATION).
        // LIVE boot without prior live audio still uses the !last_live_valid_
        // preview branch above.
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
#ifdef K1_LIVE_RUNTIME
        }
#endif
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
          visual_path_ = musical ? "effect" : "hold";
          last_dwell_reinit_ = false;
          ++effect_frames_;
        }
      }
    } else {
#ifdef K1_LIVE_RUNTIME
      preview(*channel, now_us, config_.flags & 8U);
      waiting_for_audio_ = false;
      if (channel_index == 0U) visual_path_ = "preview";
#else
      channel->clearFrame();
      waiting_for_audio_ = true;
      if (channel_index == 0U) visual_path_ = "no_audio";
#endif
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
#ifdef K1_EMOTISCOPE_PORT
  if (!out || !capacity) return 0U;
  const int n = std::snprintf(out, capacity,
#ifdef K1_TITAN_TOP_OUTPUT
      "{\"version\":1,\"engine\":\"dual_edge\",\"mode_count\":12,"
      "\"supported_modes\":[0,1,2,3,4,5,6,32,18,8,7,11],\"board_capabilities\":511,"
      "\"renderer_families\":{\"emotiscope\":[0,1,2,3,4,5,6],\"inhouse_waveform\":[32,18,8,7,11]},"
      "\"palette_count\":44,\"mode_a_equals_mode_b\":false,\"configure_opcode\":25}");
#else
      "{\"version\":1,\"engine\":\"emotiscope_v1.1\",\"mode_count\":7,"
      "\"supported_modes\":[0,1,2,3,4,5,6],\"palette_count\":44,"
      "\"configure_opcode\":25,\"mode_a_equals_mode_b\":true}");
#endif
  return n > 0 && std::size_t(n) < capacity ? std::size_t(n) : 0U;
#else
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
#endif
}
bool PaletteRuntime::setOutputIntensity(unsigned lane, float intensity) noexcept {
  std::uint32_t bits = 0U;
  std::memcpy(&bits, &intensity, sizeof(bits));
  // IEEE-754 F32 ordering accepts 0..1 and negative zero; integer validation
  // rejects nonfinite and negative subnormal values even with fast-math.
  if (lane > 1U || (bits > 0x3f800000U && bits != 0x80000000U)) return false;
  output_intensity_[lane] = intensity;
  output_intensity_q16_[lane] = static_cast<std::uint16_t>(intensity * 65535.0F + 0.5F);
  return true;
}
float PaletteRuntime::outputIntensity(unsigned lane) const noexcept {
  return lane < 2U ? output_intensity_[lane] : 0.0F;
}
#ifdef K1_TITAN_TOP_OUTPUT
bool PaletteRuntime::setBoardIntensity(unsigned board, float intensity) noexcept {
  std::uint32_t bits = 0U;
  std::memcpy(&bits, &intensity, sizeof(bits));
  if (board > 1U || (bits > 0x3f800000U && bits != 0x80000000U)) return false;
  board_intensity_[board] = intensity;
  board_intensity_q16_[board] = static_cast<std::uint16_t>(intensity * 65535.0F + 0.5F);
  return true;
}
float PaletteRuntime::boardIntensity(unsigned board) const noexcept {
  return board < 2U ? board_intensity_[board] : 0.0F;
}
#endif
bool PaletteRuntime::stageWideFrame(unsigned channel_index,
                                    const Pixel16* pixels,
                                    std::size_t count) noexcept {
  if (channel_index > 1U || pixels == nullptr || count != kPixelsPerChannel) {
    return false;
  }
  std::memcpy(wide_[channel_index], pixels, sizeof(wide_[channel_index]));
  wide_valid_[channel_index] = true;
  return true;
}
std::size_t PaletteRuntime::packBenchGrb48Lane(std::uint8_t* out,
                                             std::size_t capacity,
                                             unsigned lane) const noexcept {
#ifdef K1_TITAN_TOP_OUTPUT
  return packBoardGrb48Lane(out, capacity, 0U, lane);
}
std::size_t PaletteRuntime::packBoardGrb48Lane(std::uint8_t* out, std::size_t capacity,
                                             unsigned board, unsigned lane) const noexcept {
  if (board > 1U) return 0U;
#endif
  // Select one complete visual scene, then split at logical pixels 79/80.
  // DIN identity is not visual-channel identity (K1 Main RPL WS2816 layout).
  constexpr unsigned pixels_per_lane=kPixelsPerHalf;
  constexpr unsigned bytes_per_lane=kPackedBytesPerLane;
  if (!out || capacity < bytes_per_lane || lane > 1U) return 0U;
  const unsigned channel_index =
#if defined(K1_TITAN_TOP_OUTPUT) && defined(K1_EMOTISCOPE_PORT)
      board;
#else
      config_.output_channel;
#endif
  if (channel_index > 1U || !wide_valid_[channel_index]) return 0U;
  // Brightness is applied exactly once to the wire channels. The Emotiscope
  // profile supplies RGB8 expanded by 257; other profiles may author TRUE16.
  const auto* source = wide_[channel_index];
  const std::uint32_t gain =
#ifdef K1_TITAN_TOP_OUTPUT
      board == 0U ? output_intensity_q16_[lane] : 65535U;
  const std::uint32_t board_gain = board_intensity_q16_[board];
#else
      output_intensity_q16_[lane];
#endif
  // Maximum numerator: 65535*65535+32767 = 4294868992, within uint32_t.
  const auto attenuate = [gain
#ifdef K1_TITAN_TOP_OUTPUT
      , board_gain
#endif
      ](std::uint16_t component) noexcept {
#ifdef K1_TITAN_TOP_OUTPUT
    component = static_cast<std::uint16_t>((std::uint32_t(component) * board_gain + 32767U) / 65535U);
#endif
    return static_cast<std::uint16_t>((std::uint32_t(component) * gain + 32767U) / 65535U);
  };
  for (unsigned i = 0; i < pixels_per_lane; ++i) {
    const auto pixel = source[lane * kPixelsPerHalf + i];
    packPixel(Pixel16{attenuate(scale16(pixel.red, config_.brightness)),
                      attenuate(scale16(pixel.green, config_.brightness)),
                      attenuate(scale16(pixel.blue, config_.brightness))},
              out + i * kPackedBytesPerPixel);
  }
  return bytes_per_lane;
}
std::size_t PaletteRuntime::statusJson(char* out, std::size_t capacity) const noexcept {
  if (!out || capacity == 0U) return 0U;
  const int n = std::snprintf(out, capacity,
      "{\"version\":1,\"palette_count\":%u,\"active\":%s,\"automatic_cycle\":%s,"
      "\"emit_enabled\":%s,\"configured_backend\":\"%s\",\"output_backend\":\"%s\","
      "\"physical_admission\":\"unproven\","
      "\"waiting_for_audio\":%s,\"palette_a\":%u,\"palette_b\":%u,"
      "\"name_a\":\"%s\",\"name_b\":\"%s\",\"mode_a\":%lu,\"mode_b\":%lu,"
      "\"brightness\":%lu,\"output_channel\":%lu,\"period_us\":%lu,"
      "\"frames\":%llu,\"skipped_releases\":%llu,\"emitted\":%llu,\"emit_errors\":%llu,"
      "\"last_emit_cycles\":%lu,\"maximum_emit_cycles\":%lu,"
      "\"frame_a_crc\":%lu,\"frame_b_crc\":%lu,\"native_pixels_per_channel\":160,"
#if defined(K1_EMOTISCOPE_PORT)
      "\"bench_pixels\":160,\"wire_profile\":3,\"wire_bits_per_pixel\":48,"
      "\"din_a\":\"P601\",\"din_b\":\"P604\",\"pixels_per_din\":80,"
#ifdef K1_TITAN_TOP_OUTPUT
      "\"physical_lanes\":4,\"source_precision\":\"per_renderer_rgb16\","
      "\"renderer\":\"dual_edge\",\"renderer_a\":\"%s\",\"renderer_b\":\"%s\",\"donor_pixels\":128,\"physical_strips\":2,"
#else
      "\"physical_lanes\":2,\"source_precision\":\"donor_rgb8_dither_expanded16\","
      "\"renderer\":\"emotiscope_v1.1\",\"donor_pixels\":128,\"physical_strips\":1,"
#endif
      "\"wire_snapshot_version\":1,\"legacy_performance_controls\":false,"
#elif defined(K1_DUAL_EDGE_DONOR)
      "\"bench_pixels\":160,\"wire_profile\":3,\"wire_bits_per_pixel\":48,"
      "\"din_a\":\"P601\",\"din_b\":\"P604\",\"pixels_per_din\":80,"
      "\"physical_lanes\":2,\"source_precision\":\"donor_q15_16_to_rgb16\","
      "\"renderer\":\"donor_selected_scene\",\"physical_strips\":1,\"wire_snapshot_version\":1,"
#elif defined(K1_PALETTE_WS2816_GPT_PAIR)
      "\"bench_pixels\":160,\"wire_profile\":3,\"wire_bits_per_pixel\":48,"
      "\"din_a\":\"P601\",\"din_b\":\"P604\",\"pixels_per_din\":80,"
      "\"physical_lanes\":2,\"source_precision\":\"pixel16_channel\","
      "\"true16\":\"pack_ready\","
#elif defined(K1_PALETTE_WS2816)
      "\"bench_pixels\":160,\"wire_profile\":4,\"wire_bits_per_pixel\":48,"
      "\"din_a\":\"P601\",\"din_b\":\"P004\",\"pixels_per_din\":80,"
#else
      "\"bench_pixels\":128,\"wire_profile\":1,\"wire_bits_per_pixel\":24,"
#endif
      "\"host_pixel_stream_required\":false"
#if defined(K1_PALETTE_MORPH) && !defined(K1_EMOTISCOPE_PORT)
      ",\"morph_supported\":true,\"transition_ms\":%lu,\"transition_a_q16\":%u,"
      "\"transition_b_q16\":%u,\"contributors_a\":%u,\"contributors_b\":%u,"
      "\"centre_effects_supported\":true,\"direction\":\"%s\",\"travel_ms\":%lu,"
      "\"showcase\":%s,\"effect_a\":\"%s\",\"effect_b\":\"%s\""
#else
      ",\"morph_supported\":false"
#endif
      "}",
#ifdef K1_EMOTISCOPE_PORT
      0U,
#else
      unsigned(kProductPaletteCount),
#endif
      active() ? "true" : "false",
      (config_.flags & 2U) ? "true" : "false", emitEnabled() ? "true" : "false",
      configuredPaletteBackend(), runtimePaletteOutputBackend(emitEnabled()),
      waiting_for_audio_ ? "true" : "false", unsigned(a_.controls().palette_id),
      unsigned(b_.controls().palette_id),
#ifdef K1_EMOTISCOPE_PORT
      k1_emotiscope_mode_name(config_.mode_a), k1_emotiscope_mode_name(config_.mode_b),
#else
      productPalette(a_.controls().palette_id).name, productPalette(b_.controls().palette_id).name,
#endif
      (unsigned long)a_.controls().mode_id, (unsigned long)b_.controls().mode_id,
      (unsigned long)config_.brightness, (unsigned long)config_.output_channel,
      (unsigned long)kPalettePeriodUs, (unsigned long long)frames_, (unsigned long long)skipped_,
      (unsigned long long)emitted_, (unsigned long long)emit_errors_,
      (unsigned long)last_emit_cycles_, (unsigned long)maximum_emit_cycles_,
      (unsigned long)frameCrc(a_.frame()), (unsigned long)frameCrc(b_.frame())
#if defined(K1_EMOTISCOPE_PORT) && defined(K1_TITAN_TOP_OUTPUT)
      , k1_emotiscope_is_inhouse_mode(config_.mode_a)?"inhouse_waveform":"emotiscope",k1_emotiscope_is_inhouse_mode(config_.mode_b)?"inhouse_waveform":"emotiscope"
#endif
#if defined(K1_PALETTE_MORPH) && !defined(K1_EMOTISCOPE_PORT)
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
      "\"dwell_reinits\":%llu"
#if defined(K1_EMOTISCOPE_PORT) && defined(K1_TITAN_TOP_OUTPUT)
      ",\"inhouse_donor_sequence\":%lu,\"inhouse_donor_peak_squared\":%.6f"
      ",\"inhouse_frames\":[{\"board\":0,\"render_us\":%llu,\"generation\":%llu,\"capture_us\":%llu,\"fresh\":%u,\"source_silence\":%u,\"raw_rms\":%.6f,\"quiet_entry_rms\":%.6f,\"admission_open\":%u,\"peak\":%.6f,\"position\":%.6f,\"deposit_strength\":%.6f,\"injection_count\":%llu},"
      "{\"board\":1,\"render_us\":%llu,\"generation\":%llu,\"capture_us\":%llu,\"fresh\":%u,\"source_silence\":%u,\"raw_rms\":%.6f,\"quiet_entry_rms\":%.6f,\"admission_open\":%u,\"peak\":%.6f,\"position\":%.6f,\"deposit_strength\":%.6f,\"injection_count\":%llu}]"
#endif
#ifdef K1_PALETTE_WS2816_GPT_PAIR
      ",\"pair_backend\":\"%s\",\"submitted_generation\":%lu,"
      "\"completed_generation\":%lu,\"pair_completions\":%lu,"
      "\"pending_replacements\":%lu,\"lane_a0_dma\":%lu,\"lane_a1_dma\":%lu,"
      "\"lane_a0_stop\":%lu,\"lane_a1_stop\":%lu,\"lane_a0_fault\":%lu,"
      "\"lane_a1_fault\":%lu,\"pair_fault\":%lu,"
      "\"pair_fault_site\":%lu,\"pair_fault_lane\":%lu,"
      "\"pair_fault_generation\":%lu,\"pair_fault_dmcra\":%lu,"
      "\"pair_fault_dmcra_a\":%lu,\"pair_fault_cyccnt\":%lu,"
      "\"pair_fault_gpt8_gtcr\":%lu,\"pair_fault_gpt1_gtcnt\":%lu,"
      "\"pair_fault_dma_irqs_b\":%lu,\"pair_fault_stop_irqs_b\":%lu,"
      "\"pair_last_stop_valid_a\":%lu,\"pair_last_stop_pwm_running_a\":%lu,"
      "\"pair_last_stop_gtcnt_a\":%lu,\"pair_last_stop_gtccr_a\":%lu,"
      "\"pair_last_stop_dmcra_a\":%lu,\"pair_last_stop_valid_b\":%lu,"
      "\"pair_last_stop_pwm_running_b\":%lu,\"pair_last_stop_gtcnt_b\":%lu,"
      "\"pair_last_stop_gtccr_b\":%lu,\"pair_last_stop_dmcra_b\":%lu,"
      "\"p601_loopback_valid\":%lu,\"p601_loopback_open\":%lu,"
      "\"p601_loopback_samples\":%lu,\"p601_loopback_expected\":%lu,"
      "\"p601_loopback_rising\":%lu,\"p601_loopback_falling\":%lu,"
      "\"p601_loopback_extra\":%lu,\"p601_loopback_high_after\":%lu,"
      "\"p601_loopback_first_rise\":%lu,\"p601_loopback_last_fall\":%lu,"
      "\"p601_loopback_start\":%lu,\"p601_loopback_min_high\":%lu,"
      "\"p601_loopback_max_high\":%lu,\"p601_loopback_peek_head\":%lu,"
      "\"p601_loopback_peek_tail\":%lu,"
      "\"pair_frame_max_ch\":%lu,\"pair_frame_white\":%lu,\"pair_frame_crc\":%lu,"
      "\"pair_elc_a\":%lu,\"pair_elc_b\":%lu"
      ",\"reset_admissions\":%lu,\"reset_min_cycles\":%lu,\"reset_last_cycles\":%lu,"
      "\"reset_required_cycles\":%lu,\"reset_stale_rejections\":%lu,"
      "\"reset_generation\":%lu,\"reset_cpu_hz\":%lu"
      ",\"start_samples\":%lu,\"start_min_cycles\":%lu,\"start_previous_generation\":%lu,"
      "\"start_next_generation\":%lu,\"start_stop_a\":%lu,\"start_stop_b\":%lu,"
      "\"start_before\":%lu,\"start_after\":%lu,\"reset_admission_cycle\":%lu"
#endif
      "}",
      visual_path_ ? visual_path_ : "none",
      last_musical_ ? "true" : "false", last_in_dwell_ ? "true" : "false",
      (unsigned long long)last_live_age_us_,
      (unsigned long)last_peak_milli_, (unsigned long)last_vu_milli_,
      (unsigned long)last_chroma_milli_, (unsigned long)last_wave_milli_,
      dwell_armed_[0] ? "true" : "false",
      last_dwell_reinit_ ? "true" : "false",
      (unsigned long long)effect_frames_, (unsigned long long)dwell_frames_,
      (unsigned long long)dwell_reinits_
#if defined(K1_EMOTISCOPE_PORT) && defined(K1_TITAN_TOP_OUTPUT)
      , (unsigned long)inhouseDonorSequence(),double(inhouseDonorPeakSquared())
      , (unsigned long long)inhouse_render_us_[0],(unsigned long long)inhouse_observed_audio_[0].generation,
      (unsigned long long)inhouse_observed_audio_[0].capture_us,unsigned(inhouse_renderer_[0].observation().input_fresh),
      unsigned(inhouseSourceSilence(0)),double(inhouse_observed_audio_[0].raw_rms),
      double(inhouse_observed_audio_[0].quiet_entry_rms),unsigned(inhouseAdmissionOpen(0)),
      double(inhouse_renderer_[0].observation().peak_stage1),double(inhouse_renderer_[0].observation().position),
      double(inhouse_renderer_[0].observation().deposit_strength),(unsigned long long)inhouse_renderer_[0].observation().injection_count
      , (unsigned long long)inhouse_render_us_[1],(unsigned long long)inhouse_observed_audio_[1].generation,
      (unsigned long long)inhouse_observed_audio_[1].capture_us,unsigned(inhouse_renderer_[1].observation().input_fresh),
      unsigned(inhouseSourceSilence(1)),double(inhouse_observed_audio_[1].raw_rms),
      double(inhouse_observed_audio_[1].quiet_entry_rms),unsigned(inhouseAdmissionOpen(1)),
      double(inhouse_renderer_[1].observation().peak_stage1),double(inhouse_renderer_[1].observation().position),
      double(inhouse_renderer_[1].observation().deposit_strength),(unsigned long long)inhouse_renderer_[1].observation().injection_count
#endif
#ifdef K1_PALETTE_WS2816_GPT_PAIR
      , k1_ws281x_gpt_dma_hw_pair_backend(),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_submitted_generation(),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_completed_generation(),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_completions(),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_replacements(),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_lane_dma_complete(0),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_lane_dma_complete(1),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_lane_hw_stopped(0),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_lane_hw_stopped(1),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_lane_fault(0),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_lane_fault(1),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_fault(),
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_first_fault_record()->site,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_first_fault_record()->lane,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_first_fault_record()->generation,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_first_fault_record()->dmcra[1],
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_first_fault_record()->dmcra[0],
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_first_fault_record()->cyccnt,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_first_fault_record()->gpt8_gtcr,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_first_fault_record()->gpt1_gtcnt,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_first_fault_record()->dma_irqs[1],
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_first_fault_record()->stop_irqs[1],
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(0)->valid,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(0)->pwm_running,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(0)->pwm_gtcnt,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(0)->pwm_gtccr,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(0)->dmcra,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(1)->valid,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(1)->pwm_running,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(1)->pwm_gtcnt,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(1)->pwm_gtccr,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(1)->dmcra,
      (unsigned long)k1_p601_loopback_result()->valid,
      (unsigned long)k1_p601_loopback_result()->open_ok,
      (unsigned long)k1_p601_loopback_result()->samples,
      (unsigned long)k1_p601_loopback_result()->expected_cells,
      (unsigned long)k1_p601_loopback_result()->rising,
      (unsigned long)k1_p601_loopback_result()->falling,
      (unsigned long)k1_p601_loopback_result()->extra_rising,
      (unsigned long)k1_p601_loopback_result()->high_after_last,
      (unsigned long)k1_p601_loopback_result()->first_rise,
      (unsigned long)k1_p601_loopback_result()->last_fall,
      (unsigned long)k1_p601_loopback_result()->start_level,
      (unsigned long)k1_p601_loopback_result()->min_high,
      (unsigned long)k1_p601_loopback_result()->max_high,
      (unsigned long)k1_p601_loopback_result()->peek_head,
      (unsigned long)k1_p601_loopback_result()->peek_tail,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_frame()->max_ch,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_frame()->white_pixels,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_frame()->crc_a,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(0)->elc_gpt_a,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_last_stop(0)->elc_gpt_b,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->admissions,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->min_cycles,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->last_cycles,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->required_cycles,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->stale_clock_rejections,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->generation,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->cpu_hz,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->start_samples,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->start_min_cycles,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->previous_generation,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->next_generation,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->stop_a,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->stop_b,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->start_before,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->start_after,
      (unsigned long)k1_ws281x_gpt_dma_hw_pair_reset_witness()->admission
#endif
      );
  if (extra <= 0 || std::size_t(n - 1 + extra) >= capacity) return 0U;
  std::size_t total = std::size_t(n - 1 + extra);
#ifdef K1_EMOTISCOPE_PORT
  if (out[total - 1] != '}') return 0U;
  const int more = std::snprintf(out + total - 1, capacity - total + 1,
      ",\"output_precision\":%lu,\"output_source\":\"%s\"}", (unsigned long)output_precision_,
      output_precision_ ? "donor_float_round16" : "donor_rgb8_dither_expanded16");
  if (more <= 0 || total - 1 + std::size_t(more) >= capacity) return 0U;
  total = total - 1 + std::size_t(more);
#endif
  return total;
}
void PaletteRuntime::recordEmit(int result, std::uint32_t cycles) noexcept {
  if (result) ++emit_errors_; else ++emitted_;
  last_emit_cycles_ = cycles;
  if (cycles > maximum_emit_cycles_) maximum_emit_cycles_ = cycles;
}
} // namespace k1::titan
