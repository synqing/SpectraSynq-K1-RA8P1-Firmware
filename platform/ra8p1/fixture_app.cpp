// K1 scalar fixture transport. This is neither a capture driver nor a realtime campaign.
#include "fixture_app.h"
#define K1_TRAJECTORY_CYCLE_COUNT() k1_cycle_count()
#include "trajectory.h"
#include "time_probe.h"
#include "build_identity.h"
#include <new>
#include <cstdio>
#include <cstring>
#ifdef K1_RESIDENT_SCHEDULE
#include "resident_controls.h"
#endif
namespace {
fixture::Trajectory trajectory;
fixture::Trace trace;
std::uint8_t rx[392], tx[20000], board_uid[16];
std::size_t fill = 0, wanted = 32, tx_size = 0;
std::uint32_t started = 0, clock_hz = 0, cpu_wait = 0, rejected = 0;
bool initialised = false;
void respond(std::uint32_t status, std::uint32_t cycles, const char* payload, std::size_t size);
void error(std::uint32_t status);
#ifdef K1_RESIDENT_SCHEDULE
template<unsigned Maximum> struct Distribution {
  std::uint32_t bins[Maximum+1]{};
  std::uint64_t sum=0;
  std::uint32_t count=0, maximum=0;
  void add(std::uint32_t value) {
    ++bins[value>Maximum?Maximum:value]; sum+=value; ++count;
    if(value>maximum) maximum=value;
  }
  std::uint32_t percentile(unsigned numerator) const {
    if(!count) return 0;
    const std::uint32_t wanted=(count*numerator+99)/100;
    std::uint32_t seen=0;
    for(unsigned i=0;i<=Maximum;++i) if((seen+=bins[i])>=wanted) return i;
    return Maximum;
  }
};
struct ScheduleState {
  bool active=false, finished=false;
  std::uint32_t loops=0, loop=0, hop=0, flags=0, next_release=0;
  std::uint32_t correctness_failures=0, deadline_misses=0, render_misses=0;
  std::uint32_t release_guard_failures=0, backlog_highwater=0;
  Distribution<8000> total, tempo, ordinary;
  Distribution<2000> render;
  Distribution<1000> lateness;
  Distribution<4000> telemetry;
} schedule;
std::uint32_t microseconds(std::uint32_t cycles) {
  return clock_hz?static_cast<std::uint32_t>((std::uint64_t(cycles)*1000000U+clock_hz-1)/clock_hz):0xffffffffU;
}
template<class D> void distribution(char* output,std::size_t capacity,std::size_t& offset,const char* name,const D& d) {
  const int n=std::snprintf(output+offset,capacity-offset,
    "\"%s\":{\"count\":%lu,\"mean_us\":%.3f,\"p50_us\":%lu,\"p95_us\":%lu,\"p99_us\":%lu,\"max_us\":%lu}",
    name,(unsigned long)d.count,d.count?double(d.sum)/d.count:0.0,
    (unsigned long)d.percentile(50),(unsigned long)d.percentile(95),
    (unsigned long)d.percentile(99),(unsigned long)d.maximum);
  if(n<0 || std::size_t(n)>=capacity-offset) offset=capacity; else offset+=std::size_t(n);
}
void schedule_status() {
  std::size_t n=0;
  const int first=std::snprintf(trace.data,sizeof(trace.data),
    "{\"profile\":\"k1-production-resident-v1\",\"active\":%s,\"finished\":%s,\"loops_requested\":%lu,\"loops_complete\":%lu,\"hop\":%lu,\"queue_capacity\":1,\"drops\":0,\"coalesces\":0,\"crc_mutation_injected\":%s,\"correctness_failures\":%lu,\"deadline_misses\":%lu,\"render_misses\":%lu,\"release_guard_failures\":%lu,\"backlog_highwater\":%lu,",
    schedule.active?"true":"false",schedule.finished?"true":"false",
    (unsigned long)schedule.loops,(unsigned long)schedule.loop,(unsigned long)schedule.hop,
    (schedule.flags&2U)?"true":"false",
    (unsigned long)schedule.correctness_failures,(unsigned long)schedule.deadline_misses,
    (unsigned long)schedule.render_misses,(unsigned long)schedule.release_guard_failures,
    (unsigned long)schedule.backlog_highwater);
  if(first<0 || std::size_t(first)>=sizeof(trace.data)) { error(8); return; }
  n=std::size_t(first);
  distribution(trace.data,sizeof(trace.data),n,"total",schedule.total); if(n<sizeof(trace.data)) trace.data[n++]=',';
  distribution(trace.data,sizeof(trace.data),n,"tempo",schedule.tempo); if(n<sizeof(trace.data)) trace.data[n++]=',';
  distribution(trace.data,sizeof(trace.data),n,"ordinary",schedule.ordinary); if(n<sizeof(trace.data)) trace.data[n++]=',';
  distribution(trace.data,sizeof(trace.data),n,"render",schedule.render); if(n<sizeof(trace.data)) trace.data[n++]=',';
  distribution(trace.data,sizeof(trace.data),n,"lateness",schedule.lateness); if(n<sizeof(trace.data)) trace.data[n++]=',';
  distribution(trace.data,sizeof(trace.data),n,"telemetry",schedule.telemetry);
  if(n+2>sizeof(trace.data)) { error(8); return; }
  trace.data[n++]='}'; respond(0,0,trace.data,n);
}
#endif
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
  } else if(command==6 && size==0) {
    const std::size_t n=k1_platform_metrics(trace.data,sizeof(trace.data));
    if(!n) error(8); else respond(0,0,trace.data,n);
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
  }
