// Host visualisation of the same C++ runtime. Audio features are synthetic.
#include "palette_runtime.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace k1;
using namespace k1::core::visual;
using namespace k1::titan;
int main(int argc,char** argv) {
  if(argc!=2) return 2;
  auto* file=std::fopen(argv[1],"wb"); if(!file) return 3;
  PaletteRuntime preview,effect;
  PaletteConfig c; c.version=2; c.flags=1; c.palette_a=33; c.palette_b=43;
  c.transition_ms=1500; c.brightness=255;
  assert(preview.configure(c,0));
  auto e=c; e.mode_a=14; e.mode_b=3; assert(effect.configure(e,0));
  contract::AudioFeaturesV1 audio;
  audio.validity_flags=contract::kValidBaseSnapshot|contract::kValidSpectrum;
  audio.nyquist_safe_bin_hi=79;
  k1::core::audio::TempoTrackerEvent tempo{};
  tempo.bpm=120; tempo.confidence=0.9F; tempo.locked=true;
  VisualWaveformHistory wave{};
  // 720 native updates; every fifth frame gives a 24 fps, six-second preview.
  for(unsigned frame=0;frame<720;++frame) {
    const auto now=std::uint64_t(frame)*kPalettePeriodUs;
    if(frame==120 || frame==420) {
      c.palette_a=frame==120?43:33; c.palette_b=frame==120?33:43;
      assert(preview.configure(c,now));
      e.palette_a=c.palette_a; e.palette_b=c.palette_b; assert(effect.configure(e,now));
    }
    const float phase=float(frame%60)/60.0F;
    audio.event_flags=(frame%60==0)?contract::kEventBeat|contract::kEventOnset:0;
    audio.peak_scaled=audio.vu_level=0.3F+0.55F*std::exp(-phase*4.0F);
    audio.novelty=audio.onset_strength=audio.bass_onset_strength=audio.peak_scaled;
    audio.beat_phase=tempo.phase01=phase; audio.beat_confidence=0.9F;
    audio.low_energy=0.7F; audio.mid_energy=0.5F; audio.high_energy=0.4F;
    for(unsigned i=0;i<80;++i)
      audio.spectrum[i]=0.15F+0.7F*(0.5F+0.5F*std::sin(float(i)*0.23F+float(frame)*0.035F));
    VisualAudioFrameView view{audio,tempo,wave,0};
    assert(preview.step(now,nullptr)); assert(effect.step(now,&view));
    if(frame%5==0) {
      for(const auto* runtime:{&preview,&effect}) {
        for(unsigned channel=0;channel<2;++channel) {
          const auto pixels=runtime->channel(channel).frame();
          assert(std::fwrite(pixels.data(),3,pixels.size(),file)==pixels.size());
        }
      }
    }
  }
  return std::fclose(file);
}
