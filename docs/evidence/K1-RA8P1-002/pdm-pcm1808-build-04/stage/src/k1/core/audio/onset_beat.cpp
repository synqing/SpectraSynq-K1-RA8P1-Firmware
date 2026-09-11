#include "core/audio/onset_beat.h"

#include <cmath>
#include <limits>

namespace k1::core::audio {
namespace {

// Production K1_ONSET_V2 source snapshot:
//   SpectraSynq_K1_Firmware@c3e13ffdf1a3f8174b2c1dd3bf0af39dbe0c7b4c
//   SPECTRASYNQ_K1_FIRMWARE/audio/k1_onset_beat.cpp
//   SHA-256 b973d1bf4663bfd13a368b222659f6c4340c45a9d981b07f2e9b2aecdd249e13

constexpr std::uint8_t kTransientLow = 1;
constexpr std::uint8_t kTransientHigh = 76;
constexpr std::uint8_t kKickLow = 1;
constexpr std::uint8_t kKickHigh = 25;
constexpr std::uint8_t kSnareLow = 25;
constexpr std::uint8_t kSnareHigh = 50;
constexpr std::uint8_t kHihatLow = 70;
constexpr std::uint8_t kHihatHigh = 80;
constexpr std::uint8_t kMedianWindow = 14;
constexpr float kThresholdMultiplier = 1.6F;
constexpr float kThresholdOffset = 0.05F;
constexpr float kThresholdFloor = 0.02F;
constexpr std::uint8_t kPreMaximum = 4;
constexpr std::uint8_t kPeakWait = 4;
constexpr std::uint8_t kKickRefractory = 6;
constexpr std::uint8_t kSnareRefractory = 5;
constexpr std::uint8_t kHihatRefractory = 3;
constexpr float kKickAlpha = 0.0141F;
constexpr float kSnareAlpha = 0.0094F;
constexpr float kHihatAlpha = 0.0047F;
constexpr float kKickThreshold = 0.8F;
constexpr float kSnareThreshold = 1.0F;
constexpr float kHihatThreshold = 1.2F;
constexpr std::uint32_t kWarmupFrames = 14;
constexpr float kActivityFloor = 0.004F;
constexpr float kQuietSpectralFloor = 0.08F;
constexpr float kQuietNoveltyFloor = 0.08F;
constexpr float kEpsilon = 1.0e-6F;
constexpr float kLevelDecay = 0.86F;
constexpr std::uint32_t kEventWindowMs = 80;
constexpr std::uint32_t kBeatIntervalMinimumMs = 300;
constexpr std::uint32_t kBeatIntervalMaximumMs = 1000;
constexpr std::uint8_t kIntervalToleranceDivisor = 4;
constexpr std::uint8_t kStableIntervalMaximum = 4;

float clamp(float value, float low, float high) noexcept {
    if (!std::isfinite(value)) return low;
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

float alpha(std::uint32_t delta_ms, float time_constant_ms) noexcept {
    if (time_constant_ms <= 0.0F) return 1.0F;
    const float delta = static_cast<float>(delta_ms);
    return clamp(delta / (time_constant_ms + delta), 0.0F, 1.0F);
}

std::uint32_t eventAge(std::uint32_t now_ms, std::uint32_t event_ms) noexcept {
    if (event_ms == 0 || now_ms < event_ms) {
        return std::numeric_limits<std::uint32_t>::max();
    }
    return now_ms - event_ms;
}

bool intervalClose(std::uint32_t interval_ms,
                   std::uint32_t reference_ms) noexcept {
    if (interval_ms == 0 || reference_ms == 0) return false;
    const std::uint32_t bigger =
        interval_ms > reference_ms ? interval_ms : reference_ms;
    const std::uint32_t smaller =
        interval_ms > reference_ms ? reference_ms : interval_ms;
    return bigger - smaller <= bigger / kIntervalToleranceDivisor;
}

bool silence(const contract::AudioFeaturesV1& audio) noexcept {
    return (audio.event_flags & contract::kEventSilence) != 0U;
}

}  // namespace

void OnsetBeat::reset() noexcept {
    detector_ = {};
    event_ = {};
    onset_last_ms_ = 0;
    last_accept_ms_ = 0;
    last_interval_ms_ = 0;
    interval_estimate_ms_ = 0;
    stable_intervals_ = 0;
    novelty_fast_ = novelty_slow_ = 0.0F;
    low_fast_ = low_slow_ = 0.0F;
    peak_fast_ = peak_slow_ = 0.0F;
    previous_novelty_ = previous_low_energy_ = previous_peak_ = 0.0F;
    primed_ = false;
}

float OnsetBeat::bandFlux(const float* spectrum,
                          const float* previous,
                          std::uint8_t low,
                          std::uint8_t high) const noexcept {
    float flux = 0.0F;
    for (std::uint8_t bin = low; bin < high; ++bin) {
        const float difference =
            std::log(std::fmax(kEpsilon, spectrum[bin])) -
            std::log(std::fmax(kEpsilon, previous[bin]));
        if (difference > 0.0F) flux += difference;
    }
    return flux;
}

float OnsetBeat::median(const float* ring, std::uint8_t count) const noexcept {
    if (count == 0) return 0.0F;
    float sorted[16] = {};
    for (std::uint8_t index = 0; index < count; ++index) sorted[index] = ring[index];
    for (std::uint8_t index = 1; index < count; ++index) {
        const float key = sorted[index];
        int previous = static_cast<int>(index) - 1;
        while (previous >= 0 && sorted[previous] > key) {
            sorted[previous + 1] = sorted[previous];
            --previous;
        }
        sorted[previous + 1] = key;
    }
    return (count & 1U) != 0U
               ? sorted[count / 2U]
               : 0.5F * (sorted[count / 2U - 1U] + sorted[count / 2U]);
}

float OnsetBeat::thresholdEnvelope(float flux) noexcept {
    detector_.flux_ring[detector_.flux_write_index] = flux;
    detector_.flux_write_index =
        static_cast<std::uint8_t>((detector_.flux_write_index + 1U) % 16U);
    if (detector_.flux_count < 16U) ++detector_.flux_count;
    const std::uint8_t window = detector_.flux_count < kMedianWindow
                                    ? detector_.flux_count
                                    : kMedianWindow;
    const float threshold = std::fmax(
        median(detector_.flux_ring.data(), window) * kThresholdMultiplier +
            kThresholdOffset,
        kThresholdFloor);
    const float envelope = flux - threshold;
    return envelope > 0.0F ? envelope : 0.0F;
}

float OnsetBeat::peakPick(float envelope, bool emit) noexcept {
    const std::uint8_t candidate = detector_.envelope_write_index;
    detector_.envelope_ring[candidate] = envelope;
    detector_.envelope_write_index = static_cast<std::uint8_t>(
        (detector_.envelope_write_index + 1U) % 16U);
    if (!emit || envelope <= 0.0F || detector_.frame_count < kPreMaximum) {
        return 0.0F;
    }
    for (std::uint8_t offset = 1; offset <= kPreMaximum; ++offset) {
        const int index =
            (static_cast<int>(candidate) - offset + 16) % 16;
        if (detector_.envelope_ring[static_cast<std::size_t>(index)] >= envelope) {
            return 0.0F;
        }
    }
    if (detector_.frame_count - detector_.last_event_frame < kPeakWait) {
        return 0.0F;
    }
    detector_.last_event_frame = detector_.frame_count;
    return envelope;
}

float OnsetBeat::bandTrigger(BandState& state,
                             float flux,
                             float threshold_multiplier,
                             float update_alpha,
                             std::uint8_t refractory_frames,
                             bool emit) noexcept {
    state.flux_mean += update_alpha * (flux - state.flux_mean);
    const float threshold = state.flux_mean * (1.0F + threshold_multiplier);
    const bool local_maximum =
        state.previous_flux > state.previous2_flux &&
        state.previous_flux > flux;
    const bool above = state.previous_flux > threshold;
    const bool refractory_open =
        detector_.frame_count - state.last_trigger_frame >= refractory_frames;
    const float fired_flux = state.previous_flux;
    state.previous2_flux = state.previous_flux;
    state.previous_flux = flux;
    if (!emit) return 0.0F;
    if (local_maximum && above && refractory_open) {
        state.last_trigger_frame = detector_.frame_count;
        return fired_flux;
    }
    return 0.0F;
}

bool OnsetBeat::runV2(const contract::AudioFeaturesV1& audio,
                      OnsetBeatFrame& event) noexcept {
    const float* spectrum = audio.spectrum;
    if (!detector_.has_previous || silence(audio)) {
        if (silence(audio)) {
            detector_.has_previous = false;
        } else {
            for (std::uint8_t bin = 0; bin < contract::kSpectrumBinCount; ++bin) {
                detector_.previous_spectrum[bin] = spectrum[bin];
            }
            detector_.has_previous = true;
        }
        ++detector_.frame_count;
        detector_.transient_level *= kLevelDecay;
        detector_.kick_level *= kLevelDecay;
        detector_.snare_level *= kLevelDecay;
        detector_.hihat_level *= kLevelDecay;
        event.transient = event.kick = event.snare = event.hihat = false;
        event.transient_strength = event.kick_strength =
            event.snare_strength = event.hihat_strength = 0.0F;
        event.transient_level = detector_.transient_level;
        event.kick_level = detector_.kick_level;
        event.snare_level = detector_.snare_level;
        event.hihat_level = detector_.hihat_level;
        event.transient_event_id = detector_.transient_id;
        event.kick_event_id = detector_.kick_id;
        event.snare_event_id = detector_.snare_id;
        event.hihat_event_id = detector_.hihat_id;
        return false;
    }

    std::uint8_t safe_high = audio.nyquist_safe_bin_hi;
    if (safe_high == 0 || safe_high > contract::kSpectrumBinCount) {
        safe_high = contract::kSpectrumBinCount;
    }
    const auto clamp_high = [safe_high](std::uint8_t low, std::uint8_t high) {
        if (safe_high < low) return low;
        return safe_high < high ? safe_high : high;
    };
    const std::uint8_t transient_high =
        clamp_high(kTransientLow, kTransientHigh);
    const std::uint8_t hihat_high = clamp_high(kHihatLow, kHihatHigh);

    const float full_flux = bandFlux(spectrum,
                                     detector_.previous_spectrum.data(),
                                     kTransientLow,
                                     transient_high);
    const float kick_flux = bandFlux(spectrum,
                                     detector_.previous_spectrum.data(),
                                     kKickLow,
                                     kKickHigh);
    const float snare_flux = bandFlux(spectrum,
                                      detector_.previous_spectrum.data(),
                                      kSnareLow,
                                      kSnareHigh);
    const float hihat_flux = bandFlux(spectrum,
                                      detector_.previous_spectrum.data(),
                                      kHihatLow,
                                      hihat_high);
    const auto normalise = [](float flux, std::uint8_t low, std::uint8_t high) {
        return high <= low ? 0.0F
                           : flux / static_cast<float>(high - low);
    };
    const float kick_normalised = normalise(kick_flux, kKickLow, kKickHigh);
    const float snare_normalised = normalise(snare_flux, kSnareLow, kSnareHigh);
    const float hihat_normalised = normalise(hihat_flux, kHihatLow, hihat_high);

    const bool warmup = detector_.frame_count < kWarmupFrames;
    const bool open_quiet = audio.spectral_energy < kQuietSpectralFloor &&
                            audio.novelty < kQuietNoveltyFloor;
    const bool inactive = audio.spectral_energy < kActivityFloor || open_quiet;
    const bool emit = !warmup && !inactive;
    const float onset_envelope = peakPick(thresholdEnvelope(full_flux), emit);
    const bool transient_fired = onset_envelope > 0.0F;
    const float kick_fire = bandTrigger(detector_.bass,
                                        kick_normalised,
                                        kKickThreshold,
                                        kKickAlpha,
                                        kKickRefractory,
                                        emit);
    const float snare_fire = bandTrigger(detector_.mid,
                                         snare_normalised,
                                         kSnareThreshold,
                                         kSnareAlpha,
                                         kSnareRefractory,
                                         emit);
    const float hihat_fire = bandTrigger(detector_.high,
                                         hihat_normalised,
                                         kHihatThreshold,
                                         kHihatAlpha,
                                         kHihatRefractory,
                                         emit);
    const bool kick_fired = kick_fire > 0.0F;
    const bool snare_fired = snare_fire > 0.0F;
    const bool hihat_fired = hihat_fire > 0.0F;
    const float transient_strength =
        onset_envelope > 1.0F ? 1.0F : onset_envelope;
    const float kick_strength =
        kick_fired ? (kick_fire > 1.0F ? 1.0F : kick_fire) : 0.0F;
    const float snare_strength =
        snare_fired ? (snare_fire > 1.0F ? 1.0F : snare_fire) : 0.0F;
    const float hihat_strength =
        hihat_fired ? (hihat_fire > 1.0F ? 1.0F : hihat_fire) : 0.0F;

    detector_.transient_level *= kLevelDecay;
    detector_.kick_level *= kLevelDecay;
    detector_.snare_level *= kLevelDecay;
    detector_.hihat_level *= kLevelDecay;
    if (transient_fired && transient_strength > detector_.transient_level)
        detector_.transient_level = transient_strength;
    if (kick_fired && kick_strength > detector_.kick_level)
        detector_.kick_level = kick_strength;
    if (snare_fired && snare_strength > detector_.snare_level)
        detector_.snare_level = snare_strength;
    if (hihat_fired && hihat_strength > detector_.hihat_level)
        detector_.hihat_level = hihat_strength;
    if (transient_fired) ++detector_.transient_id;
    if (kick_fired) ++detector_.kick_id;
    if (snare_fired) ++detector_.snare_id;
    if (hihat_fired) ++detector_.hihat_id;

    event.transient = transient_fired;
    event.kick = kick_fired;
    event.snare = snare_fired;
    event.hihat = hihat_fired;
    event.transient_strength = transient_strength;
    event.kick_strength = kick_strength;
    event.snare_strength = snare_strength;
    event.hihat_strength = hihat_strength;
    event.transient_level = detector_.transient_level;
    event.kick_level = detector_.kick_level;
    event.snare_level = detector_.snare_level;
    event.hihat_level = detector_.hihat_level;
    event.transient_event_id = detector_.transient_id;
    event.kick_event_id = detector_.kick_id;
    event.snare_event_id = detector_.snare_id;
    event.hihat_event_id = detector_.hihat_id;
    for (std::uint8_t bin = 0; bin < contract::kSpectrumBinCount; ++bin) {
        detector_.previous_spectrum[bin] = spectrum[bin];
    }
    ++detector_.frame_count;
    return transient_fired;
}

void OnsetBeat::decayBeatLock() noexcept {
    if (stable_intervals_ > 0) --stable_intervals_;
    if (stable_intervals_ == 0) {
        interval_estimate_ms_ = 0;
        last_interval_ms_ = 0;
    }
}

void OnsetBeat::noteAcceptedInterval(std::uint32_t interval_ms) noexcept {
    if (interval_ms < kBeatIntervalMinimumMs ||
        interval_ms > kBeatIntervalMaximumMs) {
        decayBeatLock();
        stable_intervals_ = 0;
        return;
    }
    if (interval_estimate_ms_ == 0) {
        interval_estimate_ms_ = interval_ms;
        last_interval_ms_ = interval_ms;
        return;
    }
    const bool close = intervalClose(interval_ms, interval_estimate_ms_);
    const bool half_alias = intervalClose(interval_ms * 2U, interval_estimate_ms_);
    const bool double_alias = intervalClose(interval_ms, interval_estimate_ms_ * 2U);
    if (close && !half_alias && !double_alias) {
        interval_estimate_ms_ =
            (interval_estimate_ms_ * 3U + interval_ms) / 4U;
        last_interval_ms_ = interval_ms;
        if (stable_intervals_ < kStableIntervalMaximum) ++stable_intervals_;
    } else {
        decayBeatLock();
        stable_intervals_ = 0;
        interval_estimate_ms_ = interval_ms;
        last_interval_ms_ = interval_ms;
    }
}

OnsetBeatFrame OnsetBeat::update(
    const contract::AudioFeaturesV1& audio) noexcept {
    const std::uint32_t now_ms = audio.source_frame_ms;
    const std::uint32_t delta_ms =
        onset_last_ms_ == 0 || now_ms < onset_last_ms_
            ? 0
            : now_ms - onset_last_ms_;
    onset_last_ms_ = now_ms;
    OnsetBeatFrame event = event_;
    event.event_age_ms = eventAge(now_ms, event.event_ms);
    bool event_active = event.event_age_ms <= kEventWindowMs;
    if (!event_active) {
        event.onset = false;
        event.bass_onset = false;
        event.beat = false;
        event.onset_strength = 0.0F;
        event.bass_onset_strength = 0.0F;
    }

    const float novelty = clamp(audio.novelty, 0.0F, 1.0F);
    const float low_energy = clamp(audio.low_energy, 0.0F, 1.0F);
    const float peak_scaled = clamp(audio.peak_scaled, 0.0F, 1.0F);
    if (!primed_) {
        novelty_fast_ = novelty_slow_ = novelty;
        low_fast_ = low_slow_ = low_energy;
        peak_fast_ = peak_slow_ = peak_scaled;
        previous_novelty_ = novelty;
        previous_low_energy_ = low_energy;
        previous_peak_ = peak_scaled;
        primed_ = true;
    }

    novelty_fast_ += (novelty - novelty_fast_) * alpha(delta_ms, 80.0F);
    novelty_slow_ += (novelty - novelty_slow_) * alpha(delta_ms, 1200.0F);
    low_fast_ += (low_energy - low_fast_) * alpha(delta_ms, 80.0F);
    low_slow_ += (low_energy - low_slow_) * alpha(delta_ms, 1200.0F);
    peak_fast_ += (peak_scaled - peak_fast_) * alpha(delta_ms, 80.0F);
    peak_slow_ += (peak_scaled - peak_slow_) * alpha(delta_ms, 1200.0F);

    if (silence(audio)) {
        last_accept_ms_ = 0;
        decayBeatLock();
        stable_intervals_ = 0;
        event.event_age_ms = eventAge(now_ms, event.event_ms);
        event.onset = event.bass_onset = event.beat = false;
        event.onset_strength = event.bass_onset_strength = 0.0F;
        event.beat_phase = event.beat_confidence = 0.0F;
        static_cast<void>(runV2(audio, event));
        previous_novelty_ = novelty;
        previous_low_energy_ = low_energy;
        previous_peak_ = peak_scaled;
        event_ = event;
        return event_;
    }

    const bool transient = runV2(audio, event);
    const float onset_strength = event.transient_strength;
    const float bass_strength = event.kick_strength;
    const bool novelty_candidate = transient;
    const bool bass_candidate = event.kick;
    const bool accepted = transient || event.kick;
    if (accepted) {
        const std::uint32_t interval_ms =
            last_accept_ms_ == 0 ? 0 : now_ms - last_accept_ms_;
        if (interval_ms > 0) noteAcceptedInterval(interval_ms);
        last_accept_ms_ = now_ms;
        ++event.event_id;
        event.event_ms = now_ms;
        event.event_age_ms = 0;
        event.onset_strength = onset_strength;
        event.bass_onset_strength = bass_strength;
        event.onset = novelty_candidate;
        event.bass_onset = bass_candidate;
        event_active = true;
    }

    if (interval_estimate_ms_ >= kBeatIntervalMinimumMs && last_accept_ms_ > 0) {
        const std::uint32_t age_ms = now_ms - last_accept_ms_;
        const float phase = static_cast<float>(age_ms % interval_estimate_ms_) /
                            static_cast<float>(interval_estimate_ms_);
        event.beat_phase = clamp(phase, 0.0F, 1.0F);
        event.beat_confidence =
            clamp(static_cast<float>(stable_intervals_) / 2.0F, 0.0F, 1.0F);
        if (accepted && event.beat_confidence >= 0.5F) {
            event.beat = true;
        } else if (!event_active) {
            event.beat = false;
        }
    } else {
        event.beat_phase = 0.0F;
        event.beat_confidence = 0.0F;
        if (!event_active) event.beat = false;
    }
    if (!accepted) event.event_age_ms = eventAge(now_ms, event.event_ms);
    previous_novelty_ = novelty;
    previous_low_energy_ = low_energy;
    previous_peak_ = peak_scaled;
    event_ = event;
    return event_;
}

}  // namespace k1::core::audio
