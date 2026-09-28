#include "core/audio/tempo_field_publisher.h"
#include <cmath>
#include <limits>
namespace k1::core::audio {
contract::TempoFieldV1 publishTempoField(TempoFieldPublisher& p,
    const TempoTrackerState& tracker, AudioTime now, bool media_valid,
    std::uint64_t available_us, bool enabled, bool phase_enabled) noexcept {
    const auto& e=tracker.evidence;
    const auto& stamp=e.completed;
    const bool discontinuity=p.field.epoch!=now.epoch_id ||
        p.field.configuration_revision!=e.current.configuration_revision ||
        (p.field.valid && now.frame_index<p.field.support_end);
    if(discontinuity || !enabled || !media_valid || e.invalid) {
        p.field={}; p.tracks={}; p.was_enabled=false;
    }
    if(!enabled || !media_valid || e.invalid) return p.field;
    if(!phase_enabled && p.was_phase_enabled) {
        for(auto& t:p.tracks.slots) t.phase_valid=false;
        for(auto& t:p.field.candidates) t.phase_valid=false;
    }
    // Re-enabling waits for genuinely new evidence; cache reads never analyse.
    const bool fresh=e.generation!=p.seen_generation;
    if(!p.was_enabled) { p.field.enabled=true; p.field.epoch=now.epoch_id;
        p.field.configuration_revision=e.current.configuration_revision; }
    p.was_enabled=true;p.was_phase_enabled=phase_enabled;
    if(fresh) {
        p.seen_generation=e.generation;
        if(!stamp.valid || stamp.epoch!=now.epoch_id ||
           stamp.configuration_revision!=e.current.configuration_revision ||
           now.frame_index<stamp.endpoint || (p.field.valid && available_us<p.field.available_us)) {
            p.field={};p.field.enabled=true;p.field.epoch=now.epoch_id;
            p.field.configuration_revision=e.current.configuration_revision;
            p.field.generation=e.generation; return p.field;
        }
        contract::TempoFieldV1 next{};
        next.enabled=true; next.epoch=stamp.epoch;
        next.configuration_revision=stamp.configuration_revision;
        next.generation=e.generation; next.support_end=stamp.endpoint;
        // Support includes the oldest pooled novelty interval. Correlation
        // sample ages still run from 511 intervals ago to the endpoint.
        const auto span=kTempoAcfHistoryLength*e.interval_frames;
        next.support_start=stamp.endpoint>=span?stamp.endpoint-span:0;
        next.available_us=available_us;next.availability_valid=available_us!=0;next.warming=e.history_samples<kTempoAcfHistoryLength;
        // The incumbent ring deliberately survives namespace discontinuities.
        // Admit historical evidence only after every pooled value belongs to
        // the current epoch/configuration. Immediate features remain separate.
        next.valid=tracker.acf.valid && !next.warming;
        next.silence=tracker.silence_detected;
        double energy=0; bool finite=true;
        for(float x:e.snapshot) { if(!std::isfinite(x)||x<0) finite=false; else energy+=static_cast<double>(x)*x; }
        const double mean_square=energy/kTempoAcfHistoryLength;
        finite = finite && std::isfinite(mean_square) &&
                 mean_square <= std::numeric_limits<float>::max();
        next.activity_valid=finite && !next.warming;
        next.activity=next.activity_valid?static_cast<float>(mean_square):0.0F;
        if(!finite) next.valid=false;
        if(next.valid) updateTempoPhaseTracks(p.tracks,next,e.snapshot,tracker.acf,e.interval_frames,phase_enabled);
        for(std::size_t i=0;i<next.count;++i) {
            auto& t=next.candidates[i];
            t.raw_observation_valid=e.raw_observed_valid[t.bin_id];
            t.raw_observed_frame=e.raw_observed_frame[t.bin_id];
        }
        p.field=next;
    }
    auto& c=p.field.canonical;
    c.target_bpm=tracker.event.bpm;c.running_bpm=tracker.flywheel_run_bpm;
    c.phase01=tracker.flywheel_phase01;c.confidence=tracker.event.confidence;
    c.locked=tracker.event.locked;c.winner_id=tracker.winner_bin;
    c.reference_wall_ms=e.phase_reference_wall_ms;c.media_projection_valid=false;
    ageTempoPhaseTracks(p.field,now.frame_index);
    if(!phase_enabled) p.field.phase_authority=false;
    return p.field;
}
} // namespace k1::core::audio
