// K1 scalar fixture transport. This is neither a capture driver nor a realtime campaign.
#include "fixture_app.h"
#define K1_TRAJECTORY_CYCLE_COUNT() k1_cycle_count()
#include "trajectory.h"
#include "time_probe.h"
#include "build_identity.h"
#include "semantic_sidecar.h"
#ifdef K1_ENABLE_STAGE_PROBE
#include "k1_double_probe.h"
#include "stage_probe.h"
#include <algorithm>
#endif
#include <new>
#include <cstdio>
#include <cstring>
#ifdef K1_RESIDENT_SCHEDULE
#include "resident_controls.h"
#endif
#ifdef K1_NPU_LOAD
#include "npu_load.h"
#endif
#ifdef K1_P4_LOAD
#include "p4_runtime.h"
#endif
#include "core/visual/ws2816_pack.h"
#include "ws2816_gpio_emit.h"
#include "ws281x_diag.h"
#ifdef K1_PALETTE_WS2816_GPT_PAIR
#include "ws281x_gpt_dma_hw_pair.h"
#include "pair_fixture_admit.h"
#elif defined(K1_PALETTE_GPT_DMA)
#include "ws281x_gpt_dma_hw.h"
#endif
#include "k1_status_led.h"
#include "titan_led2_phy.h"
#ifdef K1_PALETTE_RUNTIME
#include "palette_runtime.h"
#include "palette_clock.h"
#include "hd_pixel16.h"
#include "core/visual/audio_focus.h"
#endif
#ifdef K1_PCM1808_TARGET
#include "pcm1808_target.h"
#endif
#ifdef K1_PDM_TARGET
#include "pdm_target.h"
#endif
#ifdef K1_LIVE_RUNTIME
#include "k1_live_runtime.h"
#include "k1_live_protocol.h"
#endif
namespace {
fixture::Trajectory trajectory;
fixture::Trace trace;
k1_led2_phy_trace_t led2_trace_entries[K1_LED2_PHY_TRACE_CAPACITY]{};
#ifdef K1_PALETTE_RUNTIME
k1::titan::PaletteRuntime palettes;
#ifdef K1_LIVE_RUNTIME
k1::titan::LiveAudioRuntime live_runtime;
k1::titan::LiveConfigBlob live_cfg{};
std::uint8_t live_cfg_stage[k1::titan::kLiveConfigBlobBytes]{};
std::uint32_t live_cfg_stage_len = 0;
std::uint32_t live_cfg_stage_expected = 0;
bool live_cfg_staging = false;
std::uint32_t live_cfg_revision = 0;

void live_hex32(const char* hex, std::uint8_t out[32]) {
  for (unsigned i = 0; i < 32U; ++i) {
    unsigned hi = 0;
    unsigned lo = 0;
    const char c0 = hex[i * 2U];
    const char c1 = hex[i * 2U + 1U];
    hi = static_cast<unsigned>(c0 <= '9' ? c0 - '0' : 10 + (c0 | 32) - 'a');
    lo = static_cast<unsigned>(c1 <= '9' ? c1 - '0' : 10 + (c1 | 32) - 'a');
    out[i] = static_cast<std::uint8_t>((hi << 4U) | lo);
  }
}

void live_copy_focus(k1::core::visual::AudioFocusProfile& profile, const float* src) {
  profile.level_gain = src[0];
  profile.novelty_gain = src[1];
  profile.low_gain = src[2];
  profile.mid_gain = src[3];
  profile.high_gain = src[4];
  profile.tonal_gain = src[5];
  profile.onset_gain = src[6];
  profile.bass_onset_gain = src[7];
  profile.beat_gain = src[8];
  profile.transient_gain = src[9];
  profile.kick_gain = src[10];
  profile.snare_gain = src[11];
  profile.hihat_gain = src[12];
  for (unsigned i = 0; i < 80U; ++i) profile.spectrum_bin_gain[i] = src[13U + i];
  for (unsigned i = 0; i < 12U; ++i) profile.chroma_bin_gain[i] = src[93U + i];
}

void live_read_focus(float* dest, const k1::core::visual::AudioFocusProfile& profile) {
  dest[0] = profile.level_gain;
  dest[1] = profile.novelty_gain;
  dest[2] = profile.low_gain;
  dest[3] = profile.mid_gain;
  dest[4] = profile.high_gain;
  dest[5] = profile.tonal_gain;
  dest[6] = profile.onset_gain;
  dest[7] = profile.bass_onset_gain;
  dest[8] = profile.beat_gain;
  dest[9] = profile.transient_gain;
  dest[10] = profile.kick_gain;
  dest[11] = profile.snare_gain;
  dest[12] = profile.hihat_gain;
  for (unsigned i = 0; i < 80U; ++i) dest[13U + i] = profile.spectrum_bin_gain[i];
  for (unsigned i = 0; i < 12U; ++i) dest[93U + i] = profile.chroma_bin_gain[i];
}

void live_copy_visual(k1::core::visual::ChannelVisualControls& c,
                     const k1::titan::LiveVisualControls& v) {
  using namespace k1::titan;
  c.chroma = v.chroma;
  c.mood = v.mood;
  c.saturation = v.saturation;
  c.square_iterations = v.square_iterations;
  c.sensitivity = v.sensitivity;
  c.incandescent_filter = v.incandescent_filter;
  c.bulb_opacity = v.bulb_opacity;
  c.base_coat_intensity = v.base_coat_intensity;
  c.prism_count = v.prism_count;
  c.hue_position = v.hue_position;
  c.chroma_value = v.chroma_value;
  c.hue_shifting_mix = v.hue_shifting_mix;
  c.vp_bloom_alpha = v.vp_bloom_alpha;
  c.vp_bloom_shift_scale = v.vp_bloom_shift_scale;
  c.vp_waveform_shift_rate = v.vp_waveform_shift_rate;
  c.vp_waveform_idle_fade = v.vp_waveform_idle_fade;
  c.vp_waveform_raw_margin = v.vp_waveform_raw_margin;
  c.vp_waveform_peak_floor = v.vp_waveform_peak_floor;
  c.vp_waveform_active_fade = v.vp_waveform_active_fade;
  c.vp_waveform_chroma_blend_gain = v.vp_waveform_chroma_blend_gain;
  c.vp_waveform_fallback_brightness = v.vp_waveform_fallback_brightness;
  c.vp_waveform_vu_floor = v.vp_waveform_vu_floor;
  c.sweet_spot_min_level = v.sweet_spot_min_level;
  c.samples_per_chunk = v.samples_per_chunk;
  c.enabled = (v.flags & kVisEnabled) != 0U;
  c.mirror_enabled = (v.flags & kVisMirror) != 0U;
  c.auto_colour_shift = (v.flags & kVisAutoColour) != 0U;
  c.reverse_order = (v.flags & kVisReverse) != 0U;
  c.incandescent_mode = (v.flags & kVisIncandescent) != 0U;
  c.temporal_dithering = (v.flags & kVisDither) != 0U;
  c.base_coat = (v.flags & kVisBaseCoat) != 0U;
  c.chromatic_mode = (v.flags & kVisChromatic) != 0U;
  c.vp_fix_agc_soft_knee = (v.flags & kVisFixAgc) != 0U;
  c.vp_fix_chroma_gate = (v.flags & kVisFixChromaGate) != 0U;
  c.vp_fix_prism_off = (v.flags & kVisFixPrismOff) != 0U;
  c.vp_fix_bloom_decay = (v.flags & kVisFixBloomDecay) != 0U;
  c.vp_fix_hsv_source_sat = (v.flags & kVisFixHsv) != 0U;
  c.vp_fix_secondary_clean = (v.flags & kVisFixSecondary) != 0U;
  c.vp_bloom_force_saturation = (v.flags & kVisBloomForceSat) != 0U;
  // configure() owns palette identity. A v2 blob must not disable palette mode
  // just because the UI omitted kVisPaletteMode.
  c.palette_mode_enabled = true;
}

void live_read_visual(k1::titan::LiveVisualControls& v,
                     const k1::core::visual::ChannelVisualControls& c) {
  using namespace k1::titan;
  v.chroma = c.chroma;
  v.mood = c.mood;
  v.saturation = c.saturation;
  v.square_iterations = c.square_iterations;
  v.sensitivity = c.sensitivity;
  v.incandescent_filter = c.incandescent_filter;
  v.bulb_opacity = c.bulb_opacity;
  v.base_coat_intensity = c.base_coat_intensity;
  v.prism_count = c.prism_count;
  v.hue_position = c.hue_position;
  v.chroma_value = c.chroma_value;
  v.hue_shifting_mix = c.hue_shifting_mix;
  v.vp_bloom_alpha = c.vp_bloom_alpha;
  v.vp_bloom_shift_scale = c.vp_bloom_shift_scale;
  v.vp_waveform_shift_rate = c.vp_waveform_shift_rate;
  v.vp_waveform_idle_fade = c.vp_waveform_idle_fade;
  v.vp_waveform_raw_margin = c.vp_waveform_raw_margin;
  v.vp_waveform_peak_floor = c.vp_waveform_peak_floor;
  v.vp_waveform_active_fade = c.vp_waveform_active_fade;
  v.vp_waveform_chroma_blend_gain = c.vp_waveform_chroma_blend_gain;
  v.vp_waveform_fallback_brightness = c.vp_waveform_fallback_brightness;
  v.vp_waveform_vu_floor = c.vp_waveform_vu_floor;
  v.sweet_spot_min_level = c.sweet_spot_min_level;
  v.samples_per_chunk = c.samples_per_chunk;
  v.flags = 0;
  if (c.enabled) v.flags |= kVisEnabled;
  if (c.mirror_enabled) v.flags |= kVisMirror;
  if (c.auto_colour_shift) v.flags |= kVisAutoColour;
  if (c.reverse_order) v.flags |= kVisReverse;
  if (c.incandescent_mode) v.flags |= kVisIncandescent;
  if (c.temporal_dithering) v.flags |= kVisDither;
  if (c.base_coat) v.flags |= kVisBaseCoat;
  if (c.palette_mode_enabled) v.flags |= kVisPaletteMode;
  if (c.chromatic_mode) v.flags |= kVisChromatic;
  if (c.vp_fix_agc_soft_knee) v.flags |= kVisFixAgc;
  if (c.vp_fix_chroma_gate) v.flags |= kVisFixChromaGate;
  if (c.vp_fix_prism_off) v.flags |= kVisFixPrismOff;
  if (c.vp_fix_bloom_decay) v.flags |= kVisFixBloomDecay;
  if (c.vp_fix_hsv_source_sat) v.flags |= kVisFixHsv;
  if (c.vp_fix_secondary_clean) v.flags |= kVisFixSecondary;
  if (c.vp_bloom_force_saturation) v.flags |= kVisBloomForceSat;
}

bool live_apply_config(const k1::titan::LiveConfigBlob& cfg, std::uint64_t now_us) {
  if (cfg.revision != live_cfg_revision) return false;
  k1::titan::PaletteConfig pal{};
  pal.version = cfg.palette_version;
  pal.palette_a = cfg.palette_a;
  pal.palette_b = cfg.palette_b;
  pal.mode_a = cfg.mode_a;
  pal.mode_b = cfg.mode_b;
  pal.flags = (cfg.flags | 4U) & ~2U;
  pal.brightness = cfg.brightness;
  pal.output_channel = cfg.output_channel;
  pal.transition_ms = cfg.transition_ms;
  pal.travel_ms = cfg.travel_ms;
  if (!palettes.configure(pal, now_us)) return false;
  live_copy_focus(palettes.channel(0).audioFocus(), cfg.focus_a);
  live_copy_focus(palettes.channel(1).audioFocus(), cfg.focus_b);
  if (cfg.version >= 2U) {
    live_copy_visual(palettes.channel(0).controls(), cfg.visual_a);
    live_copy_visual(palettes.channel(1).controls(), cfg.visual_b);
  }
  live_cfg = cfg;
  live_cfg.emit_on = 1;
  live_cfg.revision = ++live_cfg_revision;
  live_cfg.flags = palettes.config().flags;
  const auto& applied = palettes.config();
  live_runtime.setControlMeta(static_cast<std::uint16_t>(applied.mode_a),
                              static_cast<std::uint16_t>(applied.palette_a), 1,
                              live_cfg.revision,
                              static_cast<std::uint16_t>(applied.brightness));
  return true;
}

void live_refresh_cfg_from_runtime() {
  const auto& pal = palettes.config();
  live_cfg.version = 2;
  live_cfg.palette_version = pal.version;
  live_cfg.palette_a = pal.palette_a;
  live_cfg.palette_b = pal.palette_b;
  live_cfg.mode_a = pal.mode_a;
  live_cfg.mode_b = pal.mode_b;
  live_cfg.flags = pal.flags;
  live_cfg.brightness = pal.brightness;
  live_cfg.output_channel = pal.output_channel;
  live_cfg.transition_ms = pal.transition_ms;
  live_cfg.travel_ms = pal.travel_ms;
  live_cfg.emit_on = palettes.emitEnabled() ? 1 : 0;
  live_cfg.revision = live_cfg_revision;
  live_read_focus(live_cfg.focus_a, palettes.channel(0).audioFocus());
  live_read_focus(live_cfg.focus_b, palettes.channel(1).audioFocus());
  live_read_visual(live_cfg.visual_a, palettes.channel(0).controls());
  live_read_visual(live_cfg.visual_b, palettes.channel(1).controls());
}

void live_handle_config(std::uint32_t size);
#endif
std::uint32_t palette_clock_hz = 0;
std::uint64_t palette_time_us = 0;
k1::titan::PaletteClock palette_clock;
#if defined(K1_PALETTE_WS2816) || defined(K1_PALETTE_WS2816_GPT_PAIR)
// Last completed submission: 16 LE32 words, native RGB8, then both GRB48 lanes.
// Capture the actual emitter inputs, not a later re-render or a pre-gain tap.
std::uint8_t palette_wire_snapshot[64U + 480U + 960U]{};
std::uint32_t palette_wire_sequence = 0;
void snapshot_word(unsigned index, std::uint32_t value) {
  for (unsigned byte = 0; byte < 4; ++byte)
    palette_wire_snapshot[index*4U+byte] = value >> (8U*byte);
}
#endif
#ifdef K1_PALETTE_WS2816_GPT_PAIR
std::uint32_t palette_pair_frames=0, palette_pair_errors=0, palette_pair_generation=0;
bool palette_pair_pending=false;
bool fixture_hold=false;
std::uint32_t fixture_hold_generation=0;
std::uint8_t fixture_owned_a0[K1_PAIR_FIXTURE_LANE_BYTES];
std::uint8_t fixture_owned_a1[K1_PAIR_FIXTURE_LANE_BYTES];
void palette_pair_poll() {
  k1_ws281x_gpt_dma_hw_pair_poll();
  const auto completed=k1_ws281x_gpt_dma_hw_pair_completions();
  const auto failed=k1_ws281x_gpt_dma_hw_pair_errors();
  if (completed!=palette_pair_frames) { palettes.recordEmit(0,0); palette_pair_frames=completed; }
  if (failed!=palette_pair_errors) { palettes.recordEmit(1,0); palette_pair_errors=failed; }
}
#endif
#ifdef K1_PALETTE_GPT_DMA
std::uint32_t palette_gpt_frames=0, palette_gpt_errors=0;
bool palette_gpt_pending=false;
std::uint8_t palette_last_wire[128U * 3U];
std::size_t palette_last_wire_size=0;
void palette_gpt_poll() {
  k1_ws281x_gpt_dma_hw_poll();
  const auto completed=k1_ws281x_gpt_dma_hw_completions();
  const auto failed=k1_ws281x_gpt_dma_hw_errors();
  if (completed!=palette_gpt_frames) { palettes.recordEmit(0,0); palette_gpt_frames=completed; }
  if (failed!=palette_gpt_errors) { palettes.recordEmit(1,0); palette_gpt_errors=failed; }
}
#endif
void palette_step(bool emit) {
#ifdef K1_PALETTE_WS2816_GPT_PAIR
  palette_pair_poll();
#endif
#ifdef K1_PALETTE_GPT_DMA
  palette_gpt_poll();
#endif
#ifdef K1_LIVE_RUNTIME
  const k1::core::audio::AudioPipelineOutput* live = live_runtime.latest();
  const k1::core::visual::VisualAudioFrameView view{
      live ? live->features : trajectory.output.features,
      live ? live->tempo : trajectory.output.tempo,
      live_runtime.waveform(),
      live ? live->features.publish_time_us : 0U};
  // Freshness is a MIR flag, not a licence to starve the plate.
  const bool rendered=palettes.step(palette_time_us, (live && live->valid) ? &view : nullptr);
#else
  const k1::core::visual::VisualAudioFrameView view{
      trajectory.output.features, trajectory.output.tempo, trajectory.waveform,
      trajectory.output.features.publish_time_us};
  const bool rendered=palettes.step(palette_time_us, trajectory.output.valid ? &view : nullptr);
#endif
#ifdef K1_PALETTE_WS2816_GPT_PAIR
  if (fixture_hold) {
    palette_pair_poll();
    if (!k1_ws281x_gpt_dma_hw_pair_busy() &&
        k1_ws281x_gpt_dma_hw_pair_completed_generation() == fixture_hold_generation &&
        k1_ws281x_gpt_dma_hw_pair_lane_fault(0) == 0U &&
        k1_ws281x_gpt_dma_hw_pair_lane_fault(1) == 0U) {
      fixture_hold = false;
    }
  }
  if (rendered) palette_pair_pending=true;
  if (!rendered) return;
#else
#ifdef K1_PALETTE_GPT_DMA
  if (rendered) palette_gpt_pending=true;
  if (!palette_gpt_pending || !k1_ws281x_gpt_dma_hw_ready()) return;
#else
  if (!rendered) return;
#endif
#endif
  if (emit && palettes.emitEnabled()) {
#ifdef K1_PALETTE_WS2816_GPT_PAIR
    const auto& config = palettes.config();
    const auto& selected = palettes.channel(config.output_channel);
    snapshot_word(0, 1U); snapshot_word(1, ++palette_wire_sequence);
    snapshot_word(2, config.output_channel); snapshot_word(3, config.brightness);
    snapshot_word(4, 2U); snapshot_word(5, 80U);
    snapshot_word(6, 48U); snapshot_word(7, 3U);
    snapshot_word(8, selected.controls().mode_id);
    snapshot_word(9, selected.controls().palette_id);
    snapshot_word(10, config.flags); snapshot_word(11, k1::titan::kPalettePeriodUs);
    snapshot_word(14, static_cast<std::uint32_t>(palette_time_us));
    snapshot_word(15, static_cast<std::uint32_t>(palette_time_us >> 32U));
    static_assert(sizeof(k1::core::Pixel8) == 3U);
    std::memcpy(palette_wire_snapshot+64U, selected.frame().data(), 480U);
    auto* wire_a0 = palette_wire_snapshot + 64U + 480U;
    auto* wire_a1 = palette_wire_snapshot + 64U + 480U + 480U;
    {
      k1::core::visual::Pixel16 wide[k1::core::visual::kPixelsPerChannel]{};
      const auto frame = selected.frame();
      for (unsigned i = 0; i < k1::core::visual::kPixelsPerChannel; ++i) {
        // Staging bridge from the Pixel8 renderer. This is REPLICATE8 into the
        // wide buffer so the Pixel16 pack path can run; it is not a TRUE16
        // colour claim. Authored 0x12AB patterns must use stageWideFrame.
        wide[i] = k1::titan::liftPixel8(frame[i]);
      }
      (void)palettes.stageWideFrame(config.output_channel, wide,
                                    k1::core::visual::kPixelsPerChannel);
    }
    const auto size0 = palettes.packBenchGrb48Lane(wire_a0, 480U, 0U);
    const auto size1 = palettes.packBenchGrb48Lane(wire_a1, 480U, 1U);
    if (size0 == 480U && size1 == 480U && !fixture_hold) {
      const std::uint32_t generation = ++palette_pair_generation;
      snapshot_word(12, generation);
      snapshot_word(13, generation);
      const int status = k1_ws281x_gpt_dma_hw_pair_submit(
          wire_a0, size0, wire_a1, size1, 3U, generation);
      if (status == K1_WS281X_SUBMIT_ACCEPTED ||
          status == K1_WS281X_SUBMIT_BUSY) {
        palette_pair_pending = false;
      }
    }
#elif defined(K1_PALETTE_WS2816)
    const auto& config = palettes.config();
    const auto& selected = palettes.channel(config.output_channel);
    snapshot_word(0, 1U); snapshot_word(1, ++palette_wire_sequence);
    snapshot_word(2, config.output_channel); snapshot_word(3, config.brightness);
    snapshot_word(4, 2U); snapshot_word(5, 80U);
    snapshot_word(6, 48U); snapshot_word(7, 4U);
    snapshot_word(8, selected.controls().mode_id);
    snapshot_word(9, selected.controls().palette_id);
    snapshot_word(10, config.flags); snapshot_word(11, k1::titan::kPalettePeriodUs);
    snapshot_word(14, static_cast<std::uint32_t>(palette_time_us));
    snapshot_word(15, static_cast<std::uint32_t>(palette_time_us >> 32U));
    static_assert(sizeof(k1::core::Pixel8) == 3U);
    std::memcpy(palette_wire_snapshot+64U, selected.frame().data(), 480U);
    {
      k1::core::visual::Pixel16 wide[k1::core::visual::kPixelsPerChannel]{};
      const auto frame = selected.frame();
      for (unsigned i = 0; i < k1::core::visual::kPixelsPerChannel; ++i) {
        wide[i] = k1::titan::liftPixel8(frame[i]);
      }
      (void)palettes.stageWideFrame(config.output_channel, wide,
                                    k1::core::visual::kPixelsPerChannel);
    }
    int status = 0;
    std::uint32_t cycles = 0;
    for (unsigned lane = 0; lane < 2; ++lane) {
      auto* wire = palette_wire_snapshot + 64U + 480U + lane*480U;
      const auto size = palettes.packBenchGrb48Lane(wire, 480U, lane);
      k1_ws281x_diag_result_t result{};
      const int emitted = k1_ws281x_diag_emit(wire, size, 4U, lane, palette_clock_hz, &result);
      snapshot_word(12U+lane, static_cast<std::uint32_t>(emitted));
      if (emitted) status = emitted;
      cycles += result.emit_cycles;
    }
    palettes.recordEmit(status, cycles);
#else
    std::uint8_t wire[128U * 3U];
    const auto size = palettes.packBenchGrb(wire, sizeof(wire));
#ifdef K1_PALETTE_GPT_DMA
    // Driver owns its copied duty buffer until hardware stop + reset interval.
    const int status=k1_ws281x_gpt_dma_hw_submit(wire,size,1U);
    if (status==K1_WS281X_SUBMIT_ACCEPTED) {
      palette_gpt_pending=false;
      std::memcpy(palette_last_wire, wire, size);
      palette_last_wire_size = size;
    }
    // Submit acceptance is not an emitted frame. Only poll records completion.
#else
    k1_ws281x_diag_result_t result{};
    const int status = k1_ws281x_diag_emit(wire, size, 1U, 0U, palette_clock_hz, &result);
    palettes.recordEmit(status, result.emit_cycles);
#endif
#endif
  }
}
#endif
#ifdef K1_LIVE_RUNTIME
constexpr std::size_t kMaxRequestPayload=2048U;
#else
constexpr std::size_t kMaxRequestPayload=16U+K1_WS281X_DIAG_MAX_BYTES;
#endif
std::uint8_t rx[32U+kMaxRequestPayload], tx[20000], board_uid[16];
std::size_t fill = 0, wanted = 32, tx_size = 0;
std::uint32_t started = 0, clock_hz = 0, cpu_wait = 0, rejected = 0;
bool initialised = false;
void respond(std::uint32_t status, std::uint32_t cycles, const char* payload, std::size_t size);
void error(std::uint32_t status);
#ifdef K1_RESIDENT_SCHEDULE
template<unsigned Maximum> struct Distribution {
  std::uint32_t bins[Maximum+1]{};
  std::uint64_t sum=0;
  std::uint32_t count=0, maximum=0;
  void add(std::uint32_t value) {
    ++bins[value>Maximum?Maximum:value]; sum+=value; ++count;
    if(value>maximum) maximum=value;
  }
  std::uint32_t percentile(unsigned numerator) const {
    if(!count) return 0;
    const std::uint32_t wanted=(count*numerator+99)/100;
    std::uint32_t seen=0;
    for(unsigned i=0;i<=Maximum;++i) if((seen+=bins[i])>=wanted) return i;
    return Maximum;
  }
};
#ifdef K1_ENABLE_STAGE_PROBE
constexpr unsigned kStageBinShift=14;
constexpr unsigned kStageBinWidthCycles=1U<<kStageBinShift;
constexpr unsigned kStageOverflowBin=256;
struct StageDistribution {
  std::uint32_t bins[kStageOverflowBin+1]{};
  std::uint64_t sum=0;
  std::uint32_t count=0, maximum=0;
  void add(std::uint32_t cycles) {
    const unsigned bin=cycles>>kStageBinShift;
    ++bins[bin>kStageOverflowBin?kStageOverflowBin:bin];
    sum+=cycles; ++count; if(cycles>maximum) maximum=cycles;
  }
  std::uint32_t percentileLower(unsigned numerator) const {
    if(!count) return 0;
    const std::uint32_t wanted=(count*numerator+99)/100;
    std::uint32_t seen=0;
    for(unsigned i=0;i<=kStageOverflowBin;++i)
      if((seen+=bins[i])>=wanted) return i*kStageBinWidthCycles;
    return kStageOverflowBin*kStageBinWidthCycles;
  }
};
constexpr const char* kStageNames[k1_stage_count]={
  "ap_total","gdft_raw","gdft_postprocess","features","chord_detect",
  "onset_beat","musical_saliency",
  "tempo_total","tempo_history","tempo_acf_total","acf_prepare",
  "acf_correlate","acf_comb","acf_normalise","tempo_bank",
  "tempo_flywheel","tempo_output","clock_affine","musical_time",
  "vp_render","telemetry"
};
static_assert(sizeof(kStageNames)/sizeof(kStageNames[0])==k1_stage_count);
struct RawHop {
  std::uint32_t total_cycles, lateness_cycles, flags;
  std::uint32_t injected_delay_cycles;
  std::uint32_t double_calls, double_cycles;
  std::uint32_t stages[k1_stage_count];
};
constexpr std::uint32_t kRawTraceVersion=2;
constexpr std::uint32_t kRawTraceWords=7+k1_stage_count;
constexpr std::uint32_t kRawTraceStride=kRawTraceWords*sizeof(std::uint32_t);
constexpr std::uint32_t kRawTraceChunk=128;
#endif
struct ScheduleState {
  bool active=false, finished=false;
  std::uint32_t loops=0, loop=0, hop=0, flags=0, last_cycle=0;
  std::uint64_t elapsed_cycles=0, next_release=0;
  std::uint32_t correctness_failures=0, deadline_misses=0, render_misses=0;
  std::uint32_t release_guard_failures=0, backlog_highwater=0;
  std::uint32_t npu_invocations=0, npu_invoke_failures=0, npu_output_failures=0;
  std::uint64_t npu_cycles=0, npu_active_cycles=0, mac_active_cycles=0;
  k1_semantic_state_t semantic{};
  // Distribution<8000> saturates G4 p99. Uncapped authority is the raw hop
  // record when K1_ENABLE_STAGE_PROBE is set; these bins remain a compact
  // non-profile summary only.
  Distribution<8000> total, tempo, ordinary;
  Distribution<2000> render;
  Distribution<1000> lateness;
  Distribution<4000> telemetry;
  Distribution<8000> npu_wall;
  std::uint32_t prev_workload_cycles=0, prev_telemetry_cycles=0, last_usb_event=0;
  std::uint32_t edge_count=0;
  struct EdgeRecord {
    std::uint32_t hop, loop, lateness_cycles, workload_cycles, completion_cycles;
    std::uint32_t prev_workload_cycles, prev_telemetry_cycles, last_usb_event, flags;
  } edge[8]{};
#ifdef K1_ENABLE_STAGE_PROBE
  StageDistribution stages[k1_stage_count];
  RawHop raw[K1_RESIDENT_HOPS];
  std::uint32_t raw_count=0, raw_active=0xffffffffU;
#endif
} schedule;
std::uint32_t microseconds(std::uint32_t cycles) {
  return clock_hz?static_cast<std::uint32_t>((std::uint64_t(cycles)*1000000U+clock_hz-1)/clock_hz):0xffffffffU;
}
std::uint32_t microseconds64(std::uint64_t cycles) {
  if(!clock_hz) return 0xffffffffU;
  const std::uint64_t value=(cycles*1000000U+clock_hz-1)/clock_hz;
  return value>0xffffffffU?0xffffffffU:static_cast<std::uint32_t>(value);
}
bool format_mean(char* output,std::size_t capacity,std::uint64_t sum,std::uint32_t count) {
  const std::uint64_t scaled=count?(sum*1000U+count/2U)/count:0;
  const int n=std::snprintf(output,capacity,"%llu.%03llu",
    (unsigned long long)(scaled/1000U),(unsigned long long)(scaled%1000U));
  return n>0 && std::size_t(n)<capacity;
}
template<class D> void distribution(char* output,std::size_t capacity,std::size_t& offset,const char* name,const D& d) {
  char mean[32];
  if(!format_mean(mean,sizeof(mean),d.sum,d.count)) { offset=capacity; return; }
  const int n=std::snprintf(output+offset,capacity-offset,
    "\"%s\":{\"count\":%lu,\"mean_us\":%s,\"p50_us\":%lu,\"p95_us\":%lu,\"p99_us\":%lu,\"max_us\":%lu}",
    name,(unsigned long)d.count,mean,
    (unsigned long)d.percentile(50),(unsigned long)d.percentile(95),
    (unsigned long)d.percentile(99),(unsigned long)d.maximum);
  if(n<0 || std::size_t(n)>=capacity-offset) offset=capacity; else offset+=std::size_t(n);
}
#ifdef K1_ENABLE_STAGE_PROBE
void stage_distribution(char* output,std::size_t capacity,std::size_t& offset,const char* name,const StageDistribution& d) {
  char mean[32];
  if(!format_mean(mean,sizeof(mean),d.sum,d.count)) { offset=capacity; return; }
  const int n=std::snprintf(output+offset,capacity-offset,
    "\"%s\":{\"count\":%lu,\"mean_cycles\":%s,\"p50_bin_lower_cycles\":%lu,\"p95_bin_lower_cycles\":%lu,\"p99_bin_lower_cycles\":%lu,\"max_cycles\":%lu}",
    name,(unsigned long)d.count,mean,
    (unsigned long)d.percentileLower(50),(unsigned long)d.percentileLower(95),
    (unsigned long)d.percentileLower(99),(unsigned long)d.maximum);
  if(n<0 || std::size_t(n)>=capacity-offset) offset=capacity; else offset+=std::size_t(n);
}
void uncapped_from_values(char* output,std::size_t capacity,std::size_t& offset,
                          const char* name,std::uint32_t* values,std::uint32_t count) {
  if(!count) {
    const int n=std::snprintf(output+offset,capacity-offset,
      "\"%s\":{\"count\":0,\"mean_us\":0.000,\"min_us\":0,\"p50_us\":0,\"p95_us\":0,\"p99_us\":0,\"max_us\":0,\"authority\":\"raw_hops\",\"saturating_bins\":false}",
      name);
    if(n<0 || std::size_t(n)>=capacity-offset) offset=capacity; else offset+=std::size_t(n);
    return;
  }
  std::sort(values,values+count);
  std::uint64_t sum=0;
  for(std::uint32_t i=0;i<count;++i) sum+=values[i];
  char mean[32];
  if(!format_mean(mean,sizeof(mean),sum,count)) { offset=capacity; return; }
  const auto pct=[&](unsigned p) {
    return values[(count-1U)*p/100U];
  };
  const int n=std::snprintf(output+offset,capacity-offset,
    "\"%s\":{\"count\":%lu,\"mean_us\":%s,\"min_us\":%lu,\"p50_us\":%lu,\"p95_us\":%lu,\"p99_us\":%lu,\"max_us\":%lu,\"authority\":\"raw_hops\",\"saturating_bins\":false}",
    name,(unsigned long)count,mean,
    (unsigned long)values[0],(unsigned long)pct(50),(unsigned long)pct(95),
    (unsigned long)pct(99),(unsigned long)values[count-1U]);
  if(n<0 || std::size_t(n)>=capacity-offset) offset=capacity; else offset+=std::size_t(n);
}
void emit_uncapped_from_raw(char* output,std::size_t capacity,std::size_t& offset) {
  static std::uint32_t scratch[K1_RESIDENT_HOPS];
  const auto fill=[&](int selector) -> std::uint32_t {
    std::uint32_t count=0;
    for(std::uint32_t i=0;i<schedule.raw_count && i<K1_RESIDENT_HOPS;++i) {
      const bool tempo=(schedule.raw[i].flags&1U)!=0U;
      if(selector==0 || (selector==1 && tempo) || (selector==2 && !tempo))
        scratch[count++]=microseconds(schedule.raw[i].total_cycles);
    }
    return count;
  };
  std::uint32_t count=fill(0);
  uncapped_from_values(output,capacity,offset,"total",scratch,count);
  if(offset<capacity) output[offset++]=',';
  count=fill(1);
  uncapped_from_values(output,capacity,offset,"tempo",scratch,count);
  if(offset<capacity) output[offset++]=',';
  count=fill(2);
  uncapped_from_values(output,capacity,offset,"ordinary",scratch,count);
}
#endif
void schedule_status() {
  std::size_t n=0;
  const int first=std::snprintf(trace.data,sizeof(trace.data),
    "{\"profile\":\"k1-production-resident-v1\",\"active\":%s,\"finished\":%s,\"loops_requested\":%lu,\"loops_complete\":%lu,\"hop\":%lu,\"mode\":%lu,\"elapsed_cycles\":%llu,\"queue_capacity\":1,\"drops\":0,\"coalesces\":0,\"crc_mutation_injected\":%s,\"timing_mutation_injected\":%s,\"correctness_failures\":%lu,\"deadline_misses\":%lu,\"render_misses\":%lu,\"release_guard_failures\":%lu,\"backlog_highwater\":%lu,\"npu_ready\":%s,\"npu_invocations\":%lu,\"npu_invoke_failures\":%lu,\"npu_output_failures\":%lu,\"npu_cycles\":%llu,\"npu_active_cycles\":%llu,\"mac_active_cycles\":%llu,\"npu_wall_sum_us\":%llu,\"npu_duty_ppm\":%llu,\"semantic_valid\":%s,\"semantic_accepted\":%lu,\"semantic_loss\":%lu,\"semantic_delayed\":%lu,\"semantic_stale\":%lu,\"semantic_out_of_order\":%lu,\"semantic_invalid_identity\":%lu,\"semantic_non_finite\":%lu,\"semantic_queue_pressure\":%lu,\"semantic_accelerator_errors\":%lu,\"semantic_accelerator_timeouts\":%lu,\"semantic_age_timeouts\":%lu,\"semantic_recoveries\":%lu,\"semantic_fallback_hops\":%lu,",
    schedule.active?"true":"false",schedule.finished?"true":"false",
    (unsigned long)schedule.loops,(unsigned long)schedule.loop,(unsigned long)schedule.hop,(unsigned long)((schedule.flags>>4)&3U),(unsigned long long)schedule.elapsed_cycles,
    (schedule.flags&2U)?"true":"false",
    (schedule.flags&0x80U)?"true":"false",
    (unsigned long)schedule.correctness_failures,(unsigned long)schedule.deadline_misses,
    (unsigned long)schedule.render_misses,(unsigned long)schedule.release_guard_failures,
    (unsigned long)schedule.backlog_highwater,
#ifdef K1_NPU_LOAD
    k1_npu_ready()?"true":"false",
#else
    "false",
#endif
    (unsigned long)schedule.npu_invocations,(unsigned long)schedule.npu_invoke_failures,
    (unsigned long)schedule.npu_output_failures,(unsigned long long)schedule.npu_cycles,
    (unsigned long long)schedule.npu_active_cycles,(unsigned long long)schedule.mac_active_cycles,
    (unsigned long long)schedule.npu_wall.sum,
    (unsigned long long)(schedule.loops?schedule.npu_wall.sum*1000000ULL/(std::uint64_t(schedule.loops)*45000000ULL):0),
    schedule.semantic.valid?"true":"false",(unsigned long)schedule.semantic.accepted,
    (unsigned long)schedule.semantic.loss,(unsigned long)schedule.semantic.delayed,
    (unsigned long)schedule.semantic.stale,(unsigned long)schedule.semantic.out_of_order,
    (unsigned long)schedule.semantic.invalid_identity,(unsigned long)schedule.semantic.non_finite,
    (unsigned long)schedule.semantic.queue_pressure,(unsigned long)schedule.semantic.accelerator_errors,
    (unsigned long)schedule.semantic.accelerator_timeouts,(unsigned long)schedule.semantic.age_timeouts,
    (unsigned long)schedule.semantic.recoveries,(unsigned long)schedule.semantic.fallback_hops);
  if(first<0 || std::size_t(first)>=sizeof(trace.data)) { error(8); return; }
  n=std::size_t(first);
  if(schedule.active && !schedule.finished) {
    if(n && trace.data[n-1]==',') --n;
    if(n+2>sizeof(trace.data)) { error(8); return; }
    trace.data[n++]='}';
    respond(0,0,trace.data,n);
    return;
  }
#ifdef K1_ENABLE_STAGE_PROBE
  emit_uncapped_from_raw(trace.data,sizeof(trace.data),n); if(n<sizeof(trace.data)) trace.data[n++]=',';
#else
  distribution(trace.data,sizeof(trace.data),n,"total",schedule.total); if(n<sizeof(trace.data)) trace.data[n++]=',';
  distribution(trace.data,sizeof(trace.data),n,"tempo",schedule.tempo); if(n<sizeof(trace.data)) trace.data[n++]=',';
  distribution(trace.data,sizeof(trace.data),n,"ordinary",schedule.ordinary); if(n<sizeof(trace.data)) trace.data[n++]=',';
#endif
  distribution(trace.data,sizeof(trace.data),n,"render",schedule.render); if(n<sizeof(trace.data)) trace.data[n++]=',';
  distribution(trace.data,sizeof(trace.data),n,"lateness",schedule.lateness); if(n<sizeof(trace.data)) trace.data[n++]=',';
  distribution(trace.data,sizeof(trace.data),n,"telemetry",schedule.telemetry);
  if(n<sizeof(trace.data)) trace.data[n++]=',';
  distribution(trace.data,sizeof(trace.data),n,"npu_wall",schedule.npu_wall);
  if(n<sizeof(trace.data)) trace.data[n++]=',';
  {
    const int edge_head=std::snprintf(trace.data+n,sizeof(trace.data)-n,
      "\"release_edge\":{\"guard_us\":100,\"hold_usb_us\":800,\"hold_yield_us\":1111,\"count\":%lu,\"records\":[",
      (unsigned long)schedule.edge_count);
    if(edge_head<0 || std::size_t(edge_head)>=sizeof(trace.data)-n) { error(8); return; }
    n+=std::size_t(edge_head);
    for(std::uint32_t i=0;i<schedule.edge_count;++i) {
      const auto& e=schedule.edge[i];
      const int row=std::snprintf(trace.data+n,sizeof(trace.data)-n,
        "%s{\"hop\":%lu,\"loop\":%lu,\"lateness_us\":%lu,\"workload_us\":%lu,\"response_us\":%lu,\"prev_workload_us\":%lu,\"prev_telemetry_us\":%lu,\"last_usb_event\":%lu,\"tempo\":%s,\"rendered\":%s}",
        i?",":"",
        (unsigned long)e.hop,(unsigned long)e.loop,
        (unsigned long)microseconds(e.lateness_cycles),
        (unsigned long)microseconds(e.workload_cycles),
        (unsigned long)microseconds(e.completion_cycles),
        (unsigned long)microseconds(e.prev_workload_cycles),
        (unsigned long)microseconds(e.prev_telemetry_cycles),
        (unsigned long)e.last_usb_event,
        (e.flags&1U)?"true":"false",(e.flags&2U)?"true":"false");
      if(row<0 || std::size_t(row)>=sizeof(trace.data)-n) { error(8); return; }
      n+=std::size_t(row);
    }
    if(n+2>sizeof(trace.data)) { error(8); return; }
    trace.data[n++]=']'; trace.data[n++]='}';
  }
#ifdef K1_ENABLE_STAGE_PROBE
  if(n<sizeof(trace.data)) trace.data[n++]=',';
  const int profile=std::snprintf(trace.data+n,sizeof(trace.data)-n,
    "\"stage_profile\":{\"bin_width_cycles\":%u,\"raw_trace\":{\"version\":%lu,\"records\":%lu,\"stride\":%lu},\"stages\":{",
    kStageBinWidthCycles,(unsigned long)kRawTraceVersion,
    (unsigned long)schedule.raw_count,(unsigned long)kRawTraceStride);
  if(profile<0 || std::size_t(profile)>=sizeof(trace.data)-n) { error(8); return; }
  n+=std::size_t(profile);
  for(unsigned stage=0;stage<k1_stage_count;++stage) {
    stage_distribution(trace.data,sizeof(trace.data),n,kStageNames[stage],schedule.stages[stage]);
    if(stage+1<k1_stage_count && n<sizeof(trace.data)) trace.data[n++]=',';
  }
  if(n+2>sizeof(trace.data)) { error(8); return; }
  trace.data[n++]='}'; trace.data[n++]='}';
#endif
  if(n+2>sizeof(trace.data)) { error(8); return; }
  trace.data[n++]='}'; respond(0,0,trace.data,n);
}
#endif
std::uint32_t get32(const std::uint8_t* p) { return p[0] | (p[1]<<8) | (p[2]<<16) | (std::uint32_t(p[3])<<24); }
void put32(std::uint8_t* p, std::uint32_t x) { for (unsigned i=0;i<4;++i) p[i]=std::uint8_t(x>>(8*i)); }
std::uint32_t crc(const std::uint8_t* p, std::size_t n) {
  std::uint32_t c = 0xffffffffU;
  while (n--) { c ^= *p++; for (unsigned i=0;i<8;++i) c=(c>>1)^((c&1)?0xedb88320U:0); }
  return ~c;
}
void respond(std::uint32_t status, std::uint32_t cycles, const char* payload, std::size_t size) {
  if (size > sizeof(tx)-32 || tx_size) return;
  std::memset(tx, 0, 32); std::memcpy(tx,"K1R1",4);
  put32(tx+4,status); put32(tx+8,get32(rx+8)); put32(tx+12,trajectory.sequence);
  put32(tx+16,static_cast<std::uint32_t>(size)); put32(tx+20,cycles);
  std::memcpy(tx+32,payload,size); put32(tx+24,crc(tx+32,size)); put32(tx+28,crc(tx,28));
  tx_size = size+32;
}
void error(std::uint32_t status) { ++rejected; respond(status,0,"",0); fill=0; wanted=32; }
#ifdef K1_PALETTE_RUNTIME
#ifdef K1_LIVE_RUNTIME
void live_handle_config(std::uint32_t size) {
  if (size < 4U) { error(3); return; }
  const auto* p = rx + 32;
  const std::uint32_t sub = get32(p);
  if (sub == k1::titan::kLiveCfgGetSchema) {
    std::uint32_t offset = 0;
    std::uint32_t want = 4032;
    if (size >= 12U) {
      offset = get32(p + 4);
      want = get32(p + 8);
    }
    const auto n = k1::titan::liveEncodeSchemaPage(
        reinterpret_cast<std::uint8_t*>(trace.data), sizeof(trace.data), offset, want);
    if (!n) error(8); else respond(0, 0, trace.data, n);
    return;
  }
  if (sub == k1::titan::kLiveCfgGetConfig) {
    live_refresh_cfg_from_runtime();
    const auto n = k1::titan::liveEncodeConfig(
        reinterpret_cast<std::uint8_t*>(trace.data), sizeof(trace.data), live_cfg);
    if (!n) error(8); else respond(0, 0, trace.data, n);
    return;
  }
  if (sub == k1::titan::kLiveCfgSetConfig) {
    if (size < 4U || !k1::titan::liveConfigSizeAccepted(size - 4U)) { error(3); return; }
    k1::titan::LiveConfigBlob cfg{};
    if (!k1::titan::liveDecodeConfig(p + 4, size - 4U, &cfg)) {
      error(3); return;
    }
    if (!live_apply_config(cfg, palette_time_us)) { error(3); return; }
    live_refresh_cfg_from_runtime();
    const auto n = k1::titan::liveEncodeConfig(
        reinterpret_cast<std::uint8_t*>(trace.data), sizeof(trace.data), live_cfg);
    if (!n) error(8); else respond(0, 0, trace.data, n);
    return;
  }
  if (sub == k1::titan::kLiveCfgBeginSet && size == 8U) {
    live_cfg_stage_expected = get32(p + 4);
    if (!k1::titan::liveConfigSizeAccepted(live_cfg_stage_expected)) { error(3); return; }
    live_cfg_stage_len = 0;
    live_cfg_staging = true;
    respond(0, 0, "BEGIN", 5);
    return;
  }
  if (sub == k1::titan::kLiveCfgAppendSet && size >= 8U && live_cfg_staging) {
    const std::uint32_t offset = get32(p + 4);
    const std::uint32_t nbytes = size - 8U;
    if (offset != live_cfg_stage_len || offset + nbytes > live_cfg_stage_expected) {
      error(3); return;
    }
    std::memcpy(live_cfg_stage + offset, p + 8, nbytes);
    live_cfg_stage_len += nbytes;
    respond(0, 0, "APPEND", 6);
    return;
  }
  if (sub == k1::titan::kLiveCfgCommitSet && size == 4U && live_cfg_staging) {
    if (live_cfg_stage_len != live_cfg_stage_expected) { error(3); return; }
    k1::titan::LiveConfigBlob cfg{};
    if (!k1::titan::liveDecodeConfig(live_cfg_stage, live_cfg_stage_len, &cfg) ||
        !live_apply_config(cfg, palette_time_us)) {
      error(3); return;
    }
    live_cfg_staging = false;
    live_refresh_cfg_from_runtime();
    const auto n = k1::titan::liveEncodeConfig(
        reinterpret_cast<std::uint8_t*>(trace.data), sizeof(trace.data), live_cfg);
    if (!n) error(8); else respond(0, 0, trace.data, n);
    return;
  }
  if (sub == k1::titan::kLiveCfgAbortSet && size == 4U) {
    live_cfg_staging = false;
    live_cfg_stage_len = 0;
    respond(0, 0, "ABORT", 5);
    return;
  }
  if (sub == k1::titan::kLiveCfgTestStale && size == 4U + 16U + 32U) {
    if (std::memcmp(p + 4, board_uid, 16) != 0) { error(5); return; }
    std::uint8_t build[32];
    live_hex32(K1_BUILD_ID, build);
    if (std::memcmp(p + 20, build, 32) != 0) { error(5); return; }
#ifdef K1_PDM_TARGET
    if (k1_pdm_target_begin_stale_test(palette_time_us) != 0) { error(10); return; }
    live_runtime.setStaleMarkers(k1_pdm_target_injection_start_us(), 0);
    respond(0, 0, "STALE", 5);
#else
    error(10);
#endif
    return;
  }
  error(3);
}
#endif
#endif
static std::uint32_t led_now;
void status_command(std::uint32_t size) {
  if (size == 0) {
#ifdef K1_RESIDENT_SCHEDULE
    if (k1_fixture_schedule_active()) {
      if (k1_status_led_slim_snapshot(trace.data, sizeof(trace.data))) error(8);
      else respond(0, 0, trace.data, std::strlen(trace.data));
      return;
    }
#endif
    if (k1_status_led_snapshot(trace.data, sizeof(trace.data))) error(8);
    else respond(0, 0, trace.data, std::strlen(trace.data));
    return;
  }
  const std::uint8_t sub = rx[32];
  if (sub == 1 && size == 1) {
    const int rc = k1_status_led_identify(led_now);
    if (rc) error(10);
    else respond(0, 0, "IDENTIFY", 8);
  } else if (sub == 2 && size == 9) {
    const std::uint64_t run = get32(rx + 33) | (std::uint64_t(get32(rx + 37)) << 32);
    const int rc = k1_status_led_ack(run, led_now);
    if (rc) error(10);
    else respond(0, 0, "ACK", 3);
  } else if (sub == 3 && size == 1) {
    const int rc = k1_status_led_ack_recovery(led_now);
    if (rc) error(10);
    else respond(0, 0, "ACKR", 4);
  } else if (sub == 4 && size == 5) {
    const int rc = k1_status_led_wait(get32(rx + 33), led_now);
    if (rc) error(10);
    else respond(0, 0, "WAIT", 4);
  } else if (sub == 5 && size == 1) {
    k1_status_led_wait_clear(led_now);
    respond(0, 0, "WCLR", 4);
  } else if (sub == 6 && size == 14) {
    const std::uint64_t run = get32(rx + 33) | (std::uint64_t(get32(rx + 37)) << 32);
    const std::uint8_t outcome = rx[41];
    const std::uint32_t digest = get32(rx + 42);
    const int rc = k1_status_led_verdict(run, outcome, digest, led_now);
    if (rc) error(10);
    else respond(0, 0, "VERDICT", 7);
  } else if (sub == 7 && size == 9) {
    const std::uint64_t run = get32(rx + 33) | (std::uint64_t(get32(rx + 37)) << 32);
    k1_status_led_test_begin(run, led_now);
    respond(0, 0, "BIND", 4);
  } else if (sub == 8 && size == 1) {
#ifdef K1_RESIDENT_SCHEDULE
    if (k1_fixture_schedule_active()) { error(3); return; }
#endif
    const int rc = titan_led2_phy_start_experiment(led_now);
    if (rc) error(10);
    else respond(0, 0, "LED2SEQ", 7);
  } else if (sub == 9 && size == 1) {
#ifdef K1_RESIDENT_SCHEDULE
    if (k1_fixture_schedule_active()) { error(3); return; }
#endif
    /* Fixed binary snapshot: no formatting or transport work occurs in the
       timed MDIO transaction itself. L2T1 + count + 20 bytes per record. */
    const std::uint32_t count = titan_led2_phy_trace_snapshot(
        led2_trace_entries, K1_LED2_PHY_TRACE_CAPACITY);
    const std::size_t bytes = 8U + std::size_t(count) * 20U;
    if (bytes > sizeof(trace.data)) { error(8); return; }
    std::memcpy(trace.data, "L2T1", 4);
    put32(reinterpret_cast<std::uint8_t *>(trace.data) + 4, count);
    for (std::uint32_t i = 0; i < count; ++i) {
      const auto &entry = led2_trace_entries[i];
      auto *record = reinterpret_cast<std::uint8_t *>(trace.data) + 8U + i * 20U;
      put32(record, entry.sequence);
      put32(record + 4, entry.attempt);
      put32(record + 8, entry.reset_age_ms);
      record[12] = std::uint8_t(entry.value);
      record[13] = std::uint8_t(entry.value >> 8U);
      record[14] = entry.address;
      record[15] = entry.reg;
      record[16] = entry.ack;
      record[17] = entry.io_error;
      record[18] = entry.value_valid;
      record[19] = 0;
    }
    respond(0, 0, trace.data, bytes);
  } else error(3);
}
#ifdef K1_PALETTE_WS2816_GPT_PAIR
void handle_pair_fixture(std::uint32_t size) {
  k1_pair_fixture_gate_t gate{};
  k1_pair_fixture_view_t view{};
  gate.last_generation = k1_ws281x_gpt_dma_hw_pair_submitted_generation();
  if (fixture_hold_generation > gate.last_generation) gate.last_generation = fixture_hold_generation;
#ifdef K1_PDM_TARGET
  const auto epoch64 = k1_pdm_target_stream_epoch();
  gate.expected_epoch = epoch64 > 0xffffffffULL ? 0xffffffffU : static_cast<std::uint32_t>(epoch64);
#else
  gate.expected_epoch = 0U;
#endif
  gate.in_flight = (fixture_hold || k1_ws281x_gpt_dma_hw_pair_busy()) ? 1U : 0U;
  const int admit = k1_pair_fixture_admit(&gate, rx + 32, size, &view);
  if (admit != K1_PAIR_FIXTURE_OK) {
    const int n = std::snprintf(trace.data, sizeof(trace.data),
      "{\"accepted\":false,\"reason\":%d,\"generation\":0,\"kind\":0,\"submit\":-1}", admit);
    ++rejected;
    if (n > 0) respond(static_cast<std::uint32_t>(admit), 0, trace.data, std::size_t(n));
    else error(3);
    return;
  }
  std::memcpy(fixture_owned_a0, view.lane0, K1_PAIR_FIXTURE_LANE_BYTES);
  std::memcpy(fixture_owned_a1, view.lane1, K1_PAIR_FIXTURE_LANE_BYTES);
  const int submitted = k1_ws281x_gpt_dma_hw_pair_submit(
      fixture_owned_a0, K1_PAIR_FIXTURE_LANE_BYTES,
      fixture_owned_a1, K1_PAIR_FIXTURE_LANE_BYTES,
      K1_PAIR_FIXTURE_PROFILE, view.generation);
  if (submitted != K1_WS281X_SUBMIT_ACCEPTED) {
    const int n = std::snprintf(trace.data, sizeof(trace.data),
      "{\"accepted\":false,\"reason\":%d,\"generation\":%lu,\"kind\":%lu,\"submit\":%d}",
      submitted, (unsigned long)view.generation, (unsigned long)view.kind, submitted);
    ++rejected;
    if (n > 0) respond(static_cast<std::uint32_t>(submitted), 0, trace.data, std::size_t(n));
    else error(3);
    return;
  }
  fixture_hold = true;
  fixture_hold_generation = view.generation;
  const int n = std::snprintf(trace.data, sizeof(trace.data),
    "{\"accepted\":true,\"reason\":0,\"generation\":%lu,\"epoch\":%lu,\"kind\":%lu,\"submit\":0}",
    (unsigned long)view.generation, (unsigned long)view.epoch, (unsigned long)view.kind);
  if (n < 0 || std::size_t(n) >= sizeof(trace.data)) error(8);
  else respond(0, 0, trace.data, std::size_t(n));
}
#endif
void execute() {
#ifdef K1_PALETTE_RUNTIME
  // USB JSON can stall k1_fixture_poll. Render first so skipped_releases
  // records genuine lateness, not host-CDC blocking. Cadence unchanged.
  palette_time_us = palette_clock.sample(k1_cycle_count(), palette_clock_hz);
  palette_step(true);
#endif
  const auto command=get32(rx+4), size=get32(rx+16);
  if (crc(rx+32,size)!=get32(rx+20)) { error(4); return; }
#ifdef K1_PALETTE_WS2816_GPT_PAIR
  if (command == K1_PAIR_FIXTURE_OPCODE) {
    handle_pair_fixture(size);
    fill=0; wanted=32; return;
  }
#endif
#ifdef K1_PALETTE_GPT_DMA
  if (command == K1_WS281X_GPT_DIAG_OPCODE && size == 4 &&
      get32(rx + 32) == K1_WS281X_GPT_DIAG_V2_VERSION) {
    k1_ws281x_hw_diag_v2_t diag{};
    k1_ws281x_gpt_dma_hw_snapshot_v2(&diag);
    respond(0,0,reinterpret_cast<const char*>(&diag),sizeof(diag));
    fill=0; wanted=32; return;
  }
  if (command == K1_WS281X_GPT_DIAG_OPCODE && size == 0) {
    k1_ws281x_hw_diag_t diag{};
    k1_ws281x_gpt_dma_hw_snapshot(&diag);
    respond(0,0,reinterpret_cast<const char*>(&diag),sizeof(diag));
    fill=0; wanted=32; return;
  }
  if (command == K1_WS281X_GPT_DIAG_OPCODE) { error(3); return; }
  // A competing GPIO transaction would steal P601 from the timer.
  if (command==K1_WS281X_DIAG_OPCODE || command==K1_WS281X_FRAME_OPCODE || command==11) {
    error(10); return;
  }
#endif
  if (command == 20) { status_command(size); fill=0; wanted=32; return; }
#ifdef K1_PALETTE_RUNTIME
  if (command == k1::titan::kPaletteCatalogueOpcode && size == 0U) {
    const auto n = palettes.catalogueJson(trace.data, sizeof(trace.data));
    if (!n) error(8); else respond(0, 0, trace.data, n);
  } else if (command == k1::titan::kPaletteConfigureOpcode && (size == 32U || size == 36U || size == 40U)) {
#ifdef K1_RESIDENT_SCHEDULE
    if (k1_fixture_schedule_active()) { error(10); return; }
#endif
    const auto* p = rx + 32;
    if ((size == 32U && get32(p) != 1U) ||
        (size == 36U && get32(p) != 2U) ||
        (size == 40U && get32(p) != 3U)) { error(3); return; }
    const k1::titan::PaletteConfig config{
        get32(p), get32(p+4), get32(p+8), get32(p+12),
        get32(p+16), get32(p+20), get32(p+24), get32(p+28),
        size >= 36U ? get32(p+32) : 0U, size == 40U ? get32(p+36) : 4000U};
    if (!palettes.configure(config, palette_time_us)) { error(3); return; }
    palette_step(false);
    const auto n = palettes.statusJson(trace.data, sizeof(trace.data));
    if (!n) error(8); else respond(0, 0, trace.data, n);
  } else if (command == k1::titan::kPaletteStatusOpcode && size == 0U) {
    const auto n = palettes.statusJson(trace.data, sizeof(trace.data));
    if (!n) error(8); else respond(0, 0, trace.data, n);
#ifdef K1_LIVE_RUNTIME
  } else if (command == k1::titan::kLiveMirOpcode && size == 0U) {
    const auto n = live_runtime.mirJson(trace.data, sizeof(trace.data));
    if (!n) error(8); else respond(0, 0, trace.data, n);
  } else if (command == k1::titan::kLiveSnapshotOpcode && size == 0U) {
#ifdef K1_PDM_TARGET
    live_runtime.setCaptureMeta(k1_pdm_target_first_capture_start_us(),
                                k1_pdm_target_last_capture_end_us(),
                                k1_pdm_target_asrc_starved(),
                                k1_pdm_target_measured_hz(),
                                k1_pdm_target_last_hop_dt_us());
    live_runtime.setStaleMarkers(k1_pdm_target_injection_start_us(),
                                 k1_pdm_target_injection_end_us());
#endif
    const auto n = live_runtime.encodeSnapshot(
        reinterpret_cast<std::uint8_t*>(trace.data), sizeof(trace.data), live_runtime.nowUs());
    if (!n) error(8); else respond(0, 0, trace.data, n);
  } else if (command == k1::titan::kLiveEventsOpcode && (size == 0U || size == 8U)) {
    const std::uint64_t after = size == 8U
        ? (get32(rx+32) | (std::uint64_t(get32(rx+36)) << 32)) : 0U;
    const auto n = live_runtime.encodeEvents(
        reinterpret_cast<std::uint8_t*>(trace.data), sizeof(trace.data), after);
    if (!n) error(8); else respond(0, 0, trace.data, n);
  } else if (command == k1::titan::kLiveTimingOpcode && (size == 0U || size == 8U)) {
    const std::uint64_t after = size == 8U
        ? (get32(rx+32) | (std::uint64_t(get32(rx+36)) << 32)) : 0U;
    const auto n = live_runtime.encodeTiming(
        reinterpret_cast<std::uint8_t*>(trace.data), sizeof(trace.data), after);
    if (!n) error(8); else respond(0, 0, trace.data, n);
  } else if (command == k1::titan::kLiveConfigOpcode) {
    live_handle_config(size);
#endif
  } else if (command == k1::titan::kPaletteFrameOpcode && size == 4U) {
    const auto channel = get32(rx+32);
    if (channel > 1U) { error(3); return; }
    const auto pixels = palettes.channel(channel).frame();
    static_assert(sizeof(k1::core::Pixel8) == 3U);
    respond(0, 0, reinterpret_cast<const char*>(pixels.data()), pixels.size()*3U);
#ifdef K1_PALETTE_WS2816
  } else if (command == k1::titan::kPaletteWireSnapshotOpcode && size == 0U) {
#elif defined(K1_PALETTE_WS2816_GPT_PAIR)
  } else if (command == k1::titan::kPaletteWireSnapshotOpcode && size == 0U) {
#endif
#if defined(K1_PALETTE_WS2816) || defined(K1_PALETTE_WS2816_GPT_PAIR)
    if (!palette_wire_sequence) { error(8); return; }
    respond(0, 0, reinterpret_cast<const char*>(palette_wire_snapshot), sizeof(palette_wire_snapshot));
#endif
  } else
#endif
  if (command == 1 && size == 0) {
    char uid[33]; for(unsigned i=0;i<16;++i) std::snprintf(uid+i*2,3,"%02x",board_uid[i]);
#ifdef K1_NPU_LOAD
    const char* u55_opened=k1_npu_ready()?"true":"false";
#else
    const char* u55_opened="false";
#endif
    const int n=std::snprintf(trace.data,sizeof(trace.data),
      "{\"protocol\":1,\"uid\":\"%s\",\"build\":\"%s\",\"source\":\"%s\",\"contract\":\"sr24000.hop180.bins80.xover40\",\"clock_hz\":%lu,\"cpu1_actcsr\":%lu,\"u55_opened\":%s,\"cpp_initialised\":%s,\"rejected\":%lu,\"sequence\":%lu,\"trajectory_bytes\":%u,\"trace_bytes\":%u,\"status_led\":true"
#ifdef K1_LIVE_RUNTIME
      ",\"runtime_kind\":\"live\",\"schema_sha256\":\"%s\",\"opcodes\":{\"snapshot\":23,\"events\":24,\"config\":25,\"timing\":26,\"legacy_mir\":21},\"capabilities\":{\"snapshot\":true,\"events\":true,\"config\":true,\"timing\":true,\"legacy_mir_json\":true,\"stale_source_test\":true"
#ifdef K1_PALETTE_WS2816_GPT_PAIR
      ",\"pair_fixture\":true"
#endif
      "}"
#endif
      "}",
      uid,K1_BUILD_ID,K1_SOURCE_PIN,(unsigned long)clock_hz,(unsigned long)cpu_wait,u55_opened,initialised?"true":"false",
      (unsigned long)rejected,(unsigned long)trajectory.sequence,unsigned(sizeof(trajectory)),unsigned(sizeof(trace))
#ifdef K1_LIVE_RUNTIME
      , kTitanLiveSchemaSha256
#endif
      );
    if(n<0 || std::size_t(n)>=sizeof(trace.data)) error(8); else respond(0,0,trace.data,std::size_t(n));
  } else if(command==6 && size==0) {
    const std::size_t n=k1_platform_metrics(trace.data,sizeof(trace.data));
    if(!n) error(8); else respond(0,0,trace.data,n);
#ifdef K1_PCM1808_TARGET
  } else if(command==15 && size==0) {
    const std::size_t n=k1_pcm1808_target_metrics(trace.data,sizeof(trace.data));
    if(!n) error(8); else respond(0,0,trace.data,n);
#endif
  } else if(command==4 && size==0) {
    const auto before=k1_cycle_count(); const unsigned result=k1_time_probe();
    const auto cycles=k1_cycle_count()-before;
    const int n=std::snprintf(trace.data,sizeof(trace.data),"{\"time_probe_failure\":%u,\"million_beats\":1000000}",result);
    respond(result?10:0,cycles,trace.data,std::size_t(n));
  } else if(command==3 && size==8) {
    const std::uint64_t epoch=get32(rx+32)|(std::uint64_t(get32(rx+36))<<32);
    if (!epoch) { error(5); return; }
    trajectory.~Trajectory(); new (&trajectory) fixture::Trajectory(); trajectory.epoch=epoch;
    respond(0,0,"RESET",5);
  }
#ifdef K1_RESIDENT_SCHEDULE
  else if(command==7 && size==8) {
    const std::uint32_t loops=get32(rx+32), flags=get32(rx+36);
    const std::uint32_t mode=(flags>>4)&3U;
    std::uint32_t allowed_flags=0x73U;
#ifdef K1_ENABLE_STAGE_PROBE
    allowed_flags|=0x80U;
#endif
    if(schedule.active
#ifdef K1_P4_LOAD
       || k1_p4_active()
#endif
       || !loops || loops>40 || (flags&~allowed_flags)) { error(3); return; }
#ifdef K1_ENABLE_STAGE_PROBE
    if((flags&0x80U) && (loops!=1U || mode!=0U)) { error(3); return; }
#endif
#ifndef K1_NPU_LOAD
    if(mode || (flags&0x40U)) { error(3); return; }
#else
    if(mode && !k1_npu_ready()) { error(10); return; }
    if((flags&0x40U) && mode!=1U) { error(3); return; }
#endif
    std::memset(&schedule,0,sizeof(schedule)); schedule.active=true; schedule.loops=loops; schedule.flags=flags;
#ifdef K1_ENABLE_STAGE_PROBE
    schedule.raw_active=0xffffffffU;
#endif
    trajectory.~Trajectory(); new (&trajectory) fixture::Trajectory(); trajectory.epoch=1;
    k1_semantic_reset(&schedule.semantic);
    schedule.last_cycle=k1_cycle_count(); schedule.next_release=clock_hz/1000U;
    k1_status_led_test_begin(0,led_now);
    respond(0,0,"STARTED",7);
  } else if(command==8 && size==0) schedule_status();
#ifdef K1_ENABLE_STAGE_PROBE
  else if(command==12 && size==8) {
    const std::uint32_t first=get32(rx+32), count=get32(rx+36);
    if(schedule.active || !schedule.finished || schedule.loop!=1U || !count ||
       count>kRawTraceChunk || first>=schedule.raw_count ||
       count>schedule.raw_count-first) { error(3); return; }
    auto* output=reinterpret_cast<std::uint8_t*>(trace.data);
    std::memcpy(output,"K1T1",4);
    put32(output+4,kRawTraceVersion); put32(output+8,first);
    put32(output+12,count); put32(output+16,kRawTraceStride);
    put32(output+20,k1_stage_count);
    std::size_t offset=24;
    for(std::uint32_t index=first;index<first+count;++index) {
      const RawHop& raw=schedule.raw[index];
      put32(output+offset,index); offset+=4;
      put32(output+offset,raw.total_cycles); offset+=4;
      put32(output+offset,raw.lateness_cycles); offset+=4;
      put32(output+offset,raw.flags); offset+=4;
      put32(output+offset,raw.injected_delay_cycles); offset+=4;
      put32(output+offset,raw.double_calls); offset+=4;
      put32(output+offset,raw.double_cycles); offset+=4;
      for(unsigned stage=0;stage<k1_stage_count;++stage) {
        put32(output+offset,raw.stages[stage]); offset+=4;
      }
    }
    respond(0,0,trace.data,offset);
  }
#endif
#endif
#ifdef K1_P4_LOAD
  else if(command==9 && size==12) {
    if(k1_fixture_schedule_active() || !k1_p4_start(get32(rx+32),get32(rx+36),get32(rx+40))) error(3);
    else respond(0,0,"STARTED",7);
  } else if(command==10 && size==0) {
    const std::size_t n=k1_p4_status(trace.data,sizeof(trace.data));
    if(!n) error(8); else respond(0,0,trace.data,n);
  }
#endif
  else if(command==K1_WS281X_DIAG_OPCODE && size==32) {
    if(k1_fixture_schedule_active()) { error(3); return; }
#ifdef K1_PDM_TARGET
    if (k1_pdm_target_running()) { error(3); return; }
#endif
    const k1_ws281x_diag_request_t request = {
      get32(rx+32),get32(rx+36),get32(rx+40),get32(rx+44),
      get32(rx+48),get32(rx+52),get32(rx+56),get32(rx+60)};
    std::uint8_t wire[K1_WS281X_DIAG_MAX_BYTES];
    k1_ws281x_diag_timing_t timing{};
    const std::size_t bytes=k1_ws281x_diag_pack(&request,wire,sizeof(wire),&timing);
    if(!bytes) { error(3); return; }
    k1_ws281x_diag_result_t result{};
    const int emitted=k1_ws281x_diag_emit(wire,bytes,request.profile,request.pin,clock_hz,&result);
    const int n=std::snprintf(trace.data,sizeof(trace.data),
      "{\"op\":13,\"version\":1,\"profile\":%lu,\"pin\":%lu,\"pixels\":%lu,\"lit_pixels\":%lu,"
      "\"bytes\":%u,\"crc\":%lu,\"result\":%d,\"pfs_before\":%lu,\"pfs_after\":%lu,"
      "\"pin_config_error\":%ld,\"emit_cycles\":%lu,\"latch_cycles\":%lu,"
      "\"bit_period_min_cycles\":%lu,\"bit_period_max_cycles\":%lu,"
      "\"t0h_ns\":%lu,\"t1h_ns\":%lu,\"period_ns\":%lu,\"reset_us\":%lu,"
      "\"crc_proves\":\"packed_buffer_only\",\"wire_timing_measured\":false,\"photons\":\"NOT_CLAIMED\"}",
      (unsigned long)request.profile,(unsigned long)request.pin,(unsigned long)request.pixels,
      (unsigned long)request.lit_pixels,unsigned(bytes),(unsigned long)crc(wire,bytes),emitted,
      (unsigned long)result.pfs_before,(unsigned long)result.pfs_after,(long)result.pin_config_error,
      (unsigned long)result.emit_cycles,(unsigned long)result.latch_cycles,
      (unsigned long)result.bit_period_min_cycles,(unsigned long)result.bit_period_max_cycles,
      (unsigned long)timing.t0h_ns,(unsigned long)timing.t1h_ns,
      (unsigned long)timing.period_ns,(unsigned long)timing.reset_us);
    if(n<0 || std::size_t(n)>=sizeof(trace.data)) error(8);
    else respond(emitted?7:0,result.emit_cycles,trace.data,std::size_t(n));
  }
  else if(command==K1_WS281X_FRAME_OPCODE && size>=16) {
    if(k1_fixture_schedule_active()) { error(3); return; }
#ifdef K1_PDM_TARGET
    if (k1_pdm_target_running()) { error(3); return; }
#endif
    const std::uint32_t version=get32(rx+32);
    const std::uint32_t profile=get32(rx+36);
    const std::uint32_t pin=get32(rx+40);
    const std::uint32_t pixels=get32(rx+44);
    k1_ws281x_diag_timing_t timing{};
    if(version!=1U || pin>1U || !pixels || pixels>K1_WS281X_DIAG_MAX_PIXELS ||
       !k1_ws281x_diag_profile(profile,&timing) ||
       size!=16U+pixels*timing.bytes_per_pixel) { error(3); return; }
    const std::uint8_t* wire=rx+48;
    const std::size_t bytes=size-16U;
    k1_ws281x_diag_result_t result{};
    const int emitted=k1_ws281x_diag_emit(wire,bytes,profile,pin,clock_hz,&result);
    const int n=std::snprintf(trace.data,sizeof(trace.data),
      "{\"op\":14,\"version\":1,\"profile\":%lu,\"pin\":%lu,\"pixels\":%lu,"
      "\"bytes\":%u,\"crc\":%lu,\"result\":%d,\"pfs_after\":%lu,"
      "\"pin_config_error\":%ld,\"emit_cycles\":%lu,\"latch_cycles\":%lu,"
      "\"bit_period_min_cycles\":%lu,\"bit_period_max_cycles\":%lu,"
      "\"crc_proves\":\"submitted_frame_only\",\"wire_timing_measured\":false,"
      "\"photons\":\"NOT_CLAIMED\"}",
      (unsigned long)profile,(unsigned long)pin,(unsigned long)pixels,unsigned(bytes),
      (unsigned long)crc(wire,bytes),emitted,(unsigned long)result.pfs_after,
      (long)result.pin_config_error,(unsigned long)result.emit_cycles,
      (unsigned long)result.latch_cycles,(unsigned long)result.bit_period_min_cycles,
      (unsigned long)result.bit_period_max_cycles);
    if(n<0 || std::size_t(n)>=sizeof(trace.data)) error(8);
    else respond(emitted?7:0,result.emit_cycles,trace.data,std::size_t(n));
  }
  else if(command==11 && size==0) {
#ifdef K1_PDM_TARGET
    if (k1_pdm_target_running()) { error(3); return; }
#endif
    k1::core::visual::Pixel16 pixels[k1::core::visual::kPixelsPerChannel]{};
    k1::core::visual::Pixel16 off[k1::core::visual::kPixelsPerChannel]{};
    pixels[0] = {0x12AB, 0, 0};
    pixels[k1::core::visual::kPixelsPerHalf] = {0, 0, 0x12AB};
    for (std::size_t i = 1; i < 8; ++i) {
      pixels[i] = {0x7A3C, 0, 0};
      pixels[k1::core::visual::kPixelsPerHalf + i] = {0, 0, 0x7A3C};
    }
    std::uint8_t lane_a[k1::core::visual::kPackedBytesPerLane]{};
    std::uint8_t lane_b[k1::core::visual::kPackedBytesPerLane]{};
    std::uint8_t dark_a[k1::core::visual::kPackedBytesPerLane]{};
    std::uint8_t dark_b[k1::core::visual::kPackedBytesPerLane]{};
    if (!k1::core::visual::splitChannel160(
            pixels, k1::core::visual::kPixelsPerChannel, lane_a, lane_b) ||
        !k1::core::visual::splitChannel160(
            off, k1::core::visual::kPixelsPerChannel, dark_a, dark_b)) {
      error(7);
      fill=0; wanted=32; return;
    }
    packed_lane_completion_t done{};
    packed_submit_result_t submitted = kPackedAccepted;
    const std::uint32_t gap = clock_hz / 12U; // ~83 ms
    for (unsigned blink = 0; blink < 6; ++blink) {
      submitted = k1_ws2816_submit_packed_lanes(
          lane_a, sizeof(lane_a), lane_b, sizeof(lane_b), &done);
      if (submitted != kPackedAccepted) break;
      const std::uint32_t hold = k1_cycle_count();
      while ((k1_cycle_count() - hold) < gap) {
      }
      submitted = k1_ws2816_submit_packed_lanes(
          dark_a, sizeof(dark_a), dark_b, sizeof(dark_b), nullptr);
      if (submitted != kPackedAccepted) break;
      const std::uint32_t hold2 = k1_cycle_count();
      while ((k1_cycle_count() - hold2) < gap) {
      }
    }
    if (submitted != kPackedAccepted) {
      error(7);
      fill=0; wanted=32; return;
    }
    const unsigned long crc_a = crc(lane_a, sizeof(lane_a));
    const unsigned long crc_b = crc(lane_b, sizeof(lane_b));
    const int n = std::snprintf(
        trace.data, sizeof(trace.data),
        "{\"op\":11,\"din_a\":\"P601\",\"din_b\":\"P004\",\"pixels_a\":80,\"pixels_b\":80,"
        "\"packed_bytes_a\":480,\"packed_bytes_b\":480,\"crc_a\":%lu,\"crc_b\":%lu,"
        "\"submit_cycles\":%lu,\"transfer_done_cycles\":%lu,\"latch_ready_cycles\":%lu,"
        "\"emit_cycles\":%lu,\"latch_cycles\":%lu,\"bit_period_min_cycles\":%lu,"
        "\"bit_period_max_cycles\":%lu,\"clock_hz\":%lu,\"true16\":\"0x12AB\","
        "\"visible_u16\":\"0x7A3C\",\"visible_count\":8,\"blinks\":6,\"dither\":0,"
        "\"crc_proves\":\"emission_not_reception\","
        "\"level_shifter\":\"74HCT2G34GW\","
        "\"photon_disagreement_first_suspect\":\"bit_timing_or_data_break\"}",
        crc_a, crc_b, (unsigned long)done.submit_cycles,
        (unsigned long)done.transfer_done_cycles,
        (unsigned long)done.latch_ready_cycles, (unsigned long)done.emit_cycles,
        (unsigned long)done.latch_cycles,
        (unsigned long)done.bit_period_min_cycles,
        (unsigned long)done.bit_period_max_cycles, (unsigned long)clock_hz);
    if (n < 0 || std::size_t(n) >= sizeof(trace.data)) error(8);
    else respond(0, done.emit_cycles, trace.data, std::size_t(n));
  }
  else if((command==2 || command==5) && size==360) {
    if(get32(rx+12)!=trajectory.sequence+1) { error(6); return; }
    alignas(4) std::int16_t hop[180]; std::memcpy(hop,rx+32,sizeof(hop));
    const std::uint32_t before=k1_cycle_count(); trajectory.process(hop);
    const std::uint32_t elapsed=k1_cycle_count()-before; // modulo difference, individual work must be < one wrap.
    trace.format=command==5?fixture::Trace::Format::binary:fixture::Trace::Format::text;
    trajectory.trace(trace);
    if(!trace.valid || !trajectory.output.valid) error(7);
    else respond(0,elapsed,trace.data,trace.size);
    trace.format=fixture::Trace::Format::text;
  } else error(3);
  fill=0; wanted=32;
}
}
#ifdef K1_ENABLE_STAGE_PROBE
extern "C" std::uint32_t k1_stage_probe_begin(unsigned) noexcept {
  return k1_cycle_count();
}
extern "C" void k1_stage_probe_end(unsigned stage,std::uint32_t started) noexcept {
  if(schedule.active && stage<k1_stage_count) {
    const std::uint32_t cycles=k1_cycle_count()-started;
    schedule.stages[stage].add(cycles);
    if(schedule.loop==0U && schedule.raw_active<K1_RESIDENT_HOPS)
      schedule.raw[schedule.raw_active].stages[stage]=cycles;
  }
}
#endif
extern "C" void k1_fixture_initialise(const std::uint8_t uid[16],std::uint32_t hz,std::uint32_t wait) {
  std::memcpy(board_uid,uid,16); clock_hz=hz; cpu_wait=wait;
  k1_ws2816_set_clock(hz);
#ifdef K1_PALETTE_WS2816_GPT_PAIR
  (void)k1_ws281x_gpt_dma_hw_pair_init();
#elif defined(K1_PALETTE_GPT_DMA)
  (void)k1_ws281x_gpt_dma_hw_init();
#endif
#ifdef K1_PALETTE_RUNTIME
  palette_clock_hz = hz;
#ifdef K1_PALETTE_AUTOSTART
  k1::titan::PaletteConfig config;
  // Always boot audio-reactive WaveformK1 with physical emit on.
  // Colour palettes 33/43. No 4 s catalogue carousel. No PALETTE_BOUNCE.
  config.flags = 5U;
  config.mode_a = k1::titan::kLiveAudioBootMode;
  config.mode_b = k1::titan::kLiveAudioBootMode;
#ifdef K1_PALETTE_MORPH
  config.version = 3U; config.transition_ms = 1500U;
  config.palette_a = 33U; config.palette_b = 43U;
  config.brightness = 128U;
#endif
#ifdef K1_PALETTE_WS2816
  // Keep the selected showcase/morph defaults above across normal RESET.
  // Strip-specific gain is independent of palette and effect selection.
  config.palette_a = 33U; config.palette_b = 43U; config.brightness = 128U;
#endif
#ifdef K1_PALETTE_WS2816_GPT_PAIR
  config.palette_a = 33U; config.palette_b = 43U; config.brightness = 24U;
#ifndef K1_LIVE_EMIT
  config.flags = 1U; /* active, physical emit off until host/INFO */
#endif
#endif
  palettes.configure(config, 0U);
#endif
#endif
#ifdef K1_LIVE_RUNTIME
  live_runtime.initialise(hz);
  live_cfg.version = 1U;
  live_cfg.palette_version = 3U;
  live_cfg.mode_a = k1::titan::kLiveAudioBootMode;
  live_cfg.mode_b = k1::titan::kLiveAudioBootMode;
  live_cfg.flags = 5U;
  live_cfg.emit_on = 1;
  live_cfg.palette_a = 33U;
  live_cfg.palette_b = 43U;
  live_cfg.brightness = 128U;
#ifdef K1_PALETTE_WS2816_GPT_PAIR
  live_cfg.brightness = 24U;
#ifndef K1_LIVE_EMIT
  live_cfg.flags = 1U;
  live_cfg.emit_on = 0;
#endif
#endif
  k1::titan::liveInitFocusUnity(live_cfg.focus_a);
  k1::titan::liveInitFocusUnity(live_cfg.focus_b);
  std::uint8_t build_sha[32]{};
  live_hex32(K1_BUILD_ID, build_sha);
  live_runtime.setIdentity(uid, build_sha);
  live_runtime.setControlMeta(static_cast<std::uint16_t>(k1::titan::kLiveAudioBootMode), 33, 1, 0, 128);
#endif
  // Constructor witness: ChannelRenderState must have installed each channel ID.
  initialised=trajectory.b.channel()==k1::core::visual::PixelChannelId::kChannelB;
#ifdef K1_P4_LOAD
  k1_p4_initialise(hz);
#endif
  k1_status_led_init(0);
}
extern "C" void k1_fixture_consume(const std::uint8_t* bytes,std::size_t count,std::uint32_t now) {
  led_now=now;
  if(tx_size) return; // Single outstanding transaction; USB read arm enforces backpressure.
  for(std::size_t i=0;i<count;++i) {
    if(fill==0) started=now;
    rx[fill++]=bytes[i];
    if(fill==32) {
      if(std::memcmp(rx,"K1S1",4)!=0 || get32(rx+24)!=1 || crc(rx,28)!=get32(rx+28)) { error(1); return; }
      if(get32(rx+16)>kMaxRequestPayload) { error(2); return; }
      wanted=32+get32(rx+16);
    }
    if(fill==wanted) { execute(); return; }
  }
}
extern "C" void k1_fixture_poll(std::uint32_t now) {
  led_now=now;
  if(fill && now-started>2000) error(9);
#ifdef K1_PALETTE_RUNTIME
  palette_time_us = palette_clock.sample(k1_cycle_count(), palette_clock_hz);
  palette_step(true);
#ifdef K1_RESIDENT_SCHEDULE
  if (k1_fixture_schedule_active()) return;
#endif
#ifdef K1_PDM_TARGET
  {
#ifdef K1_LIVE_RUNTIME
    const auto hop_status = live_runtime.serviceOneHop();
    if (hop_status == k1::titan::LiveHopStatus::Discontinuity ||
        hop_status == k1::titan::LiveHopStatus::Fault) {
      palettes.resetAudioState();
    }
    live_runtime.setCaptureMeta(k1_pdm_target_first_capture_start_us(),
                                k1_pdm_target_last_capture_end_us(),
                                k1_pdm_target_asrc_starved(),
                                k1_pdm_target_measured_hz(),
                                k1_pdm_target_last_hop_dt_us());
    live_runtime.setStaleMarkers(k1_pdm_target_injection_start_us(),
                                 k1_pdm_target_injection_end_us());
    if (!live_runtime.publicationFresh(live_runtime.nowUs())) {
      live_runtime.noteStaleEntry(live_runtime.nowUs());
    } else {
      live_runtime.noteFreshPublication();
    }
    live_runtime.setControlMeta(static_cast<std::uint16_t>(palettes.config().mode_a),
                                static_cast<std::uint16_t>(palettes.config().palette_a),
                                palettes.emitEnabled() ? 1 : 0, live_cfg_revision,
                                static_cast<std::uint16_t>(palettes.config().brightness));
#else
    std::int16_t hop[180];
    if (k1_pdm_target_pull_ap_hop(hop) == 0) trajectory.process(hop);
#endif
  }
#if !defined(K1_PALETTE_GPT_DMA) && !defined(K1_PALETTE_WS2816_GPT_PAIR)
  if (k1_pdm_target_running()) return;
#endif
  if (k1_pdm_target_running() && k1_pdm_target_spare_slots() == 0u) {
#ifdef K1_PALETTE_WS2816_GPT_PAIR
    palette_pair_poll();
#endif
#ifdef K1_PALETTE_GPT_DMA
    palette_gpt_poll();
    if (palette_gpt_pending && k1_ws281x_gpt_dma_hw_ready() &&
        palette_last_wire_size != 0U) {
      const int status = k1_ws281x_gpt_dma_hw_submit(
          palette_last_wire, palette_last_wire_size, 1U);
      if (status == K1_WS281X_SUBMIT_ACCEPTED) palette_gpt_pending = false;
    }
#endif
  }
#endif
  palette_step(true);
#endif
}
extern "C" void k1_fixture_disconnect(void) { fill=0; wanted=32; tx_size=0; }
extern "C" const std::uint8_t* k1_fixture_reply(std::size_t* count) { *count=tx_size; return tx; }
extern "C" void k1_fixture_sent(void) { tx_size=0; }
#ifdef K1_RESIDENT_SCHEDULE
extern "C" bool k1_fixture_schedule_active(void) {
#ifdef K1_P4_LOAD
  return schedule.active || k1_p4_active();
#else
  return schedule.active;
#endif
}
extern "C" void k1_note_usb_event(std::uint32_t event) {
#ifdef K1_RESIDENT_SCHEDULE
  schedule.last_usb_event=event;
#else
  (void)event;
#endif
}
extern "C" std::uint32_t k1_fixture_release_remaining_cycles(void) {
#ifdef K1_RESIDENT_SCHEDULE
  if(!schedule.active || !clock_hz) return 0xffffffffU;
  const std::uint32_t current=k1_cycle_count();
  schedule.elapsed_cycles+=static_cast<std::uint32_t>(current-schedule.last_cycle);
  schedule.last_cycle=current;
  if(schedule.elapsed_cycles>=schedule.next_release) return 0;
  const std::uint64_t remaining=schedule.next_release-schedule.elapsed_cycles;
  return remaining>0xffffffffU?0xffffffffU:static_cast<std::uint32_t>(remaining);
#else
  return 0xffffffffU;
#endif
}
#ifdef K1_NPU_LOAD
void run_npu(unsigned count) {
  for(unsigned invocation=0;invocation<count;++invocation) {
    k1_npu_measurement_t measurement{};
    k1_npu_invoke(schedule.npu_invocations&1U,&measurement);
    ++schedule.npu_invocations;
    schedule.npu_wall.add(microseconds(measurement.wall_cycles));
    schedule.npu_cycles+=measurement.npu_cycles;
    schedule.npu_active_cycles+=measurement.npu_active;
    schedule.mac_active_cycles+=measurement.mac_active;
    if(measurement.invoke_status) ++schedule.npu_invoke_failures;
    if(!measurement.output_match) ++schedule.npu_output_failures;
  }
}
#endif
extern "C" void k1_fixture_schedule_step(void) {
#ifdef K1_P4_LOAD
  if(k1_p4_active()) { k1_p4_step(); return; }
#endif
  if(!schedule.active) return;
  const std::uint32_t current=k1_cycle_count();
  schedule.elapsed_cycles+=static_cast<std::uint32_t>(current-schedule.last_cycle);
  schedule.last_cycle=current;
  if(schedule.elapsed_cycles<schedule.next_release) return;
  const std::uint64_t lateness=schedule.elapsed_cycles-schedule.next_release;
  const std::uint32_t period=static_cast<std::uint32_t>((std::uint64_t(clock_hz)*3U)/400U); // 7.5 ms.
  const std::uint32_t index=schedule.hop;
  const std::uint32_t mode=(schedule.flags>>4)&3U;
#ifdef K1_NPU_LOAD
  if(mode==3U) {
    const std::uint64_t media_us=(std::uint64_t(schedule.loop)*K1_RESIDENT_HOPS+index+1U)*7500U;
    if(media_us/50000U>schedule.npu_invocations) run_npu(1);
  } else
#endif
  {
  const std::int16_t* pcm=k1_resident_pcm[k1_resident_index[index]];
#ifdef K1_ENABLE_STAGE_PROBE
  if(schedule.loop==0U) {
    std::memset(&schedule.raw[index],0,sizeof(schedule.raw[index]));
    schedule.raw_active=index;
    k1_double_probe_reset();
  } else schedule.raw_active=0xffffffffU;
#endif
  const std::uint32_t workload_started=k1_cycle_count();
#ifdef K1_ENABLE_STAGE_PROBE
  if((schedule.flags&0x80U) && schedule.loop==0U &&
     index==(K1_RESIDENT_HOPS>136U?136U:0U)) {
    const std::uint32_t delay_started=k1_cycle_count();
    const std::uint32_t delay_cycles=clock_hz/200U; // Declared 5 ms negative.
    while((k1_cycle_count()-delay_started)<delay_cycles) {
    }
    schedule.raw[index].injected_delay_cycles=k1_cycle_count()-delay_started;
  }
#endif
  trajectory.process(pcm);
#ifdef K1_NPU_LOAD
  if(mode==1U) {
    const std::uint64_t media_us=(std::uint64_t(schedule.loop)*K1_RESIDENT_HOPS+index+1U)*7500U;
    if(media_us/50000U>schedule.npu_invocations) run_npu(1);
  } else if(mode==2U) run_npu(3);
  if(schedule.flags&0x40U) k1_semantic_failure_step(&schedule.semantic,index);
#endif
  std::uint32_t workload_cycles=mode?k1_cycle_count()-workload_started:trajectory.total_cycles;
#ifdef K1_ENABLE_STAGE_PROBE
  if(mode==0U && schedule.loop==0U)
    workload_cycles+=schedule.raw[index].injected_delay_cycles;
#endif
  const std::uint64_t completion=lateness+workload_cycles;
  const std::uint32_t total_us=microseconds(workload_cycles);
  schedule.total.add(total_us);
  (trajectory.output.tempo.updated?schedule.tempo:schedule.ordinary).add(total_us);
  if(trajectory.rendered) schedule.render.add(microseconds(trajectory.render_cycles));
  schedule.lateness.add(microseconds64(lateness));
  const std::uint64_t backlog64=1+lateness/period;
  const std::uint32_t backlog=backlog64>0xffffffffU?0xffffffffU:static_cast<std::uint32_t>(backlog64);
  if(backlog>schedule.backlog_highwater) schedule.backlog_highwater=backlog;
  if(completion>period) {
    ++schedule.deadline_misses;
    k1_status_led_test_fail(k1_led_fail_deadline,"deadline",led_now);
  }
  if(trajectory.rendered && trajectory.render_cycles>clock_hz/500U) {
    ++schedule.render_misses;
    k1_status_led_test_fail(k1_led_fail_render,"render",led_now);
  }
  if(lateness>clock_hz/10000U) {
    ++schedule.release_guard_failures; // predeclared 100 us start guard.
    k1_status_led_test_fail(k1_led_fail_guard,"release_guard",led_now);
    if(schedule.edge_count<8U) {
      auto& edge=schedule.edge[schedule.edge_count++];
      edge.hop=index;
      edge.loop=schedule.loop;
      edge.lateness_cycles=lateness>0xffffffffU?0xffffffffU:static_cast<std::uint32_t>(lateness);
      edge.workload_cycles=workload_cycles;
      edge.completion_cycles=completion>0xffffffffU?0xffffffffU:static_cast<std::uint32_t>(completion);
      edge.prev_workload_cycles=schedule.prev_workload_cycles;
      edge.prev_telemetry_cycles=schedule.prev_telemetry_cycles;
      edge.last_usb_event=schedule.last_usb_event;
      edge.flags=(trajectory.output.tempo.updated?1U:0U)|(trajectory.rendered?2U:0U);
    }
  }
#ifdef K1_ENABLE_STAGE_PROBE
  if(schedule.loop==0U) {
    RawHop& raw=schedule.raw[index];
    raw.total_cycles=workload_cycles;
    raw.lateness_cycles=lateness>0xffffffffU?0xffffffffU:static_cast<std::uint32_t>(lateness);
    raw.flags=(trajectory.output.tempo.updated?1U:0U) |
              (trajectory.rendered?2U:0U) |
              (completion>period?4U:0U) |
              (lateness>clock_hz/10000U?8U:0U) |
              (raw.injected_delay_cycles?16U:0U);
    k1_double_probe_snapshot(&raw.double_calls, &raw.double_cycles);
  }
#endif
  if(schedule.flags&1U) {
    const std::uint32_t before=k1_cycle_count();
#ifdef K1_ENABLE_STAGE_PROBE
    K1_STAGE_BEGIN(telemetry, k1_stage_telemetry);
#endif
    trace.format=fixture::Trace::Format::binary; trajectory.trace(trace);
    const std::uint32_t expected_crc=k1_resident_crc[index] ^
      ((schedule.flags&2U) && schedule.loop==0 && schedule.hop==0 ? 1U : 0U);
    if(!trace.valid || trace.size!=k1_resident_length[index] ||
       crc(reinterpret_cast<const std::uint8_t*>(trace.data),trace.size)!=expected_crc) {
      ++schedule.correctness_failures;
      k1_status_led_test_fail(k1_led_fail_crc,"crc",led_now);
    }
    trace.format=fixture::Trace::Format::text;
#ifdef K1_ENABLE_STAGE_PROBE
    K1_STAGE_END(telemetry, k1_stage_telemetry);
#endif
    schedule.prev_telemetry_cycles=k1_cycle_count()-before;
    schedule.telemetry.add(microseconds(schedule.prev_telemetry_cycles));
  } else schedule.prev_telemetry_cycles=0;
  schedule.prev_workload_cycles=workload_cycles;
#ifdef K1_ENABLE_STAGE_PROBE
  if(schedule.loop==0U) schedule.raw_count=index+1U;
  schedule.raw_active=0xffffffffU;
#endif
  }
  schedule.next_release+=period;
  k1_status_led_observe_hop(schedule.loop*K1_RESIDENT_HOPS+index+1U,led_now);
  if(++schedule.hop==K1_RESIDENT_HOPS) {
    schedule.hop=0;
    if(++schedule.loop==schedule.loops) {
      schedule.active=false; schedule.finished=true;
      std::uint32_t flags=0;
      if(schedule.correctness_failures) flags|=k1_led_fail_crc;
      if(schedule.deadline_misses) flags|=k1_led_fail_deadline;
      if(schedule.release_guard_failures) flags|=k1_led_fail_guard;
      if(schedule.render_misses) flags|=k1_led_fail_render;
      k1_status_led_execution_end(schedule.loops*K1_RESIDENT_HOPS,flags,led_now);
      return;
    }
    trajectory.~Trajectory(); new (&trajectory) fixture::Trajectory(); trajectory.epoch=1;
  }
}
#else
extern "C" bool k1_fixture_schedule_active(void) { return false; }
extern "C" void k1_fixture_schedule_step(void) {}
extern "C" std::uint32_t k1_fixture_release_remaining_cycles(void) { return 0xffffffffU; }
extern "C" void k1_note_usb_event(std::uint32_t) {}
#endif
