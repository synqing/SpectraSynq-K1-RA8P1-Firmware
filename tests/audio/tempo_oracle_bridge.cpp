#include "core/audio/tempo_phase_tracks.h"
#include "core/audio/tempo_field_publisher.h"
#include <limits>
#include <cmath>
using namespace k1::core::audio;
extern "C" {
int phase_observe(const float* input, float bpm, unsigned long long interval, double* out) {
    TempoNoveltyRing ring{};
    for (unsigned i=0;i<ring.size();++i) ring[i]=input[i];
    return observeTempoPhase(ring,bpm,interval,*out);
}
int phase_derive(unsigned long long beat, double phase, unsigned p, unsigned q,
                 unsigned long long epoch, unsigned long long wanted,
                 unsigned long long* beat_out, double* phase_out) {
    TempoExtendedPhase out{};
    bool ok=deriveTempoPhase({beat,phase},p,q,epoch,wanted,out);
    *beat_out=out.beat_index; *phase_out=out.phase01; return ok;
}
int phase_project(unsigned long long beat, double phase, unsigned long long period,
                  unsigned long long reference, unsigned long long observed,
                  unsigned long long frame, unsigned long long epoch,
                  unsigned long long wanted, int valid,
                  unsigned long long* beat_out, double* phase_out) {
    k1::contract::TempoFieldCandidateV1 c{};
    c.beat_index=beat;c.phase01=phase;c.beat_period_q32=period;
    c.reference_frame=reference;c.last_observed_frame=observed;
    c.epoch=epoch;c.phase_valid=valid;
    TempoExtendedPhase out{};
    bool ok=projectTempoTrack(c,{wanted,frame},out);
    *beat_out=out.beat_index;*phase_out=out.phase01;return ok;
}
int phase_lifecycle(unsigned scenario) {
    using k1::contract::TempoFieldV1;
    using k1::contract::TempoTrackAdmission;
    TempoPhaseTracks state{};TempoFieldV1 f{};TempoAcfOutput acf{};
    TempoNoveltyRing samples{};samples[511]=1;
    f.enabled=true;f.valid=true;f.warming=false;f.activity_valid=true;f.silence=false;
    f.activity=1;f.epoch=7;f.support_end=1000000;f.support_start=448120;
    acf.valid=true;acf.comb_salience[60]=1;acf.point_salience[60]=.7F;
    updateTempoPhaseTracks(state,f,samples,acf,1080,true);
    if(f.count!=1 || !f.phase_authority || f.fresh_phase_count!=1) return 1;
    const auto original=f.candidates[0];
    if(scenario==0) {
      for(auto age:{47999ULL,48000ULL,48001ULL,95999ULL,96000ULL,96001ULL}) {
        auto copy=f;ageTempoPhaseTracks(copy,1000000+age);
        if(copy.candidates[0].phase_valid!=(age<=48000)) return 2;
        if(copy.candidates[0].last_observed_frame!=1000000 || copy.generation!=f.generation) return 3;
        if((copy.candidates[0].admission==TempoTrackAdmission::stale)!=(age>=96000)) return 4;
      }
    } else if(scenario==1) {
      acf.comb_salience[20]=2;acf.point_salience[20]=.5F;
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.count!=2) return 5;
      bool retained=false;
      for(unsigned i=0;i<f.count;++i) if(f.candidates[i].bin_id==60)
        retained=f.candidates[i].generation==original.generation && f.candidates[i].point_salience==.7F;
      if(!retained) return 6;
    } else if(scenario==2) {
      f.support_end+=1920; // .08 cycles at 120 BPM: exactly the limit.
      samples.fill(0);samples[511]=1;
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.candidates[0].generation!=original.generation) return 7;
      f.support_end+=1921;
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.candidates[0].generation==original.generation) return 8;
    } else if(scenario==3) {
      f.epoch=8;
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.candidates[0].epoch!=8 || f.candidates[0].generation==original.generation) return 9;
      updateTempoPhaseTracks(state,f,samples,acf,1080,false);
      if(f.phase_authority || f.fresh_phase_count) return 10;
    } else if(scenario==4) {
      acf.comb_salience.fill(0);f.activity=0;
      f.support_end+=48001;
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.phase_authority || f.fresh_phase_count) return 11;
      f.support_end=original.last_observed_frame+96000;
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.count) return 12;
    } else if(scenario==5) {
      state={};acf.comb_salience.fill(0);
      for(unsigned i=0;i<96;i+=10) acf.comb_salience[i]=1;
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.fresh_phase_count!=4 || f.count!=4) return 13;
      for(unsigned i=0;i<4;++i) if(f.candidates[i].bin_id!=i*10) return 14;
      acf.comb_salience.fill(0);
      for(unsigned i=5;i<96;i+=10) acf.comb_salience[i]=1;
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.fresh_phase_count>4 || f.count>8) return 15;
    } else if(scenario==6) {
      // Positive circular correction crosses wrap without changing identity.
      state.slots[0].phase01=.98;state.slots[0].beat_index=3;
      samples.fill(0);samples[511]=1;
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.candidates[0].generation!=original.generation) return 16;
      if(f.candidates[0].beat_index!=4) return 17;
    } else if(scenario==7) {
      acf.comb_salience[0]=std::numeric_limits<float>::quiet_NaN();
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.valid || f.phase_authority || f.count || f.fresh_phase_count) return 18;
    } else if(scenario==8) {
      state.slots[0].phase01=.02;state.slots[0].beat_index=3;
      samples.fill(0);samples[489]=1; // .99 phase at 120 BPM.
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.candidates[0].generation!=original.generation || f.candidates[0].beat_index!=2) return 19;
    } else if(scenario==9) {
      state.next_generation=UINT32_MAX;f.support_end+=48001;
      updateTempoPhaseTracks(state,f,samples,acf,1080,true);
      if(f.valid || f.phase_authority || f.count || f.fresh_phase_count) return 20;
    } else if(scenario==10) {
      f.count=255;
      ageTempoPhaseTracks(f,f.support_end);
      if(f.valid || f.count || f.phase_authority) return 21;
    } else if(scenario==11) {
      // Silence detector is current; old energetic support is deliberately retained.
      f.silence=true;
      for(auto age:{1ULL,47999ULL,48000ULL,48001ULL,95999ULL,96000ULL,96001ULL}) {
        f.support_end=original.last_observed_frame+age;
        updateTempoPhaseTracks(state,f,samples,acf,1080,true);
        if(f.fresh_phase_count) return 22;
        for(unsigned i=0;i<f.count;++i) {
          if(f.candidates[i].last_observed_frame!=0 &&
             f.candidates[i].last_observed_frame!=original.last_observed_frame)return 23;
          if(age>=96000 && f.candidates[i].generation==original.generation)return 25;
        }
        if(f.phase_authority!=(age<=48000))return 24;
      }
    }
    return 0;
}
int publication_case(unsigned scenario) {
    TempoTrackerState tracker{};TempoFieldPublisher publisher{};
    auto& e=tracker.evidence;e.current={7,1000000,1,true};e.completed=e.current;
    e.snapshot[511]=1;e.history_samples=512;e.generation=1;
    tracker.acf.valid=true;tracker.acf.comb_salience[60]=.8F;tracker.acf.point_salience[60]=.7F;
    e.raw_observed_valid[60]=true;e.raw_observed_frame[60]=990000;
    auto first=publishTempoField(publisher,tracker,{7,1000000},true,5000000);
    if(!first.valid || first.count!=1 || first.generation!=1) return 1;
    if(first.candidates[0].salience!=.8F || first.candidates[0].point_salience!=.7F) return 2;
    if(scenario==0) {
      auto copy=publishTempoField(publisher,tracker,{7,1000360},true,5007500);
      if(copy.generation!=1 || copy.support_end!=1000000 || copy.available_us!=5000000) return 3;
      if(copy.candidates[0].last_observed_frame!=1000000 || copy.candidates[0].observation_age_frames!=360) return 4;
      if(copy.candidates[0].raw_observation_age_frames!=10360) return 5;
      e.generation=2;e.completed.endpoint=1001080;
      auto delayed=publishTempoField(publisher,tracker,{7,1002880},true,5060000);
      if(delayed.generation!=2 || delayed.support_end!=1001080 || delayed.available_us!=5060000) return 6;
      if(delayed.support_start!=1001080-512*1080 || delayed.candidates[0].observation_age_frames!=1800) return 7;
      if(first.candidates[0].observation_age_frames!=0) return 8; // Independent owned copy.
    } else if(scenario==1) {
      auto f=publishTempoField(publisher,tracker,{8,1000360},true,5007500);
      if(f.valid || f.phase_authority || f.count) return 9;
      e.current={8,1001080,1,true};e.generation=2; // Old sliced completion cannot cross epoch.
      f=publishTempoField(publisher,tracker,{8,1001080},true,5015000);
      if(f.valid || f.phase_authority || f.count) return 10;
    } else if(scenario==2) {
      auto f=publishTempoField(publisher,tracker,{7,1000360},true,5007500,false,true);
      if(f.enabled || f.valid || f.count) return 11;
      f=publishTempoField(publisher,tracker,{7,1000720},true,5015000,true,true);
      if(f.valid || f.phase_authority || f.count) return 12;
      e.generation=2;e.completed.endpoint=1001080;
      f=publishTempoField(publisher,tracker,{7,1001080},true,5022500,true,false);
      if(!f.valid || f.phase_authority || f.fresh_phase_count) return 13;
      f=publishTempoField(publisher,tracker,{7,1001440},true,5030000,true,true);
      if(f.phase_authority || f.fresh_phase_count) return 14;
    } else if(scenario==3) {
      e.generation=2;e.snapshot[0]=std::numeric_limits<float>::max();
      auto f=publishTempoField(publisher,tracker,{7,1000360},true,5007500);
      if(f.activity_valid && !std::isfinite(f.activity)) return 15;
    } else if(scenario==4) {
      e.generation=2;e.completed.endpoint=1001080;
      auto f=publishTempoField(publisher,tracker,{7,1001080},true,4999999);
      if(f.valid || f.phase_authority) return 16;
    }
    return 0;
}
int evidence_case(unsigned scenario) {
    TempoTrackerState t{};initialiseTempoTracker(t);
    if(scenario<3) {
      for(unsigned i=0;i<512;++i)t.novelty_history[i]=float(i);
      t.history_index=73;t.evidence.current={7,1000000,1,true,5000};
      captureTempoEvidence(t);
      t.evidence.current.endpoint+=1800;t.evidence.current.wall_ms+=38;
      t.novelty_history.fill(0);
      if(scenario==1)t.evidence.current.epoch=8;
      if(scenario==2)t.evidence.current.configuration_revision=2;
      completeTempoEvidence(t);
      if(t.evidence.generation!=1 || t.evidence.completed.endpoint!=1000000) return 1;
      if(t.evidence.completed.valid!=(scenario==0)) return 2;
      if(t.evidence.phase_reference_wall_ms!=5000) return 3;
      for(unsigned i=0;i<512;++i)if(t.evidence.snapshot[i]!=float((73+i)%512))return 4;
    } else if(scenario==3) {
      t.evidence.generation=UINT64_MAX;t.evidence.current={7,1000000,1,true,5000};
      captureTempoEvidence(t);completeTempoEvidence(t);
      if(!t.evidence.invalid || t.evidence.completed.valid || t.evidence.generation!=UINT64_MAX)return 5;
    } else if(scenario==4) {
      for(unsigned hop=0;hop<13;++hop) {
        updateTempoTracker(t,{hop*8,.7F,false,1000000+hop*360,7,1,true});
        if(t.evidence.generation!=hop/3)return 6;
        if(hop%3 && t.evidence.completed.valid && t.evidence.completed.endpoint==t.evidence.current.endpoint)return 7;
      }
      unsigned observed=0;
      for(unsigned bin=0;bin<96;++bin)if(t.evidence.raw_observed_valid[bin]) {
        ++observed;
        if(t.evidence.raw_observed_frame[bin]>t.evidence.completed.endpoint)return 8;
      }
      if(observed!=8)return 9;
    }
    return 0;
}
int discontinuity_warmup_case(unsigned configuration_change) {
    TempoTrackerState tracker{};initialiseTempoTracker(tracker);
    TempoFieldPublisher publisher{};
    for(unsigned hop=0;hop<=1536;++hop) {
      updateTempoTracker(tracker,{hop*8,(hop%67<3)?.7F:.01F,false,1000000+hop*360,7,1,true});
      (void)publishTempoField(publisher,tracker,{7,1000000+hop*360},true,hop*7500);
    }
    if(tracker.evidence.history_samples!=512 || !publisher.field.valid) return 1;
    const auto epoch=configuration_change?7ULL:8ULL;
    const auto configuration=configuration_change?2U:1U;
    for(unsigned hop=1537;hop<=3072;++hop) {
      updateTempoTracker(tracker,{hop*8,(hop%67<3)?.7F:.01F,false,1000000+hop*360,epoch,configuration,true});
      auto f=publishTempoField(publisher,tracker,{epoch,1000000+hop*360},true,hop*7500);
      if(tracker.evidence.history_samples<512 && (f.valid || f.phase_authority))return 2;
      if(tracker.evidence.history_samples==512 && (!f.valid || f.warming))return 3;
    }
    if(tracker.evidence.history_samples!=512)return 4;
    return 0;
}
}
