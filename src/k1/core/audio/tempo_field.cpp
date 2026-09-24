#include "core/audio/tempo_field.h"

#include <cmath>

namespace k1::core::audio {
namespace {

using contract::kTempoFieldBankBinCount;
using contract::kTempoFieldCandidateCapacity;
using contract::kTempoFieldMaxSelected;
using contract::kTempoFieldNoBin;
using contract::kTempoFieldNoIndex;
using contract::TempoCandidateAdmission;
using contract::TempoCandidateV1;
using contract::TempoCoastReason;
using contract::TempoFieldReason;
using contract::TempoFieldV1;
using contract::TempoTrackState;

constexpr std::uint16_t kHistory =
    static_cast<std::uint16_t>(kTempoAcfHistoryLength);
constexpr int kProminenceLobe = 6;  // matches the incumbent confidence lobe
constexpr double kQ32 = 4294967296.0;
constexpr double kFramesPerMinute =
    static_cast<double>(contract::kTempoFieldMediaRateHz) * 60.0;
constexpr std::uint8_t kFamilyNumerator[] = {2U, 1U, 3U, 2U};
constexpr std::uint8_t kFamilyDenominator[] = {1U, 2U, 2U, 3U};

// Bounded peak list: two adjacent bins cannot both be peaks under the rule in
// selectTempoCandidates, so at most half the bank qualifies.
constexpr std::size_t kMaximumPeaks = kTempoFieldBankBinCount / 2U;

// Value-initialised template. Copying it avoids materialising a large
// braced temporary inside lambda-heavy functions (GCC 13.3 x86-64 raised an
// internal compiler error on `field = TempoFieldV1{};` there).
const TempoFieldV1 kEmptyField{};

float evidence(const float value, bool& invalid) noexcept {
  if (!std::isfinite(value) || value < 0.0F) {
    invalid = true;
    return 0.0F;
  }
  return value;
}

float finiteOrZero(const float value, bool& invalid) noexcept {
  if (!std::isfinite(value)) {
    invalid = true;
    return 0.0F;
  }
  return value;
}

std::uint16_t hopMediaFrames(const AudioRateConfiguration rate) noexcept {
  const std::uint64_t frames =
      analysisSpanToMediaFrames(rate.hop_samples, rate.sample_rate_hz);
  return frames > 0xFFFFU ? 0U : static_cast<std::uint16_t>(frames);
}

bool supportedRate(const AudioRateConfiguration rate) noexcept {
  return rate.hop_samples != 0U && hopMediaFrames(rate) != 0U &&
         analysisToMedia(rate.hop_samples, rate.sample_rate_hz).quarter == 0U;
}

struct ObserverContext final {
  NoveltyHistoryView view{};
  std::uint16_t window = 0U;
  std::uint32_t samples = 0U;
};

TempoPhaseObservation observeFromHistory(void* const context,
                                         const std::uint8_t /*bin*/,
                                         const double beat_period_frames) {
  auto* const observer = static_cast<ObserverContext*>(context);
  const TempoPhaseObservation observation =
      observeNoveltyPhase(observer->view, beat_period_frames, observer->window);
  observer->samples += observation.window_samples;
  return observation;
}

void recordMax(std::uint32_t& last, std::uint32_t& maximum,
               const std::uint32_t value) noexcept {
  last = value;
  if (value > maximum) maximum = value;
}

void saturatingIncrement(std::uint32_t& value) noexcept {
  if (value != UINT32_MAX) ++value;
}

float runningBpm(const std::uint64_t period_q32) noexcept {
  if (period_q32 == 0U) return 0.0F;
  return static_cast<float>(kFramesPerMinute * kQ32 /
                            static_cast<double>(period_q32));
}

float innovationRms(const TempoTrackSlot& slot) noexcept {
  double sum = 0.0;
  for (std::uint8_t index = 0U; index < slot.innovation_count; ++index) {
    const double value = static_cast<double>(slot.innovations[index]);
    sum += value * value;
  }
  return slot.innovation_count == 0U
             ? 0.0F
             : static_cast<float>(
                   std::sqrt(sum / static_cast<double>(slot.innovation_count)));
}

}  // namespace

TempoSelectionConfig makeTempoSelectionConfig(const float admission_salience,
                                              const float retention_salience,
                                              const float replacement_margin,
                                              const float separation_cents,
                                              const std::uint8_t max_selected) noexcept {
  TempoSelectionConfig config{};
  config.admission_salience =
      std::isfinite(admission_salience) && admission_salience >= 0.0F
          ? admission_salience
          : 0.40F;
  config.retention_salience =
      std::isfinite(retention_salience) && retention_salience >= 0.0F &&
              retention_salience <= config.admission_salience
          ? retention_salience
          : config.admission_salience;
  config.replacement_margin =
      std::isfinite(replacement_margin) && replacement_margin >= 0.0F
          ? replacement_margin
          : 0.10F;
  config.max_selected =
      max_selected == 0U || max_selected > kTempoFieldMaxSelected
          ? static_cast<std::uint8_t>(kTempoFieldMaxSelected)
          : max_selected;
  const double cents = std::isfinite(separation_cents) && separation_cents > 0.0F
                           ? static_cast<double>(separation_cents)
                           : 0.0;
  const double ratio = std::pow(2.0, cents / 1200.0);
  for (std::size_t bin = 0U; bin < kTempoFieldBankBinCount; ++bin) {
    const double base =
        static_cast<double>(contract::kTempoFieldBankLowBpm + bin);
    double distance = std::ceil(base * (ratio - 1.0));
    if (!(distance >= 1.0)) distance = 1.0;
    if (distance > 255.0) distance = 255.0;
    config.separation_bins[bin] = static_cast<std::uint8_t>(distance);
  }
  return config;
}

TempoSelectionResult selectTempoCandidates(const float* const comb,
                                           const std::uint8_t* const previous,
                                           const std::uint8_t previous_count,
                                           const bool allow_admission,
                                           const TempoSelectionConfig& config) noexcept {
  TempoSelectionResult result{};
  if (comb == nullptr) {
    result.invalid_input = true;
    return result;
  }
  std::array<float, kTempoFieldBankBinCount> value{};
  for (std::size_t bin = 0U; bin < kTempoFieldBankBinCount; ++bin) {
    value[bin] = evidence(comb[bin], result.invalid_input);
    ++result.steps;
  }
  const auto separated = [&](const std::uint8_t a,
                             const std::uint8_t b) noexcept {
    const std::uint8_t low = a < b ? a : b;
    const std::uint8_t high = a < b ? b : a;
    return static_cast<std::uint8_t>(high - low) >= config.separation_bins[low];
  };
  // a is presented before b: larger evidence, then the lower bin (frozen).
  const auto stronger = [&](const std::uint8_t a,
                            const std::uint8_t b) noexcept {
    return value[a] > value[b] || (value[a] == value[b] && a < b);
  };
  std::array<std::uint8_t, kTempoFieldMaxSelected> kept{};
  std::uint8_t kept_count = 0U;
  const auto separated_from_kept = [&](const std::uint8_t bin,
                                       const std::uint8_t skip) noexcept {
    for (std::uint8_t index = 0U; index < kept_count; ++index) {
      ++result.steps;
      if (index != skip && !separated(kept[index], bin)) return false;
    }
    return true;
  };

  // Retention: a previously selected bin stays while its own evidence clears
  // the retention level and is within the replacement margin of the strongest
  // bin in its unseparated neighbourhood.
  std::array<std::uint8_t, kTempoFieldMaxSelected> prior{};
  std::uint8_t prior_count = 0U;
  const std::uint8_t offered =
      previous == nullptr ? 0U
                          : (previous_count > kTempoFieldMaxSelected
                                 ? static_cast<std::uint8_t>(
                                       kTempoFieldMaxSelected)
                                 : previous_count);
  for (std::uint8_t index = 0U; index < offered; ++index) {
    const std::uint8_t bin = previous[index];
    bool duplicate = bin >= kTempoFieldBankBinCount;
    for (std::uint8_t other = 0U; other < prior_count && !duplicate; ++other) {
      duplicate = prior[other] == bin;
    }
    if (duplicate) {
      result.invalid_input = result.invalid_input || bin >= kTempoFieldBankBinCount;
      continue;
    }
    std::uint8_t position = prior_count;
    while (position > 0U && stronger(bin, prior[position - 1U])) {
      prior[position] = prior[position - 1U];
      --position;
      ++result.steps;
    }
    prior[position] = bin;
    ++prior_count;
  }
  for (std::uint8_t index = 0U; index < prior_count; ++index) {
    const std::uint8_t bin = prior[index];
    if (value[bin] < config.retention_salience) continue;
    float neighbourhood = value[bin];
    for (int other = static_cast<int>(bin) - 1;
         other >= 0 && !separated(static_cast<std::uint8_t>(other), bin);
         --other) {
      ++result.steps;
      if (value[static_cast<std::size_t>(other)] > neighbourhood) {
        neighbourhood = value[static_cast<std::size_t>(other)];
      }
    }
    for (int other = static_cast<int>(bin) + 1;
         other < static_cast<int>(kTempoFieldBankBinCount) &&
         !separated(bin, static_cast<std::uint8_t>(other));
         ++other) {
      ++result.steps;
      if (value[static_cast<std::size_t>(other)] > neighbourhood) {
        neighbourhood = value[static_cast<std::size_t>(other)];
      }
    }
    if (value[bin] * (1.0F + config.replacement_margin) < neighbourhood) {
      continue;
    }
    if (kept_count < config.max_selected &&
        separated_from_kept(bin, kTempoFieldNoIndex)) {
      kept[kept_count++] = bin;
    }
  }

  if (allow_admission) {
    std::array<std::uint8_t, kMaximumPeaks> peaks{};
    std::uint8_t peak_count = 0U;
    for (std::uint8_t bin = 0U; bin < kTempoFieldBankBinCount; ++bin) {
      ++result.steps;
      const bool rising = bin == 0U || value[bin] > value[bin - 1U];
      const bool falling = bin + 1U == kTempoFieldBankBinCount ||
                           value[bin] >= value[bin + 1U];
      if (value[bin] >= config.admission_salience && rising && falling &&
          peak_count < kMaximumPeaks) {
        std::uint8_t position = peak_count;
        while (position > 0U && stronger(bin, peaks[position - 1U])) {
          peaks[position] = peaks[position - 1U];
          --position;
          ++result.steps;
        }
        peaks[position] = bin;
        ++peak_count;
      }
    }
    bool replaced = false;
    for (std::uint8_t index = 0U; index < peak_count; ++index) {
      const std::uint8_t peak = peaks[index];
      ++result.steps;
      bool present = false;
      for (std::uint8_t other = 0U; other < kept_count; ++other) {
        present = present || kept[other] == peak;
      }
      if (present) continue;
      if (kept_count < config.max_selected) {
        if (separated_from_kept(peak, kTempoFieldNoIndex)) {
          kept[kept_count++] = peak;
        }
        continue;
      }
      if (replaced) continue;  // at most one replacement per update
      std::uint8_t weakest = 0U;
      for (std::uint8_t other = 1U; other < kept_count; ++other) {
        if (stronger(kept[weakest], kept[other])) weakest = other;
      }
      if (!(value[peak] >
            value[kept[weakest]] * (1.0F + config.replacement_margin))) {
        continue;
      }
      if (separated_from_kept(peak, weakest)) {
        kept[weakest] = peak;
        replaced = true;
      }
    }
  }
  result.count = kept_count;
  for (std::uint8_t index = 0U; index < kept_count; ++index) {
    result.bins[index] = kept[index];
  }
  return result;
}

TempoFieldCostModel tempoFieldCostModel(const TempoFieldConfig& config) noexcept {
  TempoFieldCostModel model{};
  const std::uint32_t selected =
      config.max_selected == 0U || config.max_selected > kTempoFieldMaxSelected
          ? static_cast<std::uint32_t>(kTempoFieldMaxSelected)
          : config.max_selected;
  const std::uint32_t window =
      config.phase.window_samples > contract::kTempoFieldMaxPhaseWindow
          ? static_cast<std::uint32_t>(contract::kTempoFieldMaxPhaseWindow)
          : config.phase.window_samples;
  model.max_fresh_evaluations_per_update = config.phase_enabled ? selected : 0U;
  model.max_observer_samples_per_update =
      model.max_fresh_evaluations_per_update * window;
  // Selection bound: validation scan + peak scan + insertion sort of at most
  // 48 peaks + separation checks against at most four kept bins per peak +
  // retention neighbourhood scans (<= 2 x 255 bins for each of four).
  const std::uint32_t peaks = static_cast<std::uint32_t>(kMaximumPeaks);
  model.max_selection_steps_per_update =
      2U * kTempoFieldBankBinCount + peaks * (peaks - 1U) / 2U +
      peaks * (1U + 2U * kTempoFieldMaxSelected) +
      kTempoFieldMaxSelected * (2U * kTempoFieldBankBinCount +
                                kTempoFieldMaxSelected + 1U);
  const std::uint64_t denominator =
      static_cast<std::uint64_t>(config.rate.hop_samples) *
      kProductionNoveltyDecimation;
  model.novelty_updates_per_second_x1000 =
      denominator == 0U
          ? 0U
          : static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(config.rate.sample_rate_hz) * 1000U /
                denominator);
  model.max_observer_samples_per_second =
      denominator == 0U
          ? 0U
          : static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(model.max_observer_samples_per_update) *
                config.rate.sample_rate_hz / denominator);
  model.publication_bytes = static_cast<std::uint32_t>(sizeof(TempoFieldV1));
  model.diagnostics_bytes =
      static_cast<std::uint32_t>(sizeof(contract::TempoFieldDiagnosticsV1));
  model.publisher_state_bytes =
      static_cast<std::uint32_t>(sizeof(TempoFieldPublisher));
  return model;
}

