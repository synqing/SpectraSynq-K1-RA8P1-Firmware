#include "platform/ra8p1/composed_timing.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
using composed_timing::Clock;
static float frequencies[96];
static K1EmotiscopeAudioPublication signal(unsigned t,int selected,float mass=1.f){
  K1EmotiscopeAudioPublication p{};p.sequence=t/10+1;p.audio_hops=t/5+1;p.valid=1;
  p.tempo_confidence=1.f;p.tempi_power_sum=mass;
  p.tempi_smooth[selected]=mass;
  for(unsigned i=0;i<96;++i){
    // Reference phase from elapsed time, independently of oscillator state.
    double phase=std::fmod(double(t)*.001*double(frequencies[i])*6.283185307179586+1.570796326794897,6.283185307179586);
    if(phase>3.141592653589793)phase-=6.283185307179586;
    p.tempi_phase[i]=float(phase);p.tempi_beat[i]=float(std::sin(phase));
  }
  return p;
}
static void run_signal(Clock& c,unsigned start,unsigned end,int candidate){
  for(unsigned t=start;t<=end;t+=10)c.update(signal(t,candidate),t);
}
int main(){
  // Independent literal source oracle: donor bins are 48..143 BPM.
  for(unsigned i=0;i<96;++i)frequencies[i]=float(48+i)/60.f;
  Clock c;assert(!c.locked()&&c.candidate()==-1&&!c.boundary());
  run_signal(c,0,1000,72);assert(!c.locked()); // unconfigured cannot infer Hz
  assert(c.configure(frequencies));
  float bad[96];for(unsigned i=0;i<96;++i)bad[i]=frequencies[i];bad[5]=std::numeric_limits<float>::quiet_NaN();
  assert(!c.configure(bad)); // failed configure retains last valid table
  c=Clock{};assert(c.configure(frequencies));
  run_signal(c,0,590,72);assert(!c.locked());
  c.update(signal(600,72),600);assert(c.locked()&&c.candidate()==72&&!c.boundary());
  assert(c.confidence()>.99f);
  std::vector<unsigned> ticks;
  for(unsigned t=610;t<=5010;t+=10){c.update(signal(t,72),t);if(c.boundary())ticks.push_back(t);
    c.update(signal(t,72),t);assert(!c.boundary());}
  assert(ticks.size()==9);for(unsigned t:ticks)assert(t%500<=10 || t%500>=490);
  // Tiny changing winner cannot force an octave or adjacent-bin switch.
  for(unsigned t=5020;t<=6200;t+=10){auto p=signal(t,73);p.tempi_smooth[72]=.9f;p.tempo_confidence=.6f;c.update(p,t);}
  assert(c.candidate()==72);
  run_signal(c,6210,7000,24);assert(c.candidate()==72);
  c.update(signal(7010,24),7010);assert(c.candidate()==24);
  unsigned last=0;for(unsigned t=7020;t<=12000;t+=10){c.update(signal(t,24),t);if(c.boundary()){if(last)assert(t-last>=600u);last=t;}}
  assert(last>0); // new72 BPM rate, no phase-reset burst
  auto frozen=signal(12000,24);
  for(unsigned t=12010;t<=13500;t+=10)c.update(frozen,t);
  assert(c.locked()); // bounded continuation from last supported sample
  c.update(frozen,13510);assert(!c.locked()&&c.confidence()==0);
  unsigned free_ticks=0;for(unsigned t=13520;t<15500;t+=10){c.update(frozen,t);free_ticks+=c.boundary();}
  assert(free_ticks>=2&&free_ticks<=3); // honest free-running fallback
  // Invalid/silent/all-equal candidates, NaNs and inconsistent phase never lock.
  for(unsigned variant=0;variant<5;++variant){Clock z;assert(z.configure(frequencies));
    for(unsigned t=0;t<=2000;t+=10){auto p=signal(t,72);
      if(variant==0)p.valid=0;
      if(variant==1){p.tempi_smooth[72]=0;p.tempo_confidence=0;}
      if(variant==2){for(float& x:p.tempi_smooth)x=1;p.tempo_confidence=1.f/96;}
      if(variant==3)p.tempi_phase[72]=std::numeric_limits<float>::quiet_NaN();
      if(variant==4)p.tempi_beat[72]=-p.tempi_beat[72]+3;
      z.update(p,t);assert(!z.locked());}}
  // uint32 millisecond wrap preserves acquisition and approximately500ms ticks.
  Clock wrap;assert(wrap.configure(frequencies));const unsigned origin=0xfffffc00u;unsigned count=0;
  for(unsigned t=0;t<=3000;t+=10){wrap.update(signal(t,72),origin+t);count+=wrap.boundary();}
  assert(wrap.locked()&&count>=4&&count<=5);
  wrap.update(signal(10000,72),origin+10000);assert(!wrap.boundary()&&!wrap.locked()); // no catch-up burst
  wrap.update(signal(10010,72),origin+9990);assert(!wrap.boundary()&&!wrap.locked()); // regressed caller clock
  std::printf("timing checks complete; stable grid boundaries=%zu; wrap boundaries=%u; clock_bytes=%zu\n",ticks.size(),count,sizeof(Clock));
}
