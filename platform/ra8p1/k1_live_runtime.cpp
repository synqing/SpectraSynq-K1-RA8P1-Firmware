#include "k1_live_runtime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "k1_live_protocol.h"

#ifdef K1_PDM_TARGET
#include "pdm_target.h"
#endif
#ifdef K1_RA8P1_TARGET
#include "fixture_app.h"
#endif

namespace k1::titan {
namespace {

using k1::core::audio::AudioPipelineInput;
using k1::core::audio::AudioTime;
using k1::core::audio::mediaFramesToMicros;
using k1::core::visual::pushVisualWaveform;

}  // namespace

void LiveAudioRuntime::initialise(std::uint32_t clock_hz) noexcept {
  resetAudioOwned();
  hop_ = {};
  last_hop_ = {};
  last_epoch_ = 0;
  last_sequence_ = 0;
  generation_ = 0;
  hops_consumed_ = 0;
  hops_rejected_ = 0;
  hops_recorded_ = 0;
  warmup_hops_ = 0;
  publication_deadline_misses_ = 0;
  stale_latched_ = false;
  last_reason_ = 0;
  host_cycles_ = 0;
  warming_ = true;
  inflight_ = false;
  k1_live_clock_init(&clock_, clock_hz == 0U ? 1000000000U : clock_hz, 1U);
  (void)nowUs();
  initialised_ = true;
}

void LiveAudioRuntime::resetAudioOwned() noexcept {
  pipeline_.reset();
  window_ = {};
  waveform_ = {};
  private_ = {};
  committed_ = {};
  warmup_hops_ = 0;
  warming_ = true;
}

std::uint64_t LiveAudioRuntime::nowUs() noexcept {
  k1_live_clock_sample_t sample{};
#ifdef K1_RA8P1_TARGET
  const std::uint32_t cycles = k1_cycle_count();
#else
  const std::uint32_t cycles = host_cycles_;
#endif
  if (k1_live_clock_sample(&clock_, cycles, &sample) != 0) {
    return sample.microseconds;
  }
  return sample.microseconds;
}

LiveHopStatus LiveAudioRuntime::serviceOneHop() noexcept {
  if (!initialised_) {
    return LiveHopStatus::Fault;
  }
#ifdef K1_PDM_TARGET
  const k1_audio_read_result_t result = k1_pdm_target_try_read_ap_hop(&hop_);
  if (result == K1_AUDIO_READ_NOT_READY) {
    return LiveHopStatus::NotReady;
  }
  if (result == K1_AUDIO_READ_DISCONTINUITY) {
    invalidate(hop_.discontinuity_reason == 0U ? 1U : hop_.discontinuity_reason);
    return LiveHopStatus::Discontinuity;
  }
  if (result != K1_AUDIO_READ_OK) {
    invalidate(2U);
    return LiveHopStatus::Fault;
  }
  inflight_ = true;
  const LiveHopStatus status = consume(hop_);
  inflight_ = false;
  return status;
#else
  (void)hop_;
  return LiveHopStatus::NotReady;
#endif
}

LiveHopStatus LiveAudioRuntime::consume(const k1_audio_hop_t& hop) noexcept {
  if (!initialised_) {
    return LiveHopStatus::Fault;
  }
  const int accepted = k1_audio_hop_accept(&last_epoch_, &last_sequence_, &hop);
  if (accepted == -2 || accepted == -3) {
    hops_rejected_ += 1U;
    return LiveHopStatus::Rejected;
  }
  if (accepted != 0) {
    hops_rejected_ += 1U;
    return LiveHopStatus::Fault;
  }
  if (last_sequence_ == 1U) {
    resetAudioOwned();
  }
  std::memmove(window_.data(), window_.data() + K1_AUDIO_HOP_SAMPLES,
               (window_.size() - K1_AUDIO_HOP_SAMPLES) * sizeof(std::int16_t));
  std::memcpy(window_.data() + window_.size() - K1_AUDIO_HOP_SAMPLES, hop.pcm,
              K1_AUDIO_HOP_SAMPLES * sizeof(std::int16_t));
  float peak = 0.0F;
  float energy = 0.0F;
  for (unsigned i = 0; i < K1_AUDIO_HOP_SAMPLES; ++i) {
    const float x = static_cast<float>(hop.pcm[i]) / 32768.0F;
    const float mag = x < 0.0F ? -x : x;
    if (mag > peak) {
      peak = mag;
    }
    energy += x * x;
  }
  AudioPipelineInput input{};
  input.samples = &window_;
  input.sequence = static_cast<std::uint32_t>(hop.hop_sequence);
  input.media_time = AudioTime{hop.stream_epoch, k1_audio_hop_media_end_48k(&hop)};
  input.media_time_valid = true;
  input.frame_ms = static_cast<std::uint32_t>(input.media_time.frame_index / 48U);
  input.capture_time_us = hop.capture_estimate_valid
                              ? hop.newest_sample_capture_estimate_us
                              : hop.supporting_dma_receipt_us;
  input.publish_time_us = 0;
  input.peak_scaled = peak;
  input.vu_level = std::sqrt(energy / static_cast<float>(K1_AUDIO_HOP_SAMPLES));
  input.silence = peak == 0.0F;
  const std::uint32_t ap_begin = host_cycles_;
#ifdef K1_RA8P1_TARGET
  const std::uint32_t ap_t0 = k1_cycle_count();
  (void)ap_begin;
#endif
  private_ = pipeline_.process(input);
#ifdef K1_RA8P1_TARGET
  last_ap_cycles_ = k1_cycle_count() - ap_t0;
#else
  last_ap_cycles_ = host_cycles_ - ap_begin;
#endif
  const std::uint64_t ap_finish_us = nowUs();
  if (last_sequence_ == 1U) {
    epoch_transition_us_ = ap_finish_us;
  }
  if (last_publish_us_ != 0U && ap_finish_us >= last_publish_us_) {
    const std::uint64_t dt = ap_finish_us - last_publish_us_;
    last_hop_dt_us_ = dt > 0xFFFFFFFFULL ? 0xFFFFFFFFU : static_cast<std::uint32_t>(dt);
  }
  last_publish_us_ = ap_finish_us;
  pushVisualWaveform(waveform_, hop.pcm, K1_AUDIO_HOP_SAMPLES, peak * 32768.0F,
                     peak, input.sequence);
  finalisePublication(private_, hop, ap_finish_us);
  committed_ = private_;
  last_hop_ = hop;
  generation_ += 1U;
  hops_consumed_ += 1U;
  hops_recorded_ += 1U;
  if (warmup_hops_ < kLiveWarmupHops) {
    warmup_hops_ += 1U;
  }
  warming_ = warmup_hops_ < kLiveWarmupHops;
  if (!warming_ && warmup_complete_us_ == 0U) {
    warmup_complete_us_ = ap_finish_us;
  }
  LiveEventRecord event{};
  event.capture_time_us = committed_.features.capture_time_us;
  event.publish_time_us = committed_.features.publish_time_us;
  event.hop_sequence = hop.hop_sequence;
  event.stream_epoch = hop.stream_epoch;
  event.event_mask = (committed_.features.event_flags >> 1U) & 127U;
  if (committed_.saliency.event.salient) {
    event.event_mask |= 128U;
  }
  event.onset_strength = committed_.features.onset_strength;
  event.flux = committed_.features.novelty;
  event.bass_onset = committed_.features.bass_onset_strength;
  event.kick = committed_.features.kick_strength;
  event.snare = committed_.features.snare_strength;
  event.hihat = committed_.features.hihat_strength;
  event.transient = committed_.features.transient_strength;
  event.saliency = committed_.saliency.axis.overall_saliency;
  event.peak_scaled = committed_.features.peak_scaled;
  event.beat_phase = committed_.features.beat_phase;
  event.beat_confidence = committed_.features.beat_confidence;
  event.tempo_bpm = committed_.tempo.bpm;
  event.mode_id = mode_id_;
  float spec_peak = -1.0F;
  for (unsigned i = 0; i < 80U; ++i) {
    if (committed_.features.spectrum[i] > spec_peak) {
      spec_peak = committed_.features.spectrum[i];
      event.spectrum_peak_bin = static_cast<std::uint16_t>(i);
    }
  }
  float chroma_peak = -1.0F;
  for (unsigned i = 0; i < 12U; ++i) {
    if (committed_.features.chroma_a_origin[i] > chroma_peak) {
      chroma_peak = committed_.features.chroma_a_origin[i];
      event.chroma_peak = static_cast<std::uint16_t>(i);
    }
  }
  events_.push(event);
  LiveTimingRecord timing{};
  timing.hop_sequence = hop.hop_sequence;
  timing.capture_time_us = committed_.features.capture_time_us;
  timing.publish_time_us = committed_.features.publish_time_us;
  timing.ap_cycles = last_ap_cycles_;
  timing.vp_cycles = last_vp_cycles_;
  timing.hop_dt_us = last_hop_dt_us_;
  timing.asrc_starved = asrc_starved_;
  timing.measured_hz = measured_hz_;
  timing_.push(timing);
  return LiveHopStatus::Ok;
}

void LiveAudioRuntime::finalisePublication(core::audio::AudioPipelineOutput& output,
                                           const k1_audio_hop_t& hop,
                                           std::uint64_t ap_finish_us) noexcept {
  output.features.publish_time_us = ap_finish_us;
  if (hop.capture_estimate_valid &&
      hop.newest_sample_capture_estimate_us > ap_finish_us) {
    output.features.capture_time_us = hop.supporting_dma_receipt_us;
  }
  output.predicted_next_beat_valid = false;
  (void)mediaFramesToMicros;
}

void LiveAudioRuntime::invalidate(std::uint32_t reason) noexcept {
  last_reason_ = reason;
  committed_.valid = false;
  committed_.predicted_next_beat_valid = false;
  committed_.musical_time_valid = false;
  last_epoch_ = 0;
  last_sequence_ = 0;
  resetAudioOwned();
}

const core::audio::AudioPipelineOutput* LiveAudioRuntime::latest() const noexcept {
  return committed_.valid ? &committed_ : nullptr;
}

bool LiveAudioRuntime::publicationFresh(std::uint64_t now_us) const noexcept {
  if (!committed_.valid) {
    return false;
  }
  const std::uint64_t published = committed_.features.publish_time_us;
  if (now_us < published) {
    return false;
  }
  return now_us - published <= kLiveFeatureStaleUs;
}

LiveMirSnapshot LiveAudioRuntime::snapshot() const noexcept {
  LiveMirSnapshot row{};
  row.publication_generation = generation_;
  row.stream_epoch = last_hop_.stream_epoch;
  row.hop_sequence = last_hop_.hop_sequence;
  row.publish_time_us = committed_.features.publish_time_us;
  row.capture_time_us = committed_.features.capture_time_us;
  row.warmup_hops = warmup_hops_;
  row.peak_scaled = committed_.features.peak_scaled;
  row.vu_level = committed_.features.vu_level;
  row.beat_phase = committed_.features.beat_phase;
  row.beat_confidence = committed_.features.beat_confidence;
  row.tempo_bpm = committed_.tempo.bpm;
  row.warming = warming_ ? 1 : 0;
  row.valid = committed_.valid ? 1 : 0;
  row.physical_timestamp_valid = last_hop_.physical_timestamp_valid;
  row.hops_recorded = hops_recorded_;
  return row;
}

void LiveAudioRuntime::setIdentity(const std::uint8_t uid[16],
                                   const std::uint8_t build_sha[32]) noexcept {
  if (uid != nullptr) {
    std::memcpy(uid_, uid, 16);
  }
  if (build_sha != nullptr) {
    std::memcpy(build_sha256_, build_sha, 32);
  }
}

void LiveAudioRuntime::setControlMeta(std::uint16_t mode_id, std::uint16_t palette_id,
                                      std::uint8_t emit_on, std::uint32_t revision,
                                      std::uint16_t gain_q8) noexcept {
  mode_id_ = mode_id;
  palette_id_ = palette_id;
  emit_on_ = emit_on;
  config_revision_ = revision;
  gain_q8_ = gain_q8;
}

void LiveAudioRuntime::setCaptureMeta(std::uint64_t start_us, std::uint64_t end_us,
                                      std::uint32_t starved, std::uint32_t measured_hz,
                                      std::uint32_t hop_dt_us) noexcept {
  capture_start_us_ = start_us;
  capture_end_us_ = end_us;
  asrc_starved_ = starved;
  measured_hz_ = measured_hz;
  if (hop_dt_us != 0U) {
    last_hop_dt_us_ = hop_dt_us;
  }
}

void LiveAudioRuntime::setStaleMarkers(std::uint64_t injection_start,
                                       std::uint64_t injection_end) noexcept {
  injection_start_us_ = injection_start;
  injection_end_us_ = injection_end;
}

void LiveAudioRuntime::noteStaleEntry(std::uint64_t now_us) noexcept {
  if (!stale_latched_) {
    stale_latched_ = true;
    publication_deadline_misses_ += 1U;
    if (stale_entry_us_ == 0U) {
      stale_entry_us_ = now_us;
    }
  }
}

void LiveAudioRuntime::noteFreshPublication() noexcept {
  stale_latched_ = false;
}

void LiveAudioRuntime::fillSnapshotFields(LiveSnapshotFields* fields,
                                          std::uint64_t now_us) const noexcept {
  if (fields == nullptr) {
    return;
  }
  *fields = LiveSnapshotFields{};
  std::uint16_t flags = kSnapLiveOrigin;
  if (committed_.valid) {
    flags |= kSnapValid;
  }
  if (warming_) {
    flags |= kSnapWarming;
  }
  if (!publicationFresh(now_us)) {
    flags |= kSnapStale;
  }
  if (emit_on_) {
    flags |= kSnapEmitOn;
  }
  if (last_hop_.physical_timestamp_valid) {
    flags |= kSnapPhysicalTs;
  }
  fields->flags = flags;
  fields->publication_generation = generation_;
  fields->stream_epoch = last_hop_.stream_epoch;
  fields->hop_sequence = last_hop_.hop_sequence;
  fields->capture_time_us = committed_.features.capture_time_us;
  fields->publish_time_us = committed_.features.publish_time_us;
  fields->analysis_end_exclusive = last_hop_.analysis_end_exclusive;
  fields->media_time_48k = k1_audio_hop_media_end_48k(&last_hop_);
  fields->musical_pulse = committed_.musical_time.beat_index;
  fields->musical_phase_q32 =
      static_cast<std::uint32_t>(committed_.musical_time.phase_error_q32);
  fields->tempo_bpm = committed_.tempo.bpm;
  fields->beat_phase = committed_.features.beat_phase;
  fields->beat_confidence = committed_.features.beat_confidence;
  fields->predicted_next_beat_us = 0;
  fields->peak_scaled = committed_.features.peak_scaled;
  fields->vu_level = committed_.features.vu_level;
  fields->bass_energy = committed_.features.low_energy;
  fields->mid_energy = committed_.features.mid_energy;
  fields->high_energy = committed_.features.high_energy;
  fields->onset_strength = committed_.features.onset_strength;
  fields->flux = committed_.features.novelty;
  float energy = 0.0F;
  float weighted = 0.0F;
  float hfc = 0.0F;
  float rolloff_acc = 0.0F;
  float spec_peak = 0.0F;
  for (unsigned i = 0; i < 80U; ++i) {
    const float s = committed_.features.spectrum[i];
    fields->spectrum[i] = s;
    energy += s;
    const float hz = static_cast<float>(i) * 150.0F;
    weighted += s * hz;
    hfc += s * hz;
    if (s > spec_peak) {
      spec_peak = s;
    }
  }
  fields->centroid_hz = energy > 0.0F ? weighted / energy : 0.0F;
  const float rolloff_target = energy * 0.85F;
  fields->rolloff_hz = 0.0F;
  for (unsigned i = 0; i < 80U; ++i) {
    rolloff_acc += committed_.features.spectrum[i];
    if (rolloff_acc >= rolloff_target) {
      fields->rolloff_hz = static_cast<float>(i) * 150.0F;
      break;
    }
  }
  float spread = 0.0F;
  if (energy > 0.0F) {
    for (unsigned i = 0; i < 80U; ++i) {
      const float hz = static_cast<float>(i) * 150.0F;
      const float d = hz - fields->centroid_hz;
      spread += committed_.features.spectrum[i] * d * d;
    }
    fields->bandwidth_hz = std::sqrt(spread / energy);
  }
  fields->rms = committed_.features.vu_level;
  fields->crest = (fields->rms > 1.0e-6F) ? (fields->peak_scaled / fields->rms) : 0.0F;
  fields->zcr = 0.0F;
  fields->hfc = hfc;
  fields->compactness = energy > 0.0F ? spec_peak / energy : 0.0F;
  for (unsigned i = 0; i < 12U; ++i) {
    fields->chroma[i] = committed_.features.chroma_a_origin[i];
  }
  fields->bass_onset = committed_.features.bass_onset_strength;
  fields->kick = committed_.features.kick_strength;
  fields->snare = committed_.features.snare_strength;
  fields->hihat = committed_.features.hihat_strength;
  fields->transient = committed_.features.transient_strength;
  fields->saliency = committed_.saliency.axis.overall_saliency;
  fields->harmonic_ratio = committed_.saliency.axis.harmonic_novelty;
  fields->inharmonicity = committed_.saliency.axis.timbral_novelty;
  fields->pitch_hz = 0.0F;
  fields->pitch_confidence = committed_.features.chroma_strength;
  fields->mode_id = mode_id_;
  fields->palette_id = palette_id_;
  fields->gain_q8 = gain_q8_;
  fields->warmup_hops = static_cast<std::uint16_t>(warmup_hops_);
  fields->config_revision = config_revision_;
  fields->hops_consumed = hops_consumed_;
  fields->hops_rejected = hops_rejected_;
  fields->last_invalidate_reason = last_reason_;
  fields->asrc_starved = asrc_starved_;
  fields->measured_hz = measured_hz_;
  fields->deadline_us = 7500U;
  fields->publication_deadline_misses = publication_deadline_misses_;
  fields->ap_cycles = last_ap_cycles_;
  fields->vp_cycles = last_vp_cycles_;
  fields->hop_dt_us = last_hop_dt_us_;
  fields->event_mask = events_.write_seq == 0U ? 0U : events_.rec[events_.write_seq % kLiveEventRing].event_mask;
  fields->event_write_seq = events_.write_seq;
  fields->timing_write_seq = timing_.write_seq;
  if (committed_.valid && now_us >= committed_.features.publish_time_us) {
    const std::uint64_t age = now_us - committed_.features.publish_time_us;
    fields->freshness_us = age > 0xFFFFFFFFULL ? 0xFFFFFFFFU : static_cast<std::uint32_t>(age);
  }
  fields->live_origin = 1;
  fields->physical_ts_valid = last_hop_.physical_timestamp_valid;
  fields->emit_on = emit_on_;
  fields->capture_start_us = capture_start_us_;
  fields->capture_end_us = capture_end_us_;
  fields->injection_start_us = injection_start_us_;
  fields->injection_end_us = injection_end_us_;
  fields->stale_entry_us = stale_entry_us_;
  fields->epoch_transition_us = epoch_transition_us_;
  fields->warmup_complete_us = warmup_complete_us_;
  std::memcpy(fields->uid, uid_, 16);
  std::memcpy(fields->build_sha256, build_sha256_, 32);
}

std::size_t LiveAudioRuntime::encodeSnapshot(std::uint8_t* out, std::size_t cap,
                                             std::uint64_t now_us) const noexcept {
  LiveSnapshotFields fields{};
  fillSnapshotFields(&fields, now_us);
  return liveEncodeSnapshot(out, cap, fields);
}

namespace {
template <typename Rec, std::uint32_t N>
std::size_t encodeRing(std::uint8_t* out, std::size_t cap, const LiveHistoryRing<Rec, N>& ring,
                       std::uint64_t after, std::uint32_t rec_bytes, std::uint32_t opcode,
                       std::uint32_t max_records, std::uint32_t magic,
                       std::uint64_t generation, std::uint64_t epoch,
                       void (*encode)(std::uint8_t*, const Rec&)) noexcept {
  if (out == nullptr || cap < kLiveHistoryHeaderBytes) {
    return 0;
  }
  std::uint16_t flags = 0;
  if (ring.write_seq == 0U) {
    flags |= kHistEmpty;
    liveEncodeHistoryHeader(out, magic, flags, 0, 0, after, 0, 0,
                            static_cast<std::uint16_t>(rec_bytes),
                            static_cast<std::uint16_t>(opcode), generation, epoch);
    return kLiveHistoryHeaderBytes;
  }
  std::uint64_t oldest = ring.write_seq > N ? ring.write_seq - N + 1U : 1U;
  const std::uint64_t newest = ring.write_seq;
  if (after + 1U < oldest) {
    flags |= kHistExpired | kHistGap;
    after = oldest - 1U;
  }
  std::uint64_t start = after + 1U;
  if (start < oldest) {
    start = oldest;
  }
  std::uint32_t available = 0;
  if (start <= newest) {
    available = static_cast<std::uint32_t>(newest - start + 1U);
  }
  std::uint16_t truncated = 0;
  std::uint32_t take = available;
  if (take > max_records) {
    take = max_records;
    truncated = 1;
  }
  const std::size_t need = kLiveHistoryHeaderBytes + static_cast<std::size_t>(take) * rec_bytes;
  if (need > cap) {
    take = static_cast<std::uint32_t>((cap - kLiveHistoryHeaderBytes) / rec_bytes);
    truncated = 1;
  }
  liveEncodeHistoryHeader(out, magic, flags, oldest, newest, after,
                          static_cast<std::uint16_t>(take), truncated,
                          static_cast<std::uint16_t>(rec_bytes),
                          static_cast<std::uint16_t>(opcode), generation, epoch);
  for (std::uint32_t i = 0; i < take; ++i) {
    const Rec& row = ring.rec[(start + i) % N];
    encode(out + kLiveHistoryHeaderBytes + i * rec_bytes, row);
  }
  return kLiveHistoryHeaderBytes + static_cast<std::size_t>(take) * rec_bytes;
}

void encodeEventBytes(std::uint8_t* out, const LiveEventRecord& rec) {
  liveEncodeEvent(out, rec);
}
void encodeTimingBytes(std::uint8_t* out, const LiveTimingRecord& rec) {
  liveEncodeTiming(out, rec);
}
}  // namespace

std::size_t LiveAudioRuntime::encodeEvents(std::uint8_t* out, std::size_t cap,
                                           std::uint64_t after) const noexcept {
  return encodeRing<LiveEventRecord, kLiveEventRing>(
      out, cap, events_, after, kLiveEventBytes, kLiveEventsOpcode, kLiveEventReadMax,
      0x48454C54U, generation_, last_hop_.stream_epoch, encodeEventBytes);
}

std::size_t LiveAudioRuntime::encodeTiming(std::uint8_t* out, std::size_t cap,
                                           std::uint64_t after) const noexcept {
  return encodeRing<LiveTimingRecord, kLiveTimingRing>(
      out, cap, timing_, after, kLiveTimingBytes, kLiveTimingOpcode, kLiveTimingReadMax,
      0x48544C54U, generation_, last_hop_.stream_epoch, encodeTimingBytes);
}

std::size_t LiveAudioRuntime::mirJson(char* out, std::size_t capacity) const noexcept {
  if (out == nullptr || capacity == 0U) {
    return 0;
  }
  const LiveMirSnapshot row = snapshot();
  const int n = std::snprintf(
      out, capacity,
      "{\"version\":1,\"valid\":%s,\"warming\":%s,\"physical_timestamp_valid\":false,"
      "\"generation\":%llu,\"stream_epoch\":%llu,\"hop_sequence\":%llu,"
      "\"publish_time_us\":%llu,\"capture_time_us\":%llu,\"warmup_hops\":%lu,"
      "\"peak\":%.6g,\"vu\":%.6g,\"beat_phase\":%.6g,\"beat_confidence\":%.6g,"
      "\"tempo_bpm\":%.6g,\"hops_recorded\":%lu,"
      "\"recording\":\"scheduled_phase_8\"}",
      row.valid ? "true" : "false", row.warming ? "true" : "false",
      static_cast<unsigned long long>(row.publication_generation),
      static_cast<unsigned long long>(row.stream_epoch),
      static_cast<unsigned long long>(row.hop_sequence),
      static_cast<unsigned long long>(row.publish_time_us),
      static_cast<unsigned long long>(row.capture_time_us),
      static_cast<unsigned long>(row.warmup_hops),
      static_cast<double>(row.peak_scaled), static_cast<double>(row.vu_level),
      static_cast<double>(row.beat_phase), static_cast<double>(row.beat_confidence),
      static_cast<double>(row.tempo_bpm),
      static_cast<unsigned long>(row.hops_recorded));
  if (n <= 0 || static_cast<std::size_t>(n) >= capacity) {
    return 0;
  }
  return static_cast<std::size_t>(n);
}

}  // namespace k1::titan