TempoFieldPublisher::TempoFieldPublisher() noexcept {
  configure(TempoFieldConfig{});
}

void TempoFieldPublisher::configure(const TempoFieldConfig& requested) noexcept {
  const bool had_evidence = have_epoch_;
  const bool rate_changed =
      requested.rate.sample_rate_hz != config_.rate.sample_rate_hz ||
      requested.rate.hop_samples != config_.rate.hop_samples;
  TempoFieldConfig next = requested;
  if (next.max_selected == 0U || next.max_selected > kTempoFieldMaxSelected) {
    next.max_selected = static_cast<std::uint8_t>(kTempoFieldMaxSelected);
  }
  if (next.activity_window_updates == 0U ||
      next.activity_window_updates > kHistory) {
    next.activity_window_updates = TempoFieldConfig{}.activity_window_updates;
  }
  if (next.warmup_updates == 0U || next.warmup_updates > kHistory) {
    next.warmup_updates = TempoFieldConfig{}.warmup_updates;
  }
  if (!std::isfinite(next.activity_floor) || next.activity_floor < 0.0F) {
    next.activity_floor = TempoFieldConfig{}.activity_floor;
  }
  if (!supportedRate(next.rate)) {
    // Without an exact media span per hop no support time can be stamped.
    next.publication_enabled = false;
  }
  config_ = next;
  selection_ = makeTempoSelectionConfig(
      config_.admission_salience, config_.retention_salience,
      config_.replacement_margin, config_.separation_cents,
      config_.max_selected);
  tracks_.configure(config_.phase, config_.limits);
  if (had_evidence && rate_changed) {
    history_predates_epoch_ = true;
    invalidateEvidence(TempoFieldReason::kConfigurationChanged);
    return;
  }
  publishUnavailable(config_.publication_enabled
                         ? (generation_ == 0U
                                ? TempoFieldReason::kNoEvidence
                                : TempoFieldReason::kConfigurationChanged)
                         : TempoFieldReason::kDisabled);
}

