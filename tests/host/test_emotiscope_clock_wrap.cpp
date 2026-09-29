#include "../../platform/ra8p1/emotiscope_port.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
int main(){
  assert(k1_emotiscope_init(0xACE1u));
  K1EmotiscopeConfig cfg;k1_emotiscope_default_config(&cfg);
  cfg.screensaver=false;assert(k1_emotiscope_set_config(&cfg));
  const uint64_t wrap=uint64_t(1)<<32;
  // Phase chosen to put a render between a pre-wrap novelty update and its
  // wrapped future deadline. It distinguishes unsigned>= from due-time logic.
  const uint64_t start=wrap-505000;
  uint64_t next_novelty=start+10000;
  uint32_t deadline_mismatches=0,millis_mismatches=0,updates_after_wrap=0;
  float chunk[64];uint8_t rgb[384];double t=0;
  K1EmotiscopeSnapshot before{},after{};
  for(uint32_t hop=1;hop<=180;++hop){
    uint64_t actual=start+uint64_t(hop)*5000;
    for(unsigned i=0;i<64;++i,t+=1.0/12800.0)
      chunk[i]=float(.2*std::sin(t*6.283185307179586*220.0));
    assert(k1_emotiscope_push_pcm64(chunk,uint32_t(actual)));
    if(hop%2)continue;
    k1_emotiscope_snapshot(&before);
    assert(k1_emotiscope_render(uint32_t(actual),rgb));
    k1_emotiscope_snapshot(&after);
    bool expected=actual>=next_novelty;
    if(expected)next_novelty+=20000;
    assert(before.vu_max>1.1e-6f);
    bool observed=after.vu_max<=1.1e-6f;
    if(expected!=observed){
      ++deadline_mismatches;
      std::printf("deadline mismatch real_us=%llu raw_us=%u expected=%d observed=%d\n",
        (unsigned long long)actual,uint32_t(actual),int(expected),int(observed));
    }
    if(after.now_ms!=uint32_t(actual/1000)){
      ++millis_mismatches;
      if(millis_mismatches==1)std::printf("millis mismatch real_us=%llu expected=%u observed=%u\n",
        (unsigned long long)actual,uint32_t(actual/1000),after.now_ms);
    }
    if(actual>=wrap && observed)++updates_after_wrap;
  }
  std::printf("wrap result deadline_mismatches=%u millis_mismatches=%u updates_after_wrap=%u\n",
              deadline_mismatches,millis_mismatches,updates_after_wrap);
  std::fflush(stdout);
  assert(deadline_mismatches==0);
  assert(millis_mismatches==0);
  assert(updates_after_wrap>=19);
  K1EmotiscopeStatus status{};
  assert(!k1_emotiscope_render(uint32_t(start+900000-1),rgb));
  k1_emotiscope_status(&status);assert(status.clock_regressions==1);
  assert(k1_emotiscope_render(uint32_t(start+910000),rgb));
}
