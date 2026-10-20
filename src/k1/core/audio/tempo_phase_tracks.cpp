#include "core/audio/tempo_phase_tracks.h"
#include "core/audio/musical_time.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace k1::core::audio {
namespace {
constexpr double kTau = 6.283185307179586476925286766559;
double wrap(double x) noexcept { return x - std::floor(x); }
void invalidateTracks(TempoPhaseTracks& state, contract::TempoFieldV1& field) noexcept {
    field.valid=false;field.phase_authority=false;field.coasting=false;
    field.count=0;field.fresh_phase_count=0;field.candidates={};
    state.slots={}; // Preserve generation counter; exhaustion stays latched.
}
}
bool observeTempoPhase(const TempoNoveltyRing& samples, float bpm,
                       std::uint64_t interval, double& out) noexcept {
    out = 0.0;
    if (!std::isfinite(bpm) || bpm < 1.0F || bpm > 1000.0F ||
        interval == 0 || interval > 48000) return false;
    double c = 0.0, s = 0.0, energy = 0.0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        if (!std::isfinite(samples[i]) || samples[i] < 0.0F) return false;
        const double angle = kTau * static_cast<double>(bpm) / 60.0 *
            (static_cast<double>(samples.size()-1-i) * interval / 48000.0);
        c += samples[i] * std::cos(angle);
        s += samples[i] * std::sin(angle);
        energy += samples[i];
    }
    if (!(energy > 0.0) || std::hypot(c,s) <= energy * 1.0e-12) return false;
    out = wrap(std::atan2(s,c) / kTau);
    return std::isfinite(out);
}
bool projectTempoTrack(const contract::TempoFieldCandidateV1& t,
                       AudioTime now, TempoExtendedPhase& out) noexcept {
    out = {};
    if (!t.phase_valid || t.epoch != now.epoch_id || !t.beat_period_q32 ||
        !std::isfinite(t.phase01) || t.phase01 < 0.0 || t.phase01 >= 1.0 ||
        now.frame_index < t.reference_frame || now.frame_index < t.last_observed_frame ||
        now.frame_index-t.last_observed_frame > kTempoPhaseCoastFrames) return false;
    const double advance = static_cast<double>(now.frame_index-t.reference_frame) *
        static_cast<double>(kQ32One) / static_cast<double>(t.beat_period_q32);
    const double total = t.phase01 + advance;
    const double whole = std::floor(total);
    if (!std::isfinite(whole) || whole >= static_cast<double>(UINT64_MAX)) return false;
    const auto beats = static_cast<std::uint64_t>(whole);
    if (beats > UINT64_MAX-t.beat_index) return false;
    out = {t.beat_index+beats, wrap(total)};
    return true;
}
bool deriveTempoPhase(const TempoExtendedPhase& root, std::uint32_t p,
                      std::uint32_t q, std::uint64_t epoch,
                      std::uint64_t requested_epoch, TempoExtendedPhase& out) noexcept {
    out = {};
    if (!p || !q || epoch != requested_epoch || !std::isfinite(root.phase01) ||
        root.phase01 < 0.0 || root.phase01 >= 1.0) return false;
    const auto whole = root.beat_index / q;
    if (whole > UINT64_MAX/p) return false;
    std::uint64_t result = whole*p;
    const std::uint64_t tail = (root.beat_index % q)*p;
    const double fraction_product = root.phase01 * p;
    const auto fraction_whole = static_cast<std::uint64_t>(fraction_product);
    if (tail > UINT64_MAX-fraction_whole) return false;
    const auto numerator = tail+fraction_whole;
    if (result > UINT64_MAX-numerator/q) return false;
    result += numerator/q;
    double fraction = (static_cast<double>(numerator%q) +
                       fraction_product-fraction_whole)/q;
    if (fraction >= 1.0) { if (result == UINT64_MAX) return false; ++result; fraction=0.0; }
    out = {result,fraction};
    return true;
}
void ageTempoPhaseTracks(contract::TempoFieldV1& field, std::uint64_t now) noexcept {
    field.phase_authority=false; field.coasting=false;
    if(field.count>contract::kTempoFieldCapacity) { field.valid=false;field.count=0;return; }
    for (std::size_t i=0;i<field.count;++i) {
        auto& t=field.candidates[i];
        if (now < t.last_observed_frame) { t.phase_valid=false; t.admission=contract::TempoTrackAdmission::unavailable; continue; }
        t.observation_age_frames=now-t.last_observed_frame;
        if (t.raw_observation_valid) {
            if (now<t.raw_observed_frame) t.raw_observation_valid=false;
            else t.raw_observation_age_frames=now-t.raw_observed_frame;
        }
        if (t.observation_age_frames>=kTempoPhaseStaleFrames) {
            t.phase_valid=false; t.admission=contract::TempoTrackAdmission::stale;
        } else if (t.observation_age_frames>0) {
            t.admission=contract::TempoTrackAdmission::coasting;
            if (t.observation_age_frames>kTempoPhaseCoastFrames) t.phase_valid=false;
        }
        field.phase_authority |= t.phase_valid;
        field.coasting |= t.admission==contract::TempoTrackAdmission::coasting;
    }
}
void updateTempoPhaseTracks(TempoPhaseTracks& state, contract::TempoFieldV1& field,
    const TempoNoveltyRing& samples, const TempoAcfOutput& acf,
    std::uint64_t interval, bool phase_enabled) noexcept {
    using contract::TempoTrackAdmission;
    std::array<std::uint16_t,contract::kTempoFieldFreshCapacity> selected{};
    std::size_t selected_count=0;
    std::array<bool,kTempoAcfBinCount> used{};
    float maximum=0;
    bool finite=true;
    for (std::size_t i=0;i<kTempoAcfBinCount;++i) {
        const auto s=acf.comb_salience[i];
        if (!std::isfinite(s)||s<0||!std::isfinite(acf.point_salience[i])||acf.point_salience[i]<0) finite=false;
        maximum=std::max(maximum,s);
    }
    if (!finite) {
        field.valid=false; field.activity_valid=false; field.activity=0;
        invalidateTracks(state,field);return;
    }
    if (field.activity_valid && field.activity>0 && maximum>0) {
      for (std::size_t pass=0;pass<selected.size();++pass) {
        int best=-1;
        for (std::size_t i=0;i<kTempoAcfBinCount;++i) {
            if (used[i] || (i>0 && acf.comb_salience[i]<=acf.comb_salience[i-1]) ||
                (i+1<kTempoAcfBinCount && acf.comb_salience[i]<acf.comb_salience[i+1])) continue;
            bool retained=false;
            for (const auto& t:state.slots) if(t.generation && t.bin_id==i) retained=true;
            if (acf.comb_salience[i]<maximum*(retained?0.1F:0.2F)) continue;
            bool separated=true;
            for(std::size_t j=0;j<selected_count;++j)
                if(std::fabs(static_cast<float>(i)-selected[j])/(60.0F+std::min<std::size_t>(i,selected[j]))<kTempoPeakSeparation) separated=false;
            if (!separated) continue;
            if(best<0 || acf.comb_salience[i]>acf.comb_salience[best]) best=static_cast<int>(i);
        }
        if(best<0) break;
        selected[selected_count++]=static_cast<std::uint16_t>(best); used[best]=true;
      }
    }
    for(auto& t:state.slots) {
      if(t.generation && (t.epoch!=field.epoch || field.support_end<t.last_observed_frame ||
         field.support_end-t.last_observed_frame>=kTempoPhaseStaleFrames)) t={};
      if(t.generation) t.admission=TempoTrackAdmission::coasting;
      if(!phase_enabled) t.phase_valid=false;
    }
    field.fresh_phase_count=0;
    for(std::size_t j=0;j<selected_count;++j) {
      const auto id=selected[j];
      contract::TempoFieldCandidateV1* t=nullptr;
      for(auto& slot:state.slots) if(slot.generation && slot.bin_id==id) t=&slot;
      if(!t) for(auto& slot:state.slots) if(!slot.generation) {t=&slot;break;}
      if(!t) continue; // Full overlap capacity: never evict a live object silently.
      const bool is_new=!t->generation;
      double phase=0;
      // A historic window remains useful salience evidence during silence,
      // but cannot refresh the last direct phase observation or coast budget.
      const bool observed=phase_enabled && !field.warming && !field.silence &&
          observeTempoPhase(samples,60.0F+id,interval,phase);
      TempoExtendedPhase predicted{};
      const bool projected=projectTempoTrack(*t, AudioTime{field.epoch,field.support_end},predicted);
      double error=0;
      if(observed && projected) {error=phase-predicted.phase01; error-=std::floor(error+0.5);}
      const bool restart=is_new || (observed && (!projected || std::fabs(error)>kTempoPhaseCorrectionLimit));
      if(restart) {
        if(state.next_generation==UINT32_MAX) {invalidateTracks(state,field);return;}
        *t={}; t->generation=state.next_generation++; t->beat_index=0;
      } else if(observed) {
        t->beat_index=predicted.beat_index;
        const double corrected=predicted.phase01+error;
        if(corrected>=1.0) {
          if(t->beat_index==UINT64_MAX) {t->phase_valid=false;continue;}
          ++t->beat_index;
        } else if(corrected<0.0) {
          if(t->beat_index==0) {
            if(state.next_generation==UINT32_MAX) {invalidateTracks(state,field);return;}
            t->generation=state.next_generation++;
          } else --t->beat_index;
        }
      }
      t->bin_id=id; t->bpm=60.0F+id; t->epoch=field.epoch;
      t->salience=acf.comb_salience[id];t->point_salience=acf.point_salience[id];
      t->support_start=field.support_start;t->support_end=field.support_end;
      t->root_id=id;t->root_generation=t->generation;
      t->beat_period_q32=beatPeriodFromBpm(t->bpm);
      t->admission=TempoTrackAdmission::fresh;
      if(observed) {
        t->phase01=phase;t->reference_frame=field.support_end;t->last_observed_frame=field.support_end;
        t->phase_valid=true; ++field.fresh_phase_count;
      } else if(!phase_enabled || field.warming) t->phase_valid=false;
    }
    field.count=0;
    for(const auto& t:state.slots) if(t.generation) field.candidates[field.count++]=t;
    ageTempoPhaseTracks(field,field.support_end);
}
} // namespace k1::core::audio