void TempoFieldPublisher::setEnabled(const bool enabled) noexcept {
  if (enabled && !supportedRate(config_.rate)) return;
  if (enabled == config_.publication_enabled) return;
  config_.publication_enabled = enabled;
  // Identities never survive a disabled interval: re-enabled tracks reacquire
  // as new generations instead of replaying stale phase.
  tracks_.reset();
  publishUnavailable(enabled ? TempoFieldReason::kNoEvidence
                             : TempoFieldReason::kDisabled);
}

void TempoFieldPublisher::setPhaseEnabled(const bool enabled) noexcept {
  config_.phase_enabled = enabled;
}

void TempoFieldPublisher::noteIncumbentReset() noexcept {
  tracks_.reset();
  bank_valid_.fill(false);
  have_calculation_bin_ = false;
  have_last_update_ = false;
  updates_in_epoch_ = 0U;
  regular_samples_ = 0U;
  history_predates_epoch_ = false;
  if (config_.publication_enabled) publishUnavailable(TempoFieldReason::kReset);
}

void TempoFieldPublisher::invalidateEvidence(
    const TempoFieldReason reason) noexcept {
  tracks_.reset();
  bank_valid_.fill(false);
  have_last_update_ = false;
  updates_in_epoch_ = 0U;
  regular_samples_ = 0U;
  if (config_.publication_enabled) {
    publishUnavailable(reason);
  } else {
    publishUnavailable(TempoFieldReason::kDisabled);
  }
}

