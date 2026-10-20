#include "k1_live_runtime.h"
#include "emotiscope_port.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
using namespace k1::titan;
static k1_audio_hop_t make_hop(uint64_t e,uint64_t s) {
  k1_audio_hop_t h{};
  h.contract_version=2; h.samples=64; h.analysis_rate_hz=12800;
  h.stream_epoch=e; h.hop_sequence=s;
  h.analysis_begin=(s-1)*64; h.analysis_end_exclusive=s*64;
  h.supporting_dma_receipt_us=s*5000-20;
  h.newest_sample_capture_estimate_us=s*5000-100;
  h.capture_estimate_valid=1;
  for(unsigned i=0;i<64;++i) h.pcm[i]=int16_t(8000*std::sin((h.analysis_begin+i)*6.283185307179586*440/12800));
  return h;
}
int main(){
  assert(k1_emotiscope_init(0x4b31454dU));
  LiveAudioRuntime r; r.initialise(1000000);
  K1EmotiscopeStatus status{};
  k1_emotiscope_status(&status); assert(status.audio_gaps==0);
  assert(r.latest()==nullptr);
  for(unsigned seq=1;seq<=64;++seq){
    r.advanceHostCycles(5000);
    assert(r.consume(make_hop(7,seq))==LiveHopStatus::Ok);
    assert(r.warmupHops()==seq);
    assert(r.warming()==(seq<64));
    assert((r.latest()!=nullptr)==(seq==64));
  }
  assert(r.latest()->features.validity_flags==0);
  assert(!r.latest()->tempo_field.enabled);
  assert(!r.latest()->musical_time_valid&&!r.latest()->predicted_next_beat_valid);
  for(float x:r.latest()->features.spectrum) assert(x==0);
  for(float x:r.latest()->features.chroma_a_origin) assert(x==0);
  k1_emotiscope_status(&status);
  LiveSnapshotFields f{};r.fillSnapshotFields(&f,r.observeNowUs());
  assert(f.deadline_us==5000 && f.media_time_48k==64*240);
  assert(f.analysis_end_exclusive==64*64);
  assert(f.capture_time_us==319900&&f.publish_time_us==320000);
  for(unsigned i=0;i<64;++i) assert(f.spectrum[i]==status.spectrum[i]);
  for(unsigned i=64;i<80;++i) assert(f.spectrum[i]==0);
  for(unsigned i=0;i<12;++i) assert(f.chroma[i]==status.chromagram[i]);
  assert(f.tempo_bpm==0&&f.beat_phase==0&&f.beat_confidence==0);
  assert(f.rms>0 && f.rms<1);
  char json[1536]; assert(r.mirJson(json,sizeof(json))>0);
  assert(std::strstr(json,"\"chroma_origin\":\"D-sharp\""));
  assert(std::strstr(json,"\"deadline_us\":5000"));
  assert(std::strstr(json,"\"beat_locked\":false"));
  assert(r.consume(make_hop(7,64))==LiveHopStatus::Rejected);
  k1_emotiscope_status(&status);assert(status.audio_hops==64&&status.audio_gaps==0);
  assert(r.consume(make_hop(7,66))==LiveHopStatus::Discontinuity);
  k1_emotiscope_status(&status);
  assert(status.audio_hops==64&&status.audio_gaps==1&&status.fresh_hops==0);
  assert(r.latest()==nullptr&&r.warmupHops()==0&&r.warming());
  uint8_t rgb[384]; assert(k1_emotiscope_render(static_cast<uint32_t>(r.observeNowUs()),rgb));
  for(unsigned seq=67;seq<=130;++seq){
    r.advanceHostCycles(5000);
    assert(r.consume(make_hop(7,seq))==LiveHopStatus::Ok);
  }
  assert(r.latest()!=nullptr && !r.warming());
  k1_emotiscope_status(&status);assert(status.audio_hops==128&&status.audio_gaps==1);
  auto malformed=make_hop(7,131);malformed.samples=180;
  assert(r.consume(malformed)==LiveHopStatus::Fault);
  k1_emotiscope_status(&status);assert(status.audio_hops==128&&status.audio_gaps==2);
  assert(r.latest()==nullptr);
  assert(r.consume(make_hop(8,3))==LiveHopStatus::Rejected);
  r.advanceHostCycles(5000);assert(r.consume(make_hop(8,1))==LiveHopStatus::Ok);
  assert(r.warmupHops()==1);
  assert(!r.publicationFresh(r.observeNowUs()));
  std::printf("EMOTISCOPE_LIVE_BRIDGE_PASS warmup64 media240 deadline5000 chroma_Dsharp no_legacy_features duplicate_drop gap_recovery bad_contract_rejected\n");
}
