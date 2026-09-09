// K1 scalar fixture transport. This is neither a capture driver nor a realtime campaign.
#include "fixture_app.h"
#include "trajectory.h"
#include "time_probe.h"
#include "build_identity.h"
#include <new>
#include <cstdio>
#include <cstring>
namespace {
fixture::Trajectory trajectory;
fixture::Trace trace;
std::uint8_t rx[392], tx[20000], board_uid[16];
std::size_t fill = 0, wanted = 32, tx_size = 0;
std::uint32_t started = 0, clock_hz = 0, cpu_wait = 0, rejected = 0;
bool initialised = false;
std::uint32_t get32(const std::uint8_t* p) { return p[0] | (p[1]<<8) | (p[2]<<16) | (std::uint32_t(p[3])<<24); }
void put32(std::uint8_t* p, std::uint32_t x) { for (unsigned i=0;i<4;++i) p[i]=std::uint8_t(x>>(8*i)); }
std::uint32_t crc(const std::uint8_t* p, std::size_t n) {
  std::uint32_t c = 0xffffffffU;
  while (n--) { c ^= *p++; for (unsigned i=0;i<8;++i) c=(c>>1)^((c&1)?0xedb88320U:0); }
  return ~c;
}
void respond(std::uint32_t status, std::uint32_t cycles, const char* payload, std::size_t size) {
  if (size > sizeof(tx)-32 || tx_size) return;
  std::memset(tx, 0, 32); std::memcpy(tx,"K1R1",4);
  put32(tx+4,status); put32(tx+8,get32(rx+8)); put32(tx+12,trajectory.sequence);
  put32(tx+16,static_cast<std::uint32_t>(size)); put32(tx+20,cycles);
  std::memcpy(tx+32,payload,size); put32(tx+24,crc(tx+32,size)); put32(tx+28,crc(tx,28));
  tx_size = size+32;
}
void error(std::uint32_t status) { ++rejected; respond(status,0,"",0); fill=0; wanted=32; }
void execute() {
  const auto command=get32(rx+4), size=get32(rx+16);
  if (crc(rx+32,size)!=get32(rx+20)) { error(4); return; }
  if (command == 1 && size == 0) {
    char uid[33]; for(unsigned i=0;i<16;++i) std::snprintf(uid+i*2,3,"%02x",board_uid[i]);
    const int n=std::snprintf(trace.data,sizeof(trace.data),
      "{\"protocol\":1,\"uid\":\"%s\",\"build\":\"%s\",\"source\":\"%s\",\"contract\":\"sr24000.hop180.bins80.xover40\",\"clock_hz\":%lu,\"cpu1_actcsr\":%lu,\"u55_opened\":false,\"cpp_initialised\":%s,\"rejected\":%lu,\"sequence\":%lu,\"trajectory_bytes\":%u,\"trace_bytes\":%u}",
      uid,K1_BUILD_ID,K1_SOURCE_PIN,(unsigned long)clock_hz,(unsigned long)cpu_wait,initialised?"true":"false",
      (unsigned long)rejected,(unsigned long)trajectory.sequence,unsigned(sizeof(trajectory)),unsigned(sizeof(trace)));
    if(n<0 || std::size_t(n)>=sizeof(trace.data)) error(8); else respond(0,0,trace.data,std::size_t(n));
  } else if(command==4 && size==0) {
    const auto before=k1_cycle_count(); const unsigned result=k1_time_probe();
    const auto cycles=k1_cycle_count()-before;
    const int n=std::snprintf(trace.data,sizeof(trace.data),"{\"time_probe_failure\":%u,\"million_beats\":1000000}",result);
    respond(result?10:0,cycles,trace.data,std::size_t(n));
  } else if(command==3 && size==8) {
    const std::uint64_t epoch=get32(rx+32)|(std::uint64_t(get32(rx+36))<<32);
    if (!epoch) { error(5); return; }
    trajectory.~Trajectory(); new (&trajectory) fixture::Trajectory(); trajectory.epoch=epoch;
    respond(0,0,"RESET",5);
  } else if(command==2 && size==360) {
    if(get32(rx+12)!=trajectory.sequence+1) { error(6); return; }
    alignas(4) std::int16_t hop[180]; std::memcpy(hop,rx+32,sizeof(hop));
    const std::uint32_t before=k1_cycle_count(); trajectory.process(hop);
    const std::uint32_t elapsed=k1_cycle_count()-before; // modulo difference, individual work must be < one wrap.
    trajectory.trace(trace);
    if(!trace.valid || !trajectory.output.valid) error(7);
    else respond(0,elapsed,trace.data,trace.size);
  } else error(3);
  fill=0; wanted=32;
}
}
extern "C" void k1_fixture_initialise(const std::uint8_t uid[16],std::uint32_t hz,std::uint32_t wait) {
  std::memcpy(board_uid,uid,16); clock_hz=hz; cpu_wait=wait;
  // Constructor witness: ChannelRenderState must have installed each channel ID.
  initialised=trajectory.b.channel()==k1::core::visual::PixelChannelId::kChannelB;
}
extern "C" void k1_fixture_consume(const std::uint8_t* bytes,std::size_t count,std::uint32_t now) {
  if(tx_size) return; // Single outstanding transaction; USB read arm enforces backpressure.
  for(std::size_t i=0;i<count;++i) {
    if(fill==0) started=now;
    rx[fill++]=bytes[i];
    if(fill==32) {
      if(std::memcmp(rx,"K1S1",4)!=0 || get32(rx+24)!=1 || crc(rx,28)!=get32(rx+28)) { error(1); return; }
      if(get32(rx+16)>360) { error(2); return; }
      wanted=32+get32(rx+16);
    }
    if(fill==wanted) { execute(); return; }
  }
}
extern "C" void k1_fixture_poll(std::uint32_t now) { if(fill && now-started>2000) error(9); }
extern "C" void k1_fixture_disconnect(void) { fill=0; wanted=32; tx_size=0; }
extern "C" const std::uint8_t* k1_fixture_reply(std::size_t* count) { *count=tx_size; return tx; }
extern "C" void k1_fixture_sent(void) { tx_size=0; }
