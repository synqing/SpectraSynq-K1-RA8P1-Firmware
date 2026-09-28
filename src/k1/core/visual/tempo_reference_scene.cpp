#include "core/visual/tempo_reference_scene.h"
#include "core/audio/tempo_phase_tracks.h"
#include <algorithm>
#include <cmath>

namespace k1::core::visual {
namespace {
float unit(float x) noexcept { return std::isfinite(x) ? std::clamp(x, 0.0F, 1.0F) : 0.0F; }
float pulse(float radius, float centre, float width) noexcept {
    return std::max(0.0F, 1.0F - std::fabs(radius-centre)/width);
}
}
TempoReferencePair renderTempoReferencePair(const VisualAudioFrameView& view,
    audio::AudioTime now, std::uint64_t presentation_us,
    TempoReferenceOptions options) noexcept {
    TempoReferencePair out{};
    if (presentation_us < view.source_publication_us ||
        (view.audio.validity_flags & contract::kValidBaseSnapshot) == 0U) return out;
    const auto* field = view.tempo_field;
    bool field_ok = field != nullptr && field->enabled && field->valid &&
        field->schema_version == contract::kTempoFieldSchemaVersion &&
        field->epoch == now.epoch_id && field->availability_valid &&
        field->available_us <= presentation_us &&
        field->support_start <= field->support_end &&
        field->support_end <= now.frame_index &&
        field->count <= contract::kTempoFieldCapacity;
    if (field_ok) {
        for (std::size_t j=0;j<field->count;++j) {
            const auto& t=field->candidates[j];
            if (t.bin_id>=audio::kTempoTrackerBinCount || !std::isfinite(t.salience) || t.salience<0.0F)
                field_ok=false;
            for (std::size_t k=0;k<j;++k)
                if (field->candidates[k].bin_id==t.bin_id) field_ok=false;
        }
    }
    out.field_available = field_ok;
    std::array<double, contract::kTempoFieldCapacity> phase{};
    std::array<float, contract::kTempoFieldCapacity> weight{};
    float weight_sum = 0.0F;
    if (field_ok && field->phase_authority && options.candidate_field) {
        for (std::size_t j=0; j<field->count; ++j) {
            const auto& track = field->candidates[j];
            audio::TempoExtendedPhase p{};
            if (track.salience <= 0.0F ||
                (track.admission != contract::TempoTrackAdmission::fresh &&
                 track.admission != contract::TempoTrackAdmission::coasting) ||
                track.support_end > now.frame_index ||
                !audio::projectTempoTrack(track, now, p)) continue;
            phase[j] = p.phase01;
            weight[j] = track.salience;
            weight_sum += weight[j];
            if (track.derived) ++out.derived_count;
            else ++out.measured_count;
            if (track.admission == contract::TempoTrackAdmission::coasting ||
                now.frame_index > track.last_observed_frame)
                ++out.coasting_count;
        }
    }
    // Onset timing is declared in AP wall milliseconds. Never invent a support
    // delay correction. This view's publication age extends the supplied age.
    float accent = 0.0F, accent_radius = 0.0F;
    const double age_ms = static_cast<double>(view.audio.onset_event_age_ms) +
        static_cast<double>(presentation_us-view.source_publication_us)/1000.0;
    if (options.observed_accents && view.audio.onset_event_id != 0U &&
        (view.audio.validity_flags & contract::kValidOnsetV2) != 0U && age_ms < 600.0) {
        accent_radius = static_cast<float>(age_ms/600.0);
        accent = unit(std::max(view.audio.onset_strength, view.audio.bass_onset_strength)) *
            static_cast<float>(1.0-age_ms/600.0);
        out.accent_available = accent > 0.0F;
    }
    // Incumbent phase is sampled as published. Its winner BPM must never be
    // used to extrapolate the actual flywheel running frequency.
    const float clock_phase = unit(view.tempo.phase01);
    const float clock_level = options.incumbent_clock ? unit(view.tempo.confidence) : 0.0F;
    double base_sum=0.0, candidate_sum=0.0;
    for (std::size_t i=0; i<kTempoReferencePixels; ++i) {
        const float radius = std::max(0.0F, (std::fabs(static_cast<float>(i)-79.5F)-0.5F)/79.0F);
        const float base = 0.10F + 0.7F*clock_level*pulse(radius,clock_phase,0.18F);
        float candidate = base + 1.2F*accent*pulse(radius,accent_radius,0.12F);
        if (weight_sum>0.0F && std::isfinite(weight_sum))
            for(std::size_t j=0;j<contract::kTempoFieldCapacity;++j)
                candidate += 0.6F*(weight[j]/weight_sum)*pulse(radius,static_cast<float>(phase[j]),0.10F);
        out.baseline[i]=base; out.candidate[i]=candidate;
        base_sum+=base; candidate_sum+=candidate;
    }
    // Identical audio-level envelope, exact total matching and shared headroom.
    const float envelope = (view.audio.event_flags & contract::kEventSilence) != 0U ? 0.0F : unit(view.audio.vu_level);
    float peak=0.0F;
    for(std::size_t i=0;i<kTempoReferencePixels;++i) {
        out.baseline[i]=static_cast<float>(out.baseline[i]/base_sum);
        out.candidate[i]=static_cast<float>(out.candidate[i]/candidate_sum);
        peak=std::max(peak,std::max(out.baseline[i],out.candidate[i]));
    }
    out.matched_total = peak>0.0F ? std::min(160.0F*4.0F*envelope,24.0F/peak) : 0.0F;
    for(std::size_t i=0;i<kTempoReferencePixels;++i) {
        out.baseline[i]*=out.matched_total; out.candidate[i]*=out.matched_total;
    }
    return out;
}
} // namespace k1::core::visual
