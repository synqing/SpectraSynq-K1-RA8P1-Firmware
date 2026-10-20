#pragma once
#include <array>
#include "core/visual/visual_audio_frame.h"
#include "core/audio/media_time.h"

namespace k1::core::visual {
// Host reference composition: fixed teal (R=.12,G=1,B=.72), centre 79/80.
// Level is linear channel output in [0,24]; this is not an optical calibration.
inline constexpr std::size_t kTempoReferencePixels = 160;
struct TempoReferenceOptions final {
    bool observed_accents = true;
    bool incumbent_clock = true;
    bool candidate_field = true;
};
struct TempoReferencePair final {
    std::array<float, kTempoReferencePixels> baseline{};
    std::array<float, kTempoReferencePixels> candidate{};
    float matched_total = 0.0F;
    std::uint8_t measured_count = 0;
    std::uint8_t derived_count = 0;
    std::uint8_t coasting_count = 0;
    bool field_available = false;
    bool accent_available = false;
};
// Pure bounded reader; view lifetime is the caller's immutable render scope.
// presentation_us and media coordinate must come from the qualified mapping.
[[nodiscard]] TempoReferencePair renderTempoReferencePair(
    const VisualAudioFrameView& view, audio::AudioTime presentation_media,
    std::uint64_t presentation_us, TempoReferenceOptions options = {}) noexcept;
} // namespace k1::core::visual
