#pragma once
#include "composed_scene_config.h"
#include "composed_motion.h"
#include "musical_conditioning.h"
#include "emotiscope_ml.h"
#include "composed_timing.h"
#include <initializer_list>

namespace composed_scene {
using composed_motion::unit;
enum class Phrase : uint32_t { Rest, Establish, Develop, Tension, Release, Recover };

// Composition states describe our authored behaviour, never detected song parts.
class Direction {
 public:
  void update(const K1EmotiscopeAudioPublication& p,uint32_t now,uint32_t policy) {
    if(seen_ && now==last_)return;
    const float dt=seen_?float(now-last_)*.001f:.01f;
    last_=now;seen_=true;
    const float level=p.valid?unit(p.vu_level):0.0f;
    const float prior=energy_;
    energy_+=(level-energy_)*(1.0f-std::exp(-dt/.8f));
    trend_+=((dt>0?(energy_-prior)/dt:0)-trend_)*(1.0f-std::exp(-dt/.3f));
    // Confirm quiet from raw level plus capture validity, not low novelty.
    const bool silent=!p.valid || !(p.vu_level_raw>p.vu_floor+0.00001f);
    if(!silent)last_sound_=now;
    quiet_=silent && now-last_sound_>=300u;
    if(policy==4u){select(Phrase::Rest,now);return;}
    if(policy==3u){if(previous_policy_!=3u)select(Phrase::Release,now);previous_policy_=policy;return;}
    previous_policy_=policy;
    if(policy==2u)return; // hold actual current composition state
    if(policy==0u){select(quiet_?Phrase::Rest:Phrase::Establish,now);return;}
    if(now-since_<1600u)return;
    if(quiet_){select(Phrase::Rest,now);return;}
    switch(state_){
      case Phrase::Rest: select(Phrase::Establish,now);break;
      case Phrase::Establish: if(now-since_>=3200u)select(Phrase::Develop,now);break;
      case Phrase::Develop: if(energy_>.45f && trend_>.02f)select(Phrase::Tension,now);break;
      case Phrase::Tension: if((trend_<-.02f && p.novelty_normalised>.15f)||now-since_>=6000u)select(Phrase::Release,now);break;
      case Phrase::Release: if(now-since_>=2400u)select(Phrase::Recover,now);break;
      case Phrase::Recover: if(now-since_>=3200u)select(Phrase::Establish,now);break;
    }
  }
  bool admit()const{return state_!=Phrase::Rest && state_!=Phrase::Recover;}
  float density()const{return state_==Phrase::Tension?1.0f:state_==Phrase::Develop?.75f:.5f;}
  float injection()const{return state_==Phrase::Rest||state_==Phrase::Recover?0.0f:state_==Phrase::Tension?1.0f:.65f;}
  Phrase state()const{return state_;}
  bool quiet()const{return quiet_;}
 private:
  void select(Phrase s,uint32_t now){if(s!=state_){state_=s;since_=now;}}
  Phrase state_=Phrase::Rest;
  float energy_=0,trend_=0;
  uint32_t since_=0,last_=0,last_sound_=0,previous_policy_=0;
  bool seen_=false,quiet_=true;
};

struct Board {
  K1ComposedSceneConfig requested=k1_composed_scene_defaults(),active=requested;
  composed_motion::Renderer motion;
  composed_motion::Rgb pixels[128]{};
  K1MusicalConditioning admission;
  K1EmotiscopeAudioPublication conditioned{};
  Direction direction;
  uint32_t now=0,now_us=0,last_us=0,last_cursor=0,event=0,rendered_us=0;
  float level=0,strength=0,model_delta=0;
  bool observed=false,rendered=false,fresh_event=false,forced_quiet=false;
  bool release_requested=false;
  uint32_t pending_event_time=0;float pending_strength=0;bool pending_event=false;
  bool transition_pending=false,pair_held=false,pair_drain=false,bloom_inward=false,bloom_launched=false;
  uint32_t bloom_started=0,bloom_gesture=0;float bloom_reference=0;
  composed_motion::PaletteSnapshot bloom_identity{};
  composed_motion::Palette palette=nullptr;
  void* palette_context=nullptr;
  static float gestureStrength(float energy,float accent){
    // Perceptual compression of an admitted event; zero remains zero. A
    // threshold-crossing sample must not condemn its whole journey to near-black.
    return std::sqrt(std::sqrt(unit(energy)))*(.75f+.25f*unit(accent));
  }
  void initialise(const float hz[64]){
    admission.initialise_frequencies(hz);
    admission.configure({1u,1.0f,40.0f,160.0f});
  }
  bool configure(const K1ComposedSceneConfig& c){
    if(!k1_composed_scene_valid(c))return false;
    requested=c;return true;
  }
  composed_motion::Config motionConfig(bool linked=false)const{
    composed_motion::Config c;
    c.out_travel_ms=uint32_t(active.outgoing_ms);c.return_travel_ms=uint32_t(active.return_ms);
    c.out_width=active.outgoing_width;c.return_width=active.return_width;
    c.out_feather=active.outgoing_feather;c.return_feather=active.return_feather;
    c.return_strength=active.return_strength;c.trail_ms=uint32_t(active.trail_ms);
    c.separation_ms=uint32_t(active.separation_ms);c.quiet_ms=uint32_t(active.quiet_ms);
    c.recovery_ms=uint32_t(active.recovery_ms);c.trips=linked?0u:uint8_t(active.trips);
    if(active.phrase==1u){
      const auto phrase=direction.state();
      const float size=phrase==Phrase::Tension?1.15f:phrase==Phrase::Develop?1.0f:.85f;
      c.out_width=std::fmax(.02f,std::fmin(.8f,c.out_width*size));
      c.separation_ms=uint32_t(float(c.separation_ms)*(phrase==Phrase::Tension?.75f:1.0f));
      c.recovery_ms=uint32_t(float(c.recovery_ms)*(phrase==Phrase::Establish?1.4f:1.0f));
      if(c.recovery_ms>8000u)c.recovery_ms=8000u;
    }
    // Influence is latched by Renderer at launch, and never raises the return gain.
    if(active.ml_target==1u)c.out_width=(active.scene==3u&&motion.historyCount())?motion.config().out_width:std::fmax(.02f,std::fmin(.8f,c.out_width+model_delta*.08f));
    if(active.ml_target==3u)c.return_width=(active.scene==3u&&motion.historyCount())?motion.config().return_width:std::fmax(.02f,std::fmin(.8f,c.return_width+model_delta*.04f));
    return c;
  }
  void observe(const K1EmotiscopeAudioPublication& p,const K1EmotiscopeMlResult& ml,
               const K1EmotiscopeMlConfig& mc,uint32_t stamp,uint32_t milliseconds){
    if(observed && stamp==now_us)return;
    now=milliseconds;now_us=stamp;
    const float dt=observed?float(stamp-last_us)*.000001f:.01f;
    last_us=stamp;observed=true;
    admission.apply(p,conditioned,dt,stamp);
    fresh_event=admission.event_cursor()!=last_cursor;
    last_cursor=admission.event_cursor();
    event=last_cursor;
    level=p.valid?unit(p.vu_level):0.0f;
    strength=gestureStrength(level,admission.value());
    model_delta=0;
    if(mc.enabled && EmotiscopeMl::usable(p,ml,stamp))
      model_delta=(unit(ml.latent[0])-unit(ml.latent[1]))*unit(mc.strength);
    const auto prior_phrase=direction.state();
    direction.update(p,now,requested.phrase);
    if(direction.state()==Phrase::Release && prior_phrase!=Phrase::Release)release_requested=true;
    if(requested.phrase==4u)release_requested=false;
    transition_pending=std::memcmp(&active,&requested,sizeof(active))!=0;
    // Whole scene/policy changes wait until both gesture/history have ended.
    if(!pair_held && motion.ready() && motion.historyCount()==0u){active=requested;transition_pending=false;}
    else if(active.scene==3u && transition_pending){
      // Bloom stops injecting while old material drains; its travel stays latched.
      motion.configure(motionConfig());
    }
  }
  const composed_motion::PaletteSnapshot& identity()const{return active.scene==3u?bloom_identity:motion.paletteSnapshot();}
  static composed_motion::Rgb retainedPalette(float u,void* context){return composed_motion::sample(static_cast<Board*>(context)->bloom_identity,u);}
  bool finished()const{return motion.ready() && !motion.historyCount() &&
      !(active.scene==3u && pair_held && bloom_launched && now-bloom_started<uint32_t(active.outgoing_ms*.3f));}
  bool available()const{return active.scene && finished();}
  bool eligible()const{return available() && direction.admit();}
  void render(uint32_t launch=0,float launch_strength=0,bool inward=false,
              const composed_motion::PaletteSnapshot* identity=nullptr,bool linked=false){
    forced_quiet=false;
    if(launch){release_requested=false;pending_event=false;}
    if(!active.scene){for(auto& p:pixels)p={};rendered_us=now_us;rendered=true;return;}
    motion.configure(motionConfig(linked));
    if(active.scene==3u){
      float feed=(transition_pending||pair_drain)?0.0f:level*direction.injection();
      if(linked){
        if(launch){bloom_started=now;bloom_gesture=launch;bloom_reference=launch_strength;
          bloom_inward=inward;bloom_launched=true;
          bloom_identity=identity?*identity:composed_motion::capture(palette,palette_context);}
        // One bounded stream per pair intent; no continuous unscheduled answer.
        feed=bloom_launched && now-bloom_started<uint32_t(active.outgoing_ms*.3f)?bloom_reference*(bloom_inward?active.return_strength:1.0f):0.0f;
        motion.renderBloom(now,feed,&retainedPalette,this,pixels);
        if(bloom_inward)for(unsigned i=0;i<32;++i){
          const auto p=pixels[64+i];pixels[64+i]=pixels[127-i];pixels[127-i]=p;
          pixels[63-i]=pixels[64+i];pixels[i]=pixels[127-i];
        }
      }else motion.renderBloom(now,feed,palette,palette_context,pixels);
    }else{
      motion.renderRibbon(now,launch,launch_strength,palette,palette_context,pixels,inward,identity);
    }
    rendered_us=now_us;rendered=true;
  }
  void silence(){for(auto& p:pixels)p={};forced_quiet=true;}
  K1ComposedSceneStatus status()const{
    return {active.scene,uint32_t(motion.phase()),uint32_t(transition_pending||motion.pending()),
      (active.scene==3u?bloom_gesture:motion.gestureId()),uint32_t(direction.state()),uint32_t(forced_quiet||motion.quiet()||(active.scene==3u && !motion.historyCount() && !direction.admit()))};
  }
};

// One pair owner sends intent and retained palette identity. It never reads the
// caller's RGB output. Board instances exclusively own their transport history.
class Pair {
 public:
  bool configureTiming(const float hz[96]){return clock_.configure(hz);}
  void updateTiming(const K1EmotiscopeAudioPublication& p,uint32_t now){clock_.update(p,now);}
  void render(Board* a,Board* b,uint32_t stamp){
    if(seen_ && stamp==last_)return;
    seen_=true;last_=stamp;
    if(!a||!b)return;
    Board* boards[2]={a,b};
    const auto& policy=a->active;
    const uint32_t mode=(a->active.scene==2u||b->active.scene==2u)?1u:policy.choreography;
    const bool blocked=barrier_||a->motion.quietHeld()||b->motion.quietHeld();
    if(stage_==0u && mode==0u){
      for(auto* x:boards){
        const uint32_t event=admit(x,blocked)?nextId():0u;
        x->render(event,x->pending_strength);
      }
      protect(a,b);return;
    }
    uint32_t launch[2]={0,0};float gain[2]={0,0};bool inward[2]={false,false};
    const composed_motion::PaletteSnapshot* identities[2]={nullptr,nullptr};
    if(stage_==0u){
      initiator_=policy.initiator;mode_=mode;
      auto* call=boards[initiator_];auto* answer=boards[1u-initiator_];
      if(answer->eligible() && admit(call,blocked)){
        gesture_=nextId();launch[initiator_]=gesture_;gain[initiator_]=call->pending_strength;
        started_=call->now;reference_=call->pending_strength;delay_=uint32_t(policy.answer_delay_ms);stage_=1u;
        call->pair_held=true;answer->pair_held=true;
      }
    }else{
      auto* call=boards[initiator_];auto* answer=boards[1u-initiator_];
      if(stage_==1u && ((call->active.scene==3u && call->now-started_>=uint32_t(call->active.outgoing_ms)) || call->motion.edgeArrived() || (mode_==2u && call->now-started_>=uint32_t(call->active.outgoing_ms*.25f)))){
        identity_=call->identity();handoff_=call->now;stage_=2u;
      }
      if(!blocked && stage_==2u && call->now-handoff_>=delay_ && answer->available() && !call->motion.quiet()){
        launch[1u-initiator_]=gesture_;gain[1u-initiator_]=reference_;
        inward[1u-initiator_]=true;identities[1u-initiator_]=&identity_;stage_=3u;
      }else if(!blocked && stage_==3u && answer->finished() && call->finished()){
        launch[initiator_]=nextId();gain[initiator_]=reference_*answer->active.return_strength;
        inward[initiator_]=true;identities[initiator_]=&identity_;stage_=4u;
      }else if(stage_==4u && call->finished() && answer->finished()){
        call->pair_held=false;answer->pair_held=false;stage_=0u;
      }
    }
    for(unsigned i=0;i<2;++i)boards[i]->render(launch[i],gain[i],inward[i],identities[i],true);
    protect(a,b);
  }
 private:
  bool admit(Board* board,bool blocked){
    if(blocked||!board->eligible()||board->strength<=0.0f){board->pending_event=false;return false;}
    const bool first=board->motion.gestureId()==0u && board->bloom_gesture==0u && board->level>.02f;
    if(first||board->release_requested){board->pending_strength=board->strength;return true;}
    if(board->fresh_event&&!board->pending_event){board->pending_event=true;board->pending_event_time=board->now;board->pending_strength=board->strength;}
    return board->pending_event && (board->active.phrase!=1u||!clock_.locked()||clock_.boundary()||board->now-board->pending_event_time>=250u);
  }
  composed_timing::Clock clock_;
  uint32_t nextId(){if(++counter_==0u)++counter_;return counter_;}
  void protect(Board* a,Board* b){
    const bool pair=(a->active.quiet_scope==2u||b->active.quiet_scope==2u);
    if(!pair && !barrier_)return;
    if(a->active.scene==3u && b->active.scene==3u && !a->motion.historyCount() &&
       !b->motion.historyCount() && (!a->direction.admit()||!b->direction.admit())){
      a->silence();b->silence();return;
    }
    for(auto* board:{a,b})if(board->active.scene!=3u)board->motion.holdQuiet(board->now);
    if(a->motion.quietHeld()||b->motion.quietHeld())barrier_=true;
    if(!barrier_)return;
    a->pair_drain=b->pair_drain=true;
    const auto clear=[](Board* x){return !x->motion.historyCount() &&
      (x->motion.ready()||x->motion.quietHeld()||x->motion.phase()==composed_motion::Phase::Recovery);};
    if(!plateau_ && clear(a)&&clear(b)){plateau_=true;quiet_start_=a->now;}
    if(!plateau_)return;
    const uint32_t duration=uint32_t(std::fmax(a->active.quiet_ms,b->active.quiet_ms));
    if(a->now-quiet_start_>=duration){
      a->motion.releaseQuiet(a->now);b->motion.releaseQuiet(b->now);
      a->pair_drain=b->pair_drain=false;barrier_=plateau_=false;
    }else {a->silence();b->silence();}
  }
  bool barrier_=false,plateau_=false;
  uint32_t quiet_start_=0;
  composed_motion::PaletteSnapshot identity_{};
  uint32_t last_=0,counter_=0,gesture_=0,stage_=0,mode_=0,initiator_=0,handoff_=0,delay_=0,started_=0;
  float reference_=0;
  bool seen_=false;
};
} // namespace composed_scene
