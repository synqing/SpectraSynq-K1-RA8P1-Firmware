#include "core/audio/audio_pipeline.h"
#include "resident_controls.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace k1::core::audio;

static void run_hop(AudioPipeline& ap, const std::int16_t* hop, std::uint32_t sequence,
                    AudioPipelineOutput& output) {
  GdftSampleWindow window{};
  static GdftSampleWindow held{};
  std::memmove(held.data(), held.data() + 180, (held.size() - 180) * sizeof(std::int16_t));
  std::memcpy(held.data() + held.size() - 180, hop, 180 * sizeof(std::int16_t));
  window = held;
  float peak = 0, energy = 0;
  for (unsigned i = 0; i < 180; ++i) {
    const float x = hop[i] / 32768.0F;
    peak = std::fmax(peak, std::fabs(x));
    energy += x * x;
  }
  AudioPipelineInput input;
  input.samples = &window;
  input.sequence = sequence;
  input.media_time = {1, static_cast<std::uint64_t>(sequence) * 360};
  input.media_time_valid = true;
  input.frame_ms = static_cast<std::uint32_t>(input.media_time.frame_index / 48);
  input.capture_time_us = 0;
  input.publish_time_us = 0;
  input.peak_scaled = peak;
  input.vu_level = std::sqrt(energy / 180.0F);
  input.silence = peak == 0;
  output = ap.process(input);
}

int main() {
  AudioPipeline ap;
  AudioPipelineOutput output{};
  for (unsigned hop = 0; hop < 8; ++hop) {
    run_hop(ap, k1_resident_pcm[k1_resident_index[hop]], hop + 1U, output);
    std::printf("hop=%u updated=%d bpm=%.9g phase=%.9g conf=%.9g tick=%d locked=%d strength=%.9g "
                "mt_valid=%d mt_locked=%d mt_bpm=%.9g\n",
                hop, int(output.tempo.updated), output.tempo.bpm, output.tempo.phase01,
                output.tempo.confidence, int(output.tempo.beat_tick), int(output.tempo.locked),
                output.tempo.beat_strength, int(output.musical_time_valid),
                int(output.musical_time.locked), output.musical_time.tempo_bpm);
  }
  return 0;
}
