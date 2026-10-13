#include "emotiscope_port.h"
#include "emotiscope_secondary.h"
#include "palette_runtime.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
int main(int argc,char** argv){
  assert(argc==5);const unsigned scene=unsigned(std::atoi(argv[1]));const bool background=std::atoi(argv[2]);const bool change_palette=std::atoi(argv[3]);
  assert(k1_emotiscope_init(1));assert(k1_secondary_emotiscope_init(2));
  assert(std::strcmp(k1_emotiscope_mode_name(6),"Bloom")==0);
  K1EmotiscopeConfig cfg{};k1_emotiscope_get_config(&cfg);
  cfg.current_mode=6;cfg.brightness=1;cfg.background=background?1.f:0.f;
  cfg.screensaver=background;cfg.auto_color_cycle=false;cfg.temporal_dithering=false;
  assert(k1_emotiscope_set_config(&cfg));assert(k1_secondary_emotiscope_set_config(&cfg));
  auto authored=k1_composed_scene_defaults();authored.scene=scene==4u?1u:scene;
  authored.outgoing_ms=800;authored.separation_ms=100;authored.quiet_ms=400;
  authored.return_ms=1200;authored.recovery_ms=300;authored.trail_ms=80;authored.quiet_scope=(scene==2||scene==4)?2:1;
  assert(k1_emotiscope_set_scene_config(&authored));if(scene==4u)authored.scene=0;assert(k1_secondary_emotiscope_set_scene_config(&authored));
  k1::titan::PaletteRuntime runtime;k1::titan::PaletteConfig pc;pc.palette_a=pc.palette_b=0;pc.mode_a=pc.mode_b=6;pc.flags=5;pc.brightness=100;
  assert(runtime.configure(pc,0));assert(runtime.setBoardIntensity(0,.25f));assert(runtime.setBoardIntensity(1,.4f));runtime.setSecondaryOutputEnabled(true);
  assert(k1_emotiscope_set_palette(1));assert(k1_secondary_emotiscope_set_palette(1));
  FILE* out=std::fopen(argv[4],"wb");assert(out);
  unsigned quiet[2]{},lit[2]{},moving[2]{},gestures[2]{},switch_frame=0;unsigned valid=0;
  unsigned previous_centroid[2]{};uint32_t noise=123;
  for(unsigned frame=0;frame<1600;++frame){
    const unsigned now=(frame+1)*10000;
    for(unsigned half=0;half<2;++half){float pcm[64];
      for(unsigned i=0;i<64;++i){const unsigned sample=frame*128+half*64+i;const double seconds=double(sample)/12800.;
        const double phase=std::fmod(seconds,.7);const double envelope=seconds<.5?0.:.015+.65*std::exp(-phase/.065);
        noise=noise*1664525u+1013904223u;const double transient=(double((noise>>8)&0xffff)/32768.-1.)*.12;
        pcm[i]=float(envelope*(.5*std::sin(seconds*6.283185307179586*62)+.3*std::sin(seconds*6.283185307179586*249)+transient));}
      assert(k1_emotiscope_push_pcm64(pcm,now-5000+half*5000));}
    if(change_palette && frame==400){assert(k1_emotiscope_set_palette(12));assert(k1_secondary_emotiscope_set_palette(12));switch_frame=frame;}
    assert(runtime.step(now,nullptr));
    K1EmotiscopeAudioPublication publication{};k1_emotiscope_audio_publication(&publication);valid+=publication.valid;
    for(unsigned board=0;board<2;++board){
      K1ComposedSceneStatus state{};if(board)k1_secondary_emotiscope_scene_status(&state);else k1_emotiscope_scene_status(&state);
      gestures[board]=state.gesture_id;
      const auto& pixels=runtime.channel(board).frame();unsigned energy=0,moment=0;
      unsigned char bytes[480];for(unsigned i=0;i<160;++i){bytes[3*i]=pixels[i].red;bytes[3*i+1]=pixels[i].green;bytes[3*i+2]=pixels[i].blue;
        unsigned e=pixels[i].red+pixels[i].green+pixels[i].blue;energy+=e;moment+=e*unsigned(std::abs(int(i)-79));}
      if(energy){++lit[board];unsigned centroid=moment*100/energy;if(previous_centroid[board]&&centroid!=previous_centroid[board])++moving[board];previous_centroid[board]=centroid;}
      if(state.quiet){++quiet[board];assert(energy==0);}
      std::fwrite(bytes,1,480,out);
      // Both complete-board gains remain exactly the requested attenuation.
      assert(runtime.config().brightness==100);assert(std::fabs(runtime.boardIntensity(0)-.25f)<.00002f);assert(std::fabs(runtime.boardIntensity(1)-.4f)<.00002f);
      unsigned char wire[480];for(unsigned lane=0;lane<2;++lane){assert(runtime.packBoardGrb48Lane(wire,sizeof(wire),board,lane)==480);
        for(unsigned i=0;i<80;++i){const auto& pixel=pixels[(board?1-lane:lane)*80+i];unsigned vals[3]={pixel.green,pixel.red,pixel.blue};
          for(unsigned c=0;c<3;++c){unsigned packed=unsigned(wire[6*i+2*c])<<8|wire[6*i+2*c+1];
            const double upper=double(vals[c])*257.*100./255.*(board?.4:.25)+2.;assert(double(packed)<=upper);}}}
      K1EmotiscopeConfig read{};if(board)k1_secondary_emotiscope_get_config(&read);else k1_emotiscope_get_config(&read);assert(read.current_mode==6);
    }
  }
  std::fclose(out);assert(valid>1500);assert(lit[0]>50&&lit[1]>50);assert(moving[0]>20&&moving[1]>20);
  if(scene==1||scene==2){assert(quiet[0]>10&&quiet[1]>10);assert(gestures[0]>0&&gestures[1]>0);}
  if(scene==4u)assert(quiet[0]>10&&quiet[1]>10);
  std::printf("{\"scene\":%u,\"background\":%u,\"switch_frame\":%u,\"valid_frames\":%u,\"lit\":[%u,%u],\"moving\":[%u,%u],\"quiet\":[%u,%u],\"gestures\":[%u,%u]}\n",scene,unsigned(background),switch_frame,valid,lit[0],lit[1],moving[0],moving[1],quiet[0],quiet[1],gestures[0],gestures[1]);
}