#ifdef K1_RESIDENT_SCHEDULE
  else if(command==7 && size==8) {
    const std::uint32_t loops=get32(rx+32), flags=get32(rx+36);
    if(schedule.active || !loops || loops>40 || flags>3) { error(3); return; }
    std::memset(&schedule,0,sizeof(schedule)); schedule.active=true; schedule.loops=loops; schedule.flags=flags;
    trajectory.~Trajectory(); new (&trajectory) fixture::Trajectory(); trajectory.epoch=2;
    schedule.next_release=k1_cycle_count()+clock_hz/1000U;
    respond(0,0,"STARTED",7);
  } else if(command==8 && size==0) schedule_status();
#endif
  else if((command==2 || command==5) && size==360) {
    if(get32(rx+12)!=trajectory.sequence+1) { error(6); return; }
    alignas(4) std::int16_t hop[180]; std::memcpy(hop,rx+32,sizeof(hop));
    const std::uint32_t before=k1_cycle_count(); trajectory.process(hop);
    const std::uint32_t elapsed=k1_cycle_count()-before; // modulo difference, individual work must be < one wrap.
    trace.format=command==5?fixture::Trace::Format::binary:fixture::Trace::Format::text;
    trajectory.trace(trace);
    if(!trace.valid || !trajectory.output.valid) error(7);
    else respond(0,elapsed,trace.data,trace.size);
    trace.format=fixture::Trace::Format::text;
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
#ifdef K1_RESIDENT_SCHEDULE
extern "C" bool k1_fixture_schedule_active(void) { return schedule.active; }
extern "C" void k1_fixture_schedule_step(void) {
  if(!schedule.active) return;
  const std::uint32_t now=k1_cycle_count();
  if(static_cast<std::int32_t>(now-schedule.next_release)<0) return;
  const std::uint32_t lateness=now-schedule.next_release;
  const std::uint32_t period=static_cast<std::uint32_t>((std::uint64_t(clock_hz)*3U)/400U); // 7.5 ms.
  const std::uint32_t index=schedule.hop;
  const std::int16_t* pcm=k1_resident_pcm[k1_resident_index[index]];
  trajectory.process(pcm);
  const std::uint32_t completion=lateness+trajectory.total_cycles;
  const std::uint32_t total_us=microseconds(trajectory.total_cycles);
  schedule.total.add(total_us);
  (trajectory.output.tempo.updated?schedule.tempo:schedule.ordinary).add(total_us);
  if(trajectory.rendered) schedule.render.add(microseconds(trajectory.render_cycles));
  schedule.lateness.add(microseconds(lateness));
  const std::uint32_t backlog=1+lateness/period;
  if(backlog>schedule.backlog_highwater) schedule.backlog_highwater=backlog;
  if(completion>period) ++schedule.deadline_misses;
  if(trajectory.rendered && trajectory.render_cycles>clock_hz/500U) ++schedule.render_misses;
  if(lateness>clock_hz/10000U) ++schedule.release_guard_failures; // predeclared 100 us start guard.
  if(schedule.flags&1U) {
    const std::uint32_t before=k1_cycle_count();
    trace.format=fixture::Trace::Format::binary; trajectory.trace(trace);
    const std::uint32_t expected_crc=k1_resident_crc[index] ^
      ((schedule.flags&2U) && schedule.loop==0 && schedule.hop==0 ? 1U : 0U);
    if(!trace.valid || trace.size!=k1_resident_length[index] ||
       crc(reinterpret_cast<const std::uint8_t*>(trace.data),trace.size)!=expected_crc) ++schedule.correctness_failures;
    trace.format=fixture::Trace::Format::text;
    schedule.telemetry.add(microseconds(k1_cycle_count()-before));
  }
  schedule.next_release+=period;
  if(++schedule.hop==K1_RESIDENT_HOPS) {
    schedule.hop=0;
    if(++schedule.loop==schedule.loops) { schedule.active=false; schedule.finished=true; return; }
    trajectory.~Trajectory(); new (&trajectory) fixture::Trajectory(); trajectory.epoch=2;
  }
}
#else
extern "C" bool k1_fixture_schedule_active(void) { return false; }
extern "C" void k1_fixture_schedule_step(void) {}
#endif
