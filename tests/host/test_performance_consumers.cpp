// Actual fixture command graph with hostile lifecycle stubs, not a reimplementation.
#include <cassert>
#include <vector>
#include <cstring>
#include <cstdio>
#include "fixture_app.cpp"

static uint32_t fake_cycles, cycle_increment, pair_polls, pair_starts;
static bool pair_active, queued_generation, inject_reentry;
extern "C" uint32_t k1_cycle_count(void) { fake_cycles+=cycle_increment; return fake_cycles; }
extern "C" size_t k1_platform_metrics(char* p,size_t n) {
  assert(!pair_active); if(n<3) return 0; std::memcpy(p,"{}",3);return 2;
}
void k1_ws2816_set_clock(uint32_t) {}
packed_submit_result_t k1_ws2816_submit_packed_lanes(const uint8_t*,size_t,
  const uint8_t*,size_t,packed_lane_completion_t*) { assert(false); return kPackedWrongCount; }
int k1_ws281x_diag_emit(const uint8_t*,size_t,uint32_t,uint32_t,uint32_t,k1_ws281x_diag_result_t*) {
  assert(false); return -1;
}
extern "C" int k1_ws281x_gpt_dma_hw_pair_poll(void) {
  ++pair_polls;
  if(inject_reentry) { inject_reentry=false; k1_fixture_poll(10); }
  if(queued_generation) {queued_generation=false;pair_active=true;++pair_starts;}
  return 0;
}
extern "C" int k1_ws281x_gpt_dma_hw_pair_submit(const uint8_t*,size_t,const uint8_t*,size_t,uint32_t,uint32_t) {
  ++pair_starts;pair_active=true;return 0;
}
extern "C" int k1_ws281x_gpt_dma_hw_pair_transmitting(void) { return pair_active; }
extern "C" int k1_ws281x_gpt_dma_hw_pair_busy(void) {k1_ws281x_gpt_dma_hw_pair_poll();return pair_active;}
extern "C" int k1_ws281x_gpt_dma_hw_pair_ready(void) {k1_ws281x_gpt_dma_hw_pair_poll();return !pair_active;}

static void send(unsigned op, const std::vector<uint8_t>& body={}) {
  std::vector<uint8_t> p(32+body.size()); std::memcpy(p.data(),"K1S1",4);
  put32(p.data()+4,op);put32(p.data()+8,42);put32(p.data()+16,body.size());put32(p.data()+24,1);
  std::memcpy(p.data()+32,body.data(),body.size());put32(p.data()+20,crc(p.data()+32,body.size()));
  put32(p.data()+28,crc(p.data(),28));
  k1_fixture_consume(p.data(),p.size(),100);
}
static void retire_reply(unsigned expected=0) {
  size_t n=0;const auto* p=k1_fixture_reply(&n);assert(n>=32);if(get32(p+4)!=expected) std::fprintf(stderr,"response op=%u status=%u expected=%u\n",get32(rx+4),get32(p+4),expected);assert(get32(p+4)==expected);
  assert(crc(p,28)==get32(p+28)); k1_fixture_sent();
}

int main() {
 using namespace k1::titan;using namespace k1::core::visual;
 const uint8_t uid[16]{};k1_fixture_initialise(uid,1000000000U,0);
 const char* names[]={"level","tonal","colour_floor","fallback","motion","tail","warmth","bulb","novelty","bass_onset","kick","snare","hihat","transient","mood","low"};
 auto field=[](LiveConfigBlob& c,unsigned i)->float& {
  switch(i) {
   case 0:return c.focus_a[0];case 1:return c.focus_a[5];case 2:return c.visual_a.chroma;
   case 3:return c.visual_a.vp_waveform_fallback_brightness;case 4:return c.visual_a.vp_waveform_shift_rate;
   case 5:return c.visual_a.vp_waveform_idle_fade;case 6:return c.visual_a.incandescent_filter;case 7:return c.visual_a.bulb_opacity;
   case 8:return c.focus_a[1];case 9:return c.focus_a[7];case 10:return c.focus_a[10];case 11:return c.focus_a[11];case 12:return c.focus_a[12];case 13:return c.focus_a[9];case 14:return c.visual_a.mood;default:return c.focus_a[2];
  }
 };
 for(unsigned mode:{3U,7U,8U,9U,11U,12U,13U,14U,15U,16U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U,28U,29U,32U}) {
  for(unsigned id=0;id<16;++id) {
   PaletteRuntime low,high;PaletteConfig cfg;cfg.mode_a=cfg.mode_b=mode;cfg.palette_a=33;cfg.palette_b=43;cfg.brightness=24;cfg.flags=1;
   assert(low.configure(cfg,0)&&high.configure(cfg,0));auto a=live_cfg,b=live_cfg;
   a.visual_a.vp_waveform_fallback_brightness=b.visual_a.vp_waveform_fallback_brightness=.5F;
   a.visual_a.flags&=~(kVisFixSecondary|kVisIncandescent);b.visual_a.flags=a.visual_a.flags;
   field(a,id)=id==5?.90F:0.F;field(b,id)=id==4?240.F:id==5?.999F:(id<2||(id>=8&&id!=14))?2.F:1.F;
   live_copy_visual(low.channel(0).controls(),a.visual_a);live_copy_focus(low.channel(0).audioFocus(),a.focus_a);
   live_copy_visual(high.channel(0).controls(),b.visual_a);live_copy_focus(high.channel(0).audioFocus(),b.focus_a);
   k1::contract::AudioFeaturesV1 audio{};k1::core::audio::TempoTrackerEvent tempo{};VisualWaveformHistory wave{};
   audio.validity_flags=k1::contract::kValidBaseSnapshot|k1::contract::kValidChroma|k1::contract::kValidSpectrum;
   audio.peak_scaled=.65F;audio.vu_level=.65F;audio.chroma_strength=.45F;audio.chroma_a_origin[0]=.9F;audio.chroma_a_origin[4]=.4F;
   audio.spectral_energy=.35F;audio.novelty=.3F;audio.low_energy=.4F;audio.transient_level=.45F;
   audio.bass_onset_strength=.4F;audio.kick_strength=.4F;audio.snare_strength=.4F;audio.hihat_strength=.4F;audio.transient_strength=.8F;
   for(unsigned i=0;i<80;++i)audio.spectrum[i]=.2F+.002F*i;
   wave.peak_scaled=.65F;wave.raw_maximum=4000.F;bool differs=false;
   for(unsigned i=1;i<=240;++i) {
    audio.sequence=i;audio.publish_time_us=i*kPalettePeriodUs;
    audio.event_flags=(i%24==1)?k1::contract::kEventKick|k1::contract::kEventSnare|k1::contract::kEventHihat|k1::contract::kEventBassOnset|k1::contract::kEventTransient:0;
    audio.kick_event_id=audio.snare_event_id=audio.hihat_event_id=audio.onset_event_id=audio.transient_event_id=i/24+1;
    VisualAudioFrameView view{audio,tempo,wave,audio.publish_time_us};assert(low.step(audio.publish_time_us,&view));assert(high.step(audio.publish_time_us,&view));
    differs|=std::memcmp(low.channel(0).frame().data(),high.channel(0).frame().data(),sizeof(k1::core::Pixel8)*160)!=0;
   }
   std::printf("CONSUMER %u %s %u\n",mode,names[id],differs?1:0);
  }
 }
}
