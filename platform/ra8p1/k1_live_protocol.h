#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "k1_live_schema.inc"

namespace k1::titan {

inline constexpr std::uint32_t kLiveSnapshotOpcode = 23U;
inline constexpr std::uint32_t kLiveEventsOpcode = 24U;
inline constexpr std::uint32_t kLiveConfigOpcode = 25U;
inline constexpr std::uint32_t kLiveTimingOpcode = 26U;

inline constexpr std::uint32_t kLiveCfgGetSchema = 1U;
inline constexpr std::uint32_t kLiveCfgGetConfig = 2U;
inline constexpr std::uint32_t kLiveCfgSetConfig = 3U;
inline constexpr std::uint32_t kLiveCfgBeginSet = 4U;
inline constexpr std::uint32_t kLiveCfgAppendSet = 5U;
inline constexpr std::uint32_t kLiveCfgCommitSet = 6U;
inline constexpr std::uint32_t kLiveCfgAbortSet = 7U;
inline constexpr std::uint32_t kLiveCfgTestStale = 11U;

inline constexpr std::size_t kLiveSnapshotBytes = 1184U;
inline constexpr std::size_t kLiveEventBytes = 192U;
inline constexpr std::size_t kLiveTimingBytes = 56U;
inline constexpr std::size_t kLiveHistoryHeaderBytes = 64U;
inline constexpr std::size_t kLiveMaxResponseBody = 4096U;
inline constexpr std::uint32_t kLiveEventRing = 256U;
inline constexpr std::uint32_t kLiveTimingRing = 1024U;
inline constexpr std::uint32_t kLiveEventReadMax = 20U;
inline constexpr std::uint32_t kLiveTimingReadMax = 64U;
inline constexpr std::uint32_t kLiveStaleSourceUs = 120000U;
inline constexpr std::uint32_t kLiveConfigBlobBytesV1 = 896U;
inline constexpr std::uint32_t kLiveVisualControlsBytes = 128U;
inline constexpr std::uint32_t kLiveConfigBlobBytesV2 = 1152U;
inline constexpr std::uint32_t kLiveConfigBlobBytes = kLiveConfigBlobBytesV2;

inline constexpr std::uint32_t kVisEnabled = 1U;
inline constexpr std::uint32_t kVisMirror = 2U;
inline constexpr std::uint32_t kVisAutoColour = 4U;
inline constexpr std::uint32_t kVisReverse = 8U;
inline constexpr std::uint32_t kVisIncandescent = 16U;
inline constexpr std::uint32_t kVisDither = 32U;
inline constexpr std::uint32_t kVisBaseCoat = 64U;
inline constexpr std::uint32_t kVisPaletteMode = 128U;
inline constexpr std::uint32_t kVisChromatic = 256U;
inline constexpr std::uint32_t kVisFixAgc = 512U;
inline constexpr std::uint32_t kVisFixChromaGate = 1024U;
inline constexpr std::uint32_t kVisFixPrismOff = 2048U;
inline constexpr std::uint32_t kVisFixBloomDecay = 4096U;
inline constexpr std::uint32_t kVisFixHsv = 8192U;
inline constexpr std::uint32_t kVisFixSecondary = 16384U;
inline constexpr std::uint32_t kVisBloomForceSat = 32768U;
inline constexpr std::uint32_t kLiveStaleReason = 120U;

inline constexpr std::uint16_t kHistEmpty = 1U;
inline constexpr std::uint16_t kHistGap = 2U;
inline constexpr std::uint16_t kHistExpired = 4U;

inline constexpr std::uint16_t kSnapValid = 1U;
inline constexpr std::uint16_t kSnapWarming = 2U;
inline constexpr std::uint16_t kSnapStale = 4U;
inline constexpr std::uint16_t kSnapLiveOrigin = 8U;
inline constexpr std::uint16_t kSnapEmitOn = 16U;
inline constexpr std::uint16_t kSnapPhysicalTs = 32U;

inline void livePutU16(std::uint8_t* p, std::uint16_t v) noexcept {
  p[0] = static_cast<std::uint8_t>(v);
  p[1] = static_cast<std::uint8_t>(v >> 8U);
}
inline void livePutU32(std::uint8_t* p, std::uint32_t v) noexcept {
  p[0] = static_cast<std::uint8_t>(v);
  p[1] = static_cast<std::uint8_t>(v >> 8U);
  p[2] = static_cast<std::uint8_t>(v >> 16U);
  p[3] = static_cast<std::uint8_t>(v >> 24U);
}
inline void livePutU64(std::uint8_t* p, std::uint64_t v) noexcept {
  livePutU32(p, static_cast<std::uint32_t>(v));
  livePutU32(p + 4, static_cast<std::uint32_t>(v >> 32U));
}
inline void livePutF32(std::uint8_t* p, float v) noexcept {
  std::uint32_t bits = 0;
  std::memcpy(&bits, &v, 4);
  livePutU32(p, bits);
}
inline std::uint16_t liveGetU16(const std::uint8_t* p) noexcept {
  return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8U));
}
inline std::uint32_t liveGetU32(const std::uint8_t* p) noexcept {
  return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8U) |
         (static_cast<std::uint32_t>(p[2]) << 16U) | (static_cast<std::uint32_t>(p[3]) << 24U);
}
inline std::uint64_t liveGetU64(const std::uint8_t* p) noexcept {
  return static_cast<std::uint64_t>(liveGetU32(p)) |
         (static_cast<std::uint64_t>(liveGetU32(p + 4)) << 32U);
}
inline float liveGetF32(const std::uint8_t* p) noexcept {
  const std::uint32_t bits = liveGetU32(p);
  float v = 0.0F;
  std::memcpy(&v, &bits, 4);
  return v;
}

