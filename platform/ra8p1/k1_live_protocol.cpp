#include "k1_live_protocol.h"

namespace k1::titan {
namespace {

constexpr std::uint32_t kSnapMagic = 0x53564C54U; /* TLVS */
constexpr std::uint32_t kHistMagicEvents = 0x48454C54U; /* TLEH */
constexpr std::uint32_t kHistMagicTiming = 0x48544C54U; /* TLTH */

}  // namespace

std::size_t liveEncodeSnapshot(std::uint8_t* out, std::size_t cap,
                               const LiveSnapshotFields& f) noexcept {
  if (out == nullptr || cap < kLiveSnapshotBytes) {
    return 0;
  }
  std::memset(out, 0, kLiveSnapshotBytes);
  livePutU32(out + 0, kSnapMagic);
  livePutU16(out + 4, 1U);
  livePutU16(out + 6, f.flags);
  livePutU64(out + 8, f.publication_generation);
  livePutU64(out + 16, f.stream_epoch);
  livePutU64(out + 24, f.hop_sequence);
  livePutU64(out + 32, f.capture_time_us);
  livePutU64(out + 40, f.publish_time_us);
  livePutU64(out + 48, f.analysis_end_exclusive);
  livePutU64(out + 56, f.media_time_48k);
  livePutU64(out + 64, f.musical_pulse);
  livePutU32(out + 72, f.musical_phase_q32);
  livePutF32(out + 76, f.tempo_bpm);
  livePutF32(out + 80, f.beat_phase);
  livePutF32(out + 84, f.beat_confidence);
  livePutU64(out + 88, f.predicted_next_beat_us);
  livePutF32(out + 96, f.peak_scaled);
  livePutF32(out + 100, f.vu_level);
  livePutF32(out + 104, f.bass_energy);
  livePutF32(out + 108, f.mid_energy);
  livePutF32(out + 112, f.high_energy);
  livePutF32(out + 116, f.onset_strength);
  livePutF32(out + 120, f.flux);
  livePutF32(out + 124, f.centroid_hz);
  livePutF32(out + 128, f.rolloff_hz);
  livePutF32(out + 132, f.bandwidth_hz);
  livePutF32(out + 136, f.rms);
  livePutF32(out + 140, f.crest);
  livePutF32(out + 144, f.zcr);
  livePutF32(out + 148, f.hfc);
  livePutF32(out + 152, f.compactness);
  for (unsigned i = 0; i < 12U; ++i) {
    livePutF32(out + 156 + i * 4U, f.chroma[i]);
  }
  for (unsigned i = 0; i < 80U; ++i) {
    livePutF32(out + 204 + i * 4U, f.spectrum[i]);
  }
  livePutF32(out + 524, f.bass_onset);
  livePutF32(out + 528, f.kick);
  livePutF32(out + 532, f.snare);
  livePutF32(out + 536, f.hihat);
  livePutF32(out + 540, f.transient);
  livePutF32(out + 544, f.saliency);
  livePutF32(out + 548, f.harmonic_ratio);
  livePutF32(out + 552, f.inharmonicity);
  livePutF32(out + 556, f.pitch_hz);
  livePutF32(out + 560, f.pitch_confidence);
  livePutU16(out + 596, f.mode_id);
  livePutU16(out + 598, f.palette_id);
  livePutU16(out + 600, f.gain_q8);
  livePutU16(out + 602, f.warmup_hops);
  livePutU32(out + 604, f.config_revision);
  livePutU32(out + 608, f.hops_consumed);
  livePutU32(out + 612, f.hops_rejected);
  livePutU32(out + 616, f.last_invalidate_reason);
  livePutU32(out + 620, f.asrc_starved);
  livePutU32(out + 624, f.measured_hz);
  livePutU32(out + 628, f.deadline_us);
  livePutU32(out + 632, f.ap_cycles);
  livePutU32(out + 636, f.vp_cycles);
  livePutU32(out + 640, f.hop_dt_us);
  livePutU32(out + 644, f.event_mask);
  livePutU64(out + 648, f.event_write_seq);
  livePutU64(out + 656, f.timing_write_seq);
  livePutU32(out + 664, f.freshness_us);
  out[668] = f.live_origin;
  out[669] = f.physical_ts_valid;
  out[670] = f.emit_on;
  out[671] = 0;
  livePutU64(out + 672, f.capture_start_us);
  livePutU64(out + 680, f.capture_end_us);
  livePutU64(out + 688, f.injection_start_us);
  livePutU64(out + 696, f.injection_end_us);
  livePutU64(out + 704, f.stale_entry_us);
  livePutU64(out + 712, f.epoch_transition_us);
  livePutU64(out + 720, f.warmup_complete_us);
  std::memcpy(out + 728, f.uid, 16);
  std::memcpy(out + 744, f.build_sha256, 32);
  std::memcpy(out + 776, kTitanLiveSchemaJson, 0); /* schema hash below */
  for (unsigned i = 0; i < 32U; ++i) {
    unsigned hi = 0;
    unsigned lo = 0;
    const char c0 = kTitanLiveSchemaSha256[i * 2U];
    const char c1 = kTitanLiveSchemaSha256[i * 2U + 1U];
    hi = static_cast<unsigned>(c0 <= '9' ? c0 - '0' : 10 + (c0 - 'a'));
    lo = static_cast<unsigned>(c1 <= '9' ? c1 - '0' : 10 + (c1 - 'a'));
    out[776 + i] = static_cast<std::uint8_t>((hi << 4U) | lo);
  }
  livePutU32(out + 808, f.publication_deadline_misses);
  return kLiveSnapshotBytes;
}

std::size_t liveEncodeEvent(std::uint8_t* out, const LiveEventRecord& rec) noexcept {
  std::memset(out, 0, kLiveEventBytes);
  livePutU64(out + 0, rec.write_seq);
  livePutU64(out + 8, rec.capture_time_us);
  livePutU64(out + 16, rec.publish_time_us);
  livePutU64(out + 24, rec.hop_sequence);
  livePutU64(out + 32, rec.stream_epoch);
  livePutU32(out + 40, rec.event_mask);
  livePutF32(out + 44, rec.onset_strength);
  livePutF32(out + 48, rec.flux);
  livePutF32(out + 52, rec.bass_onset);
  livePutF32(out + 56, rec.kick);
  livePutF32(out + 60, rec.snare);
  livePutF32(out + 64, rec.hihat);
  livePutF32(out + 68, rec.transient);
  livePutF32(out + 72, rec.saliency);
  livePutF32(out + 76, rec.peak_scaled);
  livePutF32(out + 80, rec.beat_phase);
  livePutF32(out + 84, rec.beat_confidence);
  livePutF32(out + 88, rec.tempo_bpm);
  livePutU16(out + 92, rec.mode_id);
  livePutU16(out + 94, rec.flags);
  livePutU16(out + 96, rec.spectrum_peak_bin);
  livePutU16(out + 98, rec.chroma_peak);
  return kLiveEventBytes;
}

std::size_t liveEncodeTiming(std::uint8_t* out, const LiveTimingRecord& rec) noexcept {
  std::memset(out, 0, kLiveTimingBytes);
  livePutU64(out + 0, rec.write_seq);
  livePutU64(out + 8, rec.hop_sequence);
  livePutU64(out + 16, rec.capture_time_us);
  livePutU64(out + 24, rec.publish_time_us);
  livePutU32(out + 32, rec.ap_cycles);
  livePutU32(out + 36, rec.vp_cycles);
  livePutU32(out + 40, rec.hop_dt_us);
  livePutU32(out + 44, rec.asrc_starved);
  livePutU32(out + 48, rec.measured_hz);
  livePutU16(out + 52, rec.flags);
  livePutU16(out + 54, rec.reserved);
  return kLiveTimingBytes;
}

std::size_t liveEncodeHistoryHeader(std::uint8_t* out, std::uint32_t magic,
                                    std::uint16_t flags, std::uint64_t oldest,
                                    std::uint64_t newest, std::uint64_t after,
                                    std::uint16_t count, std::uint16_t truncated,
                                    std::uint16_t rec_bytes, std::uint16_t opcode,
                                    std::uint64_t generation,
                                    std::uint64_t epoch) noexcept {
  std::memset(out, 0, kLiveHistoryHeaderBytes);
  livePutU32(out + 0, magic);
  livePutU16(out + 4, 1U);
  livePutU16(out + 6, flags);
  livePutU64(out + 8, oldest);
  livePutU64(out + 16, newest);
  livePutU64(out + 24, after);
  livePutU16(out + 32, count);
  livePutU16(out + 34, truncated);
  livePutU16(out + 36, rec_bytes);
  livePutU16(out + 38, opcode);
  livePutU64(out + 40, generation);
  livePutU64(out + 48, epoch);
  (void)kHistMagicEvents;
  (void)kHistMagicTiming;
  return kLiveHistoryHeaderBytes;
}

static void liveEncodeVisual(std::uint8_t* out, const LiveVisualControls& v) noexcept {
  livePutF32(out + 0, v.chroma);
  livePutF32(out + 4, v.mood);
  livePutF32(out + 8, v.saturation);
  livePutF32(out + 12, v.square_iterations);
  livePutF32(out + 16, v.sensitivity);
  livePutF32(out + 20, v.incandescent_filter);
  livePutF32(out + 24, v.bulb_opacity);
  livePutF32(out + 28, v.base_coat_intensity);
  livePutF32(out + 32, v.prism_count);
  livePutF32(out + 36, v.hue_position);
  livePutF32(out + 40, v.chroma_value);
  livePutF32(out + 44, v.hue_shifting_mix);
  livePutF32(out + 48, v.vp_bloom_alpha);
  livePutF32(out + 52, v.vp_bloom_shift_scale);
  livePutF32(out + 56, v.vp_waveform_shift_rate);
  livePutF32(out + 60, v.vp_waveform_idle_fade);
  livePutF32(out + 64, v.vp_waveform_raw_margin);
  livePutF32(out + 68, v.vp_waveform_peak_floor);
  livePutF32(out + 72, v.vp_waveform_active_fade);
  livePutF32(out + 76, v.vp_waveform_chroma_blend_gain);
  livePutF32(out + 80, v.vp_waveform_fallback_brightness);
  livePutF32(out + 84, v.vp_waveform_vu_floor);
  livePutU32(out + 88, v.sweet_spot_min_level);
  livePutU16(out + 92, v.samples_per_chunk);
  livePutU16(out + 94, 0);
  livePutU32(out + 96, v.flags);
}

static void liveDecodeVisual(const std::uint8_t* in, LiveVisualControls* v) noexcept {
  v->chroma = liveGetF32(in + 0);
  v->mood = liveGetF32(in + 4);
  v->saturation = liveGetF32(in + 8);
  v->square_iterations = liveGetF32(in + 12);
  v->sensitivity = liveGetF32(in + 16);
  v->incandescent_filter = liveGetF32(in + 20);
  v->bulb_opacity = liveGetF32(in + 24);
  v->base_coat_intensity = liveGetF32(in + 28);
  v->prism_count = liveGetF32(in + 32);
  v->hue_position = liveGetF32(in + 36);
  v->chroma_value = liveGetF32(in + 40);
  v->hue_shifting_mix = liveGetF32(in + 44);
  v->vp_bloom_alpha = liveGetF32(in + 48);
  v->vp_bloom_shift_scale = liveGetF32(in + 52);
  v->vp_waveform_shift_rate = liveGetF32(in + 56);
  v->vp_waveform_idle_fade = liveGetF32(in + 60);
  v->vp_waveform_raw_margin = liveGetF32(in + 64);
  v->vp_waveform_peak_floor = liveGetF32(in + 68);
  v->vp_waveform_active_fade = liveGetF32(in + 72);
  v->vp_waveform_chroma_blend_gain = liveGetF32(in + 76);
  v->vp_waveform_fallback_brightness = liveGetF32(in + 80);
  v->vp_waveform_vu_floor = liveGetF32(in + 84);
  v->sweet_spot_min_level = liveGetU32(in + 88);
  v->samples_per_chunk = liveGetU16(in + 92);
  v->flags = liveGetU32(in + 96);
}

std::size_t liveEncodeConfig(std::uint8_t* out, std::size_t cap,
                             const LiveConfigBlob& cfg) noexcept {
  const bool v2 = cfg.version >= 2U;
  const std::size_t need = v2 ? kLiveConfigBlobBytesV2 : kLiveConfigBlobBytesV1;
  if (out == nullptr || cap < need) {
    return 0;
  }
  std::memset(out, 0, need);
  livePutU32(out + 0, cfg.version);
  livePutU32(out + 4, cfg.revision);
  livePutU32(out + 8, cfg.palette_version);
  livePutU32(out + 12, cfg.palette_a);
  livePutU32(out + 16, cfg.palette_b);
  livePutU32(out + 20, cfg.mode_a);
  livePutU32(out + 24, cfg.mode_b);
  livePutU32(out + 28, cfg.flags);
  livePutU32(out + 32, cfg.brightness);
  livePutU32(out + 36, cfg.output_channel);
  livePutU32(out + 40, cfg.transition_ms);
  livePutU32(out + 44, cfg.travel_ms);
  out[48] = cfg.emit_on;
  for (unsigned i = 0; i < 105U; ++i) {
    livePutF32(out + 56 + i * 4U, cfg.focus_a[i]);
  }
  for (unsigned i = 0; i < 105U; ++i) {
    livePutF32(out + 476 + i * 4U, cfg.focus_b[i]);
  }
  if (v2) {
    liveEncodeVisual(out + kLiveConfigBlobBytesV1, cfg.visual_a);
    liveEncodeVisual(out + kLiveConfigBlobBytesV1 + kLiveVisualControlsBytes,
                     cfg.visual_b);
  }
  return need;
}

bool liveDecodeConfig(const std::uint8_t* in, std::size_t size,
                      LiveConfigBlob* cfg) noexcept {
  if (in == nullptr || cfg == nullptr || !liveConfigSizeAccepted(size)) {
    return false;
  }
  *cfg = LiveConfigBlob{};
  cfg->version = liveGetU32(in + 0);
  cfg->revision = liveGetU32(in + 4);
  cfg->palette_version = liveGetU32(in + 8);
  cfg->palette_a = liveGetU32(in + 12);
  cfg->palette_b = liveGetU32(in + 16);
  cfg->mode_a = liveGetU32(in + 20);
  cfg->mode_b = liveGetU32(in + 24);
  cfg->flags = liveGetU32(in + 28);
  cfg->brightness = liveGetU32(in + 32);
  cfg->output_channel = liveGetU32(in + 36);
  cfg->transition_ms = liveGetU32(in + 40);
  cfg->travel_ms = liveGetU32(in + 44);
  cfg->emit_on = in[48];
  for (unsigned i = 0; i < 105U; ++i) {
    cfg->focus_a[i] = liveGetF32(in + 56 + i * 4U);
    cfg->focus_b[i] = liveGetF32(in + 476 + i * 4U);
  }
  if (cfg->version == 1U && size == kLiveConfigBlobBytesV1) {
    return true;
  }
  if (cfg->version >= 2U && size == kLiveConfigBlobBytesV2) {
    liveDecodeVisual(in + kLiveConfigBlobBytesV1, &cfg->visual_a);
    liveDecodeVisual(in + kLiveConfigBlobBytesV1 + kLiveVisualControlsBytes,
                     &cfg->visual_b);
    return true;
  }
  return false;
}

std::size_t liveEncodeSchemaPage(std::uint8_t* out, std::size_t cap,
                                 std::uint32_t offset, std::uint32_t want) noexcept {
  if (out == nullptr || cap < 12U) {
    return 0;
  }
  if (offset > kTitanLiveSchemaLen) {
    return 0;
  }
  const std::uint32_t remain = kTitanLiveSchemaLen - offset;
  std::uint32_t take = want;
  if (take > remain) {
    take = remain;
  }
  if (cap < 12U + take) {
    take = static_cast<std::uint32_t>(cap - 12U);
  }
  livePutU32(out + 0, offset);
  livePutU32(out + 4, kTitanLiveSchemaLen);
  livePutU32(out + 8, take);
  if (take) {
    std::memcpy(out + 12, kTitanLiveSchemaJson + offset, take);
  }
  return 12U + take;
}

}  // namespace k1::titan
