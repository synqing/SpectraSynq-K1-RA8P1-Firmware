#include "platform/ra8p1/composed_scene_runtime.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace composed_scene;
static composed_motion::Rgb colours(float u,void*) {return u<.5f?composed_motion::mix({.03f,.4f,1},{.8f,.07f,.3f},u*2):composed_motion::mix({.8f,.07f,.3f},{1,.5f,.02f},(u-.5f)*2);}
static void init(Board& b,unsigned scene){float hz[64];for(unsigned i=0;i<64;++i)hz[i]=30.0f+50.0f*float(i);b.initialise(hz);b.palette=colours;auto c=b.requested;c.scene=scene;assert(b.configure(c));}
static void observe(Board& b,unsigned ms,float level=.8f){K1EmotiscopeAudioPublication p{};p.valid=1;p.sequence=ms/10+1;p.audio_hops=p.sequence;p.now_us=ms*1000;p.vu_level=level;p.vu_level_raw=level;p.vu_floor=.01f;p.novelty_normalised=(ms%1000u)<100u?1.0f:0.0f;for(unsigned i=0;i<64;++i)p.spectrum[i]=p.spectrum_smooth[i]=level;b.observe(p,{}, {0,0},p.now_us,ms);}
static void tick(Pair& pair,Board& a,Board& b,unsigned ms){observe(a,ms);observe(b,ms);pair.render(&a,&b,ms*1000);}

