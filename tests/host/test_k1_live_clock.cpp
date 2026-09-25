#include "k1_live_clock.h"
#include "k1/core/audio/k1_audio_hop.h"
#include <cassert>
#include <cstdio>

int main() {
  k1_live_clock_t clock;
  k1_live_clock_sample_t sample;
  k1_live_clock_init(&clock, 1000000000U, 1U);
  assert(k1_live_clock_sample(&clock, 0xffff0000U, &sample) == 0);
  assert(sample.microseconds == 0);
  assert(sample.origin == 1U);
  assert(sample.health == K1_LIVE_CLOCK_OK);
  assert(k1_live_clock_sample(&clock, 0x000e4240U, &sample) == 0);
  assert(sample.microseconds == 1000); /* 1e6 cycles over wrap at 1 GHz */
  assert(k1_live_clock_sample(&clock, 0x59767140U, &sample) == 0);
  assert(sample.microseconds == 1501000);

  k1_live_clock_t fraction;
  k1_live_clock_init(&fraction, 123456789U, 1U);
  k1_live_clock_sample(&fraction, 0, &sample);
  for (unsigned i = 1; i <= 10000; ++i) {
    k1_live_clock_sample(&fraction, i * 1234U, &sample);
  }
  assert(k1_live_clock_sample(&fraction, 12340000U, &sample) == 0);
  assert(sample.microseconds == 99954U);

  k1_live_clock_t changed;
  k1_live_clock_init(&changed, 1000000000U, 2U);
  k1_live_clock_sample(&changed, 0, &sample);
  assert(k1_live_clock_bind_hz(&changed, 800000000U) == -1);
  assert(k1_live_clock_sample(&changed, 1000, &sample) == -1);
  assert(sample.health == K1_LIVE_CLOCK_INVALID);

  k1_audio_hop_t hop1;
  k1_audio_hop_t hop2;
  assert(k1_audio_hop_init_interval(&hop1, 7, 1) == 0);
  assert(hop1.analysis_begin == 0 && hop1.analysis_end_exclusive == 180);
  assert(k1_audio_hop_media_end_48k(&hop1) == 360);
  assert(hop1.physical_timestamp_valid == 0);
  assert(k1_audio_hop_init_interval(&hop2, 7, 2) == 0);
  assert(hop2.analysis_begin == 180 && hop2.analysis_end_exclusive == 360);
  assert(k1_audio_hop_media_end_48k(&hop2) == 720);

  uint64_t epoch = 0;
  uint64_t sequence = 0;
  assert(k1_audio_hop_accept(&epoch, &sequence, &hop1) == 0);
  assert(k1_audio_hop_accept(&epoch, &sequence, &hop1) == -2);
  assert(k1_audio_hop_accept(&epoch, &sequence, &hop2) == 0);
  k1_audio_hop_t stale;
  assert(k1_audio_hop_init_interval(&stale, 7, 2) == 0);
  assert(k1_audio_hop_accept(&epoch, &sequence, &stale) == -2);
  k1_audio_hop_t skip;
  assert(k1_audio_hop_init_interval(&skip, 7, 4) == 0);
  assert(k1_audio_hop_accept(&epoch, &sequence, &skip) == -3);

  std::printf("LIVE_CLOCK_CONTRACT_PASS wrap_us=1000 media_end_hop1=360 media_end_hop2=720 physical_ts=false\n");
  return 0;
}
