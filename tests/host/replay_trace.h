#pragma once
#include "k1_live_runtime.h"
#include <cstdio>
#include "core/visual/tempo_reference_scene.h"
// Host-only field-by-field trace. No padding bytes or future-audio shifts.
inline void writeReplayTrace(FILE* file, const k1::core::audio::AudioPipelineOutput& out,
                             unsigned long long hop, unsigned long long available, bool scene = false) {
  std::fprintf(file, "{\"hop\":%llu,\"media_end_48k\":%llu,\"available_us\":%llu,\"features\":{", hop, hop*360ULL, available);
  std::fprintf(file, "\"sequence\":%.17g", double(out.features.sequence));
  std::fprintf(file, ",\"source_frame_ms\":%.17g", double(out.features.source_frame_ms));
  std::fprintf(file, ",\"capture_time_us\":%.17g", double(out.features.capture_time_us));
  std::fprintf(file, ",\"publish_time_us\":%.17g", double(out.features.publish_time_us));
  std::fprintf(file, ",\"validity_flags\":%.17g", double(out.features.validity_flags));
  std::fprintf(file, ",\"event_flags\":%.17g", double(out.features.event_flags));
  std::fprintf(file, ",\"peak_scaled\":%.17g", double(out.features.peak_scaled));
  std::fprintf(file, ",\"vu_level\":%.17g", double(out.features.vu_level));
  std::fprintf(file, ",\"novelty\":%.17g", double(out.features.novelty));
  std::fprintf(file, ",\"spectral_energy\":%.17g", double(out.features.spectral_energy));
  std::fprintf(file, ",\"low_energy\":%.17g", double(out.features.low_energy));
  std::fprintf(file, ",\"mid_energy\":%.17g", double(out.features.mid_energy));
  std::fprintf(file, ",\"high_energy\":%.17g", double(out.features.high_energy));
  std::fprintf(file, ",\"chroma_strength\":%.17g", double(out.features.chroma_strength));
  std::fprintf(file, ",\"nyquist_safe_bin_hi\":%.17g", double(out.features.nyquist_safe_bin_hi));
  std::fprintf(file, ",\"spectrum\":[");
  for (unsigned i=0; i<k1::contract::kSpectrumBinCount; ++i) std::fprintf(file, "%s%.9g", i ? "," : "", double(out.features.spectrum[i]));
  std::fprintf(file, "]");
  std::fprintf(file, ",\"chroma_a_origin\":[");
  for (unsigned i=0;i<k1::contract::kChromaBinCount;++i) std::fprintf(file,"%s%.9g",i ? "," : "",double(out.features.chroma_a_origin[i]));
  std::fprintf(file,"]");
  std::fprintf(file, ",\"chord_type\":%.17g", double(out.features.chord_type));
  std::fprintf(file, ",\"chord_root_a_origin\":%.17g", double(out.features.chord_root_a_origin));
  std::fprintf(file, ",\"chord_confidence\":%.17g", double(out.features.chord_confidence));
  std::fprintf(file, ",\"chord_root_strength\":%.17g", double(out.features.chord_root_strength));
  std::fprintf(file, ",\"chord_third_strength\":%.17g", double(out.features.chord_third_strength));
  std::fprintf(file, ",\"chord_fifth_strength\":%.17g", double(out.features.chord_fifth_strength));
  std::fprintf(file, ",\"onset_event_id\":%.17g", double(out.features.onset_event_id));
  std::fprintf(file, ",\"onset_event_ms\":%.17g", double(out.features.onset_event_ms));
  std::fprintf(file, ",\"onset_event_age_ms\":%.17g", double(out.features.onset_event_age_ms));
  std::fprintf(file, ",\"onset_strength\":%.17g", double(out.features.onset_strength));
  std::fprintf(file, ",\"bass_onset_strength\":%.17g", double(out.features.bass_onset_strength));
  std::fprintf(file, ",\"beat_phase\":%.17g", double(out.features.beat_phase));
  std::fprintf(file, ",\"beat_confidence\":%.17g", double(out.features.beat_confidence));
  std::fprintf(file, ",\"transient_strength\":%.17g", double(out.features.transient_strength));
  std::fprintf(file, ",\"kick_strength\":%.17g", double(out.features.kick_strength));
  std::fprintf(file, ",\"snare_strength\":%.17g", double(out.features.snare_strength));
  std::fprintf(file, ",\"hihat_strength\":%.17g", double(out.features.hihat_strength));
  std::fprintf(file, ",\"transient_level\":%.17g", double(out.features.transient_level));
  std::fprintf(file, ",\"kick_level\":%.17g", double(out.features.kick_level));
  std::fprintf(file, ",\"snare_level\":%.17g", double(out.features.snare_level));
  std::fprintf(file, ",\"hihat_level\":%.17g", double(out.features.hihat_level));
  std::fprintf(file, ",\"transient_event_id\":%.17g", double(out.features.transient_event_id));
  std::fprintf(file, ",\"kick_event_id\":%.17g", double(out.features.kick_event_id));
  std::fprintf(file, ",\"snare_event_id\":%.17g", double(out.features.snare_event_id));
  std::fprintf(file, ",\"hihat_event_id\":%.17g", double(out.features.hihat_event_id));
  std::fprintf(file, "},\"tempo\":{");
  std::fprintf(file, "\"bpm\":%.9g", double(out.tempo.bpm));
  std::fprintf(file, ",\"phase01\":%.9g", double(out.tempo.phase01));
  std::fprintf(file, ",\"confidence\":%.9g", double(out.tempo.confidence));
  std::fprintf(file, ",\"beat_tick\":%.9g", double(out.tempo.beat_tick));
  std::fprintf(file, ",\"locked\":%.9g", double(out.tempo.locked));
  std::fprintf(file, ",\"beat_strength\":%.9g", double(out.tempo.beat_strength));
  std::fprintf(file, ",\"updated\":%.9g", double(out.tempo.updated));
  std::fprintf(file, "},\"valid\":%d,\"musical_time_valid\":%d,\"predicted_next_beat_valid\":%d,\"gdft_overflows\":%u", out.valid, out.musical_time_valid, out.predicted_next_beat_valid, out.gdft_overflow_count);
  std::fprintf(file, ",\"epoch\":1,\"tempo_field\":{");
  std::fprintf(file, "\"schema_version\":%.17g", double(out.tempo_field.schema_version));
  std::fprintf(file, ",\"configuration_revision\":%.17g", double(out.tempo_field.configuration_revision));
  std::fprintf(file, ",\"epoch\":%.17g", double(out.tempo_field.epoch));
  std::fprintf(file, ",\"generation\":%.17g", double(out.tempo_field.generation));
  std::fprintf(file, ",\"support_start\":%.17g", double(out.tempo_field.support_start));
  std::fprintf(file, ",\"support_end\":%.17g", double(out.tempo_field.support_end));
  std::fprintf(file, ",\"available_us\":%.17g", double(out.tempo_field.available_us));
  std::fprintf(file, ",\"activity\":%.17g", double(out.tempo_field.activity));
  std::fprintf(file, ",\"count\":%.17g", double(out.tempo_field.count));
  std::fprintf(file, ",\"fresh_phase_count\":%.17g", double(out.tempo_field.fresh_phase_count));
  std::fprintf(file, ",\"availability_valid\":%.17g", double(out.tempo_field.availability_valid));
  std::fprintf(file, ",\"enabled\":%.17g", double(out.tempo_field.enabled));
  std::fprintf(file, ",\"valid\":%.17g", double(out.tempo_field.valid));
  std::fprintf(file, ",\"warming\":%.17g", double(out.tempo_field.warming));
  std::fprintf(file, ",\"silence\":%.17g", double(out.tempo_field.silence));
  std::fprintf(file, ",\"activity_valid\":%.17g", double(out.tempo_field.activity_valid));
  std::fprintf(file, ",\"phase_authority\":%.17g", double(out.tempo_field.phase_authority));
  std::fprintf(file, ",\"coasting\":%.17g", double(out.tempo_field.coasting));
  std::fprintf(file, ",\"canonical\":{");
  std::fprintf(file, "\"target_bpm\":%.17g", double(out.tempo_field.canonical.target_bpm));
  std::fprintf(file, ",\"running_bpm\":%.17g", double(out.tempo_field.canonical.running_bpm));
  std::fprintf(file, ",\"phase01\":%.17g", double(out.tempo_field.canonical.phase01));
  std::fprintf(file, ",\"confidence\":%.17g", double(out.tempo_field.canonical.confidence));
  std::fprintf(file, ",\"reference_wall_ms\":%.17g", double(out.tempo_field.canonical.reference_wall_ms));
  std::fprintf(file, ",\"winner_id\":%.17g", double(out.tempo_field.canonical.winner_id));
  std::fprintf(file, ",\"locked\":%.17g", double(out.tempo_field.canonical.locked));
  std::fprintf(file, ",\"media_projection_valid\":%.17g", double(out.tempo_field.canonical.media_projection_valid));
  std::fprintf(file, "},\"candidates\":[");
  for (unsigned j=0; j<out.tempo_field.count && j<8; ++j) {
    std::fprintf(file, "%s{", j ? "," : "");
  std::fprintf(file, "\"bin_id\":%.17g", double(out.tempo_field.candidates[j].bin_id));
  std::fprintf(file, ",\"generation\":%.17g", double(out.tempo_field.candidates[j].generation));
  std::fprintf(file, ",\"bpm\":%.17g", double(out.tempo_field.candidates[j].bpm));
  std::fprintf(file, ",\"salience\":%.17g", double(out.tempo_field.candidates[j].salience));
  std::fprintf(file, ",\"point_salience\":%.17g", double(out.tempo_field.candidates[j].point_salience));
  std::fprintf(file, ",\"epoch\":%.17g", double(out.tempo_field.candidates[j].epoch));
  std::fprintf(file, ",\"support_start\":%.17g", double(out.tempo_field.candidates[j].support_start));
  std::fprintf(file, ",\"support_end\":%.17g", double(out.tempo_field.candidates[j].support_end));
  std::fprintf(file, ",\"last_observed_frame\":%.17g", double(out.tempo_field.candidates[j].last_observed_frame));
  std::fprintf(file, ",\"reference_frame\":%.17g", double(out.tempo_field.candidates[j].reference_frame));
  std::fprintf(file, ",\"observation_age_frames\":%.17g", double(out.tempo_field.candidates[j].observation_age_frames));
  std::fprintf(file, ",\"raw_observed_frame\":%.17g", double(out.tempo_field.candidates[j].raw_observed_frame));
  std::fprintf(file, ",\"raw_observation_age_frames\":%.17g", double(out.tempo_field.candidates[j].raw_observation_age_frames));
  std::fprintf(file, ",\"beat_index\":%.17g", double(out.tempo_field.candidates[j].beat_index));
  std::fprintf(file, ",\"beat_period_q32\":%.17g", double(out.tempo_field.candidates[j].beat_period_q32));
  std::fprintf(file, ",\"phase01\":%.17g", double(out.tempo_field.candidates[j].phase01));
  std::fprintf(file, ",\"phase_uncertainty_cycles\":%.17g", double(out.tempo_field.candidates[j].phase_uncertainty_cycles));
  std::fprintf(file, ",\"root_generation\":%.17g", double(out.tempo_field.candidates[j].root_generation));
  std::fprintf(file, ",\"root_id\":%.17g", double(out.tempo_field.candidates[j].root_id));
  std::fprintf(file, ",\"ratio_numerator\":%.17g", double(out.tempo_field.candidates[j].ratio_numerator));
  std::fprintf(file, ",\"ratio_denominator\":%.17g", double(out.tempo_field.candidates[j].ratio_denominator));
  std::fprintf(file, ",\"admission\":%.17g", double(out.tempo_field.candidates[j].admission));
  std::fprintf(file, ",\"phase_valid\":%.17g", double(out.tempo_field.candidates[j].phase_valid));
  std::fprintf(file, ",\"uncertainty_valid\":%.17g", double(out.tempo_field.candidates[j].uncertainty_valid));
  std::fprintf(file, ",\"raw_observation_valid\":%.17g", double(out.tempo_field.candidates[j].raw_observation_valid));
  std::fprintf(file, ",\"derived\":%.17g", double(out.tempo_field.candidates[j].derived));
    std::fprintf(file, "}");
  }
  std::fprintf(file, "]}");
  if (scene) {
    static const k1::core::visual::VisualWaveformHistory waveform{};
    const k1::core::visual::VisualAudioFrameView view{out.features,out.tempo,waveform,out.features.publish_time_us,&out.tempo_field};
    std::fprintf(file, ",\"scene_variants\":[");
    for (unsigned mask=0; mask<8; ++mask) {
      const k1::core::visual::TempoReferenceOptions options{bool(mask&1),bool(mask&2),bool(mask&4)};
      const auto pair=k1::core::visual::renderTempoReferencePair(view,{1,hop*360ULL},available,options);
      std::fprintf(file, "%s{\"baseline\":[",mask ? "," : "");
      for (unsigned i=0;i<80;++i) std::fprintf(file,"%s%.9g",i ? "," : "",double(pair.baseline[i]));
      std::fprintf(file,"],\"candidate\":[");
      for (unsigned i=0;i<80;++i) std::fprintf(file,"%s%.9g",i ? "," : "",double(pair.candidate[i]));
      std::fprintf(file,"],\"matched_total\":%.9g,\"measured_count\":%u,\"coasting_count\":%u}", double(pair.matched_total),unsigned(pair.measured_count),unsigned(pair.coasting_count));
    }
    std::fprintf(file,"]");
  }
  std::fprintf(file,"}\n");
}