void TempoFieldPublisher::fillStaticHeader(TempoFieldV1& field) const noexcept {
  const std::uint16_t hop_frames = hopMediaFrames(config_.rate);
  field.configuration_revision = config_.configuration_revision;
  field.analysis_rate_hz = config_.rate.sample_rate_hz;
  field.hop_samples = config_.rate.hop_samples;
  field.hop_media_frames = hop_frames;
  field.novelty_interval_media_frames =
      static_cast<std::uint32_t>(hop_frames) * kProductionNoveltyDecimation;
  field.fresh_limit_media_frames = tracks_.limits().fresh_limit_media_frames;
  field.coast_limit_media_frames = tracks_.limits().coast_limit_media_frames;
  field.stale_limit_media_frames = tracks_.limits().stale_limit_media_frames;
  field.warmup_updates = config_.warmup_updates;
  field.activity_floor = config_.activity_floor;
  field.canonical.time_base =
      static_cast<std::uint8_t>(config_.canonical_time_base);
}

void TempoFieldPublisher::publishUnavailable(
    const TempoFieldReason reason) noexcept {
  TempoFieldV1& field = building_;
  field = kEmptyField;
  fillStaticHeader(field);
  if (config_.publication_enabled) {
    field.flags = contract::kTempoFieldEnabled |
                  (config_.phase_enabled ? contract::kTempoFieldPhaseEnabled
                                         : 0U);
  }
  field.reason = static_cast<std::uint8_t>(reason);
  field.stream_epoch = epoch_;
  field.generation = generation_;
  field.updates_in_epoch = updates_in_epoch_;
  field.overflow_drops = counters_.overflow_drops;
  published_ = field;
}

bool TempoFieldPublisher::bankObserved(const std::uint8_t bin,
                                       std::uint64_t& media_frame) const noexcept {
  if (bin >= kTempoFieldBankBinCount || !bank_valid_[bin]) return false;
  media_frame = bank_observed_[bin];
  return true;
}

bool TempoFieldPublisher::observe(const TempoTrackerState& incumbent,
                                  const TempoFieldHop& hop) noexcept {
  ++counters_.hops_observed;
  const std::uint16_t calculation_bin = static_cast<std::uint16_t>(
      incumbent.calculation_bin % kTempoFieldBankBinCount);
  const auto recomputed = [&](const std::uint16_t back) noexcept {
    return static_cast<std::uint8_t>(
        (calculation_bin + kTempoFieldBankBinCount - back) %
        kTempoFieldBankBinCount);
  };
  const auto forgetRecomputedBins = [&]() noexcept {
    // The incumbent recomputed two bins at a time this sidecar cannot stamp:
    // their latest observation time is unknown, not the older stamp.
    bank_valid_[recomputed(1U)] = false;
    bank_valid_[recomputed(2U)] = false;
    last_calculation_bin_ = calculation_bin;
    have_calculation_bin_ = true;
  };

  if (!hop.media_time_valid) {
    if (incumbent.event.updated) {
      ++counters_.updates_processed;
      saturatingIncrement(counters_.invalid_time_events);
      forgetRecomputedBins();
      have_last_update_ = false;
      regular_samples_ = 0U;
      if (config_.publication_enabled) {
        publishUnavailable(TempoFieldReason::kInvalidTime);
      }
    }
    return false;
  }
  const AudioTime now = hop.media_time;
  if (!have_epoch_ || now.epoch_id != epoch_) {
    const bool had_epoch = have_epoch_;
    have_epoch_ = true;
    epoch_ = now.epoch_id;
    last_hop_frame_ = now.frame_index;
    if (had_epoch) {
      history_predates_epoch_ = true;
      invalidateEvidence(TempoFieldReason::kEpochChanged);
    }
  } else if (now.frame_index <= last_hop_frame_) {
    saturatingIncrement(counters_.invalid_time_events);
    last_hop_frame_ = now.frame_index;
    history_predates_epoch_ = true;
    invalidateEvidence(TempoFieldReason::kInvalidTime);
  } else {
    last_hop_frame_ = now.frame_index;
  }
  if (!incumbent.event.updated) return false;  // cached hop: nothing new

  ++counters_.updates_processed;
  const AudioTime evidence_end =
      hop.evidence_stamp_valid ? hop.evidence_end : now;
  if (evidence_end.epoch_id != epoch_ ||
      evidence_end.frame_index > now.frame_index ||
      (have_last_update_ && evidence_end.frame_index <= last_update_end_)) {
    // A completed job whose support is in another epoch, in the future or not
    // after the previous support is discarded, never republished.
    saturatingIncrement(counters_.discarded_jobs);
    forgetRecomputedBins();
    if (config_.publication_enabled) {
      publishUnavailable(TempoFieldReason::kDiscardedJob);
    }
    return false;
  }
  const std::uint32_t interval =
      static_cast<std::uint32_t>(hopMediaFrames(config_.rate)) *
      kProductionNoveltyDecimation;
  if (have_last_update_ && interval != 0U &&
      evidence_end.frame_index - last_update_end_ == interval) {
    if (regular_samples_ < kHistory) ++regular_samples_;
  } else {
    regular_samples_ = 1U;
  }
  last_update_end_ = evidence_end.frame_index;
  have_last_update_ = true;
  saturatingIncrement(updates_in_epoch_);

  if (!have_calculation_bin_ ||
      (last_calculation_bin_ + 2U) % kTempoFieldBankBinCount != calculation_bin) {
    bank_valid_.fill(false);  // unseen bank steps: every other age is unknown
  }
  for (std::uint16_t back = 1U; back <= 2U; ++back) {
    bank_observed_[recomputed(back)] = evidence_end.frame_index;
    bank_valid_[recomputed(back)] = true;
  }
  last_calculation_bin_ = calculation_bin;
  have_calculation_bin_ = true;

  if (!config_.publication_enabled) return false;
  const bool timed = hook_.now_ticks != nullptr;
  const std::uint32_t started = timed ? hook_.now_ticks(hook_.context) : 0U;
  const bool published = buildPublication(incumbent, hop, evidence_end);
  if (timed) {
    const std::uint32_t finished = hook_.now_ticks(hook_.context);
    recordMax(counters_.stage_ticks_last, counters_.stage_ticks_max,
              finished - started);
  }
  return published;
}

