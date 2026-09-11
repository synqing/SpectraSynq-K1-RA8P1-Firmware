#include "core/visual/visual_audio_frame.h"

#include <cmath>

namespace k1::core::visual {

void pushVisualWaveform(VisualWaveformHistory& history,
                        const std::int16_t* samples, std::size_t count,
                        const float raw_maximum, const float peak_scaled,
                        const std::uint32_t sequence) noexcept {
  for (std::size_t frame = 1U; frame < history.frames.size(); ++frame) {
    history.frames[frame - 1U] = history.frames[frame];
  }
  auto& newest = history.frames.back();
  newest.fill(0);
  if (samples == nullptr || count == 0U) {
    history.sample_count = 0U;
  } else {
    if (count > newest.size()) {
      samples += count - newest.size();
      count = newest.size();
    }
    for (std::size_t index = 0U; index < count; ++index) {
      newest[index] = samples[index];
    }
    history.sample_count = count;
  }
  history.raw_maximum =
      std::isfinite(raw_maximum) && raw_maximum > 0.0F ? raw_maximum : 0.0F;
  history.peak_scaled = std::isfinite(peak_scaled) ? peak_scaled : 0.0F;
  history.sequence = sequence;
}

}  // namespace k1::core::visual
