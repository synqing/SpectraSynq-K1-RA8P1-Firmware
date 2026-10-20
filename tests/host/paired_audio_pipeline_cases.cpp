#include "k1_live_runtime.h"
#include "pdm_target.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
static unsigned allocations;
void* operator new(std::size_t n) {++allocations;if(void* p=std::malloc(n))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
extern "C" {
void host_capture_init(void);
void host_capture_raw(const int16_t*,uint64_t);
uint32_t host_capture_drops(void);
// Donor acceptance seam only. The changed capture/runtime/inhouse path is actual source.
bool k1_emotiscope_push_pcm64(const float*,uint32_t) {return true;}
void k1_emotiscope_status(K1EmotiscopeStatus* out) {*out={};out->valid=true;}
void k1_emotiscope_audio_gap(void) {}
}
static k1::titan::LiveAudioRuntime runtime;
int main() {
 host_capture_init();runtime.initialise(1000000000);runtime.setInhouseActive(true);
 k1::titan::InhouseAudioCursor cursor{};unsigned fresh=0,quiet=0,loud=0;uint64_t previous=0;
 const auto before=allocations;
 for(unsigned slot=0;slot<1100;++slot) {
  const int amplitude=(slot/80)%2 ? 16000 : 400;
  int16_t raw[296];for(unsigned i=0;i<296;++i)raw[i]=int16_t(std::lrint(amplitude*std::sin(2*3.141592653589793*440*(slot*296+i)/41422)));
  const uint64_t now=1000000+uint64_t(slot+1)*296*1000000/41422;
  const auto delta=now-previous;previous=now;runtime.advanceHostCycles(uint32_t(delta*1000));
  host_capture_raw(raw,now);
  while(runtime.serviceOneHop()==k1::titan::LiveHopStatus::Ok) {}
  k1::core::visual::InhouseWaveformAudio out{};
  if(runtime.acquireInhouse(cursor,runtime.nowUs(),out)) {
   ++fresh;assert(out.unguarded_vu_valid);assert(std::isfinite(out.unguarded_vu));
   if(slot%80>5) {
    if(amplitude==400) {++quiet;assert(out.unguarded_vu==out.vu_level);}
    else {++loud;assert(out.unguarded_vu>5 && out.unguarded_vu<6);}
   }
  }
 }
 assert(fresh>900 && quiet>300 && loud>300);assert(host_capture_drops()==0);assert(allocations==before);
 std::printf("PASS: actual raw-slot guard/resampler/descriptor/runtime/publication: fresh=%u quiet=%u loud=%u, drops=0, allocations=0\n",fresh,quiet,loud);
}
