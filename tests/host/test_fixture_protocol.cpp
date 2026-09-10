#include "fixture_app.h"
#include "trajectory.h"
#include "core/visual/ws2816_pack.h"
#include "ws2816_gpio_emit.h"
#include "ws281x_diag.h"
#include <limits>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
extern "C" std::uint32_t k1_cycle_count() { static std::uint32_t counter=0; return counter+=100; }
static unsigned diagnostic_emits;
int k1_ws281x_diag_emit(const uint8_t* bytes,size_t size,uint32_t profile,
                       uint32_t pin,uint32_t,k1_ws281x_diag_result_t* result) {
  assert(bytes && size==240 && profile==1 && pin==0);
  assert(bytes[0]==0x34 && bytes[1]==0x12 && bytes[2]==0x56);
  ++diagnostic_emits;
  *result={0,4,2400000,300000,1250,1260,0};
  return 0;
}
void k1_ws2816_set_clock(std::uint32_t) {}
packed_submit_result_t k1_ws2816_submit_packed_lanes(
    const std::uint8_t* lane_a, std::size_t a_bytes, const std::uint8_t* lane_b,
    std::size_t b_bytes, packed_lane_completion_t* completion) {
  if (k1_ws2816_require_packed_lanes(a_bytes, b_bytes) != kPackedAccepted) {
    return kPackedWrongCount;
  }
  (void)lane_a;
  (void)lane_b;
  if (completion) {
    completion->submit_cycles = 10;
    completion->transfer_done_cycles = 20;
    completion->latch_ready_cycles = 30;
    completion->emit_cycles = 10;
    completion->latch_cycles = 10;
    completion->bit_period_min_cycles = 1200;
    completion->bit_period_max_cycles = 1300;
  }
  return kPackedAccepted;
}
extern "C" std::size_t k1_platform_metrics(char* output,std::size_t capacity) {
  const char* text="{\"label\":\"HOST_STUB\"}";
  assert(capacity>std::strlen(text)); std::strcpy(output,text); return std::strlen(text);
}
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
  assert(get(send(packet(6,0)).data()+4)==0);
  {
    std::vector<std::uint8_t> request(32);
    const unsigned words[]{1,1,0,80,8,0x12,0x34,0x56};
    for(unsigned i=0;i<8;++i) put(request.data()+4*i,words[i]);
    const auto reply=send(packet(K1_WS281X_DIAG_OPCODE,0,request));
    assert(get(reply.data()+4)==0 && diagnostic_emits==1);
    const std::string body(reinterpret_cast<const char*>(reply.data()+32),reply.size()-32);
    assert(body.find("\"op\":13")!=std::string::npos);
    assert(body.find("\"bytes\":240")!=std::string::npos);
    assert(body.find("\"pfs_after\":4")!=std::string::npos);
    assert(body.find("\"photons\":\"NOT_CLAIMED\"")!=std::string::npos);
    put(request.data()+12,81);
    assert(get(send(packet(K1_WS281X_DIAG_OPCODE,0,request)).data()+4)==3);
    assert(get(send(packet(K1_WS281X_DIAG_OPCODE,0)).data()+4)==3);
    assert(diagnostic_emits==1); // Rejected requests must not touch GPIO.
  }
  {
    k1::core::visual::Pixel16 pixels[k1::core::visual::kPixelsPerChannel]{};
    pixels[0] = {0x12AB, 0, 0};
    pixels[k1::core::visual::kPixelsPerHalf] = {0, 0, 0x12AB};
    for (std::size_t i = 1; i < 8; ++i) {
      pixels[i] = {0x7A3C, 0, 0};
      pixels[k1::core::visual::kPixelsPerHalf + i] = {0, 0, 0x7A3C};
    }
    std::uint8_t lane_a[k1::core::visual::kPackedBytesPerLane]{};
    std::uint8_t lane_b[k1::core::visual::kPackedBytesPerLane]{};
    assert(k1::core::visual::splitChannel160(
        pixels, k1::core::visual::kPixelsPerChannel, lane_a, lane_b));
    const unsigned long expect_a = crc(lane_a, sizeof(lane_a));
    const unsigned long expect_b = crc(lane_b, sizeof(lane_b));
    auto led = send(packet(11, 0));
    assert(get(led.data() + 4) == 0);
    const std::string body(reinterpret_cast<char*>(led.data() + 32),
                           led.size() - 32);
    assert(body.find("\"din_a\":\"P601\"") != std::string::npos);
    assert(body.find("\"din_b\":\"P004\"") != std::string::npos);
    assert(body.find("\"pixels_a\":80") != std::string::npos);
    assert(body.find("\"true16\":\"0x12AB\"") != std::string::npos);
    assert(body.find("\"visible_u16\":\"0x7A3C\"") != std::string::npos);
    assert(body.find("\"level_shifter\":\"74HCT2G34GW\"") != std::string::npos);
    assert(body.find("\"crc_proves\":\"emission_not_reception\"") !=
           std::string::npos);
    assert(body.find("\"bit_period_min_cycles\":1200") != std::string::npos);
    char crc_a[64], crc_b[64];
    std::snprintf(crc_a, sizeof(crc_a), "\"crc_a\":%lu", expect_a);
    std::snprintf(crc_b, sizeof(crc_b), "\"crc_b\":%lu", expect_b);
    assert(body.find(crc_a) != std::string::npos);
    assert(body.find(crc_b) != std::string::npos);
  }
  assert(get(send(packet(3,0,reset)).data()+4)==0);
  auto compact=send(packet(5,1,silence)); assert(get(compact.data()+4)==0);
  static fixture::Trajectory separate; static fixture::Trace encoded;
  std::int16_t zero[180]{}; separate.process(zero); encoded.format=fixture::Trace::Format::binary;
  separate.trace(encoded);
  assert(encoded.valid && compact.size()==32+encoded.size);
  assert(!std::memcmp(compact.data()+32,encoded.data,encoded.size));
  std::puts("K1_FIXTURE_PROTOCOL=PASS chunks=7 reset=PASS crc=PASS sequence=PASS overflow=PASS timeout_wrap=PASS reconnect=PASS");
}
