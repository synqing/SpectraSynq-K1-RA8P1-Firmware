#pragma once
#include "core/audio/media_time.h"
#include "core/audio/musical_time.h"
#include "core/audio/clock_affine.h"
#include <cmath>
// Deterministic target assertions, supplementing the pinned independent HOST tests.
inline unsigned k1_time_probe() {
  using namespace k1::core::audio;
  if(analysisToMedia(180,24000).frame!=360 || analysisToMedia(96,12800).frame!=360) return 1;
  if(analysisToMedia(1,12800).frame!=3 || analysisToMedia(1,12800).quarter!=3) return 2;
  if(mediaToAnalysis(48000,24000)!=24000 || mediaToAnalysis(48000,12800)!=12800) return 3;
  const std::uint64_t large=1ULL<<40;
  if(analysisToMedia(large,24000).frame!=large*2 || mediaFramesToMicros(48000)!=1000000) return 4;
  MusicalTime mt; mt.anchor={9,large}; mt.beat_period_q32=beatPeriodFromBpm(120); mt.locked=true;
  if(mt.beat_period_q32!=(24000ULL<<32)) return 5;
  FramePositionQ32 position;
  for(std::uint64_t i=1;i<=1000000;++i)
    if(!predictBeatFrame(mt,i,position) || position.frame!=large+i*24000 || position.frac_q32) return 6;
  invalidateForEpochChange(mt,10);
  if(mt.valid() || predictBeatFrame(mt,1000001,position) || mt.anchor.epoch_id!=10) return 7;
  AffineClockEstimator clocks;
  for(unsigned i=0;i<8;++i) {
    const std::uint64_t local=1000000ULL*i;
    if(!clocks.addObservation({1,2,local,local+local/10000+1234,0})) return 8;
  }
  double peer=0;
  if(!clocks.localToPeer(1,8000000,2,peer) || std::fabs(peer-8002034)>0.01) return 9;
  if(clocks.localToPeer(2,8000000,2,peer) || clocks.localToPeer(1,8000000,3,peer)) return 10;
  if(!clocks.stale(9000000,1000000)) return 11;
  return 0;
}
