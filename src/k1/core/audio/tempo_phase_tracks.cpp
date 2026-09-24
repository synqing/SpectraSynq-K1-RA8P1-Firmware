#include "core/audio/tempo_phase_tracks.h"

#include <cmath>

namespace k1::core::audio {
namespace {

using contract::TempoBeatPositionV1;
using contract::TempoCandidateAdmission;
using contract::TempoProjectionStatus;
using contract::TempoTrackState;

constexpr double kTwoPi = 6.28318530717958647692;
constexpr double kQ32 = 4294967296.0;
constexpr std::uint64_t kFramesPerMinute =
    static_cast<std::uint64_t>(contract::kTempoFieldMediaRateHz) * 60U;

bool finiteInRange(const float value, const float low,
                   const float high) noexcept {
  return std::isfinite(value) && value >= low && value <= high;
}

TempoPhaseTrackConfig sanitise(const TempoPhaseTrackConfig& requested) noexcept {
  const TempoPhaseTrackConfig defaults{};
  TempoPhaseTrackConfig config = requested;
  if (config.window_samples < 2U ||
      config.window_samples > contract::kTempoFieldMaxPhaseWindow) {
    config.window_samples = defaults.window_samples;
  }
  if (config.minimum_window_samples < 2U ||
      config.minimum_window_samples > config.window_samples) {
    config.minimum_window_samples = config.window_samples <
                                            defaults.minimum_window_samples
                                        ? config.window_samples
                                        : defaults.minimum_window_samples;
  }
  if (!finiteInRange(config.minimum_coherence, 0.0F, 1.0F)) {
    config.minimum_coherence = defaults.minimum_coherence;
  }
  if (!finiteInRange(config.correction_gain, 0.0F, 1.0F)) {
    config.correction_gain = defaults.correction_gain;
  }
  if (!finiteInRange(config.max_correction_cycles, 0.0F, 0.5F)) {
    config.max_correction_cycles = defaults.max_correction_cycles;
  }
  if (!finiteInRange(config.demote_threshold_cycles, 0.0F, 0.5F)) {
    config.demote_threshold_cycles = defaults.demote_threshold_cycles;
  }
  if (config.reinitialise_after == 0U) {
    config.reinitialise_after = defaults.reinitialise_after;
  }
  if (!finiteInRange(config.frequency_gain, 0.0F, 1.0F)) {
    config.frequency_gain = defaults.frequency_gain;
  }
  if (!finiteInRange(config.max_frequency_deviation, 0.0F, 0.25F)) {
    config.max_frequency_deviation = defaults.max_frequency_deviation;
  }
  if (config.derived_ratio_count > contract::kTempoFieldMaxDerived) {
    config.derived_ratio_count = 0U;
  }
  for (std::size_t index = 0U; index < config.derived_ratio_count; ++index) {
    if (config.derived_numerator[index] == 0U ||
        config.derived_denominator[index] == 0U) {
      config.derived_ratio_count = 0U;
    }
  }
  return config;
}

TempoTrackLimits sanitise(const TempoTrackLimits& requested) noexcept {
  TempoTrackLimits limits = requested;
  if (limits.fresh_limit_media_frames > limits.coast_limit_media_frames ||
      limits.coast_limit_media_frames > limits.stale_limit_media_frames) {
    limits = TempoTrackLimits{};
  }
  return limits;
}

double periodFrames(const std::uint64_t period_q32) noexcept {
  return static_cast<double>(period_q32) / kQ32;
}

std::uint64_t periodFromFrequency(const double beats_per_frame) noexcept {
  const double period = kQ32 / beats_per_frame;
  if (!(period >= 1.0) || period >= 9.0e18) return 0U;
  return static_cast<std::uint64_t>(period + 0.5);
}

}  // namespace

TempoPhaseObservation observeNoveltyPhase(const NoveltyHistoryView& history,
                                          const double beat_period_frames,
                                          const std::uint16_t window_samples) noexcept {
  TempoPhaseObservation observation{};
  if (history.ring == nullptr || history.ring_length == 0U ||
      history.newest_index >= history.ring_length ||
      history.interval_media_frames == 0U ||
      !std::isfinite(beat_period_frames) || !(beat_period_frames >= 1.0)) {
    return observation;
  }
  std::uint16_t length = window_samples;
  if (length > history.usable_samples) length = history.usable_samples;
  if (length > history.ring_length) length = history.ring_length;
  if (length > contract::kTempoFieldMaxPhaseWindow) {
    length = static_cast<std::uint16_t>(contract::kTempoFieldMaxPhaseWindow);
  }
  if (length < 2U) return observation;

  const double window_step = kTwoPi / static_cast<double>(length);
  const double window_rotate_c = std::cos(window_step);
  const double window_rotate_s = std::sin(window_step);
  double window_c = std::cos(0.5 * window_step);
  double window_s = std::sin(0.5 * window_step);
  const double signal_step =
      kTwoPi * static_cast<double>(history.interval_media_frames) /
      beat_period_frames;
  const double signal_rotate_c = std::cos(signal_step);
  const double signal_rotate_s = std::sin(signal_step);
  double signal_c = 1.0;
  double signal_s = 0.0;

  double sum_w = 0.0;
  double sum_wx = 0.0;
  double sum_wz_re = 0.0;   // window transform at the candidate frequency
  double sum_wz_im = 0.0;
  double sum_wxz_re = 0.0;  // windowed signal transform
  double sum_wxz_im = 0.0;
  for (std::uint16_t k = 0U; k < length; ++k) {
    const std::uint16_t index = static_cast<std::uint16_t>(
        (static_cast<std::uint32_t>(history.newest_index) + history.ring_length -
         (k % history.ring_length)) %
        history.ring_length);
    const float sample = history.ring[index];
    if (!std::isfinite(sample) || sample < 0.0F) return TempoPhaseObservation{};
    const double x = static_cast<double>(sample);
    const double w = 0.5 - 0.5 * window_c;
    sum_w += w;
    sum_wx += w * x;
    sum_wz_re += w * signal_c;
    sum_wz_im += w * signal_s;
    sum_wxz_re += w * x * signal_c;
    sum_wxz_im += w * x * signal_s;
    const double next_window_c =
        window_c * window_rotate_c - window_s * window_rotate_s;
    window_s = window_s * window_rotate_c + window_c * window_rotate_s;
    window_c = next_window_c;
    const double next_signal_c =
        signal_c * signal_rotate_c - signal_s * signal_rotate_s;
    signal_s = signal_s * signal_rotate_c + signal_c * signal_rotate_s;
    signal_c = next_signal_c;
  }
  observation.window_samples = length;
  if (!(sum_w > 0.0) || !(sum_wx > 0.0)) return observation;
  const double mean = sum_wx / sum_w;
  const double c_re = sum_wxz_re - mean * sum_wz_re;
  const double c_im = sum_wxz_im - mean * sum_wz_im;
  const double magnitude = std::sqrt(c_re * c_re + c_im * c_im);
  if (!std::isfinite(magnitude) || !(magnitude > 0.0)) return observation;
  double phase = std::atan2(c_im, c_re) / kTwoPi;
  if (phase < 0.0) phase += 1.0;
  if (phase >= 1.0) phase -= 1.0;
  // Refer the phase to the window centroid t_c = R - offset (unbiased to
  // first order in detune); the frame offset is exact integer arithmetic.
  const std::uint64_t offset =
      (static_cast<std::uint64_t>(length - 1U) * history.interval_media_frames) / 2U;
  phase -= static_cast<double>(offset) / beat_period_frames;
  phase -= std::floor(phase);
  if (phase >= 1.0) phase -= 1.0;
  if (!(phase >= 0.0 && phase < 1.0)) return observation;
  observation.reference_offset_frames = offset;
  double coherence = magnitude / sum_wx;
  if (coherence > 1.0) coherence = 1.0;
  observation.valid = true;
  observation.phase_cycles = phase;
  observation.coherence = static_cast<float>(coherence);
  return observation;
}

std::uint64_t tempoBinPeriodQ32(const std::uint8_t bin) noexcept {
  if (bin >= contract::kTempoFieldBankBinCount) return 0U;
  return (kFramesPerMinute << 32U) /
         (static_cast<std::uint64_t>(contract::kTempoFieldBankLowBpm) + bin);
}

void TempoPhaseTracks::configure(const TempoPhaseTrackConfig& config,
                                 const TempoTrackLimits& limits) noexcept {
  config_ = sanitise(config);
  limits_ = sanitise(limits);
  reset();
}

void TempoPhaseTracks::reset() noexcept {
  for (TempoTrackSlot& slot : slots_) slot = TempoTrackSlot{};
}

std::uint32_t TempoPhaseTracks::nextGeneration() noexcept {
  ++last_generation_;
  if (last_generation_ == 0U) last_generation_ = 1U;  // zero means none
  return last_generation_;
}

std::uint8_t TempoPhaseTracks::selectedBins(
    std::array<std::uint8_t, contract::kTempoFieldMaxSelected>& bins)
    const noexcept {
  std::uint8_t count = 0U;
  for (const TempoTrackSlot& slot : slots_) {
    if (!slot.occupied ||
        slot.admission != TempoCandidateAdmission::kSelected ||
        count >= contract::kTempoFieldMaxSelected) {
      continue;
    }
    std::uint8_t position = count;
    while (position > 0U && bins[position - 1U] > slot.bin) {
      bins[position] = bins[position - 1U];
      --position;
    }
    bins[position] = slot.bin;
    ++count;
  }
  return count;
}

void TempoPhaseTracks::initialiseAnchor(TempoTrackSlot& slot,
                                        const std::uint64_t epoch_id,
                                        const std::uint64_t reference,
                                        const std::uint64_t now,
                                        const double phase_cycles,
                                        const float coherence) noexcept {
  std::uint64_t phase_q32 = static_cast<std::uint64_t>(phase_cycles * kQ32);
  if (phase_q32 >= contract::kTempoQ32One) {
    phase_q32 = contract::kTempoQ32One - 1U;
  }
  slot.anchor.epoch_id = epoch_id;
  slot.anchor.reference_media_frame = reference;
  slot.anchor.beat_position_q32 =
      (contract::kTempoTrackBeatOrigin << 32U) | phase_q32;
  slot.anchor.beat_period_q32 = slot.bin_period_q32;
  slot.phase_valid = true;
  slot.phase_observed_media_frame = now;
  slot.state = TempoTrackState::kObserved;
  slot.fresh_this_update = true;
  slot.rejections = 0U;
  slot.innovation = 0.0F;
  slot.applied_correction = 0.0F;
  slot.innovation_count = 0U;
  slot.innovation_write = 0U;
  slot.innovations.fill(0.0F);
  slot.coherence = coherence;
  slot.frequency_clamped = false;
  slot.correction_clamped = false;
}

void TempoPhaseTracks::acceptObservation(
    TempoTrackSlot& slot, const std::uint64_t epoch_id, const std::uint64_t now,
    const TempoPhaseObservation& observation,
    TempoTrackUpdateResult& result) noexcept {
  slot.coherence = observation.coherence;
  // The observation refers to its window centroid, not to the support end.
  const std::uint64_t reference =
      now > observation.reference_offset_frames
          ? now - observation.reference_offset_frames
          : 0U;
  if (!slot.phase_valid) {
    // First phase of this generation.
    initialiseAnchor(slot, epoch_id, reference, now, observation.phase_cycles,
                     observation.coherence);
    ++result.accepted;
    return;
  }
  if (now <= slot.phase_observed_media_frame) {
    // A repeated or earlier reference cannot be a new observation; the track is
    // left exactly as it was rather than accepting the same evidence twice.
    return;
  }
  const std::uint64_t age = now - slot.phase_observed_media_frame;
  TempoBeatPositionV1 predicted{};
  const TempoProjectionStatus status =
      contract::projectTempoAnchor(slot.anchor, epoch_id, reference, 1U, 1U, predicted);
  if (age > limits_.coast_limit_media_frames ||
      status != TempoProjectionStatus::kOk) {
    // A stale (or unprojectable) track has no authority to be retained: the
    // observation starts a genuinely new generation.
    slot.generation = nextGeneration();
    slot.admitted_media_frame = now;
    initialiseAnchor(slot, epoch_id, reference, now, observation.phase_cycles,
                     observation.coherence);
    slot.reinitialised = true;
    ++result.reinitialisations;
    ++result.accepted;
    return;
  }
  const double predicted_phase =
      static_cast<double>(predicted.phase_q32) / kQ32;
  const double difference = observation.phase_cycles - predicted_phase;
  const double innovation = difference - std::ceil(difference - 0.5);
  slot.innovation = static_cast<float>(innovation);
  if (std::fabs(innovation) >
      static_cast<double>(config_.demote_threshold_cycles)) {
    ++result.rejected;
    if (slot.rejections < 255U) ++slot.rejections;
    if (slot.rejections >= config_.reinitialise_after) {
      slot.generation = nextGeneration();
      slot.admitted_media_frame = now;
      initialiseAnchor(slot, epoch_id, reference, now, observation.phase_cycles,
                       observation.coherence);
      slot.reinitialised = true;
      ++result.reinitialisations;
      ++result.accepted;
    } else {
      // Authority demoted; the anchor and last observation time stay put.
      slot.state = TempoTrackState::kCoasting;
    }
    return;
  }
  const double requested =
      static_cast<double>(config_.correction_gain) * innovation;
  const double bound = static_cast<double>(config_.max_correction_cycles);
  double correction = requested;
  slot.correction_clamped = false;
  if (correction > bound) {
    correction = bound;
    slot.correction_clamped = true;
  } else if (correction < -bound) {
    correction = -bound;
    slot.correction_clamped = true;
  }
  const std::int64_t correction_q32 =
      static_cast<std::int64_t>(std::llround(correction * kQ32));
  const std::uint64_t position =
      (predicted.beat_index << 32U) | predicted.phase_q32;
  std::uint64_t corrected = position;
  if (correction_q32 >= 0) {
    corrected = position + static_cast<std::uint64_t>(correction_q32);
  } else {
    const std::uint64_t magnitude = static_cast<std::uint64_t>(-correction_q32);
    corrected = magnitude > position ? 0U : position - magnitude;
  }

  // Frequency error over the interval between the two anchor references.
  const std::uint64_t elapsed =
      reference > slot.anchor.reference_media_frame
          ? reference - slot.anchor.reference_media_frame
          : age;
  const double frequency = kQ32 / static_cast<double>(slot.anchor.beat_period_q32);
  const double bin_frequency = kQ32 / static_cast<double>(slot.bin_period_q32);
  const double deviation = static_cast<double>(config_.max_frequency_deviation);
  const double low = bin_frequency * (1.0 - deviation);
  const double high = bin_frequency * (1.0 + deviation);
  double next_frequency =
      frequency + static_cast<double>(config_.frequency_gain) * innovation /
                      static_cast<double>(elapsed);
  slot.frequency_clamped = false;
  if (next_frequency < low) {
    next_frequency = low;
    slot.frequency_clamped = true;
  } else if (next_frequency > high) {
    next_frequency = high;
    slot.frequency_clamped = true;
  }
  const std::uint64_t next_period = periodFromFrequency(next_frequency);

  slot.anchor.epoch_id = epoch_id;
  slot.anchor.reference_media_frame = reference;
  slot.anchor.beat_position_q32 = corrected;
  if (next_period != 0U) slot.anchor.beat_period_q32 = next_period;
  slot.phase_observed_media_frame = now;
  slot.state = TempoTrackState::kObserved;
  slot.fresh_this_update = true;
  slot.rejections = 0U;
  slot.applied_correction = static_cast<float>(correction);
  slot.innovations[slot.innovation_write] = static_cast<float>(innovation);
  slot.innovation_write = static_cast<std::uint8_t>(
      (slot.innovation_write + 1U) % kTempoTrackInnovationHistory);
  if (slot.innovation_count < kTempoTrackInnovationHistory) {
    ++slot.innovation_count;
  }
  ++result.accepted;
}

TempoTrackUpdateResult TempoPhaseTracks::update(
    const TempoTrackUpdate& update) noexcept {
  TempoTrackUpdateResult result{};
  if (update.selected_count > contract::kTempoFieldMaxSelected ||
      (update.selected_count > 0U && update.selected_bins == nullptr)) {
    result.input_rejected = true;
    return result;
  }
  std::array<std::uint8_t, contract::kTempoFieldMaxSelected> selected{};
  for (std::uint8_t index = 0U; index < update.selected_count; ++index) {
    const std::uint8_t bin = update.selected_bins[index];
    if (bin >= contract::kTempoFieldBankBinCount) {
      result.input_rejected = true;
      return result;
    }
    std::uint8_t position = index;
    while (position > 0U && selected[position - 1U] > bin) {
      selected[position] = selected[position - 1U];
      --position;
    }
    selected[position] = bin;
  }
  for (std::uint8_t index = 1U; index < update.selected_count; ++index) {
    if (selected[index] == selected[index - 1U]) {
      result.input_rejected = true;
      return result;
    }
  }
  const std::uint64_t now = update.reference_media_frame;
  const auto is_selected = [&](const std::uint8_t bin) noexcept {
    for (std::uint8_t index = 0U; index < update.selected_count; ++index) {
      if (selected[index] == bin) return true;
    }
    return false;
  };

  for (TempoTrackSlot& slot : slots_) {
    slot.fresh_this_update = false;
    slot.reinitialised = false;
    slot.correction_clamped = false;
    if (slot.occupied && slot.admission == TempoCandidateAdmission::kSelected &&
        !is_selected(slot.bin)) {
      slot.admission = TempoCandidateAdmission::kRetiring;
      ++result.retirements;
    }
  }

  for (std::uint8_t index = 0U; index < update.selected_count; ++index) {
    const std::uint8_t bin = selected[index];
    TempoTrackSlot* target = nullptr;
    for (TempoTrackSlot& slot : slots_) {
      if (slot.occupied && slot.bin == bin) target = &slot;
    }
    if (target == nullptr) {
      for (TempoTrackSlot& slot : slots_) {
        if (!slot.occupied) {
          target = &slot;
          break;
        }
      }
    }
    if (target == nullptr) {
      // Capacity: evict the retiring slot with the oldest evidence; ties go to
      // the lower bin. Selected slots are never evicted.
      std::uint64_t oldest = UINT64_MAX;
      for (TempoTrackSlot& slot : slots_) {
        if (slot.admission != TempoCandidateAdmission::kRetiring) continue;
        const std::uint64_t reference = slot.phase_valid
                                            ? slot.phase_observed_media_frame
                                            : slot.last_selected_media_frame;
        if (target == nullptr || reference < oldest ||
            (reference == oldest && slot.bin < target->bin)) {
          target = &slot;
          oldest = reference;
        }
      }
      if (target == nullptr) continue;  // unreachable: <= 4 selected of 8
      ++result.evictions;
      target->occupied = false;
    }
    if (!target->occupied) {
      *target = TempoTrackSlot{};
      target->occupied = true;
      target->bin = bin;
      target->generation = nextGeneration();
      target->admitted_media_frame = now;
      target->bin_period_q32 = tempoBinPeriodQ32(bin);
      target->anchor.epoch_id = update.epoch_id;
      target->anchor.beat_period_q32 = target->bin_period_q32;
      ++result.admissions;
    }
    target->admission = TempoCandidateAdmission::kSelected;
    target->last_selected_media_frame = now;
  }

  if (update.observe_phase && update.observer != nullptr) {
    for (std::uint8_t index = 0U; index < update.selected_count; ++index) {
      for (TempoTrackSlot& slot : slots_) {
        if (!slot.occupied || slot.bin != selected[index]) continue;
        const TempoPhaseObservation observation = update.observer(
            update.observer_context, slot.bin,
            periodFrames(slot.bin_period_q32));
        ++result.fresh_evaluations;
        if (!observation.valid || !std::isfinite(observation.phase_cycles) ||
            !(observation.coherence >= config_.minimum_coherence)) {
          slot.coherence = observation.valid ? observation.coherence : 0.0F;
          ++result.low_coherence;
          continue;
        }
        acceptObservation(slot, update.epoch_id, now, observation, result);
      }
    }
  }

  for (TempoTrackSlot& slot : slots_) {
    if (!slot.occupied) continue;
    if (!slot.fresh_this_update) {
      if (!slot.phase_valid) {
        slot.state = TempoTrackState::kInvalid;
      } else {
        const std::uint64_t age = now >= slot.phase_observed_media_frame
                                      ? now - slot.phase_observed_media_frame
                                      : 0U;
        slot.state = age > limits_.coast_limit_media_frames
                         ? TempoTrackState::kStale
                         : TempoTrackState::kCoasting;
      }
    }
    if (slot.admission == TempoCandidateAdmission::kRetiring) {
      const std::uint64_t reference = slot.phase_valid
                                          ? slot.phase_observed_media_frame
                                          : slot.last_selected_media_frame;
      const std::uint64_t age = now >= reference ? now - reference : 0U;
      if (age > limits_.stale_limit_media_frames) {
        slot = TempoTrackSlot{};
        ++result.removals;
      }
    }
  }
  return result;
}

}  // namespace k1::core::audio