struct LiveEventRecord final {
  std::uint64_t write_seq = 0;
  std::uint64_t capture_time_us = 0;
  std::uint64_t publish_time_us = 0;
  std::uint64_t hop_sequence = 0;
  std::uint64_t stream_epoch = 0;
  std::uint32_t event_mask = 0;
  float onset_strength = 0.0F;
  float flux = 0.0F;
  float bass_onset = 0.0F;
  float kick = 0.0F;
  float snare = 0.0F;
  float hihat = 0.0F;
  float transient = 0.0F;
  float saliency = 0.0F;
  float peak_scaled = 0.0F;
  float beat_phase = 0.0F;
  float beat_confidence = 0.0F;
  float tempo_bpm = 0.0F;
  std::uint16_t mode_id = 0;
  std::uint16_t flags = 0;
  std::uint16_t spectrum_peak_bin = 0;
  std::uint16_t chroma_peak = 0;
};

struct LiveTimingRecord final {
  std::uint64_t write_seq = 0;
  std::uint64_t hop_sequence = 0;
  std::uint64_t capture_time_us = 0;
  std::uint64_t publish_time_us = 0;
  std::uint32_t ap_cycles = 0;
  std::uint32_t vp_cycles = 0;
  std::uint32_t hop_dt_us = 0;
  std::uint32_t asrc_starved = 0;
  std::uint32_t measured_hz = 0;
  std::uint16_t flags = 0;
  std::uint16_t reserved = 0;
};

struct LiveSnapshotFields final {
  std::uint16_t flags = 0;
  std::uint64_t publication_generation = 0;
  std::uint64_t stream_epoch = 0;
  std::uint64_t hop_sequence = 0;
  std::uint64_t capture_time_us = 0;
  std::uint64_t publish_time_us = 0;
  std::uint64_t analysis_end_exclusive = 0;
  std::uint64_t media_time_48k = 0;
  std::uint64_t musical_pulse = 0;
  std::uint32_t musical_phase_q32 = 0;
  float tempo_bpm = 0.0F;
  float beat_phase = 0.0F;
  float beat_confidence = 0.0F;
  std::uint64_t predicted_next_beat_us = 0;
  float peak_scaled = 0.0F;
  float vu_level = 0.0F;
  float bass_energy = 0.0F;
  float mid_energy = 0.0F;
  float high_energy = 0.0F;
  float onset_strength = 0.0F;
  float flux = 0.0F;
  float centroid_hz = 0.0F;
  float rolloff_hz = 0.0F;
  float bandwidth_hz = 0.0F;
  float rms = 0.0F;
  float crest = 0.0F;
  float zcr = 0.0F;
  float hfc = 0.0F;
  float compactness = 0.0F;
  float chroma[12]{};
  float spectrum[80]{};
  float bass_onset = 0.0F;
  float kick = 0.0F;
  float snare = 0.0F;
  float hihat = 0.0F;
  float transient = 0.0F;
  float saliency = 0.0F;
  float harmonic_ratio = 0.0F;
  float inharmonicity = 0.0F;
  float pitch_hz = 0.0F;
  float pitch_confidence = 0.0F;
  std::uint16_t mode_id = 0;
  std::uint16_t palette_id = 0;
  std::uint16_t gain_q8 = 256;
  std::uint16_t warmup_hops = 0;
  std::uint32_t config_revision = 0;
  std::uint32_t hops_consumed = 0;
  std::uint32_t hops_rejected = 0;
  std::uint32_t last_invalidate_reason = 0;
  std::uint32_t asrc_starved = 0;
  std::uint32_t measured_hz = 0;
  std::uint32_t deadline_us = 7500;
  std::uint32_t publication_deadline_misses = 0;
  std::uint32_t ap_cycles = 0;
  std::uint32_t vp_cycles = 0;
  std::uint32_t hop_dt_us = 0;
  std::uint32_t event_mask = 0;
  std::uint64_t event_write_seq = 0;
  std::uint64_t timing_write_seq = 0;
  std::uint32_t freshness_us = 0;
  std::uint8_t live_origin = 1;
  std::uint8_t physical_ts_valid = 0;
  std::uint8_t emit_on = 0;
  std::uint64_t capture_start_us = 0;
  std::uint64_t capture_end_us = 0;
  std::uint64_t injection_start_us = 0;
  std::uint64_t injection_end_us = 0;
  std::uint64_t stale_entry_us = 0;
  std::uint64_t epoch_transition_us = 0;
  std::uint64_t warmup_complete_us = 0;
  std::uint8_t uid[16]{};
  std::uint8_t build_sha256[32]{};
};

