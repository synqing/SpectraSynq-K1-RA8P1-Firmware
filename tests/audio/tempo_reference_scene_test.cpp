#include "core/visual/tempo_reference_scene.h"
#include "core/audio/tempo_phase_tracks.h"
#include "core/audio/musical_time.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
using namespace k1;
int main() {
    contract::AudioFeaturesV1 features{};
    features.validity_flags=contract::kValidBaseSnapshot|contract::kValidOnsetV2;
    features.event_flags=0; features.vu_level=0.7F;
    features.onset_event_id=1; features.onset_strength=1.0F;
    core::audio::TempoTrackerEvent tempo{};
    tempo.phase01=0.2F; tempo.confidence=0.7F;
    core::visual::VisualWaveformHistory waveform{};
    contract::TempoFieldV1 field{};
    field.enabled=field.valid=field.phase_authority=true;
    field.warming=false; field.epoch=7; field.count=1; field.available_us=1000000;
    field.availability_valid=true;
    field.support_end=48000;
    auto& t=field.candidates[0];
    t.epoch=7;t.generation=1;t.bin_id=60;t.salience=1;t.phase01=0.25;
    t.phase_valid=true;t.admission=contract::TempoTrackAdmission::fresh;
    t.last_observed_frame=t.reference_frame=48000;
    t.beat_period_q32=core::audio::beatPeriodFromBpm(120);
    core::visual::VisualAudioFrameView view{features,tempo,waveform,1000000,&field};
    const auto before=field;
    const auto a=core::visual::renderTempoReferencePair(view,{7,48000},1000000);
    const auto b=core::visual::renderTempoReferencePair(view,{7,48000},1000000);
    assert(std::memcmp(&before,&field,sizeof(field))==0);
    assert(a.baseline==b.baseline && a.candidate==b.candidate);
    assert(a.measured_count==1 && a.accent_available);
    double baseline=0,candidate=0,max_level=0;
    for(std::size_t i=0;i<160;++i) {
        assert(std::isfinite(a.candidate[i]));
        assert(std::fabs(a.baseline[i]-a.baseline[159-i])<1e-6);
        assert(std::fabs(a.candidate[i]-a.candidate[159-i])<1e-6);
        assert(a.candidate[i]>=0 && a.candidate[i]<=24.00001);
        baseline+=a.baseline[i];candidate+=a.candidate[i];
        max_level=std::max(max_level,double(a.candidate[i]));
    }
    assert(std::fabs(baseline-candidate)<0.001);
    assert(core::visual::renderTempoReferencePair(view,{8,48000},1000000).measured_count==0);
    assert(core::visual::renderTempoReferencePair(view,{7,96001},2000021).measured_count==0);
    assert(core::visual::renderTempoReferencePair(view,{7,48000},999999).matched_total==0);
    field.availability_valid=false;
    assert(!core::visual::renderTempoReferencePair(view,{7,48000},1000000).field_available);
    field.availability_valid=true;
    field.schema_version=2;
    assert(!core::visual::renderTempoReferencePair(view,{7,48000},1000000).field_available);
    field.schema_version=1;field.count=9;
    assert(!core::visual::renderTempoReferencePair(view,{7,48000},1000000).field_available);
    field.count=2;field.candidates[1]=field.candidates[0];
    assert(!core::visual::renderTempoReferencePair(view,{7,48000},1000000).field_available);
    field.count=1;t.admission=contract::TempoTrackAdmission::stale;
    assert(core::visual::renderTempoReferencePair(view,{7,48000},1000000).measured_count==0);
    t.admission=contract::TempoTrackAdmission::fresh;
    field.phase_authority=false;
    auto accents_only=core::visual::renderTempoReferencePair(view,{7,48000},1000000,{true,false,false});
    assert(accents_only.accent_available && accents_only.candidate[79]>accents_only.candidate[30]);
    features.onset_event_age_ms=300;
    const auto outward=core::visual::renderTempoReferencePair(view,{7,48000},1000000,{true,false,false});
    assert(outward.candidate[39]>outward.candidate[79]); // Actual travel, not just symmetry.
    auto off=core::visual::renderTempoReferencePair(view,{7,48000},1000000,{false,true,false});
    assert(off.baseline==off.candidate);
    features.event_flags=contract::kEventSilence;
    assert(core::visual::renderTempoReferencePair(view,{7,48000},1000000).matched_total==0);
    features.event_flags=0;features.vu_level=std::numeric_limits<float>::quiet_NaN();
    assert(core::visual::renderTempoReferencePair(view,{7,48000},1000000).matched_total==0);
    std::printf("scene tests passed: immutable repeated reads, epoch/stale/future rejection, immediate accents, centre-outward travel, mirror geometry, toggles, silence, NaN; baseline_total=%.9f candidate_total=%.9f maximum_level=%.9f\n",baseline,candidate,max_level);
}
