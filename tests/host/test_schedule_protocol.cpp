#include "fixture_app.h"
#include "ws2816_gpio_emit.h"
#ifdef K1_ENABLE_STAGE_PROBE
#include "stage_probe.h"
#endif
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static bool floating_status_format;
extern "C" int snprintf(char* output,std::size_t capacity,const char* format,...) {
  for(const char* cursor=format;*cursor;++cursor) {
    if(*cursor!='%') continue;
    ++cursor;
    if(*cursor=='%') continue;
    while(*cursor && std::strchr("-+ #0.*0123456789hlLjzt",*cursor)) ++cursor;
    if(std::strchr("aAeEfFgG",*cursor)) floating_status_format=true;
    if(!*cursor) break;
  }
  std::va_list arguments;
  va_start(arguments,format);
  const int count=std::vsnprintf(output,capacity,format,arguments);
  va_end(arguments);
  return count;
}
#ifdef K1_NPU_LOAD
#include "npu_load.h"
extern "C" bool k1_npu_ready() { return true; }
extern "C" bool k1_npu_initialise() { return true; }
extern "C" void k1_npu_invoke(std::uint32_t, k1_npu_measurement_t* measurement) {
  measurement->invoke_status=0; measurement->wall_cycles=50;
  measurement->npu_cycles=40; measurement->npu_active=30;
  measurement->mac_active=20; measurement->output_match=true;
}
#endif

static std::uint32_t cycles;
extern "C" std::uint32_t k1_cycle_count() { return cycles+=100; }
void k1_ws2816_set_clock(std::uint32_t) {}
packed_submit_result_t k1_ws2816_submit_packed_lanes(
    const std::uint8_t*, std::size_t a_bytes, const std::uint8_t*,
    std::size_t b_bytes, packed_lane_completion_t* completion) {
  if (k1_ws2816_require_packed_lanes(a_bytes, b_bytes) != kPackedAccepted) {
    return kPackedWrongCount;
  }
  if (completion != nullptr) {
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
static std::vector<std::uint8_t> packet(unsigned op,std::vector<std::uint8_t> payload={}) {
  std::vector<std::uint8_t> p(32+payload.size()); std::memcpy(p.data(),"K1S1",4);
  put(p.data()+4,op); put(p.data()+8,42); put(p.data()+16,payload.size());
  std::memcpy(p.data()+32,payload.data(),payload.size()); put(p.data()+20,crc(p.data()+32,payload.size()));
  put(p.data()+24,1); put(p.data()+28,crc(p.data(),28)); return p;
}
static std::vector<std::uint8_t> send(const std::vector<std::uint8_t>& p) {
  k1_fixture_consume(p.data(),p.size(),100);
  std::size_t size=0; const auto data=k1_fixture_reply(&size); assert(size>=32);
  assert(get(data+28)==crc(data,28)); assert(get(data+24)==crc(data+32,size-32));
  std::vector<std::uint8_t> answer(data,data+size); k1_fixture_sent(); return answer;
}
static std::vector<std::uint8_t> start(unsigned loops,unsigned flags) {
  std::vector<std::uint8_t> payload(8); put(payload.data(),loops); put(payload.data()+4,flags);
  return send(packet(7,payload));
}
int main() {
  std::uint8_t uid[16]{}; k1_fixture_initialise(uid,1000000,0);
  assert(get(start(0,0).data()+4)==3);
  assert(get(start(1,4).data()+4)==3);
#ifndef K1_NPU_LOAD
  assert(get(start(1,0x10).data()+4)==3);
#endif
  auto accepted=start(1,0); assert(get(accepted.data()+4)==0);
  assert(std::string(reinterpret_cast<char*>(accepted.data()+32),accepted.size()-32)=="STARTED");
  assert(get(start(1,0).data()+4)==3);
  for(unsigned i=0;i<1000 && k1_fixture_schedule_active();++i) k1_fixture_schedule_step();
  assert(!k1_fixture_schedule_active());
  auto status=send(packet(8)); assert(get(status.data()+4)==0);
  std::string json(reinterpret_cast<char*>(status.data()+32),status.size()-32);
  assert(json.find("\"finished\":true")!=std::string::npos);
  assert(json.find("\"loops_complete\":1")!=std::string::npos);
  assert(json.find("\"queue_capacity\":1")!=std::string::npos);
  assert(json.find("\"count\":1")!=std::string::npos);
#ifdef K1_ENABLE_STAGE_PROBE
  assert(json.find("\"raw_trace\":{\"version\":1,\"records\":1")!=std::string::npos);
  std::vector<std::uint8_t> raw_request(8);
  put(raw_request.data(),0); put(raw_request.data()+4,1);
  auto raw=send(packet(12,raw_request));
  assert(get(raw.data()+4)==0);
  const auto* body=raw.data()+32;
  assert(std::memcmp(body,"K1T1",4)==0);
  assert(get(body+4)==1 && get(body+8)==0 && get(body+12)==1);
  assert(get(body+16)==(5U+k1_stage_count)*4U);
  assert(get(body+20)==k1_stage_count);
  assert(get(body+24)==0 && get(body+28)>0);
  put(raw_request.data()+4,0);
  assert(get(send(packet(12,raw_request)).data()+4)==3);
#else
  assert(json.find("\"mean_us\":100.000")!=std::string::npos);
#endif
  cycles=0xfffffc00U;
  accepted=start(1,0); assert(get(accepted.data()+4)==0);
  for(unsigned i=0;i<1000 && k1_fixture_schedule_active();++i) k1_fixture_schedule_step();
  assert(!k1_fixture_schedule_active());
#ifdef K1_NPU_LOAD
  accepted=start(1,0x20); assert(get(accepted.data()+4)==0);
  for(unsigned i=0;i<1000 && k1_fixture_schedule_active();++i) k1_fixture_schedule_step();
  assert(!k1_fixture_schedule_active());
  status=send(packet(8)); json.assign(reinterpret_cast<char*>(status.data()+32),status.size()-32);
  assert(json.find("\"mode\":2")!=std::string::npos);
  assert(json.find("\"npu_invocations\":3")!=std::string::npos);
  assert(json.find("\"npu_ready\":true")!=std::string::npos);
#endif
  assert(!floating_status_format);
  std::puts("K1_RESIDENT_SCHEDULE_PROTOCOL=PASS bounds=PASS start_exclusion=PASS completion=PASS cycle_wrap=PASS npu_mode=PASS status=PASS");
}
