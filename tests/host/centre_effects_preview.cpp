#include "palette_runtime.h"
#include <cassert>
#include <cstdio>
using namespace k1::titan;
int main(int argc,char** argv) {
  if(argc!=2) return 2;
  auto* file=std::fopen(argv[1],"wb"); if(!file) return 3;
  PaletteRuntime runtime[4];
  PaletteConfig config[4];
  for(unsigned mode=0;mode<4;++mode) {
    auto& c=config[mode]; c.version=3; c.flags=1; constexpr unsigned start[]{33,2,23,37}; c.palette_a=start[mode]; c.palette_b=43;
    c.mode_a=c.mode_b=100+mode; c.transition_ms=1500; c.travel_ms=3000;
    assert(runtime[mode].configure(c,0));
  }
  for(unsigned frame=0;frame<960;++frame) {
    const auto now=std::uint64_t(frame)*kPalettePeriodUs;
    for(unsigned mode=0;mode<4;++mode) {
      if(frame==180 || frame==600) {
        constexpr unsigned start[]{33,2,23,37}, target[]{34,22,38,32};
        config[mode].palette_a=frame==180?target[mode]:start[mode];
        assert(runtime[mode].configure(config[mode],now));
      }
      assert(runtime[mode].step(now,nullptr));
    }
    if(frame%5==0) for(unsigned mode=0;mode<4;++mode) {
      const auto pixels=runtime[mode].channel(0).frame();
      assert(std::fwrite(pixels.data(),3,pixels.size(),file)==pixels.size());
    }
  }
  return std::fclose(file);
}
