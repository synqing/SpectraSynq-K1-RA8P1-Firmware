#include "emotiscope_port.h"
#include "emotiscope_secondary.h"
#include "palette_runtime.h"
#include "composed_scene_runtime.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static composed_motion::Rgb palette(float u,void*){return {u,1-u,.2f};}
static void body_level_response_checks(){
  float hz[64];for(unsigned i=0;i<64;++i)hz[i]=100.f+i;
  K1MusicalConditioning body;assert(body.initialise_frequencies(hz));
  assert(body.configure({1,1,40,160}));
  K1EmotiscopeAudioPublication p{},out{};p.valid=1;p.vu_level=.8f;
  p.novelty_normalised=1;for(float &x:p.spectrum)x=1;
  // A constant body target reaches 1-exp(-1) after the selected 40 ms attack.
  // This independent step-response oracle rejects two cascaded 40 ms filters.
  for(unsigned i=1;i<=4;++i){p.sequence=p.audio_hops=i;p.now_us=i*10000;
    assert(body.apply(p,out,.01f,p.now_us));}
  const float expected=.8f*(1.f-std::exp(-1.f));
  std::printf("body 40ms step: observed=%f expected=%f\n",out.vu_level,expected);
  std::fflush(stdout);assert(std::fabs(out.vu_level-expected)<1e-6f);
  body.reset();p.sequence=p.audio_hops=5;p.now_us=50000;
  assert(body.apply(p,out,.01f,p.now_us));const float hit=out.vu_level;
  p.novelty_normalised=0;p.sequence=p.audio_hops=6;p.now_us=60000;
  assert(body.apply(p,out,.01f,p.now_us));
  // Once the transient target ends, release starts immediately: no delayed rise.
  assert(std::fabs(out.vu_level-hit*std::exp(-.01f/.16f))<1e-6f);
  const float released=out.vu_level;
  assert(body.apply(p,out,.01f,p.now_us));assert(out.vu_level==released);
}
static void boundary_checks(){
  K1EmotiscopeAudioPublication p{};p.valid=1;p.sequence=1;p.audio_hops=1;
  p.now_us=p.audio_time_us=10000;p.amplitude_valid=1;p.pcm_peak_squared=.001f;p.vu_floor=.0005f;p.vu_level_raw=.0001f;
  assert(composed_scene::audible(p,10000)); // squared-domain comparison
  p.pcm_peak_squared=.0004f;p.vu_level_raw=1;assert(!composed_scene::audible(p,10000));
  p.pcm_peak_squared=.001f;assert(!composed_scene::audible(p,50001));
  float hz[64];for(unsigned i=0;i<64;++i)hz[i]=300.f+30.f*i;
  K1MusicalConditioning a,b;assert(a.initialise_frequencies(hz)&&b.initialise_frequencies(hz));
  assert(a.configure({2,1,40,160})&&b.configure({2,1,40,160}));
  for(unsigned i=0;i<64;++i)p.spectrum[i]=p.spectrum_smooth[i]=p.frequency_magnitude[i]=.5f;
  p.vu_level=.7f;K1EmotiscopeAudioPublication out{};
  assert(a.apply(p,out,.01f,10000)&&b.apply(p,out,.01f,10000));
  p.sequence=3;p.audio_hops=3;p.now_us=p.audio_time_us=30000;
  assert(a.apply(p,out,.001f,30000)&&b.apply(p,out,.02f,30000));
  assert(std::fabs(a.value()-b.value())<1e-7f);
  const auto prior=a.value();const auto cursor=a.event_cursor();
  p.sequence=4;p.now_us=40000;assert(a.apply(p,out,.01f,40000));
  assert(a.value()==prior&&a.event_cursor()==cursor); // render != audio generation
  composed_motion::Renderer r;composed_motion::Config cfg;cfg.out_travel_ms=800;cfg.return_travel_ms=1000;
  assert(r.configure(cfg));composed_motion::Rgb pixels[128];
  r.renderBloom(0,0,palette,nullptr,pixels);r.renderBloom(10,1,palette,nullptr,pixels);
  r.renderBloom(20,0,palette,nullptr,pixels);bool seen=false;
  for(unsigned t=30;t<600;t+=10){r.renderBloom(t,0,palette,nullptr,pixels);for(auto c:pixels)seen|=c.r+c.g+c.b>0;}
  assert(seen); // pulse entirely between two old 25 ms injection ticks survives
  for(unsigned t=600;t<3500;t+=10)r.renderBloom(t,0,palette,nullptr,pixels);
  r.renderBloom(3510,1,palette,nullptr,pixels);r.renderBloom(3520,0,palette,nullptr,pixels);
  seen=false;for(unsigned t=3530;t<4000;t+=10){r.renderBloom(t,0,palette,nullptr,pixels);for(auto c:pixels)seen|=c.r+c.g+c.b>0;}assert(seen);
}
int main(int argc,char** argv){
  assert(argc==3);body_level_response_checks();boundary_checks();const int variant=std::atoi(argv[1]);
  assert(k1_emotiscope_init(1)&&k1_secondary_emotiscope_init(2));
  K1EmotiscopeConfig c{};k1_emotiscope_get_config(&c);c.current_mode=6;c.brightness=1;c.background=0;c.screensaver=false;c.temporal_dithering=false;c.auto_color_cycle=false;
  assert(k1_emotiscope_set_config(&c)&&k1_secondary_emotiscope_set_config(&c));
  assert(k1_emotiscope_set_palette(1)&&k1_secondary_emotiscope_set_palette(1));
  K1EmotiscopeMusicalConfig body{1,1,40,160},detail{2,1,40,160};
  assert(k1_emotiscope_set_musical_config(&body)&&k1_secondary_emotiscope_set_musical_config(&detail));
  K1EmotiscopeMlConfig ml{0,0};assert(k1_emotiscope_set_ml_config(&ml)&&k1_secondary_emotiscope_set_ml_config(&ml));
  auto scene=k1_composed_scene_defaults();scene.scene=3;scene.phrase=0;scene.choreography=0;scene.quiet_scope=1;scene.outgoing_ms=900;scene.return_ms=1300;
  assert(k1_emotiscope_set_scene_config(&scene)&&k1_secondary_emotiscope_set_scene_config(&scene));
  k1::titan::PaletteRuntime runtime;k1::titan::PaletteConfig pc;pc.palette_a=pc.palette_b=0;pc.mode_a=pc.mode_b=6;pc.flags=5;pc.brightness=100;
  assert(runtime.configure(pc,0));assert(runtime.setBoardIntensity(0,.25f)&&runtime.setBoardIntensity(1,.25f));runtime.setSecondaryOutputEnabled(true);
  FILE* f=std::fopen(argv[2],"wb");assert(f);
  for(unsigned frame=0;frame<1200;++frame){const unsigned now=(frame+1)*10000;
    if(variant!=2||frame<500)for(unsigned half=0;half<2;++half){float pcm[64];
      for(unsigned i=0;i<64;++i){double t=double(frame*128+half*64+i)/12800.;double pulse=std::fmod(t,.8)<.09?.75:0;
        pcm[i]=t>.5&&t<5?float(pulse*std::sin(6.283185307*t*82)+.08*std::sin(6.283185307*t*2200)):0;}
      assert(k1_emotiscope_push_pcm64(pcm,now-5000+half*5000));}
    if(variant==1&&frame==300)assert(k1_emotiscope_set_edge_response(2,1));
    assert(runtime.step(now,nullptr));
    K1EmotiscopeAudioPublication p{};k1_emotiscope_audio_publication(&p);
    if(frame<500){assert(p.audio_time_us==now);assert(p.amplitude_valid);}
    for(unsigned board=0;board<2;++board){unsigned char bytes[960];
      for(unsigned lane=0;lane<2;++lane)assert(runtime.packBoardGrb48Lane(bytes+480*lane,480,board,lane)==480);
      std::fwrite(bytes,1,960,f);
      if(frame>1000)for(auto x:bytes)assert(x==0); // both silence and capture stall drain
    }
  }
  std::fclose(f);puts("continuous Bloom boundary and production-path checks passed");
}
