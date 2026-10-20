#include "inhouse_audio.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <cstdlib>
#include <new>
static unsigned allocations;
void* operator new(std::size_t n) { ++allocations; if (void* p=std::malloc(n)) return p; throw std::bad_alloc(); }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
using k1::titan::InhouseAudio;
static InhouseAudio audio, legacy;
static k1_audio_hop_t make_hop(uint64_t epoch,uint64_t sequence) {
 k1_audio_hop_t h{};h.contract_version=2;h.samples=180;h.analysis_rate_hz=24000;
 h.stream_epoch=epoch;h.hop_sequence=sequence;h.analysis_begin=(sequence-1)*180;h.analysis_end_exclusive=sequence*180;
 for(unsigned i=0;i<180;++i)h.pcm[i]=int16_t(8000*std::sin(double(i+sequence*180)*.1));
 h.paired_level={1,1,epoch,sequence,.4F};return h;
}
int main() {
 k1::titan::InhouseAudioConfig cfg{};cfg.analysis_rate_hz=24000;
 assert(audio.configure(cfg));assert(legacy.configure(cfg));
 const auto initial_allocations=allocations;
 for(uint64_t seq=1;seq<=300;++seq) {
  auto h=make_hop(7,seq);auto old=h;old.paired_level={};
  audio.setGuardGain(512);legacy.setGuardGain(512);
  assert(audio.consume(h,1000000+seq*7500,true));assert(legacy.consume(old,1000000+seq*7500,true));
  audio.stampPublication(1000100+seq*7500);legacy.stampPublication(1000100+seq*7500);
  const auto& a=audio.latest();const auto& b=legacy.latest();
  assert(a.vu_level==b.vu_level&&a.raw_rms==b.raw_rms&&a.peak_scaled==b.peak_scaled);
  assert(a.spectrum==b.spectrum&&a.pcm_history==b.pcm_history);
  assert(a.guard_headroom==1 && b.guard_headroom==8);
  assert(a.unguarded_vu_valid && a.unguarded_vu==6.4F && !b.unguarded_vu_valid);
 }
 // All malformed optional metadata falls back without rejecting valid PCM.
 unsigned invalid=0;
 for(unsigned fault=0;fault<9;++fault) {
  auto h=make_hop(7,301+fault);
  switch(fault) {
   case 0:h.paired_level.version=0;break;
   case 1:h.paired_level.version=2;break;
   case 2:++h.paired_level.stream_epoch;break;
   case 3:--h.paired_level.hop_sequence;break;
   case 4:h.paired_level.pre_guard_rms=std::numeric_limits<float>::quiet_NaN();break;
   case 5:h.paired_level.pre_guard_rms=std::numeric_limits<float>::infinity();break;
   case 6:h.paired_level.pre_guard_rms=-.1F;break;
   case 7:h.paired_level.guarded_support=2;break;
   case 8:h.paired_level.pre_guard_rms=std::numeric_limits<float>::max();break;
  }
  assert(!InhouseAudio::pairedLevelValid(h));
  audio.setGuardGain(512);assert(audio.consume(h,1000000+h.hop_sequence*7500,true));
  assert(!audio.latest().unguarded_vu_valid && audio.latest().guard_headroom==8);++invalid;
 }
 auto quiet=make_hop(7,310);quiet.paired_level.guarded_support=0;
 assert(audio.consume(quiet,3325000,true));audio.stampPublication(3325100);
 assert(audio.latest().unguarded_vu==audio.latest().vu_level && audio.latest().guard_headroom==1);
 k1::titan::InhouseAudioCursor cursor{};k1::core::visual::InhouseWaveformAudio out{};
 assert(audio.acquire(cursor,3325200,50000,out));assert(out.unguarded_vu_valid);
 assert(!audio.acquire(cursor,3400000,50000,out));assert(!out.unguarded_vu_valid);
 assert(!audio.acquire(cursor,3325200,50000,out,9));assert(!out.unguarded_vu_valid);
 // Duplicate, gap and epoch transitions must not carry the previous raw level.
 assert(!audio.consume(quiet,3325000,true));
 auto gap=make_hop(7,312);assert(!audio.consume(gap,3340000,true));assert(!audio.latest().unguarded_vu_valid);
 auto next=make_hop(8,1);next.paired_level={};assert(audio.consume(next,4000000,true));assert(!audio.latest().unguarded_vu_valid);
 assert(allocations==initial_allocations);
 std::printf("PASS: 300 paired publications; %u malformed/missing fallbacks; quiet arithmetic; stale, board, duplicate, gap, epoch controls; zero allocations. hop=%zu audio=%zu publication=%zu\n",invalid,sizeof(k1_audio_hop_t),sizeof(audio),sizeof(out));
}
