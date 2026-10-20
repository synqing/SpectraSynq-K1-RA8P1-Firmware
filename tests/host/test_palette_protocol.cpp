#include "fixture_app.h"
#include "palette_runtime.h"
#include "core/visual/product_palette.h"
#include "ws281x_diag.h"
#include "ws2816_gpio_emit.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using namespace k1::core::visual;
static unsigned emits;
static std::vector<std::uint8_t> last_wire;
static std::vector<std::uint8_t> lane_wire[2];
static int emission_failure = 0;
static std::uint32_t hardware_cycles;
extern "C" std::uint32_t k1_cycle_count() { return hardware_cycles; }
static void poll(unsigned ms) { hardware_cycles=ms*1000000U; k1_fixture_poll(ms); }
extern "C" std::size_t k1_platform_metrics(char*,std::size_t) { return 0; }
void k1_ws2816_set_clock(std::uint32_t) {}
packed_submit_result_t k1_ws2816_submit_packed_lanes(const std::uint8_t*,std::size_t,const std::uint8_t*,std::size_t,packed_lane_completion_t*) { return kPackedWrongCount; }
int k1_ws281x_diag_emit(const std::uint8_t* data,std::size_t size,std::uint32_t profile,
                       std::uint32_t pin,std::uint32_t hz,k1_ws281x_diag_result_t* result) {
#ifdef K1_PALETTE_WS2816
  assert(size==480 && profile==4 && pin==emits%2 && hz==1000000000U);
#else
  assert(size==384 && profile==1 && pin==0 && hz==1000000000U);
#endif
  last_wire.assign(data,data+size); lane_wire[pin]=last_wire;
  ++emits; *result={}; result->emit_cycles=3872400U; return emission_failure;
}
static std::uint32_t crc(const std::uint8_t* p,std::size_t n) {
  std::uint32_t c=~0U; while(n--) { c^=*p++; for(unsigned i=0;i<8;++i)c=(c>>1)^((c&1)?0xedb88320U:0); } return ~c;
}
static void put(std::uint8_t* p,std::uint32_t n) { for(unsigned i=0;i<4;++i)p[i]=n>>(8*i); }
static std::uint32_t get(const std::uint8_t* p) { return p[0]|(p[1]<<8)|(p[2]<<16)|(std::uint32_t(p[3])<<24); }
static std::vector<std::uint8_t> request(unsigned op,const std::vector<std::uint8_t>& payload={}) {
  std::vector<std::uint8_t> p(32+payload.size()); std::memcpy(p.data(),"K1S1",4);
  put(p.data()+4,op); put(p.data()+8,42); put(p.data()+16,payload.size()); put(p.data()+24,1);
  std::memcpy(p.data()+32,payload.data(),payload.size()); put(p.data()+20,crc(p.data()+32,payload.size())); put(p.data()+28,crc(p.data(),28));
  for(std::size_t i=0;i<p.size();i+=7) k1_fixture_consume(p.data()+i,std::min<std::size_t>(7,p.size()-i),100);
  std::size_t size; const auto data=k1_fixture_reply(&size); assert(size>=32);
  assert(get(data+28)==crc(data,28) && get(data+24)==crc(data+32,size-32));
  std::vector<std::uint8_t> out(data,data+size); k1_fixture_sent(); return out;
}
static std::vector<std::uint8_t> config(unsigned a,unsigned b,unsigned flags) {
  const unsigned words[]{1,a,b,0,0,flags,255,0};
  std::vector<std::uint8_t> p(32); for(unsigned i=0;i<8;++i)put(p.data()+4*i,words[i]);return p;
}
int main() {
  const std::uint8_t uid[16]{}; k1_fixture_initialise(uid,1000000000U,0);
  const auto boot=request(17);
  const std::string boot_body(reinterpret_cast<const char*>(boot.data()+32),boot.size()-32);
#ifdef K1_PALETTE_AUTOSTART
  auto require_boot = [](bool ok, const char* what) {
    if (!ok) {
      std::fprintf(stderr, "AUTOSTART_REQUIRE %s\n", what);
      std::_Exit(2);
    }
  };
  require_boot(boot_body.find("\"automatic_cycle\":false")!=std::string::npos, "automatic_cycle");
  require_boot(boot_body.find("\"emit_enabled\":true")!=std::string::npos, "emit_enabled");
  require_boot(boot_body.find("\"mode_a\":32")!=std::string::npos, "mode_a");
  require_boot(boot_body.find("\"mode_b\":32")!=std::string::npos, "mode_b");
  require_boot(boot_body.find("PALETTE_BOUNCE")==std::string::npos, "no_palette_bounce");
#ifdef K1_PALETTE_MORPH
  assert(boot_body.find("\"showcase\":false")!=std::string::npos);
  assert(boot_body.find("\"transition_ms\":1500")!=std::string::npos);
#ifndef K1_PALETTE_WS2816
  assert(boot_body.find("\"brightness\":24")!=std::string::npos);
#endif
#endif
#else
  assert(boot_body.find("\"active\":false")!=std::string::npos);
#endif
#ifdef K1_PALETTE_WS2816
  assert(get(request(19).data()+4)==8); // No completed submission yet.
#endif
  auto catalogue=request(15); assert(get(catalogue.data()+4)==0);
  const std::string list(reinterpret_cast<const char*>(catalogue.data()+32),catalogue.size()-32);
  assert(list.find("\"count\":44")!=std::string::npos);
  for(const auto& item:productPaletteCatalogue()) assert(list.find(item.name)!=std::string::npos);
  for(unsigned id=0;id<44;++id) {
    auto selected=request(16,config(id,43-id,1)); assert(get(selected.data()+4)==0);
    for(unsigned ch=0;ch<2;++ch) {
      std::vector<std::uint8_t> which(4); put(which.data(),ch);
      auto frame=request(18,which); assert(get(frame.data()+4)==0 && frame.size()==512);
      for(unsigned i=0;i<160;++i) {
        const auto p=sampleProductPaletteFastLed16(ch?43-id:id,(i<80U?79U-i:i-80U)*255U/79U);
        assert(frame[32+i*3]==p.red && frame[33+i*3]==p.green && frame[34+i*3]==p.blue);
      }
    }
    assert(get(request(16,config(id,44,1)).data()+4)==3);
    const auto state=request(17); assert(get(state.data()+4)==0);
  }
#ifdef K1_PALETTE_MORPH
  auto v2=config(33,43,1); v2.resize(36); put(v2.data(),2); put(v2.data()+32,1000);
  assert(get(request(16,v2).data()+4)==0);
  auto bad=v2; put(bad.data()+32,10001); assert(get(request(16,bad).data()+4)==3);
  bad=v2; put(bad.data(),1); assert(get(request(16,bad).data()+4)==3);
  bad=v2; bad.resize(32); assert(get(request(16,bad).data()+4)==3);
#endif
  assert(emits==0);
  assert(get(request(16,config(43,0,5)).data()+4)==0);
  constexpr unsigned period_ms=(k1::titan::kPalettePeriodUs+999U)/1000U;
#ifdef K1_PALETTE_WS2816
  constexpr unsigned calls=2, wire_size=480;
#else
  constexpr unsigned calls=1, wire_size=384;
#endif
  poll(0); poll(period_ms); assert(emits==calls && last_wire.size()==wire_size);
  {
    const auto live=request(17);
    const std::string live_body(reinterpret_cast<const char*>(live.data()+32),live.size()-32);
#ifdef K1_PALETTE_WS2816
    assert(live_body.find("\"configured_backend\":\"ws2816_gpio\"")!=std::string::npos);
    assert(live_body.find("\"output_backend\":\"ws2816_gpio\"")!=std::string::npos);
#else
    assert(live_body.find("\"configured_backend\":\"gpio_diagnostic\"")!=std::string::npos);
    assert(live_body.find("\"output_backend\":\"gpio_diagnostic\"")!=std::string::npos);
#endif
    assert(live_body.find("\"physical_admission\":\"unproven\"")!=std::string::npos);
    assert(live_body.find("wire_verified")==std::string::npos);
  }
#ifdef K1_PALETTE_WS2816
  const auto capture=request(19);
  assert(get(capture.data()+4)==0 && capture.size()==32+1504);
  const auto* snapshot=capture.data()+32;
  assert(get(snapshot)==1 && get(snapshot+4)==1 && get(snapshot+12)==255);
  assert(get(snapshot+16)==2 && get(snapshot+20)==80 && get(snapshot+24)==48);
  for(unsigned lane=0;lane<2;++lane) {
    assert(!std::memcmp(snapshot+544+lane*480,lane_wire[lane].data(),480));
    for(unsigned i=0;i<80;++i) for(unsigned grb=0;grb<3;++grb) {
      const unsigned rgb=grb==0?1:grb==1?0:2;
      const unsigned native=snapshot[64+(lane*80+i)*3+rgb];
      const unsigned offset=544+lane*480+i*6+grb*2;
      assert((unsigned(snapshot[offset])*256+snapshot[offset+1])==native*257);
    }
  }
  assert(get(request(19,std::vector<std::uint8_t>(4)).data()+4)!=0);
#endif
  k1_fixture_disconnect();
  poll(2*period_ms); assert(emits==2*calls); // Autonomous output survives CDC disconnect.
  assert(get(request(16,config(43,0,0)).data()+4)==0);
  poll(4*period_ms); assert(emits==2*calls);
  const auto stopped=request(17);
  const std::string body(reinterpret_cast<const char*>(stopped.data()+32),stopped.size()-32);
  assert(body.find("\"active\":false")!=std::string::npos);
  assert(body.find("\"output_backend\":\"disabled\"")!=std::string::npos);
#ifdef K1_PALETTE_WS2816
  assert(body.find("\"configured_backend\":\"ws2816_gpio\"")!=std::string::npos);
#else
  assert(body.find("\"configured_backend\":\"gpio_diagnostic\"")!=std::string::npos);
#endif
  assert(body.find("\"physical_admission\":\"unproven\"")!=std::string::npos);
#ifdef K1_PALETTE_WS2816
  assert(body.find("\"wire_profile\":4")!=std::string::npos);
  assert(body.find("\"bench_pixels\":160")!=std::string::npos);
#endif
#ifdef K1_PALETTE_MORPH
  auto c3=config(33,43,5); c3.resize(40); put(c3.data(),3);
  put(c3.data()+12,100); put(c3.data()+16,101); put(c3.data()+36,4000);
  assert(get(request(16,c3).data()+4)==0);
  put(c3.data()+4,43); put(c3.data()+8,33); put(c3.data()+32,1500);
  assert(get(request(16,c3).data()+4)==0);
  hardware_cycles+=750000000U; k1_fixture_poll(40); // RTOS tick deliberately frozen.
  auto half=request(17);
  std::string half_body(reinterpret_cast<const char*>(half.data()+32),half.size()-32);
  assert(half_body.find("\"transition_a_q16\":32767")!=std::string::npos);
  hardware_cycles+=750000000U; k1_fixture_poll(40);
  auto full=request(17);
  std::string full_body(reinterpret_cast<const char*>(full.data()+32),full.size()-32);
  assert(full_body.find("\"transition_a_q16\":65535")!=std::string::npos);
#endif
  // The tap must retain failed submission status rather than certify it as light.
#ifdef K1_PALETTE_WS2816
  emission_failure=-7;
  assert(get(request(16,config(33,43,5)).data()+4)==0);
  hardware_cycles+=100000000U; k1_fixture_poll(40);
  const auto failed=request(19);
  assert(get(failed.data()+32+48)==static_cast<std::uint32_t>(-7));
  assert(get(failed.data()+32+52)==static_cast<std::uint32_t>(-7));
#endif
  std::puts("PALETTE_PROTOCOL_PASS palettes=44 channels=2 native_output_after_disconnect=true");
}