struct LiveVisualControls final {
  float chroma = 0.0F;
  float mood = 0.0F;
  float saturation = 1.0F;
  float square_iterations = 0.0F;
  float sensitivity = 1.0F;
  float incandescent_filter = 0.0F;
  float bulb_opacity = 0.0F;
  float base_coat_intensity = 0.0F;
  float prism_count = 0.0F;
  float hue_position = 0.0F;
  float chroma_value = 0.0F;
  float hue_shifting_mix = 0.0F;
  float vp_bloom_alpha = 0.99F;
  float vp_bloom_shift_scale = 1.0F;
  float vp_waveform_shift_rate = 120.0F;
  float vp_waveform_idle_fade = 0.985F;
  float vp_waveform_raw_margin = 1.10F;
  float vp_waveform_peak_floor = 0.08F;
  float vp_waveform_active_fade = 0.04F;
  float vp_waveform_chroma_blend_gain = 2.0F;
  float vp_waveform_fallback_brightness = 1.0F;
  float vp_waveform_vu_floor = 0.02F;
  std::uint32_t sweet_spot_min_level = 350U;
  std::uint16_t samples_per_chunk = 180U;
  std::uint32_t flags = kVisEnabled | kVisMirror | kVisChromatic | kVisBloomForceSat;
};

struct LiveConfigBlob final {
  std::uint32_t version = 1;
  std::uint32_t revision = 0;
  std::uint32_t palette_version = 1;
  std::uint32_t palette_a = 0;
  std::uint32_t palette_b = 1;
  std::uint32_t mode_a = 32;
  std::uint32_t mode_b = 0;
  std::uint32_t flags = 1;
  std::uint32_t brightness = 24;
  std::uint32_t output_channel = 0;
  std::uint32_t transition_ms = 0;
  std::uint32_t travel_ms = 4000;
  std::uint8_t emit_on = 0;
  float focus_a[105]{};
  float focus_b[105]{};
  LiveVisualControls visual_a{};
  LiveVisualControls visual_b{};
};

inline bool liveConfigSizeAccepted(std::size_t size) noexcept {
  return size == kLiveConfigBlobBytesV1 || size == kLiveConfigBlobBytesV2;
}

inline void liveInitFocusUnity(float* dest) noexcept {
  for (unsigned i = 0; i < 13U; ++i) dest[i] = 1.0F;
  for (unsigned i = 13U; i < 105U; ++i) dest[i] = 1.0F;
}

std::size_t liveEncodeSnapshot(std::uint8_t* out, std::size_t cap,
                               const LiveSnapshotFields& fields) noexcept;
std::size_t liveEncodeEvent(std::uint8_t* out, const LiveEventRecord& rec) noexcept;
std::size_t liveEncodeTiming(std::uint8_t* out, const LiveTimingRecord& rec) noexcept;
std::size_t liveEncodeHistoryHeader(std::uint8_t* out, std::uint32_t magic,
                                    std::uint16_t flags, std::uint64_t oldest,
                                    std::uint64_t newest, std::uint64_t after,
                                    std::uint16_t count, std::uint16_t truncated,
                                    std::uint16_t rec_bytes, std::uint16_t opcode,
                                    std::uint64_t generation,
                                    std::uint64_t epoch) noexcept;
std::size_t liveEncodeConfig(std::uint8_t* out, std::size_t cap,
                             const LiveConfigBlob& cfg) noexcept;
bool liveDecodeConfig(const std::uint8_t* in, std::size_t size,
                      LiveConfigBlob* cfg) noexcept;
std::size_t liveEncodeSchemaPage(std::uint8_t* out, std::size_t cap,
                                 std::uint32_t offset, std::uint32_t want) noexcept;

template <typename Rec, std::uint32_t N>
struct LiveHistoryRing final {
  Rec rec[N]{};
  std::uint64_t write_seq = 0;
  void push(Rec row) noexcept {
    write_seq += 1U;
    row.write_seq = write_seq;
    rec[write_seq % N] = row;
  }
};

}  // namespace k1::titan
