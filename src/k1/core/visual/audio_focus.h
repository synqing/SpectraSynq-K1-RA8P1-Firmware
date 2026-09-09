#pragma once

#include <cstdint>

#include "contract/audio_features_v1.h"

namespace k1::core::visual {

// A fixed-size, per-channel weighting profile over one shared AP snapshot.
// Classified kick/snare/hi-hat gains are useful focus controls, not source
// separation: overlapping instruments can still share spectral content.
struct AudioFocusProfile final {
  AudioFocusProfile() noexcept;

  float level_gain = 1.0F;
  float novelty_gain = 1.0F;
  float low_gain = 1.0F;
  float mid_gain = 1.0F;
  float high_gain = 1.0F;
  float tonal_gain = 1.0F;
  float onset_gain = 1.0F;
  float bass_onset_gain = 1.0F;
  float beat_gain = 1.0F;
  float transient_gain = 1.0F;
  float kick_gain = 1.0F;
  float snare_gain = 1.0F;
  float hihat_gain = 1.0F;
  float spectrum_bin_gain[contract::kSpectrumBinCount]{};
  float chroma_bin_gain[contract::kChromaBinCount]{};
};

void applyAudioFocus(const contract::AudioFeaturesV1& shared,
                     const AudioFocusProfile& profile,
                     contract::AudioFeaturesV1& focused) noexcept;

}  // namespace k1::core::visual
