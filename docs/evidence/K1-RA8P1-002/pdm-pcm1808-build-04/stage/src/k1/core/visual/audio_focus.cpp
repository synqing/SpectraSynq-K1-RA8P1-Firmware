#include "core/visual/audio_focus.h"

#include <cmath>
#include <cstddef>

namespace k1::core::visual {
namespace {

float sanitiseGain(const float gain) noexcept {
  if (!std::isfinite(gain) || gain <= 0.0F) {
    return 0.0F;
  }
  return gain > 4.0F ? 4.0F : gain;
}

float scaled(const float value, const float gain) noexcept {
  if (!std::isfinite(value)) {
    return 0.0F;
  }
  const float result = value * sanitiseGain(gain);
  return std::isfinite(result) ? result : 0.0F;
}

void clearEventWhenMuted(std::uint32_t& event_flags,
                         const std::uint32_t event,
                         const float gain) noexcept {
  if (sanitiseGain(gain) == 0.0F) {
    event_flags &= ~event;
  }
}

}  // namespace

AudioFocusProfile::AudioFocusProfile() noexcept {
  for (float& gain : spectrum_bin_gain) {
    gain = 1.0F;
  }
  for (float& gain : chroma_bin_gain) {
    gain = 1.0F;
  }
}

void applyAudioFocus(const contract::AudioFeaturesV1& shared,
                     const AudioFocusProfile& profile,
                     contract::AudioFeaturesV1& focused) noexcept {
  focused = shared;

  focused.peak_scaled = scaled(shared.peak_scaled, profile.level_gain);
  focused.vu_level = scaled(shared.vu_level, profile.level_gain);
  focused.spectral_energy =
      scaled(shared.spectral_energy, profile.level_gain);
  focused.novelty = scaled(shared.novelty, profile.novelty_gain);
  focused.low_energy = scaled(shared.low_energy, profile.low_gain);
  focused.mid_energy = scaled(shared.mid_energy, profile.mid_gain);
  focused.high_energy = scaled(shared.high_energy, profile.high_gain);

  for (std::size_t bin = 0U; bin < contract::kSpectrumBinCount; ++bin) {
    focused.spectrum[bin] =
        scaled(shared.spectrum[bin], profile.spectrum_bin_gain[bin]);
  }
  for (std::size_t bin = 0U; bin < contract::kChromaBinCount; ++bin) {
    focused.chroma_a_origin[bin] =
        scaled(shared.chroma_a_origin[bin],
               profile.tonal_gain * profile.chroma_bin_gain[bin]);
  }
  focused.chroma_strength =
      scaled(shared.chroma_strength, profile.tonal_gain);
  focused.chord_root_strength =
      scaled(shared.chord_root_strength, profile.tonal_gain);
  focused.chord_third_strength =
      scaled(shared.chord_third_strength, profile.tonal_gain);
  focused.chord_fifth_strength =
      scaled(shared.chord_fifth_strength, profile.tonal_gain);
  if (sanitiseGain(profile.tonal_gain) == 0.0F) {
    focused.validity_flags &=
        ~(contract::kValidChroma | contract::kValidChord);
    focused.chord_type = contract::ChordTypeV1::kNone;
    focused.chord_confidence = 0.0F;
  }

  focused.onset_strength =
      scaled(shared.onset_strength, profile.onset_gain);
  focused.bass_onset_strength =
      scaled(shared.bass_onset_strength, profile.bass_onset_gain);
  focused.beat_confidence =
      scaled(shared.beat_confidence, profile.beat_gain);
  focused.transient_strength =
      scaled(shared.transient_strength, profile.transient_gain);
  focused.transient_level =
      scaled(shared.transient_level, profile.transient_gain);
  focused.kick_strength = scaled(shared.kick_strength, profile.kick_gain);
  focused.kick_level = scaled(shared.kick_level, profile.kick_gain);
  focused.snare_strength =
      scaled(shared.snare_strength, profile.snare_gain);
  focused.snare_level = scaled(shared.snare_level, profile.snare_gain);
  focused.hihat_strength =
      scaled(shared.hihat_strength, profile.hihat_gain);
  focused.hihat_level = scaled(shared.hihat_level, profile.hihat_gain);

  clearEventWhenMuted(focused.event_flags, contract::kEventOnset,
                      profile.onset_gain);
  clearEventWhenMuted(focused.event_flags, contract::kEventBassOnset,
                      profile.bass_onset_gain);
  clearEventWhenMuted(focused.event_flags, contract::kEventBeat,
                      profile.beat_gain);
  clearEventWhenMuted(focused.event_flags, contract::kEventTransient,
                      profile.transient_gain);
  clearEventWhenMuted(focused.event_flags, contract::kEventKick,
                      profile.kick_gain);
  clearEventWhenMuted(focused.event_flags, contract::kEventSnare,
                      profile.snare_gain);
  clearEventWhenMuted(focused.event_flags, contract::kEventHihat,
                      profile.hihat_gain);
}

}  // namespace k1::core::visual