static float energy(const Board& b){float e=0;for(const auto& p:b.pixels)e+=p.r+p.g+p.b;return e;}
static void configurePolicy(Board& b,unsigned initiator,unsigned choreography,unsigned quiet=1){auto c=b.requested;c.initiator=initiator;c.choreography=choreography;c.quiet_scope=quiet;assert(b.configure(c));}
static void positiveChecks(){
  // Reversed initiator and counter-motion use independent board-owned transport.
  for(unsigned initiator=0;initiator<2;++initiator){
    Board a,b;Pair p;init(a,1);init(b,1);configurePolicy(a,initiator,2);configurePolicy(b,initiator,2);
    tick(p,a,b,0);Board& caller=initiator?b:a;Board& answer=initiator?a:b;
    const auto id=caller.motion.gestureId();assert(id);assert(answer.motion.gestureId()==0);
    auto c=answer.requested;c.return_width=.2f;answer.configure(c);
    bool answered=false;
    for(unsigned t=10;t<4200;t+=10){tick(p,a,b,t);if(t<1000)assert(caller.motion.gestureId()==id);if(answer.motion.active()){assert(answer.motion.phase()==composed_motion::Phase::Return || answer.motion.phase()==composed_motion::Phase::Recovery);answered=true;}if(t<1400)assert(answer.active.return_width!=.2f);}
    assert(answered);
  }
  // Pair quiet on the default shared schedule produces a real all-black interval.
  {Board a,b;Pair p;init(a,2);init(b,2);configurePolicy(a,0,1,2);configurePolicy(b,0,1,2);bool quiet=false;
    for(unsigned t=0;t<7500;t+=10){tick(p,a,b,t);if(a.forced_quiet&&b.forced_quiet){quiet=true;assert(energy(a)==0&&energy(b)==0);}}
    assert(quiet);
  }
  // A usable model affects launch width once; an expired result cannot change it.
  {Board a;init(a,1);auto c=a.requested;c.ml_target=1;a.configure(c);
    K1EmotiscopeAudioPublication raw{};raw.valid=1;raw.sequence=1;raw.audio_hops=1;raw.vu_level=raw.vu_level_raw=.8f;
    K1EmotiscopeMlResult ml{};ml.valid=1;ml.sequence=1;ml.input_sequence=1;ml.model_id=EmotiscopeMl::model_id;ml.latent[0]=.9f;ml.latent[1]=.1f;
    a.observe(raw,ml,{1,1},0,0);assert(a.model_delta>.79f);a.render(1,.8f);const float captured=a.motion.config().out_width;assert(captured>c.outgoing_width);
    raw.sequence=21;raw.audio_hops=21;raw.now_us=200000;a.observe(raw,ml,{1,1},200000,200);assert(a.model_delta==0);a.render();assert(a.motion.config().out_width==captured);assert(a.motion.pending());
    raw.sequence=1001;raw.audio_hops=1001;raw.now_us=10000000;a.observe(raw,ml,{1,1},10000000,10000);a.render();assert(a.motion.ready());assert(a.motion.config().out_width==c.outgoing_width);
  }
}
static int failureCase(const char* name){
  if(std::strcmp(name,"phrase_reconfigure")==0){Board a,b;Pair p;init(a,2);init(b,2);tick(p,a,b,0);auto c=b.requested;c.phrase=4;b.configure(c);for(unsigned t=10;t<16000;t+=10)tick(p,a,b,t);std::printf("rest while waiting: a_held=%u b_held=%u caller=%u answer=%u\n",a.pair_held,b.pair_held,a.motion.gestureId(),b.motion.gestureId());std::fflush(stdout);assert(!a.pair_held&&!b.pair_held);return 0;}
  if(std::strcmp(name,"linked_bloom")==0){Board a,b;Pair p;init(a,3);init(b,3);configurePolicy(a,0,1);configurePolicy(b,0,1);for(unsigned t=0;t<=100;t+=10)tick(p,a,b,t);std::printf("linked Bloom at100ms: call energy=%.3f answer energy=%.3f\n",double(energy(a)),double(energy(b)));std::fflush(stdout);assert(energy(a)>0);assert(energy(b)==0);return 0;}
  if(std::strcmp(name,"mismatched_quiet")==0){Board a,b;Pair p;init(a,1);init(b,1);auto c=a.requested;c.quiet_scope=2;c.outgoing_ms=300;c.return_ms=8000;c.quiet_ms=100;c.separation_ms=0;c.trail_ms=0;a.configure(c);c=b.requested;c.quiet_scope=2;c.outgoing_ms=3000;c.return_ms=8000;c.quiet_ms=100;c.separation_ms=0;c.trail_ms=0;b.configure(c);bool plateau=false;for(unsigned t=0;t<7000;t+=10){tick(p,a,b,t);plateau|=a.forced_quiet&&b.forced_quiet;}std::printf("mismatched travel protected shared quiet=%u\n",plateau);std::fflush(stdout);assert(plateau);return 0;}
  return -1;
}
int main(int argc,char** argv){
  if(argc>1){int result=failureCase(argv[1]);if(result>=0)return result;}
  positiveChecks();
  const bool failure=argc>1&&std::strcmp(argv[1],"reconfigure")==0;
  if(failure){
    Board a,b;Pair pair;init(a,2);init(b,2);tick(pair,a,b,0);const auto first=a.motion.gestureId();assert(first);
    auto c=b.requested;c.scene=0;b.configure(c);
    for(unsigned t=10;t<10000;t+=10)tick(pair,a,b,t);
    c=a.requested;c.scene=1;c.choreography=0;a.configure(c);
    for(unsigned t=10000;t<15000;t+=10)tick(pair,a,b,t);
    std::printf("after answer config: caller scene=%u phase=%u gesture=%u first=%u\n",a.active.scene,unsigned(a.motion.phase()),a.motion.gestureId(),first);std::fflush(stdout);
    assert(a.motion.gestureId()!=first); // Independent scene must regain launch ownership.
    return 0;
  }
  Board empty;init(empty,3);observe(empty,0,0);empty.render();assert(empty.motion.historyCount()==0);
  auto c=empty.requested;c.scene=1;assert(empty.configure(c));observe(empty,10,0);assert(empty.active.scene==1);
  Board drain;init(drain,3);for(unsigned t=0;t<500;t+=10){observe(drain,t);drain.render();}assert(drain.motion.historyCount()>0);
  c=drain.requested;c.scene=1;drain.configure(c);for(unsigned t=500;t<10000;t+=10){observe(drain,t);drain.render();}assert(drain.active.scene==1);assert(drain.motion.historyCount()==0);
  Board a,b;Pair pair;init(a,2);init(b,2);FILE* out=argc>1?std::fopen(argv[1],"wb"):nullptr;
  unsigned answer_id=0,call_first=0;bool answer_seen=false,caller_return=false;
  for(unsigned t=0;t<=14000;t+=10){tick(pair,a,b,t);if(t==0)call_first=a.motion.gestureId();if(b.motion.active()){answer_seen=true;answer_id=b.motion.gestureId();}if(a.motion.phase()==composed_motion::Phase::Return)caller_return=true;if(out){std::fwrite(a.pixels,sizeof(composed_motion::Rgb),128,out);std::fwrite(b.pixels,sizeof(composed_motion::Rgb),128,out);}}
  if(out)std::fclose(out);assert(answer_seen);assert(answer_id);assert(call_first);assert(caller_return);
  std::puts("scene switch/drain and call-answer host assertions passed");
}
