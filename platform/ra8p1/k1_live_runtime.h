#pragma once

#include <cstdint>

#include "core/audio/audio_pipeline.h"
#include "core/audio/gdft_goertzel.h"
#include "core/visual/visual_audio_frame.h"
#include "k1/core/audio/k1_audio_hop.h"
#include "k1_live_clock.h"
#include "k1_live_protocol.h"

namespace k1::titan {

inline constexpr std::uint32_t kLiveWarmupHops = 23U;
inline constexpr std::uint64_t kLiveFeatureStaleUs = 30000U;
inline constexpr std::uint32_t kLiveMirOpcode = 21U;

enum class LiveHopStatus : std::uint32_t {
  Ok = 0,
  NotReady = 1,
  Discontinuity = 2,
  Fault = 3,
  Rejected = 4
};

struct LiveMirSnapshot final {
  std::uint64_t publication_generation = 0;
  std::uint64_t stream_epoch = 0;
  std::uint64_t hop_sequence = 0;
  std::uint64_t publish_time_us = 0;
  std::uint64_t capture_time_us = 0;
  std::uint32_t warmup_hops = 0;
  float peak_scaled = 0.0F;
  float vu_level = 0.0F;
  float beat_phase = 0.0F;
  float beat_confidence = 0.0F;
  float tempo_bpm = 0.0F;
  std::uint8_t warming = 1;
  std::uint8_t valid = 0;
  std::uint8_t physical_timestamp_valid = 0;
  std::uint32_t hops_recorded = 0;
};

class LiveAudioRuntime final {
 public:
  LiveAudioRuntime() = default;
  LiveAudioRuntime(const LiveAudioRuntime&) = delete;
  LiveAudioRuntime& operator=(const LiveAudioRuntime&) = delete;

  void initialise(std::uint32_t clock_hz) noexcept;
  LiveHopStatus serviceOneHop() noexcept;
  LiveHopStatus consume(const k1_audio_hop_t& hop) noexcept;
  void invalidate(std::uint32_t reason) noexcept;
  const core::audio::AudioPipelineOutput* latest() const noexcept;
  const core::visual::VisualWaveformHistory& waveform() const noexcept {
    return waveform_;
  }
  LiveMirSnapshot snapshot() const noexcept;
  void fillSnapshotFields(LiveSnapshotFields* fields, std::uint64_t now_us) const noexcept;
  std::size_t encodeSnapshot(std::uint8_t* out, std::size_t cap, std::uint64_t now_us) const noexcept;
  std::size_t encodeEvents(std::uint8_t* out, std::size_t cap, std::uint64_t after) const noexcept;
  std::size_t encodeTiming(std::uint8_t* out, std::size_t cap, std::uint64_t after) const noexcept;
  std::size_t mirJson(char* out, std::size_t capacity) const noexcept;
  bool publicationFresh(std::uint64_t now_us) const noexcept;
  std::uint64_t publicationGeneration() const noexcept { return generation_; }
  std::uint32_t hopsConsumed() const noexcept { return hops_consumed_; }
  std::uint32_t hopsRejected() const noexcept { return hops_rejected_; }
  std::uint32_t warmupHops() const noexcept { return warmup_hops_; }
  bool warming() const noexcept { return warming_; }
  std::uint32_t lastInvalidateReason() const noexcept { return last_reason_; }
  void advanceHostCycles(std::uint32_t cycles) noexcept { host_cycles_ += cycles; }
  void setVpCycles(std::uint32_t cycles) noexcept { last_vp_cycles_ = cycles; }
  void setIdentity(const std::uint8_t uid[16], const std::uint8_t build_sha[32]) noexcept;
  void setControlMeta(std::uint16_t mode_id, std::uint16_t palette_id, std::uint8_t emit_on,
                      std::uint32_t revision, std::uint16_t gain_q8) noexcept;
  void setCaptureMeta(std::uint64_t start_us, std::uint64_t end_us, std::uint32_t starved,
                      std::uint32_t measured_hz, std::uint32_t hop_dt_us) noexcept;
  void setStaleMarkers(std::uint64_t injection_start, std::uint64_t injection_end) noexcept;
  void noteStaleEntry(std::uint64_t now_us) noexcept;
  void noteFreshPublication() noexcept;
  std::uint32_t publicationDeadlineMisses() const noexcept {
    return publication_deadline_misses_;
  }
  std::uint64_t nowUs() noexcept;
  std::uint64_t eventWriteSeq() const noexcept { return events_.write_seq; }
  std::uint64_t timingWriteSeq() const noexcept { return timing_.write_seq; }

 private:
  void resetAudioOwned() noexcept;
  void finalisePublication(core::audio::AudioPipelineOutput& output,
                           const k1_audio_hop_t& hop,
                           std::uint64_t ap_finish_us) noexcept;

  core::audio::AudioPipeline pipeline_{};
  core::audio::GdftSampleWindow window_{};
  core::visual::VisualWaveformHistory waveform_{};
  k1_audio_hop_t hop_{};
  k1_audio_hop_t last_hop_{};
  core::audio::AudioPipelineOutput private_{};
  core::audio::AudioPipelineOutput committed_{};
  k1_live_clock_t clock_{};
  std::uint64_t last_epoch_ = 0;
  std::uint64_t last_sequence_ = 0;
  std::uint64_t generation_ = 0;
  std::uint32_t hops_consumed_ = 0;
  std::uint32_t hops_rejected_ = 0;
  std::uint32_t hops_recorded_ = 0;
  std::uint32_t warmup_hops_ = 0;
  std::uint32_t last_reason_ = 0;
  std::uint32_t host_cycles_ = 0;
  std::uint32_t last_ap_cycles_ = 0;
  std::uint32_t last_vp_cycles_ = 0;
  std::uint32_t last_hop_dt_us_ = 0;
  std::uint32_t asrc_starved_ = 0;
  std::uint32_t measured_hz_ = 0;
  std::uint32_t config_revision_ = 0;
  std::uint16_t mode_id_ = 32;
  std::uint16_t palette_id_ = 0;
  std::uint16_t gain_q8_ = 256;
  std::uint8_t emit_on_ = 0;
  std::uint8_t uid_[16]{};
  std::uint8_t build_sha256_[32]{};
  std::uint64_t capture_start_us_ = 0;
  std::uint64_t capture_end_us_ = 0;
  std::uint64_t injection_start_us_ = 0;
  std::uint64_t injection_end_us_ = 0;
  std::uint64_t stale_entry_us_ = 0;
  std::uint64_t epoch_transition_us_ = 0;
  std::uint64_t warmup_complete_us_ = 0;
  std::uint64_t last_publish_us_ = 0;
  std::uint32_t publication_deadline_misses_ = 0;
  bool stale_latched_ = false;
  LiveHistoryRing<LiveEventRecord, kLiveEventRing> events_{};
  LiveHistoryRing<LiveTimingRecord, kLiveTimingRing> timing_{};
  bool warming_ = true;
  bool inflight_ = false;
  bool initialised_ = false;
};

}  // namespace k1::titan
