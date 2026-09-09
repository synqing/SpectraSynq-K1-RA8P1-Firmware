#pragma once
// Fixture adapter, not a physical capture driver. No heap; one owner per instance.
#include <cmath>
#include <cstdio>
#include <cstring>
#include "core/audio/audio_pipeline.h"
#include "core/visual/product_effect_renderer.h"

namespace fixture {
using namespace k1::core;
using namespace k1::core::audio;
using namespace k1::core::visual;

struct Trace {
  char data[16384]{};
  std::size_t size = 0;
  bool valid = true;
  void add(const char* name, double value) {
    if (!valid || !std::isfinite(value)) { valid = false; return; }
    const int n = std::snprintf(data + size, sizeof(data) - size, "%s=%.17g\n", name, value);
    if (n < 0 || static_cast<std::size_t>(n) >= sizeof(data) - size) valid = false;
    else size += static_cast<std::size_t>(n);
  }
  // Integer fields stay integer; never round 64-bit media/beat state through double.
  void integer(const char* name, std::uint64_t value) {
    if (!valid) return;
    const int n = std::snprintf(data + size, sizeof(data) - size, "%s=%llu\n", name,
                              static_cast<unsigned long long>(value));
    if (n < 0 || static_cast<std::size_t>(n) >= sizeof(data) - size) valid = false;
    else size += static_cast<std::size_t>(n);
  }
  template<class T> void field(const char* name, T value) { integer(name, static_cast<std::uint64_t>(value)); }
  void field(const char* name, float value) { add(name, value); }
  void field(const char* name, std::int64_t value) {
    // Preserve signed Q32.32 bits explicitly in the trace ABI.
    integer(name, static_cast<std::uint64_t>(value));
  }
};

inline void serialise(Trace& trace, const AudioPipelineOutput& o) {
#define F(path) trace.field(#path, o.path)
  F(valid); F(gdft_overflow_count); F(musical_time_valid); F(predicted_next_beat_valid);
  F(features.sequence); F(features.source_frame_ms); F(features.capture_time_us); F(features.publish_time_us);
  F(features.validity_flags); F(features.event_flags); F(features.peak_scaled); F(features.vu_level);
  F(features.novelty); F(features.spectral_energy); F(features.low_energy); F(features.mid_energy);
  F(features.high_energy); F(features.chroma_strength); F(features.nyquist_safe_bin_hi);
  char name[64];
  for (unsigned i = 0; i < 80; ++i) { std::snprintf(name, sizeof(name), "features.spectrum[%u]", i); trace.field(name, o.features.spectrum[i]); }
  for (unsigned i = 0; i < 12; ++i) { std::snprintf(name, sizeof(name), "features.chroma_a_origin[%u]", i); trace.field(name, o.features.chroma_a_origin[i]); }
  F(features.chord_type); F(features.chord_root_a_origin); F(features.chord_confidence);
  F(features.chord_root_strength); F(features.chord_third_strength); F(features.chord_fifth_strength);
  F(features.onset_event_id); F(features.onset_event_ms); F(features.onset_event_age_ms);
  F(features.onset_strength); F(features.bass_onset_strength); F(features.beat_phase); F(features.beat_confidence);
#define B(path) F(path.transient_strength); F(path.kick_strength); F(path.snare_strength); F(path.hihat_strength); \
  F(path.transient_level); F(path.kick_level); F(path.snare_level); F(path.hihat_level); \
  F(path.transient_event_id); F(path.kick_event_id); F(path.snare_event_id); F(path.hihat_event_id)
  B(features);
  F(onset.event_id); F(onset.event_ms); F(onset.event_age_ms); F(onset.onset); F(onset.bass_onset); F(onset.beat);
  F(onset.onset_strength); F(onset.bass_onset_strength); F(onset.beat_phase); F(onset.beat_confidence);
  F(onset.transient); F(onset.kick); F(onset.snare); F(onset.hihat); B(onset);
  F(tempo.bpm); F(tempo.phase01); F(tempo.confidence); F(tempo.beat_tick); F(tempo.locked); F(tempo.beat_strength); F(tempo.updated);
  F(saliency.axis.harmonic_novelty); F(saliency.axis.rhythmic_novelty); F(saliency.axis.timbral_novelty); F(saliency.axis.dynamic_novelty);
  F(saliency.axis.harmonic_novelty_smooth); F(saliency.axis.rhythmic_novelty_smooth); F(saliency.axis.timbral_novelty_smooth); F(saliency.axis.dynamic_novelty_smooth);
  F(saliency.axis.overall_saliency); F(saliency.axis.dominant_type); F(saliency.event.frame_ms); F(saliency.event.salient);
  F(saliency.event.overall_saliency); F(saliency.event.adaptive_threshold); F(saliency.event.age_ms); F(saliency.event.flags);
  F(saliency.input_accepted); F(saliency.event_emitted);
  F(musical_time.anchor.epoch_id); F(musical_time.anchor.frame_index); F(musical_time.beat_period_q32); F(musical_time.phase_error_q32);
  F(musical_time.beat_index); F(musical_time.tempo_bpm); F(musical_time.confidence); F(musical_time.locked);
  F(predicted_next_beat.epoch_id); F(predicted_next_beat.beat_event_frame.frame); F(predicted_next_beat.beat_event_frame.frac_q32);
  F(predicted_next_beat.beat_result_available_frame); F(predicted_next_beat.beat_index); F(predicted_next_beat.confidence); F(predicted_next_beat.predicted);
#undef B
#undef F
}

struct Trajectory {
  AudioPipeline ap;
  GdftSampleWindow window{};
  VisualWaveformHistory waveform{};
  ChannelRenderState a{PixelChannelId::kChannelA}, b{PixelChannelId::kChannelB};
  AudioPipelineOutput output{};
  std::uint32_t sequence = 0;
  std::uint64_t epoch = 1, next_render = 400;
  unsigned rendered = 0;
  static constexpr unsigned modes[]{3, 7, 8, 9, 11, 12, 13, 14, 15, 16, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 32};
  void process(const std::int16_t* hop) {
    std::memmove(window.data(), window.data() + 180, (window.size() - 180) * sizeof(std::int16_t));
    std::memcpy(window.data() + window.size() - 180, hop, 180 * sizeof(std::int16_t));
    float peak = 0, energy = 0;
    for (unsigned i = 0; i < 180; ++i) { const float x = hop[i] / 32768.0F; peak = std::fmax(peak, std::fabs(x)); energy += x*x; }
    AudioPipelineInput input;
    input.samples = &window; input.sequence = ++sequence;
    input.media_time = {epoch, static_cast<std::uint64_t>(sequence) * 360}; input.media_time_valid = true;
    input.frame_ms = static_cast<std::uint32_t>(input.media_time.frame_index / 48);
    input.capture_time_us = mediaFramesToMicros(input.media_time.frame_index);
    input.publish_time_us = input.capture_time_us; // Explicit zero-cost HOST fixture model only.
    input.peak_scaled = peak; input.vu_level = std::sqrt(energy / 180.0F); input.silence = peak == 0;
    output = ap.process(input);
    pushVisualWaveform(waveform, hop, 180, peak * 32768.0F, peak, sequence);
    a.prepareAudio(output.features); b.prepareAudio(output.features);
    rendered = 0;
    // AP at n*360; render at m*400. Consume only already published AP state.
    if (next_render < input.media_time.frame_index + 360) {
      auto& ca = a.controls(); auto& cb = b.controls();
      ca.mode_id = modes[(sequence / 120) % 23]; cb.mode_id = modes[((sequence / 120) + 11) % 23];
      ca.palette_id = (sequence / 240) % 44; cb.palette_id = (ca.palette_id + 9) % 44;
      const VisualAudioFrameView view{output.features, output.tempo, waveform, input.publish_time_us};
      const auto ra = renderProductChannel(a, view, 1.0F / 120.0F);
      const auto rb = renderProductChannel(b, view, 1.0F / 120.0F);
      rendered = ra.rendered && rb.rendered;
      next_render += 400;
    }
  }
  void trace(Trace& t) {
    t.size = 0; t.valid = true;
    serialise(t, output);
    t.field("rendered", rendered); t.field("next_render", next_render);
    t.field("mode_a", a.controls().mode_id); t.field("mode_b", b.controls().mode_id);
    char name[32];
    for (unsigned channel = 0; channel < 2; ++channel) {
      const auto pixels = (channel == 0 ? a.frame() : b.frame());
      for (unsigned i = 0; i < 160; ++i) {
        std::snprintf(name, sizeof(name), "pixel[%u]", channel*160+i);
        const auto p = pixels[i]; t.integer(name, (p.red << 16) | (p.green << 8) | p.blue);
      }
    }
  }
};
} // namespace fixture
