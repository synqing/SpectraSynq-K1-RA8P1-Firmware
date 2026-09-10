#include "palette_runtime.h"
#include "palette_clock.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
using namespace k1::titan;
using namespace k1::core;
using namespace k1::core::visual;
static bool same(Pixel8 a,Pixel8 b) { return !std::memcmp(&a,&b,sizeof(a)); }
int main() {
  PaletteClock clock;
  assert(clock.sample(0xffff0000U,1000000000U)==0);
  assert(clock.sample(0x000e4240U,1000000000U)==1000); // exactly 1M cycles over wrap
  assert(clock.sample(0x59767140U,1000000000U)==1501000); // another 1.5B cycles
  PaletteClock fraction;
  fraction.sample(0,123456789);
  for(unsigned i=1;i<=10000;++i) fraction.sample(i*1234U,123456789U);
  assert(fraction.sample(12340000U,123456789U)==99954U);
  unsigned peak_out=0,peak_in=0;
  float maximum_out=-1,maximum_in=-1;
  for(unsigned r=0;r<80;++r) {
    auto out=centreLayers(103,centreTravelCoordinate(r,false,1000000,4000));
    auto in=centreLayers(103,centreTravelCoordinate(r,true,1000000,4000));
    if(out.body+out.accent>maximum_out) { maximum_out=out.body+out.accent; peak_out=r; }
    if(in.body+in.accent>maximum_in) { maximum_in=in.body+in.accent; peak_in=r; }
  }
  assert(peak_out==20 && peak_in==59); // quarter of the journey, opposite directions
  unsigned compared=0,visible=0;
  for(unsigned id=0;id<44;++id) for(unsigned mode=100;mode<104;++mode) {
    PaletteRuntime outward,inward;
    PaletteConfig c; c.version=3; c.palette_a=c.palette_b=id;
    c.mode_a=c.mode_b=mode; c.flags=1; c.brightness=255;
    assert(outward.configure(c,0)); c.flags|=8; assert(inward.configure(c,0));
    for(unsigned f=0;f<24;++f) {
      const auto now=std::uint64_t(f)*166660U;
      assert(outward.step(now,nullptr) && inward.step(now,nullptr));
      auto a=outward.channel(0).frame(), b=inward.channel(0).frame();
      bool nonzero=false;
      for(unsigned r=0;r<80;++r) {
        assert(same(a[79-r],a[80+r])); assert(same(b[79-r],b[80+r]));
        assert(same(a[80+r],b[159-r]));
        nonzero |= a[80+r].red || a[80+r].green || a[80+r].blue;
        ++compared;
      }
      visible+=nonzero;
      std::uint8_t wire[384]; assert(outward.packBenchGrb(wire,sizeof(wire))==384);
      for(unsigned i=0;i<64;++i) assert(!std::memcmp(wire+3*i,wire+3*(127-i),3));
    }
  }
  assert(visible>0);
  PaletteRuntime showcase;
  PaletteConfig c; c.version=3; c.flags=17; c.mode_a=100; c.mode_b=101;
  assert(showcase.configure(c,0));
  for(unsigned i=0;i<=4;++i) {
    assert(showcase.step(std::uint64_t(i)*12000000U,nullptr));
    assert(showcase.channel(0).controls().mode_id==100+i%4);
    assert(showcase.channel(1).controls().mode_id==100+(i+1)%4);
  }
  auto bad=c; bad.travel_ms=0; assert(!showcase.configure(bad,0));
  bad=c; bad.travel_ms=30001; assert(!showcase.configure(bad,0));
  bad=c; bad.version=2; assert(!showcase.configure(bad,0));
  bad=c; bad.mode_a=14; bad.flags=9; assert(!showcase.configure(bad,0));
  std::printf("CENTRE_EFFECTS_PASS combinations=352 mirror_and_direction_samples=%u visible_frames=%u physical_128_mirror=true peak_out=%u peak_in=%u clock_wrap_and_fraction=true\n",compared,visible,peak_out,peak_in);
}
