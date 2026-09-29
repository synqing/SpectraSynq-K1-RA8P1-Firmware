#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "contract/audio_features_v1.h"
#include "contract/tempo_field_v1.h"
#include "core/audio/tempo_tracker.h"

namespace k1::core::visual {

inline constexpr std::size_t kVisualWaveformHistoryFrames = 4U;
inline constexpr std::size_t kVisualWaveformMaximumSamples = 1024U;

struct VisualWaveformHistory final {
  std::array<std::array<std::int16_t, kVisualWaveformMaximumSamples>,
             kVisualWaveformHistoryFrames>
      frames{};
  std::size_t sample_count = 0U;
  float raw_maximum = 0.0F;
  float peak_scaled = 0.0F;
  std::uint32_t sequence = 0U;
};

void pushVisualWaveform(VisualWaveformHistory& history,
                        const std::int16_t* samples, std::size_t count,
                        float raw_maximum, float peak_scaled,
                        std::uint32_t sequence) noexcept;

struct VisualAudioFrameView final {
  const contract::AudioFeaturesV1& audio;
  const audio::TempoTrackerEvent& tempo;
  const VisualWaveformHistory& waveform;
  std::uint64_t source_publication_us;
  // Scoped to this render call, pointing only at the runtime's committed copy.
  // A persistent consumer must own a bounded copy; reads never advance AP.
  const contract::TempoFieldV1* tempo_field = nullptr;
  // Optional pre-AGC magnitudes, same committed publication as audio.
  const std::array<float, contract::kSpectrumBinCount>* visual_magnitudes = nullptr;
};

}  // namespace k1::core::visual
