#include "palette_transition.h"
#include "palette_runtime.h"
#include "core/visual/product_effect_renderer.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
using namespace k1::core;
using namespace k1::core::visual;
using namespace k1::titan;
static bool same(Pixel8 a, Pixel8 b) { return !std::memcmp(&a, &b, sizeof(a)); }
static void close(PaletteLinearRgb a, PaletteLinearRgb b) {
  assert(std::fabs(a.red-b.red)<0.000002F);
  assert(std::fabs(a.green-b.green)<0.000002F);
  assert(std::fabs(a.blue-b.blue)<0.000002F);
}
int main() {
  PaletteTransition t;
  unsigned endpoints = 0;
  for (unsigned id=0; id<44; ++id) {
    t.select(id, 0, 0);
    for (unsigned index=0; index<256; ++index) {
      assert(same(t.fast(index), sampleProductPaletteFastLed16(id,index)));
      const float phase = float(index)/255.0F;
      const auto a=t.hd(phase,0.73F), b=sampleProductPaletteHd(id,phase,0.73F);
      assert(!std::memcmp(&a,&b,sizeof(a))); ++endpoints;
    }
    const unsigned other=(id+17)%44;
    t.select(other,1000,0); t.advance(500000);
    for (unsigned index=0;index<256;++index) {
      auto a=sampleProductPaletteHd(id,float(index)/255.0F,0.73F);
      auto b=sampleProductPaletteHd(other,float(index)/255.0F,0.73F);
      close(t.hd(float(index)/255.0F,0.73F),
            {(a.red+b.red)*0.5F,(a.green+b.green)*0.5F,(a.blue+b.blue)*0.5F});
    }
    const auto before=t.hd(0.317F); const auto fast=t.fast(83);
    const auto phase=t.brightestPhase();
    t.select((id+29)%44,1000,500000);
    close(t.hd(0.317F),before); assert(same(t.fast(83),fast));
    assert(t.brightestPhase()==phase);
    t.advance(1000000); const auto progress=t.progress();
    t.select((id+29)%44,1000,1000000); assert(t.progress()==progress);
    t.advance(1500000); assert(!t.active());
    for(unsigned i=0;i<256;++i)
      assert(same(t.fast(i),sampleProductPaletteFastLed16((id+29)%44,i)));
  }
  // Every catalogue member can contribute after repeated interruptions. Work and
  // storage remain bounded even for this adversarial control pattern.
  t.select(0,0,0);
  for(unsigned i=1;i<44;++i) { t.select(i,10000,i*10000U); t.advance(i*10000U+5000U); }
  assert(t.contributors()==44);
  const auto begin=std::chrono::steady_clock::now();
  volatile float sink=0.0F;
  for(unsigned i=0;i<16000;++i) sink+=t.hd(float(i%997)/997.0F).red;
  const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-begin).count();
  t.select(43,0,1000000); assert(!t.active() && t.contributors()==1);
  PaletteRuntime runtime;
  PaletteConfig c; c.flags=1; c.palette_b=43;
  assert(runtime.configure(c,0) && runtime.step(0,nullptr));
  c.version=2; c.transition_ms=1000; c.palette_a=43; c.palette_b=0;
  assert(runtime.configure(c,10000) && runtime.step(10000,nullptr));
  assert(same(runtime.channel(0).frame()[0],sampleProductPaletteFastLed16(0,0)));
  assert(same(runtime.channel(1).frame()[0],sampleProductPaletteFastLed16(43,0)));
  assert(runtime.step(510000,nullptr));
  const auto offset=static_cast<std::uint8_t>(510000/20000);
  const auto a=sampleProductPaletteFastLed16(0,offset), b=sampleProductPaletteFastLed16(43,offset);
  const Pixel8 mid{static_cast<std::uint8_t>((unsigned(a.red)+b.red+1)/2),
                   static_cast<std::uint8_t>((unsigned(a.green)+b.green+1)/2),
                   static_cast<std::uint8_t>((unsigned(a.blue)+b.blue+1)/2)};
  assert(same(runtime.channel(0).frame()[0],mid));
  auto invalid=c; invalid.transition_ms=10001; assert(!runtime.configure(invalid,510000));
  invalid=c; invalid.version=1; assert(!runtime.configure(invalid,510000));
  assert(runtime.config().transition_ms==1000);
  assert(runtime.step(1010000,nullptr));
  assert(!runtime.channel(0).controls().palette_transition->active());
  k1::contract::AudioFeaturesV1 audio;
  audio.validity_flags = k1::contract::kValidBaseSnapshot | k1::contract::kValidSpectrum;
  audio.event_flags = k1::contract::kEventBeat | k1::contract::kEventOnset;
  audio.peak_scaled=0.7F; audio.vu_level=0.6F; audio.novelty=0.5F;
  audio.low_energy=audio.mid_energy=audio.high_energy=0.5F;
  audio.beat_phase=0.25F; audio.beat_confidence=0.9F;
  audio.onset_strength=audio.bass_onset_strength=0.8F;
  audio.transient_strength=audio.kick_strength=0.7F; audio.nyquist_safe_bin_hi=79;
  for(unsigned i=0;i<80;++i) audio.spectrum[i]=float(i+1)/80.0F;
  k1::core::audio::TempoTrackerEvent tempo{};
  tempo.bpm=120; tempo.phase01=0.25F; tempo.confidence=0.9F; tempo.locked=true;
  VisualWaveformHistory wave{};
  VisualAudioFrameView view{audio,tempo,wave,0};
  constexpr unsigned modes[]{3,7,8,9,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,28,29,32};
  unsigned changed_modes=0;
  for(auto mode:modes) {
    PaletteTransition fade; fade.select(33,0,0); fade.select(43,1000,0);
    ChannelRenderState fading{PixelChannelId::kChannelA}, old{PixelChannelId::kChannelA};
    for(auto* ch:{&fading,&old}) {
      ch->controls().mode_id=mode; ch->controls().palette_mode_enabled=true;
      ch->controls().palette_id=33; ch->controls().photons_id=65535;
    }
    fading.controls().palette_id=43; fading.controls().palette_transition=&fade;
    for(unsigned frame=0;frame<4;++frame) {
      for(auto* ch:{&fading,&old}) {
        ch->prepareAudio(audio); (void)renderProductChannel(*ch,view,1.0F/120.0F);
      }
      // Target ID has changed, but the zero-progress fade must still render
      // exactly as the old palette, including its accent-selection phase.
      assert(!std::memcmp(fading.frame().data(),old.frame().data(),480));
    }
    fade.advance(500000);
    fading.prepareAudio(audio); old.prepareAudio(audio);
    (void)renderProductChannel(fading,view,1.0F/120.0F);
    (void)renderProductChannel(old,view,1.0F/120.0F);
    changed_modes += std::memcmp(fading.frame().data(),old.frame().data(),480)!=0;
  }
  assert(changed_modes>0);
  std::printf("ACTIVE_EFFECT_MORPH_PASS modes_at_start=23 modes_changed_at_midpoint=%u\n",changed_modes);
  std::printf("PALETTE_TRANSITION_PASS exact_endpoints=%u interrupted_pairs=44 contributors=44 bytes_per_channel=%zu host_us_per_160_worst=%.2f\n",endpoints,sizeof(t),double(elapsed)/100.0);
}
