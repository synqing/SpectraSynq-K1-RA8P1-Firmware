#include "k1_live_protocol.h"

#include <cmath>

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
  livePutU32(out + 812, f.last_rendered_request_id);
  livePutU32(out + 816, f.last_rendered_revision);
  livePutU32(out + 820, f.last_rendered_frame);
  livePutU32(out + 824, f.last_rendered_hop);
  livePutU32(out + 828, f.superseded_request_id);
  livePutU32(out + 832, f.accepted_request_id);
  livePutU32(out + 836, f.accepted_revision);
  livePutU32(out + 840, f.last_emitted_request_id);
  livePutU64(out + 844, f.last_rendered_publication_us);
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
  livePutU32(out + 52, cfg.last_patch_request_id);
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
    livePutU32(out + kLiveConfigBootIdOffset, cfg.boot_id);
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
  cfg->last_patch_request_id = liveGetU32(in + 52);
  for (unsigned i = 0; i < 105U; ++i) {
    cfg->focus_a[i] = liveGetF32(in + 56 + i * 4U);
    cfg->focus_b[i] = liveGetF32(in + 476 + i * 4U);
  }
  if (cfg->version == 1U && size == kLiveConfigBlobBytesV1) {
    return true;
  }
  if (cfg->version >= 2U &&
      (size == kLiveConfigBlobBytesV2 || size == kLiveConfigBlobBytesV2Legacy)) {
    liveDecodeVisual(in + kLiveConfigBlobBytesV1, &cfg->visual_a);
    liveDecodeVisual(in + kLiveConfigBlobBytesV1 + kLiveVisualControlsBytes,
                     &cfg->visual_b);
    if (size >= kLiveConfigBlobBytesV2) {
      cfg->boot_id = liveGetU32(in + kLiveConfigBootIdOffset);
    }
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

namespace {

LiveVisualControls* patchChannel(LiveConfigBlob* cfg, std::uint16_t channel) noexcept {
  if (cfg == nullptr) {
    return nullptr;
  }
  if (channel == 0U) {
    return &cfg->visual_a;
  }
  if (channel == 1U) {
    return &cfg->visual_b;
  }
  return nullptr;
}

float* patchFloatField(LiveVisualControls* vis, std::uint16_t field_id) noexcept {
  if (vis == nullptr) {
    return nullptr;
  }
  switch (field_id) {
    case kPatchVisFallbackBrightness:
      return &vis->vp_waveform_fallback_brightness;
    case kPatchVisSensitivity:
      return &vis->sensitivity;
    case kPatchVisMood:
      return &vis->mood;
    case kPatchVisChroma:
      return &vis->chroma;
    case kPatchVisSaturation:
      return &vis->saturation;
    case kPatchVisHuePosition:
      return &vis->hue_position;
    case kPatchVisShiftRate:
      return &vis->vp_waveform_shift_rate;
    case kPatchVisIdleFade:
      return &vis->vp_waveform_idle_fade;
    default:
      return nullptr;
  }
}

bool patchRangeOk(std::uint16_t field_id, float value) noexcept {
  if (!std::isfinite(value)) {
    return false;
  }
  if (field_id == kPatchVisShiftRate) {
    return value >= 0.0F && value <= 240.0F;
  }
  if (field_id == kPatchVisIdleFade) {
    return value >= 0.90F && value <= 0.999F;
  }
  return value >= 0.0F && value <= 1.0F;
}

}  // namespace

bool liveDecodePatch(const std::uint8_t* in, std::size_t size, std::uint32_t* expected_revision,
                     std::uint32_t* request_id, std::uint16_t* channel, std::uint32_t* boot_id,
                     LivePatchEntry* entries, std::uint16_t* count,
                     std::uint16_t max_count) noexcept {
  if (in == nullptr || expected_revision == nullptr || request_id == nullptr || channel == nullptr ||
      boot_id == nullptr || count == nullptr) {
    return false;
  }
  if (size < kLivePatchHeaderBytes) {
    return false;
  }
  const std::uint16_t n = liveGetU16(in + 8);
  const std::uint16_t ch = liveGetU16(in + 10);
  if (n == 0U || n > max_count || ch > 1U) {
    return false;
  }
  if (size != kLivePatchHeaderBytes + static_cast<std::size_t>(n) * 8U) {
    return false;
  }
  if (entries == nullptr) {
    return false;
  }
  *expected_revision = liveGetU32(in);
  *request_id = liveGetU32(in + 4);
  *channel = ch;
  *boot_id = liveGetU32(in + 12);
  for (std::uint16_t i = 0; i < n; ++i) {
    const std::uint8_t* row = in + kLivePatchHeaderBytes + static_cast<std::size_t>(i) * 8U;
    entries[i].field_id = liveGetU16(row);
    entries[i].kind = liveGetU16(row + 2);
    entries[i].value = liveGetF32(row + 4);
  }
  *count = n;
  return true;
}

std::size_t liveEncodePatch(std::uint8_t* out, std::size_t cap, std::uint32_t expected_revision,
                            std::uint32_t request_id, std::uint16_t channel, std::uint32_t boot_id,
                            const LivePatchEntry* entries, std::uint16_t count) noexcept {
  if (out == nullptr || entries == nullptr || count == 0U || count > kLivePatchMaxEntries ||
      channel > 1U) {
    return 0;
  }
  const std::size_t need = kLivePatchHeaderBytes + static_cast<std::size_t>(count) * 8U;
  if (cap < need) {
    return 0;
  }
  livePutU32(out, expected_revision);
  livePutU32(out + 4, request_id);
  livePutU16(out + 8, count);
  livePutU16(out + 10, channel);
  livePutU32(out + 12, boot_id);
  for (std::uint16_t i = 0; i < count; ++i) {
    std::uint8_t* row = out + kLivePatchHeaderBytes + static_cast<std::size_t>(i) * 8U;
    livePutU16(row, entries[i].field_id);
    livePutU16(row + 2, entries[i].kind);
    livePutF32(row + 4, entries[i].value);
  }
  return need;
}

LivePatchStatus liveApplyPatch(LiveConfigBlob* cfg, std::uint32_t expected_revision,
                               std::uint32_t expected_boot_id, std::uint16_t channel,
                               const LivePatchEntry* entries, std::uint16_t count) noexcept {
  if (cfg == nullptr || entries == nullptr || count == 0U || count > kLivePatchMaxEntries) {
    return LivePatchStatus::Invalid;
  }
  LiveVisualControls* vis = patchChannel(cfg, channel);
  if (vis == nullptr) {
    return LivePatchStatus::Forbidden;
  }
  if (cfg->boot_id == 0U || expected_boot_id == 0U || cfg->boot_id != expected_boot_id) {
    return LivePatchStatus::WrongBoot;
  }
  if (cfg->revision != expected_revision) {
    return LivePatchStatus::Stale;
  }
  std::uint16_t seen[kLivePatchMaxEntries]{};
  for (std::uint16_t i = 0; i < count; ++i) {
    if (entries[i].kind != kLivePatchKindF32) {
      return LivePatchStatus::Invalid;
    }
    if (patchFloatField(vis, entries[i].field_id) == nullptr) {
      return LivePatchStatus::Forbidden;
    }
    if (!patchRangeOk(entries[i].field_id, entries[i].value)) {
      return LivePatchStatus::Invalid;
    }
    for (std::uint16_t j = 0; j < i; ++j) {
      if (seen[j] == entries[i].field_id) {
        return LivePatchStatus::Invalid;
      }
    }
    seen[i] = entries[i].field_id;
  }
  LiveConfigBlob next = *cfg;
  LiveVisualControls* next_vis = patchChannel(&next, channel);
  const std::uint8_t emit = cfg->emit_on;
  const std::uint32_t flags = cfg->flags;
  const std::uint32_t mode_a = cfg->mode_a;
  const std::uint32_t mode_b = cfg->mode_b;
  const std::uint32_t palette_a = cfg->palette_a;
  const std::uint32_t palette_b = cfg->palette_b;
  for (std::uint16_t i = 0; i < count; ++i) {
    float* dest = patchFloatField(next_vis, entries[i].field_id);
    *dest = entries[i].value;
  }
  next.emit_on = emit;
  next.flags = flags;
  next.mode_a = mode_a;
  next.mode_b = mode_b;
  next.palette_a = palette_a;
  next.palette_b = palette_b;
  next.revision = cfg->revision + 1U;
  *cfg = next;
  return LivePatchStatus::Ok;
}

void liveNotePatchAccepted(LiveRenderReceipt* receipt, std::uint32_t request_id,
                           std::uint32_t revision) noexcept {
  if (receipt == nullptr || request_id == 0U) {
    return;
  }
  if (receipt->pending_request_id != 0U && receipt->pending_request_id != request_id) {
    receipt->superseded_request_id = receipt->pending_request_id;
  }
  receipt->accepted_request_id = request_id;
  receipt->accepted_revision = revision;
  receipt->pending_request_id = request_id;
  receipt->pending_revision = revision;
}

void liveNoteRendererConsumed(LiveRenderReceipt* receipt, std::uint32_t hop,
                              std::uint32_t frame, std::uint64_t publication_us) noexcept {
  if (receipt == nullptr || receipt->pending_request_id == 0U) {
    return;
  }
  receipt->rendered_request_id = receipt->pending_request_id;
  receipt->rendered_revision = receipt->pending_revision;
  receipt->rendered_hop = hop;
  receipt->rendered_frame = frame;
  receipt->rendered_publication_us = publication_us;
  receipt->pending_request_id = 0;
  receipt->pending_revision = 0;
}

void liveNotePhysicalEmit(LiveRenderReceipt* receipt) noexcept {
  if (receipt == nullptr || receipt->rendered_request_id == 0U) {
    return;
  }
  receipt->emitted_request_id = receipt->rendered_request_id;
}

void liveNotePhysicalEmitCompleted(LiveRenderReceipt* receipt,
                                   std::uint32_t completed_request_id) noexcept {
  if (receipt == nullptr || completed_request_id == 0U) {
    return;
  }
  receipt->emitted_request_id = completed_request_id;
}

bool liveNotePairCompleted(LiveRenderReceipt* receipt, std::uint32_t completed_generation,
                           std::uint32_t pending_generation,
                           std::uint32_t pending_request_id) noexcept {
  if (receipt == nullptr || pending_request_id == 0U ||
      completed_generation != pending_generation) {
    return false;
  }
  liveNotePhysicalEmitCompleted(receipt, pending_request_id);
  return true;
}

}  // namespace k1::titan