bool TempoFieldPublisher::buildPublication(const TempoTrackerState& incumbent,
                                           const TempoFieldHop& hop,
                                           const AudioTime& evidence_end) noexcept {
  TempoFieldV1& field = building_;
  field = kEmptyField;
  fillStaticHeader(field);
  bool invalid = false;

  const std::uint32_t fill = incumbent.confidence_updates < kHistory
                                 ? incumbent.confidence_updates
                                 : kHistory;
  std::uint32_t in_epoch = fill;
  if (history_predates_epoch_ && updates_in_epoch_ < in_epoch) {
    in_epoch = updates_in_epoch_;
  }
  if (in_epoch == 0U) in_epoch = 1U;  // this completed update, at least
  const std::uint16_t usable = static_cast<std::uint16_t>(
      regular_samples_ < in_epoch ? regular_samples_ : in_epoch);
  const std::uint64_t interval = field.novelty_interval_media_frames;
  const std::uint64_t span = interval * in_epoch;

  ++generation_;
  field.stream_epoch = epoch_;
  field.generation = generation_;
  field.support_end_media_frame = evidence_end.frame_index;
  field.support_start_media_frame = evidence_end.frame_index > span
                                        ? evidence_end.frame_index - span
                                        : 0U;
  field.result_available_us = hop.result_available_us;
  field.updates_in_epoch = updates_in_epoch_;

  const std::uint16_t newest = static_cast<std::uint16_t>(
      (incumbent.history_index + kHistory - 1U) % kHistory);
  const std::uint32_t window =
      config_.activity_window_updates < in_epoch ? config_.activity_window_updates
                                                 : in_epoch;
  double activity_sum = 0.0;
  for (std::uint32_t k = 0U; k < window; ++k) {
    const std::uint16_t index =
        static_cast<std::uint16_t>((newest + kHistory - k) % kHistory);
    activity_sum += static_cast<double>(
        evidence(incumbent.novelty_history[index], invalid));
  }
  field.activity =
      static_cast<float>(activity_sum / static_cast<double>(window));
  field.latest_novelty = evidence(incumbent.novelty_history[newest], invalid);

  std::array<float, kTempoFieldBankBinCount> comb{};
  float comb_sum = 0.0F;
  float comb_max = 0.0F;
  for (std::size_t bin = 0U; bin < kTempoFieldBankBinCount; ++bin) {
    comb[bin] = evidence(incumbent.acf.comb_salience[bin], invalid);
    comb_sum += comb[bin];
    if (comb[bin] > comb_max) comb_max = comb[bin];
  }
  field.concentration = comb_sum > 0.0F ? comb_max / comb_sum : 0.0F;
  if (field.concentration > 1.0F) field.concentration = 1.0F;

  // Canonical clock, described read-only.
  const std::uint8_t winner = static_cast<std::uint8_t>(
      incumbent.winner_bin < kTempoFieldBankBinCount
          ? incumbent.winner_bin
          : kTempoFieldBankBinCount / 2U);
  contract::TempoCanonicalClockV1& canonical = field.canonical;
  canonical.winner_bin = winner;
  canonical.winner_target_bpm =
      evidence(incumbent.bins[winner].target_bpm, invalid);
  canonical.running_bpm = evidence(incumbent.flywheel_run_bpm, invalid);
  canonical.phase01 = evidence(incumbent.event.phase01, invalid);
  canonical.confidence = evidence(incumbent.event.confidence, invalid);
  canonical.beat_strength = evidence(incumbent.event.beat_strength, invalid);
  canonical.coast_updates_remaining = incumbent.coast_updates_remaining;
  canonical.tracker_reference_ms = incumbent.last_emit_ms;
  std::uint32_t flags = contract::kTempoFieldEnabled;
  if (config_.canonical_time_base ==
      contract::TempoCanonicalTimeBase::kTrackerMediaMs) {
    // frame_ms = floor(media_frame / 48): the reference lies in
    // [48 ms, 48 ms + 47] media frames of this epoch.
    canonical.media_reference_frame =
        static_cast<std::uint64_t>(incumbent.last_emit_ms) *
        (contract::kTempoFieldMediaRateHz / 1000U);
    canonical.media_uncertainty_frames =
        contract::kTempoFieldMediaRateHz / 1000U - 1U;
    flags |= contract::kTempoFieldCanonicalMediaProjection;
  }
  if (incumbent.event.locked) flags |= contract::kTempoFieldCanonicalLocked;
  if (!incumbent.locked_v2 && incumbent.coast_updates_remaining > 0U) {
    flags |= contract::kTempoFieldCanonicalCoasting;
  }
  if (incumbent.event.beat_tick) flags |= contract::kTempoFieldCanonicalBeatTick;

  const bool silence = incumbent.silence_detected;
  const bool activity_valid = std::isfinite(field.activity) &&
                              field.activity >= config_.activity_floor &&
                              field.activity > 0.0F;
  const bool warm = in_epoch >= config_.warmup_updates;
  if (config_.phase_enabled) flags |= contract::kTempoFieldPhaseEnabled;
  if (silence) flags |= contract::kTempoFieldSilence;
  if (hop.input_silence) flags |= contract::kTempoFieldInputSilence;
  if (activity_valid) flags |= contract::kTempoFieldActivityValid;
  if (incumbent.acf.valid) flags |= contract::kTempoFieldAcfValid;
  if (fill > in_epoch) flags |= contract::kTempoFieldSupportPreEpoch;
  if (regular_samples_ < in_epoch) flags |= contract::kTempoFieldIrregularSupport;
  if (hop.evidence_stamp_valid &&
      evidence_end.frame_index != hop.media_time.frame_index) {
    flags |= contract::kTempoFieldSlicedCompletion;
  }

  std::array<float, kTempoFieldBankBinCount> point{};
  for (std::size_t bin = 0U; bin < kTempoFieldBankBinCount; ++bin) {
    point[bin] = evidence(incumbent.acf.point_salience[bin], invalid);
  }
  if (invalid) saturatingIncrement(counters_.invalid_evidence_events);

  if (!warm || invalid) {
    if (!warm) flags |= contract::kTempoFieldWarmup;
    if (invalid) flags |= contract::kTempoFieldInvalidEvidence;
    field.flags = flags;
    field.reason = static_cast<std::uint8_t>(
        invalid ? TempoFieldReason::kInvalidEvidence : TempoFieldReason::kWarmup);
    field.coast_reason = static_cast<std::uint8_t>(
        invalid ? TempoCoastReason::kNone : TempoCoastReason::kWarmup);
    field.overflow_drops = counters_.overflow_drops;
    published_ = field;
    ++counters_.publications;
    return true;
  }

  std::array<std::uint8_t, kTempoFieldMaxSelected> previous{};
  const std::uint8_t previous_count = tracks_.selectedBins(previous);
  const bool allow_admission = incumbent.acf.valid && activity_valid &&
                               !silence && !hop.input_silence;
  const TempoSelectionResult selection = selectTempoCandidates(
      comb.data(), previous.data(), previous_count, allow_admission, selection_);
  recordMax(counters_.selection_steps_last, counters_.selection_steps_max,
            selection.steps);

  const bool observe_phase =
      config_.phase_enabled && activity_valid && !silence &&
      !hop.input_silence && usable >= tracks_.config().minimum_window_samples;
  TempoCoastReason coast_reason = TempoCoastReason::kNone;
  if (!config_.phase_enabled) {
    coast_reason = TempoCoastReason::kPhaseSuspended;
  } else if (silence || hop.input_silence) {
    coast_reason = TempoCoastReason::kSilence;
  } else if (!activity_valid) {
    coast_reason = TempoCoastReason::kLowActivity;
  } else if (usable < tracks_.config().minimum_window_samples) {
    coast_reason = TempoCoastReason::kWarmup;
  }

  ObserverContext observer{};
  observer.view.ring = incumbent.novelty_history.data();
  observer.view.ring_length = kHistory;
  observer.view.newest_index = newest;
  observer.view.usable_samples = usable;
  observer.view.interval_media_frames = field.novelty_interval_media_frames;
  observer.window = tracks_.config().window_samples;
  TempoTrackUpdate update{};
  update.epoch_id = epoch_;
  update.reference_media_frame = evidence_end.frame_index;
  update.selected_bins = selection.bins.data();
  update.selected_count = selection.count;
  update.observe_phase = observe_phase;
  update.observer = &observeFromHistory;
  update.observer_context = &observer;
  const TempoTrackUpdateResult result = tracks_.update(update);
  recordMax(counters_.fresh_evaluations_last, counters_.fresh_evaluations_max,
            result.fresh_evaluations);
  recordMax(counters_.observer_samples_last, counters_.observer_samples_max,
            observer.samples);
  counters_.evictions += result.evictions;
  if (observe_phase && coast_reason == TempoCoastReason::kNone) {
    if (result.low_coherence > 0U) {
      coast_reason = TempoCoastReason::kLowCoherence;
    } else if (result.rejected > result.reinitialisations) {
      coast_reason = TempoCoastReason::kCorrectionExceeded;
    }
  }

  // Presentation order: selected by evidence (then bin), derived, retiring by
  // recency (then bin). Identity is bin + generation, never this order.
  const auto& slots = tracks_.slots();
  std::array<std::uint8_t, contract::kTempoFieldCandidateCapacity> selected_order{};
  std::uint8_t selected_count = 0U;
  std::array<std::uint8_t, contract::kTempoFieldCandidateCapacity> retiring_order{};
  std::uint8_t retiring_count = 0U;
  const auto reference = [&](const TempoTrackSlot& slot) noexcept {
    return slot.phase_valid ? slot.phase_observed_media_frame
                            : slot.last_selected_media_frame;
  };
  for (std::uint8_t index = 0U; index < slots.size(); ++index) {
    const TempoTrackSlot& slot = slots[index];
    if (!slot.occupied) continue;
    if (slot.admission == TempoCandidateAdmission::kSelected) {
      std::uint8_t position = selected_count;
      while (position > 0U) {
        const TempoTrackSlot& before = slots[selected_order[position - 1U]];
        const bool earlier =
            comb[slot.bin] > comb[before.bin] ||
            (comb[slot.bin] == comb[before.bin] && slot.bin < before.bin);
        if (!earlier) break;
        selected_order[position] = selected_order[position - 1U];
        --position;
      }
      selected_order[position] = index;
      ++selected_count;
    } else if (slot.admission == TempoCandidateAdmission::kRetiring) {
      std::uint8_t position = retiring_count;
      while (position > 0U) {
        const TempoTrackSlot& before = slots[retiring_order[position - 1U]];
        const bool earlier =
            reference(slot) > reference(before) ||
            (reference(slot) == reference(before) && slot.bin < before.bin);
        if (!earlier) break;
        retiring_order[position] = retiring_order[position - 1U];
        --position;
      }
      retiring_order[position] = index;
      ++retiring_count;
    }
  }

  float total = 0.0F;
  for (const float value : comb) total += value;
  const auto fillObserved = [&](TempoCandidateV1& candidate,
                                const TempoTrackSlot& slot) noexcept {
    const std::uint8_t bin = slot.bin;
    candidate.salience_media_frame = evidence_end.frame_index;
    std::uint64_t observed = 0U;
    if (bankObserved(bin, observed)) {
      candidate.bank_observed_media_frame = observed;
      candidate.flags |= contract::kCandidateBankObserved;
    }
    candidate.admitted_media_frame = slot.admitted_media_frame;
    candidate.track_generation = slot.generation;
    candidate.family_root_generation = slot.generation;
    candidate.family_root_bin = bin;
    candidate.comb_salience = comb[bin];
    candidate.point_salience = point[bin];
    float lobe = 0.0F;
    int lobe_count = 0;
    for (int other = static_cast<int>(bin) - kProminenceLobe;
         other <= static_cast<int>(bin) + kProminenceLobe; ++other) {
      if (other < 0 || other >= static_cast<int>(kTempoFieldBankBinCount)) {
        continue;
      }
      lobe += comb[static_cast<std::size_t>(other)];
      ++lobe_count;
    }
    const int outside = static_cast<int>(kTempoFieldBankBinCount) - lobe_count;
    const float background =
        outside > 0 ? (total - lobe) / static_cast<float>(outside) : 0.0F;
    const float prominence = comb[bin] - background;
    candidate.prominence =
        std::isfinite(prominence) && prominence > 0.0F ? prominence : 0.0F;
    candidate.bank_smoothed = evidence(incumbent.smoothed[bin], invalid);
    candidate.bin_id = bin;
    candidate.admission = static_cast<std::uint8_t>(slot.admission);
    candidate.state = static_cast<std::uint8_t>(slot.state);
    if (bin == winner) candidate.flags |= contract::kCandidateCanonicalWinner;
    candidate.running_bpm = runningBpm(slot.anchor.beat_period_q32);
    if (slot.phase_valid) {
      candidate.anchor = slot.anchor;
      candidate.phase_observed_media_frame = slot.phase_observed_media_frame;
      candidate.flags |= contract::kCandidatePhaseValid;
      float coherence = slot.coherence;
      if (!std::isfinite(coherence) || coherence < 0.0F) coherence = 0.0F;
      if (coherence > 1.0F) coherence = 1.0F;
      candidate.phase_coherence = coherence;
      candidate.phase_innovation_cycles =
          finiteOrZero(slot.innovation, invalid);
      candidate.applied_correction_cycles =
          finiteOrZero(slot.applied_correction, invalid);
      if (slot.innovation_count >= kTempoTrackUncertaintyMinimum) {
        candidate.phase_uncertainty_cycles = innovationRms(slot);
        candidate.flags |= contract::kCandidateUncertaintyKnown;
      }
    } else {
      candidate.anchor.epoch_id = epoch_;
    }
    if (slot.correction_clamped) {
      candidate.flags |= contract::kCandidateCorrectionClamped;
    }
    if (slot.frequency_clamped) {
      candidate.flags |= contract::kCandidateFrequencyClamped;
    }
    if (slot.reinitialised) candidate.flags |= contract::kCandidateReinitialised;
    if (slot.fresh_this_update) {
      candidate.flags |= contract::kCandidateFreshThisUpdate;
    }
  };

  std::uint8_t count = 0U;
  std::uint8_t primary_slot = kTempoFieldNoIndex;
  for (std::uint8_t order = 0U; order < selected_count; ++order) {
    const TempoTrackSlot& slot = slots[selected_order[order]];
    fillObserved(field.candidates[count], slot);
    const bool eligible = slot.phase_valid &&
                          (slot.state == TempoTrackState::kObserved ||
                           slot.state == TempoTrackState::kCoasting);
    if (eligible && (field.primary_index == kTempoFieldNoIndex ||
                     (slot.bin == winner &&
                      field.candidates[field.primary_index].bin_id != winner))) {
      field.primary_index = count;
      primary_slot = selected_order[order];
    }
    ++count;
  }
  field.selected_count = selected_count;
  if (primary_slot != kTempoFieldNoIndex) {
    const TempoTrackSlot& root = slots[primary_slot];
    const TempoPhaseTrackConfig& phase = tracks_.config();
    for (std::uint8_t ratio = 0U;
         ratio < phase.derived_ratio_count && count < kTempoFieldCandidateCapacity;
         ++ratio) {
      TempoCandidateV1& derived = field.candidates[count++];
      derived.anchor = root.anchor;
      derived.salience_media_frame = evidence_end.frame_index;
      derived.phase_observed_media_frame = root.phase_observed_media_frame;
      derived.admitted_media_frame = root.admitted_media_frame;
      derived.track_generation = root.generation;
      derived.family_root_generation = root.generation;
      derived.family_root_bin = root.bin;
      derived.family_ratio_numerator = phase.derived_numerator[ratio];
      derived.family_ratio_denominator = phase.derived_denominator[ratio];
      derived.projection_ratio_numerator = phase.derived_numerator[ratio];
      derived.projection_ratio_denominator = phase.derived_denominator[ratio];
      derived.running_bpm = runningBpm(root.anchor.beat_period_q32) *
                            static_cast<float>(phase.derived_numerator[ratio]) /
                            static_cast<float>(phase.derived_denominator[ratio]);
      derived.bin_id = kTempoFieldNoBin;
      derived.admission = static_cast<std::uint8_t>(TempoCandidateAdmission::kDerived);
      // Derived arithmetic inherits its root's freshness but is never an
      // observation of its own: kDerived, or the root's coasting state.
      derived.state = static_cast<std::uint8_t>(
          root.state == TempoTrackState::kObserved ? TempoTrackState::kDerived
                                                   : root.state);
      derived.flags = contract::kCandidatePhaseValid |
                      contract::kCandidateFamilyRelated;
    }
  }
  for (std::uint8_t order = 0U; order < retiring_count; ++order) {
    if (count >= kTempoFieldCandidateCapacity) {
      saturatingIncrement(counters_.overflow_drops);
      continue;
    }
    fillObserved(field.candidates[count++], slots[retiring_order[order]]);
  }
  field.candidate_count = count;

  // Metrical-family annotations between observed entries and the primary.
  // Interpretation only: entries keep their own anchors and observed state.
  if (field.primary_index != kTempoFieldNoIndex) {
    const TempoCandidateV1& root = field.candidates[field.primary_index];
    const std::uint32_t root_bpm =
        contract::kTempoFieldBankLowBpm + static_cast<std::uint32_t>(root.bin_id);
    for (std::uint8_t index = 0U; index < count; ++index) {
      TempoCandidateV1& candidate = field.candidates[index];
      if (index == field.primary_index ||
          candidate.admission ==
              static_cast<std::uint8_t>(TempoCandidateAdmission::kDerived)) {
        continue;
      }
      const std::uint32_t bpm = contract::kTempoFieldBankLowBpm +
                                static_cast<std::uint32_t>(candidate.bin_id);
      for (std::size_t ratio = 0U; ratio < sizeof(kFamilyNumerator); ++ratio) {
        const std::uint32_t lhs = bpm * kFamilyDenominator[ratio];
        const std::uint32_t rhs = root_bpm * kFamilyNumerator[ratio];
        const std::uint32_t difference = lhs > rhs ? lhs - rhs : rhs - lhs;
        if (difference * 1000U <= 15U * rhs) {  // within 1.5 %
          candidate.family_root_bin = root.bin_id;
          candidate.family_root_generation = root.track_generation;
          candidate.family_ratio_numerator = kFamilyNumerator[ratio];
          candidate.family_ratio_denominator = kFamilyDenominator[ratio];
          candidate.flags |= contract::kCandidateFamilyRelated;
          break;
        }
      }
    }
  }

  bool authority = false;
  bool coasting = false;
  for (std::uint8_t index = 0U; index < count; ++index) {
    const TempoCandidateV1& candidate = field.candidates[index];
    authority = authority ||
                candidate.state == static_cast<std::uint8_t>(TempoTrackState::kObserved);
    coasting = coasting ||
               (candidate.admission ==
                    static_cast<std::uint8_t>(TempoCandidateAdmission::kSelected) &&
                candidate.state ==
                    static_cast<std::uint8_t>(TempoTrackState::kCoasting));
  }
  if (authority) flags |= contract::kTempoFieldPhaseAuthority;
  if (coasting) flags |= contract::kTempoFieldCoasting;
  if (invalid || selection.invalid_input) {
    flags |= contract::kTempoFieldInvalidEvidence;
  } else {
    flags |= contract::kTempoFieldValid;
  }
  field.flags = flags;
  field.reason = static_cast<std::uint8_t>(
      (flags & contract::kTempoFieldValid) != 0U
          ? TempoFieldReason::kAvailable
          : TempoFieldReason::kInvalidEvidence);
  field.coast_reason = static_cast<std::uint8_t>(
      coasting || !observe_phase ? coast_reason : TempoCoastReason::kNone);
  field.overflow_drops = counters_.overflow_drops;
  published_ = field;
  ++counters_.publications;
  return true;
}

bool TempoFieldPublisher::captureDiagnostics(
    const TempoTrackerState& incumbent,
    contract::TempoFieldDiagnosticsV1& out) const noexcept {
  if (!config_.publication_enabled || generation_ == 0U) return false;
  out = contract::TempoFieldDiagnosticsV1{};
  out.stream_epoch = published_.stream_epoch;
  out.generation = published_.generation;
  out.support_end_media_frame = published_.support_end_media_frame;
  out.flags = published_.flags;
  bool invalid = false;
  for (std::size_t bin = 0U; bin < kTempoFieldBankBinCount; ++bin) {
    out.bank_observed_valid[bin] = bank_valid_[bin] ? 1U : 0U;
    out.bank_observed_media_frame[bin] = bank_valid_[bin] ? bank_observed_[bin] : 0U;
    out.comb_salience[bin] = evidence(incumbent.acf.comb_salience[bin], invalid);
    out.point_salience[bin] = evidence(incumbent.acf.point_salience[bin], invalid);
    out.bank_smoothed[bin] = evidence(incumbent.smoothed[bin], invalid);
  }
  if (invalid) out.flags |= contract::kTempoFieldInvalidEvidence;
  return true;
}

}  // namespace k1::core::audio
