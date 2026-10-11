#include "platform/ra8p1/composed_motion.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace composed_motion;
static Rgb palette(float u,void*) { return u<0.5f?mix({0.03f,0.45f,1.0f},{0.8f,0.08f,0.34f},u*2):mix({0.8f,0.08f,0.34f},{1.0f,0.5f,0.03f},(u-0.5f)*2); }
static Rgb green(float,void*) { return {0,1,0}; }
static float energy(const Rgb* a) { float x=0;for(unsigned i=0;i<128;++i)x+=a[i].r+a[i].g+a[i].b;return x; }
static void same(const Rgb* a,const Rgb* b) { for(unsigned i=0;i<128;++i){assert(std::fabs(a[i].r-b[i].r)<1e-5f);assert(std::fabs(a[i].g-b[i].g)<1e-5f);assert(std::fabs(a[i].b-b[i].b)<1e-5f);} }
static Config cfg(){Config c;c.out_travel_ms=1000;c.return_travel_ms=1200;c.out_width=.2f;c.return_width=.3f;c.separation_ms=300;c.trail_ms=200;c.quiet_ms=400;c.recovery_ms=200;return c;}
int main(int argc,char** argv) {
  Rgb a[128],b[128];Config c=cfg();Renderer r,s;
  assert(r.configure(c));assert(s.configure(c));
  // Timestamp result independent of render cadence; ignored loud/palette edits.
  r.renderRibbon(0,1,.7f,palette,nullptr,a);s.renderRibbon(0,1,.7f,palette,nullptr,b);
  for(unsigned t=10;t<=2400;t+=10)r.renderRibbon(t,2,1,green,nullptr,a);
  s.renderRibbon(2400,1,.01f,palette,nullptr,b);same(a,b);assert(r.gestureId()==1);assert(r.phase()==Phase::Return);
  for(unsigned i=0;i<64;++i){assert(a[i].r==a[127-i].r);assert(a[i].g==a[127-i].g);}
  // Tail ends at second launch 300 + travel 1000 + half width 100 + trail 200.
  Renderer tail;tail.configure(c);tail.renderRibbon(0,1,1,palette,nullptr,a);
  tail.renderRibbon(1599,1,1,palette,nullptr,a);assert(!tail.quiet());assert(energy(a)>0);
  tail.renderRibbon(1600,1,1,palette,nullptr,a);assert(tail.quiet());assert(energy(a)==0);
  tail.renderRibbon(1999,1,1,palette,nullptr,a);assert(tail.quiet());
  tail.renderRibbon(2000,1,1,palette,nullptr,a);assert(tail.phase()==Phase::Return);assert(energy(a)>0);
  // Events during active interval are consumed, not queued at Ready.
  tail.renderRibbon(9000,1,1,palette,nullptr,a);assert(tail.ready());
  r.renderRibbon(9000,2,1,palette,nullptr,a);assert(r.ready());
  r.renderRibbon(9001,3,1,palette,nullptr,a);assert(r.active());assert(r.gestureId()==3);
  // Pending configuration latches after old gesture, and zero quiet is valid.
  Config d=c;d.out_travel_ms=500;d.quiet_ms=0;assert(r.configure(d));assert(r.pending());assert(r.config().out_travel_ms==1000);
  r.renderRibbon(19000,3,1,palette,nullptr,a);assert(!r.pending());assert(r.config().out_travel_ms==500);
  r.renderRibbon(19001,4,1,palette,nullptr,a);r.renderRibbon(20051,4,1,palette,nullptr,a);assert(!r.quiet());
  // Clock wrap preserves motion and palette snapshot transfer answers.
  Renderer w,n;w.configure(c);n.configure(c);const unsigned start=0xffffff00u;
  w.renderRibbon(start,1,.8f,palette,nullptr,a);n.renderRibbon(0,1,.8f,palette,nullptr,b);
  w.renderRibbon(start+700u,1,.8f,palette,nullptr,a);n.renderRibbon(700,1,.8f,palette,nullptr,b);same(a,b);
  Renderer answer;answer.configure(c);answer.renderRibbon(0,8,.8f,green,nullptr,a,true,&n.paletteSnapshot());assert(answer.phase()==Phase::Return);assert(a[0].b>0);assert(energy(a)>0);
  // NaN/Inf settings rejected, input strength invalid becomes silence.
  d=c;d.out_width=std::numeric_limits<float>::quiet_NaN();assert(!r.configure(d));
  d=c;d.return_strength=1;assert(!r.configure(d));d=c;d.out_feather=0;assert(r.configure(d));
  Renderer invalid;invalid.renderRibbon(0,1,std::numeric_limits<float>::infinity(),palette,nullptr,a);assert(invalid.ready());
  // Each return count completes exactly its own finite timeline.
  for(unsigned trips=0;trips<3;++trips){Renderer q;d=c;d.trips=static_cast<unsigned char>(trips);q.configure(d);q.renderRibbon(0,1,1,palette,nullptr,a);const unsigned end=2000+(trips?1880:0)+(trips==2?3480:0)+200;q.renderRibbon(end-1,1,1,palette,nullptr,a);assert(q.active());q.renderRibbon(end,1,1,palette,nullptr,a);assert(q.ready());}
  // Bloom cadence, retained deposited RGB, bounded config drain and board isolation.
  Renderer bloom,slow; bloom.configure(c);slow.configure(c);
  bloom.renderBloom(0,.7f,palette,nullptr,a);slow.renderBloom(0,.7f,palette,nullptr,b);
  for(unsigned t=10;t<=1500;t+=10)bloom.renderBloom(t,.7f,palette,nullptr,a);
  for(unsigned t=25;t<=1500;t+=25)slow.renderBloom(t,.7f,palette,nullptr,b);
  same(a,b);assert(energy(a)>0);
  // No observation for 1.5 s cannot imply 60 copies of the latest input.
  Renderer missed;missed.configure(c);missed.renderBloom(0,0,palette,nullptr,b);
  missed.renderBloom(1500,.7f,palette,nullptr,b);assert(energy(b)==0);
  missed.renderBloom(1525,0,palette,nullptr,b);assert(energy(b)>0);
  bloom.renderBloom(1510,0,green,nullptr,a);assert(a[0].b>0);
  d=c;d.out_travel_ms=600;assert(bloom.configure(d));assert(bloom.pending());
  for(unsigned t=1520;t<=6000;t+=10)bloom.renderBloom(t,1,green,nullptr,a);
  assert(!bloom.pending());assert(bloom.config().out_travel_ms==600);assert(slow.config().out_travel_ms==1000);
  // A pulse reaches the edge before it returns to centre. No fresh level powers it.
  Renderer pulse; pulse.configure(c);pulse.renderBloom(0,0,palette,nullptr,a);pulse.renderBloom(25,1,palette,nullptr,a);
  pulse.renderBloom(1050,0,green,nullptr,a);assert(a[0].r+a[0].b>0);
  pulse.renderBloom(2250,0,green,nullptr,a);assert(a[63].r+a[63].b>0);
  // Interpolation must not blank the centre between history tick boundaries.
  Renderer continuous;continuous.configure(c);
  for(unsigned t=0;t<=500;t+=10){continuous.renderBloom(t,1,palette,nullptr,a);if(t>=100)assert(a[63].r+a[63].g+a[63].b>0.1f);}
  // Pair barrier holds only after the tail clears, including zero authored quiet.
  {Renderer held,free;d=c;d.quiet_ms=0;held.configure(d);free.configure(d);
    held.renderRibbon(0,1,.8f,palette,nullptr,a);held.holdQuiet(0);
    held.renderRibbon(1599,0,1,green,nullptr,a);assert(!held.quietHeld());assert(energy(a)>0);
    held.renderRibbon(1600,0,1,green,nullptr,a);assert(held.quietHeld());assert(energy(a)==0);
    held.renderRibbon(9000,0,1,green,nullptr,a);assert(held.quietHeld());assert(energy(a)==0);
    held.releaseQuiet(9000);held.renderRibbon(9000,0,1,green,nullptr,a);
    free.renderRibbon(0,1,.8f,palette,nullptr,b);free.renderRibbon(1600,0,1,green,nullptr,b);same(a,b);
    assert(held.phase()==Phase::Return);held.holdQuiet(9000);assert(!held.quietHeld());
    held.renderRibbon(12000,0,1,green,nullptr,a);assert(held.ready());
  }
  // Empty Bloom owns no live history and cannot block a scene/config change.
  Renderer empty;empty.renderBloom(0,0,palette,nullptr,a);assert(empty.historyCount()==0);assert(empty.ready());
  for(unsigned t=10;t<1000;t+=10)empty.renderBloom(t,0,palette,nullptr,a);
  assert(empty.historyCount()==0);d=c;d.out_travel_ms=200;assert(empty.configure(d));assert(!empty.pending());
  empty.renderRibbon(1000,4,1,palette,nullptr,a);assert(empty.active());
  empty.renderRibbon(9000,0,1,palette,nullptr,a);assert(empty.ready());
  empty.renderRibbon(9001,4,1,palette,nullptr,a);assert(empty.ready()); // no-intent does not erase consumed event
  // Outgoing strength scales retained return material as well, exactly once.
  Renderer bright,dim;d=c;d.out_strength=.1f;bright.configure(c);dim.configure(d);
  bright.renderRibbon(0,1,.8f,palette,nullptr,a);dim.renderRibbon(0,1,.8f,palette,nullptr,b);
  bright.renderRibbon(2600,1,1,palette,nullptr,a);dim.renderRibbon(2600,1,1,palette,nullptr,b);
  assert(std::fabs(energy(b)/energy(a)-.1f)<1e-5f);
  Renderer bright_flow,dim_flow;bright_flow.configure(c);dim_flow.configure(d);
  for(unsigned t=0;t<=3000;t+=25){bright_flow.renderBloom(t,.8f,palette,nullptr,a);dim_flow.renderBloom(t,.8f,palette,nullptr,b);}
  assert(std::fabs(energy(b)/energy(a)-.1f)<1e-5f);
  // Every public limit is admitted; just-outside values are rejected.
  d=c;d.out_travel_ms=8000;d.return_travel_ms=8000;d.separation_ms=5000;d.quiet_ms=8000;d.recovery_ms=8000;d.trail_ms=2000;d.out_width=.8f;d.return_width=.8f;d.out_feather=1;d.return_feather=1;d.return_strength=.95f;d.trips=2;assert(empty.configure(d));
  d.out_travel_ms=8001;assert(!empty.configure(d));d.out_travel_ms=8000;d.trail_ms=2001;assert(!empty.configure(d));d.trail_ms=2000;d.trips=3;assert(!empty.configure(d));
  // Maximum parameter extent retains the second return rather than overwriting it.
  Renderer maximum;d=c;d.out_travel_ms=8000;d.return_travel_ms=8000;d.out_width=.8f;d.return_width=.8f;d.trail_ms=2000;d.trips=2;assert(maximum.configure(d));
  maximum.renderBloom(0,0,palette,nullptr,a);maximum.renderBloom(25,1,palette,nullptr,a);
  for(unsigned t=50;t<=32050;t+=25)maximum.renderBloom(t,0,palette,nullptr,a);
  assert(a[63].r+a[63].b>0);assert(sizeof(Renderer)<=24576);
  if(argc>1){ FILE* f=std::fopen(argv[1],"wb");assert(f);Renderer ribbon,flow;ribbon.configure(c);flow.configure(c);
    for(unsigned t=0;t<=8500;t+=10){ribbon.renderRibbon(t,1,.9f,palette,nullptr,a);float level=t<3000?(.25f+.65f*std::pow(std::sin(static_cast<float>(t)*.006f),2.0f)):0;flow.renderBloom(t,level,palette,nullptr,b);std::fwrite(a,sizeof(Rgb),128,f);std::fwrite(b,sizeof(Rgb),128,f);} std::fclose(f);}
  std::printf("composed motion assertions passed; Renderer bytes=%zu; host only\n",sizeof(Renderer));
}
