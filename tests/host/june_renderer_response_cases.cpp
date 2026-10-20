#include "core/visual/inhouse_waveform.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
using namespace k1::core::visual;
using Frame = std::array<Pixel16,160>;
static InhouseWaveformRenderer renderer;
static InhouseWaveformAudio input(unsigned frame, float peak, bool reverse=false) {
  InhouseWaveformAudio a{};
  a.valid=true; a.silence=false; a.epoch=1; a.generation=frame;
  a.capture_us=a.publish_us=frame*10000ULL;
  a.analysis_begin_sample=(frame-1)*180ULL; a.analysis_end_sample_exclusive=frame*180ULL;
  a.peak_scaled=peak; a.vu_level=a.vu_average=.2F;
  a.unguarded_vu_valid=true; a.unguarded_vu=2.0F;
  a.raw_peak_pcm=1000; a.raw_rms=.2F; a.pcm_history_valid=true;
  a.linear_shape[20]=.4F; a.linear_shape[24]=.2F;
  for(auto& row:a.pcm_history) for(unsigned i=0;i<row.size();++i)
    row[i]=((i<48)!=reverse)?1000:250;
  return a;
}
static double energy(const Frame& p) {
  double sum=0; for(const auto& x:p)sum+=x.red+x.green+x.blue; return sum;
}
static Frame run(float peak,bool reverse=false,bool history=true) {
  renderer.reset(); Frame p{}; InhouseWaveformControls c{}; c.mode=32;c.palette_id=2;
  for(unsigned i=1;i<=60;++i) {
    auto a=input(i,peak,reverse);a.pcm_history_valid=history;
    assert(renderer.render(a,c,i*10000ULL,p.data(),p.size()));
    assert(renderer.observation().position==80.0F);
    for(unsigned j=0;j<80;++j) {
      assert(p[j].red==p[159-j].red && p[j].green==p[159-j].green && p[j].blue==p[159-j].blue);
    }
  }
  return p;
}
static float centre_amplitude(float peak, bool chromatic, float absolute_level=2.0F) {
  renderer.reset(); Frame p{}; InhouseWaveformControls c{};
  c.mode=32;c.palette_id=2;c.motion_scale=0;c.chroma_range=12;
  for(unsigned i=1;i<=600;++i) {
    auto a=input(i,peak);a.unguarded_vu=absolute_level;
    for(auto& row:a.pcm_history)row.fill(1000); // Unit PCM shape at every position.
    if(chromatic)for(unsigned n=0;n<7;++n)a.spectrum[n]=1.0F;
    assert(renderer.render(a,c,i*10000ULL,p.data(),p.size()));
  }
  float mean=0;for(float x:renderer.observation().chroma)mean+=x/12.0F;
  assert(chromatic ? mean>.5F : mean==0.0F); // June blend is entirely main/fallback.
  // Motion is zero, shape is one and radius is constant from the first frame.
  // Sum of June's radius+1 width samples is 1 + .625*radius. Read the actual
  // intensity field before the independently chosen palette's dark stops.
  const unsigned radius=3U+static_cast<unsigned>(peak*7.0F);
  return renderer.observation().intensity_mean*80.0F/(1.0F+.625F*float(radius));
}
int main(int argc,char** argv) {
  const char* test=argc>1?argv[1]:"all";
  if(!std::strcmp(test,"peak") || !std::strcmp(test,"all")) {
    const auto low=run(.2F),high=run(.8F);
    std::printf("peak_only low=%.0f high=%.0f\n",energy(low),energy(high));std::fflush(stdout);
    assert(energy(high)>energy(low)*1.5);
  }
  if(!std::strcmp(test,"shape") || !std::strcmp(test,"all")) {
    const auto a=run(.8F,false),b=run(.8F,true);
    unsigned changes=0;for(unsigned i=0;i<160;++i)
      changes+=a[i].red!=b[i].red || a[i].green!=b[i].green || a[i].blue!=b[i].blue;
    std::printf("equal_rms_peak_pcm_permutation changed_pixels=%u\n",changes);std::fflush(stdout);
    assert(changes>10);
    assert(energy(run(.8F,false,false))>0); // Missing history has a bounded seed.
  }
  if(!std::strcmp(test,"transport") || !std::strcmp(test,"all")) {
    for(unsigned step:{10000U,20000U}) {
      renderer.reset();Frame p{};InhouseWaveformControls c{};c.mode=32;c.palette_id=2;
      auto a=input(1,.8F);assert(renderer.render(a,c,10000,p.data(),p.size()));
      unsigned moves=0;
      for(unsigned t=step;t<=1000000;t+=step) {
        a=input(t/step+1,.8F);a.capture_us=a.publish_us=10000+t;
        assert(renderer.render(a,c,10000+t,p.data(),p.size()));moves+=renderer.observation().scroll_steps;
      }
      std::printf("transport dt_us=%u moves=%u\n",step,moves);std::fflush(stdout);
      assert(moves>=119 && moves<=121);
    }
  }
  if(!std::strcmp(test,"quiet") || !std::strcmp(test,"all")) {
    auto p=run(.8F);InhouseWaveformControls c{};c.mode=32;c.palette_id=2;
    for(unsigned i=61;i<=500;++i) {
      auto a=input(i,0);a.silence=true;
      assert(renderer.render(a,c,i*10000ULL,p.data(),p.size()));
    }
    assert(energy(p)==0);
  }
  if(!std::strcmp(test,"replace") || !std::strcmp(test,"all")) {
    renderer.reset();Frame p{};InhouseWaveformControls c{};c.mode=32;c.palette_id=2;c.motion_scale=0;
    for(unsigned i=1;i<=30;++i) {
      auto a=input(i,.8F);assert(renderer.render(a,c,i*10000ULL,p.data(),p.size()));
    }
    assert(p[80].red+p[80].green+p[80].blue>0);
    auto a=input(31,.8F); for(auto& row:a.pcm_history)row.fill(0);
    assert(renderer.render(a,c,310000,p.data(),p.size()));
    assert(p[80].red==0 && p[80].green==0 && p[80].blue==0);
    a=input(32,.8F);a.raw_peak_pcm=std::numeric_limits<float>::quiet_NaN();
    assert(renderer.render(a,c,320000,p.data(),p.size()));
    assert(energy(p)>0); // Invalid normaliser selects the safe missing-history fallback.
    a=input(33,.8F);a.raw_peak_pcm=32768;
    for(auto& row:a.pcm_history)row.fill(std::numeric_limits<std::int16_t>::min());
    assert(renderer.render(a,c,330000,p.data(),p.size())); // abs(-32768) stays defined.
  }
  if(!std::strcmp(test,"amplitude") || !std::strcmp(test,"all")) {
    const float main_low=centre_amplitude(.2F,true),main_high=centre_amplitude(.8F,true);
    const float fallback_low=centre_amplitude(.2F,false),fallback_high=centre_amplitude(.8F,false);
    std::printf("june_amplitude main_low=%.6f main_high=%.6f fallback_low=%.6f fallback_high=%.6f\n",
        main_low,main_high,fallback_low,fallback_high);std::fflush(stdout);
    // June Hybrid lines 138-194: main colour gets seed_gain only. A second
    // seed multiplier would make this ratio four times the source's 0.86/0.44.
    assert(std::fabs(main_high/main_low-1.95454545F)<.005F);
    // Seven strong notes leave mean > .5, selecting the main branch. June's
    // palette helper energy is about .72, so .44*energy is about .316.
    assert(main_low>.30F && main_low<.34F);
    // Zero chroma selects June's seed-valued fallback (globals.h:474 = 1).
    assert(std::fabs(fallback_low-.088F)<.001F);
    assert(std::fabs(fallback_high-.688F)<.001F);
    // Absolute level admits the source; it is not another amplitude envelope.
    assert(std::fabs(centre_amplitude(.2F,true,.1F)-main_low)<.001F);
    assert(std::fabs(centre_amplitude(.8F,false,.1F)-fallback_high)<.001F);
  }
  if(!std::strcmp(test,"tempo")) {
    renderer.reset();Frame p{};InhouseWaveformControls c{};c.mode=18;c.palette_id=2;
    std::uint64_t hash=1469598103934665603ULL;
    for(unsigned i=1;i<=200;++i) {
      auto a=input(i,.15F+float(i%17)/20.0F);a.unguarded_vu=.02F+float(i%31)/8.0F;
      a.tempo_valid=true;a.tempo_capture_us=a.capture_us;a.tempo_bpm=120;
      a.tempo_phase01=float(i%50)/50.0F;a.tempo_confidence=.8F;a.novelty=float(i%11)/30.0F;
      assert(renderer.render(a,c,i*10000ULL,p.data(),p.size()));
      for(const auto& x:p)for(auto v:{x.red,x.green,x.blue})hash=(hash^v)*1099511628211ULL;
    }
    std::printf("tempo_rgb_hash=%llu\n",static_cast<unsigned long long>(hash));
  }
}
