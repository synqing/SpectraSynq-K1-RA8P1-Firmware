#include "bloom_donors.h"
#include <cassert>
#include <cstdio>
int main() {
  k1::reference::SensoryRgbBloom rgb;
  k1::reference::EmotiscopeScalarBloom scalar;
  std::array<k1::reference::Rgb,64> colours{}; colours.fill({1,0,0});
  rgb.step({1,0,0},0); scalar.step(1,0,colours);
  assert(rgb.history[64].red==1 && scalar.history[0]==1);
  // Original source's exact first-step split in the declared Q15.16 / FP32 boundaries.
  rgb.step({},0); scalar.step(0,0,colours);
  assert(std::fabs(rgb.history[65].red-.2475F)<1.0F/65536);
  assert(std::fabs(scalar.history[1]-.12375F)<1e-7F);
  for(unsigned i=0;i<12;++i) {rgb.step({},.5F); scalar.step(0,.5F,colours);}
  colours.fill({0,1,0}); rgb.step({},.5F); scalar.step(0,.5F,colours);
  float rr=0,rg=0,sr=0,sg=0;
  for(unsigned i=0;i<128;++i) {rr+=rgb.frame[i].red;rg+=rgb.frame[i].green;sr+=scalar.frame[i].red;sg+=scalar.frame[i].green;}
#ifdef MUTATE_HISTORY_EXPECTATION
  assert(rg>0 && rr==0); // Must fail: deposited RGB cannot recolour old red.
#else
  assert(rr>0 && rg==0 && sr==0 && sg>0);
#endif
  std::puts("Donor kernel split and palette-history distinction PASS (declared adaptations)");
}
