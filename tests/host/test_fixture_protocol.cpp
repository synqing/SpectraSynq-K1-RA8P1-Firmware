#include "fixture_app.h"
#include "trajectory.h"
#include <limits>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
extern "C" std::uint32_t k1_cycle_count() { static std::uint32_t counter=0; return counter+=100; }
static std::uint32_t crc(const std::uint8_t* p,std::size_t n) {
  std::uint32_t c=~0U; while(n--) { c^=*p++; for(unsigned i=0;i<8;++i)c=(c>>1)^((c&1)?0xedb88320U:0); } return ~c;
}
static void put(std::uint8_t* p,std::uint32_t n) { for(unsigned i=0;i<4;++i)p[i]=n>>(i*8); }
static std::uint32_t get(const std::uint8_t* p) { return p[0]|(p[1]<<8)|(p[2]<<16)|(std::uint32_t(p[3])<<24); }
static std::vector<std::uint8_t> packet(unsigned op,unsigned seq,std::vector<std::uint8_t> payload={}) {
  std::vector<std::uint8_t> p(32+payload.size()); std::memcpy(p.data(),"K1S1",4);
  put(p.data()+4,op); put(p.data()+8,42); put(p.data()+12,seq); put(p.data()+16,payload.size());
  std::memcpy(p.data()+32,payload.data(),payload.size()); put(p.data()+20,crc(p.data()+32,payload.size()));
  put(p.data()+24,1); put(p.data()+28,crc(p.data(),28)); return p;
}
static std::vector<std::uint8_t> send(const std::vector<std::uint8_t>& p,unsigned chunk=64) {
  for(std::size_t offset=0;offset<p.size();offset+=chunk) k1_fixture_consume(p.data()+offset,std::min<std::size_t>(chunk,p.size()-offset),100);
  std::size_t size; const auto data=k1_fixture_reply(&size); assert(size>=32 && size<=20000);
  assert(get(data+28)==crc(data,28)); assert(get(data+24)==crc(data+32,size-32));
  std::vector<std::uint8_t> answer(data,data+size); k1_fixture_sent(); return answer;
}
int main() {
  // Null input is rejected without advancing AP state. Non-finite envelope
  // inputs follow the pinned AP's explicit sanitising policy, not a new DSP.
  static fixture::AudioPipeline ap_a, ap_b;
  static fixture::GdftSampleWindow window;
  fixture::AudioPipelineInput invalid; assert(!ap_a.process(invalid).valid);
  fixture::AudioPipelineInput input; input.samples=&window;
  input.peak_scaled=std::numeric_limits<float>::quiet_NaN(); input.vu_level=std::numeric_limits<float>::infinity();
  auto oa=ap_a.process(input), ob=ap_b.process(input);
  assert(oa.valid && oa.features.peak_scaled==0 && oa.features.vu_level==0);
  static fixture::Trace ta,tb; fixture::serialise(ta,oa); fixture::serialise(tb,ob);
  assert(ta.valid && tb.valid && ta.size==tb.size && !std::memcmp(ta.data,tb.data,ta.size));
  assert(input.gdft.frame_dt_seconds==0.010F); // K1-DM-112 frozen AGC clock, deliberately distinct from AP release period.
  std::uint8_t uid[16]{}; k1_fixture_initialise(uid,0,0);
  auto info=send(packet(1,0)); assert(get(info.data()+4)==0);
  auto time=send(packet(4,0)); assert(get(time.data()+4)==0);
  const std::vector<std::uint8_t> reset{1,0,0,0,0,0,0,0}, silence(360);
  std::vector<std::uint8_t> baseline;
  for(unsigned chunk: {1,7,32,63,64,127,392}) {
    assert(get(send(packet(3,0,reset)).data()+4)==0);
    auto answer=send(packet(2,1,silence),chunk); assert(get(answer.data()+4)==0);
    if(baseline.empty()) baseline=answer; else assert(answer==baseline);
    auto duplicate=send(packet(2,1,silence)); assert(get(duplicate.data()+4)==6 && get(duplicate.data()+12)==1);
    assert(get(send(packet(2,2,silence)).data()+4)==0);
  }
  auto p=packet(2,3,silence); p[28]^=1; assert(get(send(p).data()+4)==1);
  p=packet(2,3,silence); p.back()^=1; assert(get(send(p).data()+4)==4);
  p=packet(2,3,silence); put(p.data()+16,361); put(p.data()+28,crc(p.data(),28)); assert(get(send(p).data()+4)==2);
  assert(get(send(packet(99,3)).data()+4)==3);
  p=packet(2,3,silence); k1_fixture_consume(p.data(),35,0xfffffff0U); k1_fixture_poll(0x800U);
  std::size_t size; const auto timeout=k1_fixture_reply(&size); assert(size==32 && get(timeout+4)==9); k1_fixture_sent();
  assert(get(send(packet(2,3,silence)).data()+4)==0);
  k1_fixture_consume(p.data(),20,100); k1_fixture_disconnect();
  assert(get(send(packet(3,0,reset)).data()+4)==0);
  assert(send(packet(2,1,silence))==baseline);
  std::puts("K1_FIXTURE_PROTOCOL=PASS chunks=7 reset=PASS crc=PASS sequence=PASS overflow=PASS timeout_wrap=PASS reconnect=PASS");
}
