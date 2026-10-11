#pragma once
// Fixed-storage scene transport. All time is unsigned monotonic milliseconds;
// consecutive calls must be less than 2^31 ms apart. No hardware/output control.
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace composed_motion {
struct Rgb { float r=0, g=0, b=0; };
using Palette = Rgb (*)(float, void*);
struct PaletteSnapshot { Rgb colour[16]{}; };
enum class Phase : std::uint8_t { Ready, Outward, Quiet, Return, Recovery };
struct Config {
  std::uint32_t out_travel_ms=1100, return_travel_ms=1500;
  float out_width=0.16f, return_width=0.24f;
  float out_feather=0.3f, return_feather=0.5f;
  float out_strength=1.0f, return_strength=0.38f;
  std::uint32_t separation_ms=240, quiet_ms=420, trail_ms=180, recovery_ms=240;
  std::uint8_t trips=1; // 0: outward only; 1/2: complete edge returns.
};
inline bool finite(float x) { std::uint32_t u; std::memcpy(&u,&x,4); return (u&0x7f800000u)!=0x7f800000u; }
inline float unit(float x) { return !finite(x)||x<0 ? 0 : x>1 ? 1 : x; }
inline Rgb clean(Rgb c) { return {unit(c.r),unit(c.g),unit(c.b)}; }
inline Rgb scale(Rgb c,float s) { return {c.r*s,c.g*s,c.b*s}; }
inline Rgb mix(Rgb a,Rgb b,float x) { return {a.r+(b.r-a.r)*x,a.g+(b.g-a.g)*x,a.b+(b.b-a.b)*x}; }
inline Rgb add(Rgb a,Rgb b) { return {a.r+b.r,a.g+b.g,a.b+b.b}; }
inline std::uint32_t elapsed(std::uint32_t now,std::uint32_t then) { return now-then; }
inline PaletteSnapshot capture(Palette sampler,void* context) {
  PaletteSnapshot p;
  if(sampler) for(unsigned i=0;i<16;++i) p.colour[i]=clean(sampler(static_cast<float>(i)/15.0f,context));
  return p;
}
inline Rgb sample(const PaletteSnapshot& p,float u) {
  const float x=unit(u)*15.0f; const unsigned i=static_cast<unsigned>(x);
  return i==15 ? p.colour[15] : mix(p.colour[i],p.colour[i+1],x-static_cast<float>(i));
}
class Renderer {
 public:
  static constexpr unsigned kPixels=128, kHistory=1920, kHopMs=25;
  static_assert(kHistory*kHopMs>46800, "History must cover maximum two trips and material trail");
  bool configure(const Config& c) {
    if(!valid(c)) return false;
    if(equal(c,requested_)) return true;
    requested_=c;
    if(active()||history_alive_) pending_=true; else { config_=c; pending_=false; }
    return true;
  }
  const Config& config() const { return config_; }
  const Config& requestedConfig() const { return requested_; }
  bool pending() const { return pending_; }
  Phase phase() const { return phase_; }
  bool ready() const { return phase_==Phase::Ready; }
  bool active() const { return !ready(); }
  bool quiet() const { return phase_==Phase::Quiet; }
  bool edgeArrived() const { return edge_arrived_; }
  // Pair barrier: arm while outbound, including when authored quiet is zero.
  // The real tail still clears first. Holding never touches emission or clocks.
  void holdQuiet(std::uint32_t now) {
    if(!active() || inward_ || quiet_released_) return;
    if(phase_==Phase::Outward || phase_==Phase::Quiet) {
      quiet_hold_=true; updateRibbon(now);
    }
  }
  bool quietHeld() const { return quiet_hold_ && quiet(); }
  // Pair has supplied the authored quiet interval. Continue at the beginning
  // of this board's own return, preserving its captured colour and reference.
  void releaseQuiet(std::uint32_t now) {
    if(!quietHeld()) return;
    quiet_hold_=false; quiet_released_=true;
    start_=now-(outDuration()+latched_.quiet_ms);
    updateRibbon(now);
  }
  std::uint32_t gestureId() const { return gesture_; }
  const PaletteSnapshot& paletteSnapshot() const { return palette_; }
  float referenceStrength() const { return reference_; }
  unsigned historyCount() const { return history_alive_?history_count_:0u; }
  void reset() { *this=Renderer{}; }
  // Inward intent is an independently scheduled answer, using a transferred
  // snapshot/reference when supplied. Events observed while busy are discarded.
  void renderRibbon(std::uint32_t now,std::uint32_t event,float strength,
                    Palette sampler,void* context,Rgb* output,
                    bool inward=false,const PaletteSnapshot* identity=nullptr) {
    clear(output); edge_arrived_=false;
    updateRibbon(now);
    if(event && event!=last_event_) {
      last_event_=event;
      if(ready() && event && unit(strength)>0) {
        applyPending(); latched_=config_; start_=now; gesture_=event;
        reference_=unit(strength); inward_=inward;quiet_hold_=false;quiet_released_=false;
        palette_=identity?*identity:capture(sampler,context);
        for(auto& c:palette_.colour)c=clean(c);
        phase_=inward?Phase::Return:Phase::Outward;
      }
    }
    if(!active()) return;
    updateRibbon(now);
    if(quiet()||phase_==Phase::Recovery||ready()) return;
    const std::uint32_t age=elapsed(now,start_);
    const std::uint32_t out_end=outDuration();
    const std::uint32_t ret_duration=returnDuration();
    float attenuation=1.0f;
    std::uint32_t leg_age=age;
    bool returning=inward_;
    if(!inward_ && age>=out_end+latched_.quiet_ms) {
      std::uint32_t t=age-out_end-latched_.quiet_ms;
      returning=true;
      if(t>=ret_duration) { t-=ret_duration; attenuation=latched_.return_strength;
        if(t<out_end) returning=false;
        else {t-=out_end; returning=true;}
      }
      leg_age=t;
    }
    const float travel=static_cast<float>(returning?latched_.return_travel_ms:latched_.out_travel_ms);
    const float width=returning?latched_.return_width:latched_.out_width;
    const float feather=(returning?latched_.return_feather:latched_.out_feather)*width*0.5f;
    const float gain=reference_*latched_.out_strength*attenuation*(returning?latched_.return_strength:1.0f)*0.5f;
    for(unsigned i=0;i<64;++i) {
      const float x=static_cast<float>(i)/63.0f;
      Rgb c{};
      for(unsigned band=0;band<2;++band) {
        const std::uint32_t delay=band*latched_.separation_ms;
        if(leg_age<delay) continue;
        const float age_ms=static_cast<float>(leg_age-delay);
        const float centre=age_ms/travel;
        const float path=returning?1.0f-x:x;
        // Analytic trail integrates a finite delayed copy, never a frame decay.
        for(unsigned trail=0;trail<3;++trail) {
          const float lag=static_cast<float>(latched_.trail_ms)*static_cast<float>(trail)*0.5f;
          if(age_ms<lag)continue;
          const float delta=path-(centre-lag/travel);
          const float shape=envelope(delta,width,feather);
          const float colour_u=unit(0.5f+delta/width);
          c=add(c,scale(sample(palette_,colour_u),shape*gain*(trail==0?0.66f:0.17f)));
        }
      }
      output[63-i]=output[64+i]=clean(c);
    }
  }
  // Continuous RGB injection onto a 25 ms content clock. History stores colour,
  // not audio magnitude; later palette/level edits cannot recolour old material.
  // configure() drains old history by stopping injection before applying changes.
  void renderBloom(std::uint32_t now,float level,Palette sampler,void* context,Rgb* output) {
    clear(output); edge_arrived_=false; phase_=Phase::Ready;
    if(!bloom_clock_) { bloom_clock_=true; newest_=now; history_count_=1; history_[0]={}; next_=1; }
    const std::uint32_t delta=elapsed(now,newest_);
    if(delta>=0x80000000u) return; // stale timestamp
    unsigned steps=delta/kHopMs;
    if(steps>kHistory) { history_count_=0; next_=0; newest_=now-now%kHopMs; steps=0; }
    for(unsigned n=0;n<steps;++n) {
      newest_+=kHopMs;
      Rgb colour{};
      if(!pending_ && unit(level)>0 && sampler) {
        if(!history_alive_) latched_=config_;
        // Curated palette traversal, never hue synthesis or hue-wheel fallback.
        const float u=static_cast<float>(newest_%1600u)/1599.0f;
        colour=scale(clean(sampler(u,context)),unit(level));
        if(colour.r+colour.g+colour.b>0) { last_injection_=newest_; history_alive_=true; }
      }
      history_[next_]=colour; next_=(next_+1)%kHistory;
      if(history_count_<kHistory)++history_count_;
    }
    const auto& c=latched_;
    const std::uint32_t lifetime=(c.out_travel_ms+c.return_travel_ms)*c.trips+(c.trips?0:c.out_travel_ms )+c.trail_ms+kHopMs+static_cast<std::uint32_t>(c.out_width*static_cast<float>(c.out_travel_ms)+c.return_width*static_cast<float>(c.return_travel_ms));
    if(history_alive_ && elapsed(now,last_injection_)>lifetime) { history_alive_=false; history_count_=0; next_=0; applyPending(); }
    if(!history_alive_) { if(pending_){history_count_=0;applyPending();} return; }
    phase_=Phase::Outward;
    const float headroom=1.0f/(1.0f+c.return_strength+(c.trips==2?c.return_strength+c.return_strength*c.return_strength:0.0f));
    for(unsigned i=0;i<64;++i) {
      const float x=static_cast<float>(i)/63.0f;
      Rgb v=bloomTap(now,x*static_cast<float>(c.out_travel_ms),false);
      if(c.trips) v=add(v,scale(bloomTap(now,static_cast<float>(c.out_travel_ms)+(1-x)*static_cast<float>(c.return_travel_ms),true),c.return_strength));
      if(c.trips==2) {
        const float cycle=static_cast<float>(c.out_travel_ms+c.return_travel_ms);
        v=add(v,scale(bloomTap(now,cycle+x*static_cast<float>(c.out_travel_ms),false),c.return_strength));
        v=add(v,scale(bloomTap(now,cycle+static_cast<float>(c.out_travel_ms)+(1-x)*static_cast<float>(c.return_travel_ms),true),c.return_strength*c.return_strength));
      }
      output[63-i]=output[64+i]=clean(scale(v,headroom));
    }
    edge_arrived_=elapsed(now,last_injection_)>=c.out_travel_ms;
  }
 private:
  static bool equal(const Config& a,const Config& b) {
    return a.out_travel_ms==b.out_travel_ms&&a.return_travel_ms==b.return_travel_ms&&a.out_width==b.out_width&&a.return_width==b.return_width&&a.out_feather==b.out_feather&&a.return_feather==b.return_feather&&a.out_strength==b.out_strength&&a.return_strength==b.return_strength&&a.separation_ms==b.separation_ms&&a.quiet_ms==b.quiet_ms&&a.trail_ms==b.trail_ms&&a.recovery_ms==b.recovery_ms&&a.trips==b.trips;
  }
  static bool valid(const Config& c) {
    return c.out_travel_ms>=200&&c.out_travel_ms<=8000&&c.return_travel_ms>=200&&c.return_travel_ms<=8000&&
      c.separation_ms<=5000&&c.quiet_ms<=8000&&c.trail_ms<=2000&&c.recovery_ms<=8000&&c.trips<=2&&
      finite(c.out_width)&&c.out_width>=0.02f&&c.out_width<=0.8f&&finite(c.return_width)&&c.return_width>=0.02f&&c.return_width<=0.8f&&
      finite(c.out_feather)&&c.out_feather>=0&&c.out_feather<=1&&finite(c.return_feather)&&c.return_feather>=0&&c.return_feather<=1&&
      finite(c.out_strength)&&c.out_strength>0&&c.out_strength<=1&&finite(c.return_strength)&&c.return_strength>=0&&c.return_strength<=0.95f;
  }
  static void clear(Rgb* p) { for(unsigned i=0;i<kPixels;++i)p[i]={}; }
  static float envelope(float delta,float width,float feather) {
    const float a=std::fabs(delta), half=width*0.5f;
    if(a>=half)return 0;
    if(feather<=0)return 1.0f;
    const float u=unit((half-a)/feather); return u*u*(3.0f-2.0f*u);
  }
  void applyPending() { if(pending_){config_=requested_;pending_=false;} }
  std::uint32_t outDuration()const { return latched_.separation_ms+latched_.out_travel_ms+static_cast<std::uint32_t>(std::ceil(latched_.out_width*0.5f*static_cast<float>(latched_.out_travel_ms)))+latched_.trail_ms; }
  std::uint32_t returnDuration()const { return latched_.separation_ms+latched_.return_travel_ms+static_cast<std::uint32_t>(std::ceil(latched_.return_width*0.5f*static_cast<float>(latched_.return_travel_ms)))+latched_.trail_ms; }
  void updateRibbon(std::uint32_t now) {
    if(!active())return;
    const auto t=elapsed(now,start_); if(t>=0x80000000u)return;
    const auto out=outDuration(), ret=returnDuration();
    if(quiet_hold_ && !inward_ && t>=out) { phase_=Phase::Quiet;edge_arrived_=true;return; }
    const auto return_start=out+latched_.quiet_ms;
    const auto end=inward_?ret:return_start+(latched_.trips?ret:0)+(latched_.trips==2?out+ret:0);
    if(t>=end+latched_.recovery_ms){phase_=Phase::Ready;applyPending();return;}
    if(t>=end){phase_=Phase::Recovery;return;}
    if(inward_){phase_=Phase::Return;return;}
    edge_arrived_=t>=latched_.out_travel_ms;
    if(t<out)phase_=Phase::Outward;
    else if(t<return_start)phase_=Phase::Quiet;
    else if(latched_.trips==2&&t>=return_start+ret&&t<return_start+ret+out)phase_=Phase::Outward;
    else phase_=Phase::Return;
  }
  Rgb historyAt(std::uint32_t now,float age)const {
    const float delay=age+static_cast<float>(kHopMs)-static_cast<float>(elapsed(now,newest_));
    if(delay<0)return {};
    const float back=delay/static_cast<float>(kHopMs);
    const unsigned n=static_cast<unsigned>(back);
    if(n+1>=history_count_)return {};
    // One-hop reconstruction delay makes both interpolation sources causal:
    // neither sample is newer than the queried material timestamp.
    const Rgb recent=history_[(next_+kHistory-1-n)%kHistory];
    const Rgb older=history_[(next_+kHistory-2-n)%kHistory];
    return mix(recent,older,back-static_cast<float>(n));
  }
  Rgb bloomTap(std::uint32_t now,float age,bool returning)const {
    const float travel=static_cast<float>(returning?latched_.return_travel_ms:latched_.out_travel_ms);
    const float width=(returning?latched_.return_width:latched_.out_width)*travel;
    const float feather=(returning?latched_.return_feather:latched_.out_feather)*width*0.5f;
    // Causal three-tap finite material spread, with independently set return span.
    Rgb c{}; float total=0;
    for(unsigned j=0;j<3;++j) {
      const float lag=static_cast<float>(j)*0.5f*(width+static_cast<float>(latched_.trail_ms));
      const float gain=j==0?1.0f:unit(1.0f-lag/(width+static_cast<float>(latched_.trail_ms)+feather));
      c=add(c,scale(historyAt(now,age+lag),gain)); total+=gain;
    }
    return scale(c,latched_.out_strength/total);
  }
  Config config_{}, requested_{}, latched_{};
  PaletteSnapshot palette_{};
  Rgb history_[kHistory]{};
  unsigned next_=0,history_count_=0;
  Phase phase_=Phase::Ready;
  std::uint32_t start_=0,last_event_=0,gesture_=0,newest_=0,last_injection_=0;
  float reference_=0;
  bool quiet_hold_=false,quiet_released_=false;
  bool inward_=false,pending_=false,edge_arrived_=false,bloom_clock_=false,history_alive_=false;
};
} // namespace composed_motion
