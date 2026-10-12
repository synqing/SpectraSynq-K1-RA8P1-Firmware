// Actual shared PCM -> donor publication + unchanged 96-sample source analysis
// -> PaletteRuntime -> final GRB48. Titan admission intentionally differs from
// the source microphone's .08 RMS wake boundary; no source oracle is changed.
#include "emotiscope_port.h"
#include "emotiscope_secondary.h"
#include "inhouse_audio.h"
#include "palette_runtime.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc,char** argv){
  assert(argc==4);const bool changed=std::atoi(argv[1]);
  FILE* trace=std::fopen(argv[2],"w");FILE* pixels=std::fopen(argv[3],"wb");assert(trace&&pixels);
  assert(k1_emotiscope_init(1)&&k1_secondary_emotiscope_init(2));
  K1EmotiscopeConfig cfg{};k1_emotiscope_get_config(&cfg);cfg.current_mode=6;
  cfg.background=cfg.screensaver=cfg.auto_color_cycle=cfg.temporal_dithering=0;cfg.brightness=1;
  assert(k1_emotiscope_set_config(&cfg)&&k1_secondary_emotiscope_set_config(&cfg));
  k1::titan::PaletteRuntime runtime;k1::titan::PaletteConfig pc{};
  pc.palette_a=pc.palette_b=0;pc.mode_a=pc.mode_b=6;pc.flags=5;pc.brightness=255;
  assert(runtime.configure(pc,0));runtime.setSecondaryOutputEnabled(true);
  auto mode=[&](unsigned m){cfg.current_mode=m;assert(k1_emotiscope_set_config(&cfg));runtime.noteDonorMode(m,0);};
  mode(32);assert(k1_secondary_emotiscope_set_config(&cfg));runtime.noteDonorMode(32,1);
  k1::titan::InhouseAudio producer;assert(producer.configure({}));
  k1::titan::InhouseAudioCursor cursors[2]{};std::uint64_t seq=0;
  std::fputs("frame,source_silence,admission0,admission1,raw_rms,donor_peak_squared,donor_sequence,injection0,injection1\n",trace);
  for(unsigned frame=0;frame<1530;++frame){
    const std::uint64_t now=std::uint64_t(frame+1)*10000;
    // .079 RMS sinusoid, then .063 peak/.0447 RMS sustain in the existing
    // hysteresis band, then room. .02 RMS room is safely below measured gate.
    float amplitude=std::sqrt(2.0f)*.02f;
    if(frame>=650&&frame<750)amplitude=std::sqrt(2.0f)*.079f;
    if(frame>=750&&frame<850)amplitude=std::sqrt(.004f);
    if(frame>=1450)amplitude=std::sqrt(.004f);
    if(frame==1450||frame==1454||frame==1457||frame==1460||frame==1463||frame==1466||frame==1469||frame==1515)
      amplitude=std::sqrt(2.0f)*.079f;
    if(frame==1481)k1_emotiscope_audio_gap();
    for(unsigned half=0;half<2;++half){
      k1_audio_hop_t h{};h.contract_version=2;h.samples=64;h.analysis_rate_hz=12800;h.stream_epoch=1;
      h.hop_sequence=++seq;h.analysis_begin=(seq-1)*64;h.analysis_end_exclusive=seq*64;
      float pcm[64];for(unsigned i=0;i<64;++i){
        h.pcm[i]=static_cast<std::int16_t>(std::lround(amplitude*32768.0f*std::sin(double(h.analysis_begin+i)*6.283185307179586/64.0)));
        pcm[i]=float(h.pcm[i])/32768.0f;
      }
      const auto capture=now-5000+half*5000;
      if(frame<1470||frame>=1481)assert(k1_emotiscope_push_pcm64(pcm,static_cast<std::uint32_t>(capture)));
      (void)producer.consume(h,capture,true);producer.stampPublication(capture);
    }
    for(unsigned board=0;board<2;++board){
      k1::core::visual::InhouseWaveformAudio input{};
      (void)producer.acquire(cursors[board],now,30000,input,board);
      if(changed&&board==0){
        if(frame==1452)input.valid=false;
        if(frame==1455){input.capture_us=now-30001;input.publish_us=now;}
        if(frame>=1461)input.epoch+=1;
        if(frame==1464)input.generation=1; // regressive within the same epoch
        if(frame==1467)input.analysis_end_sample_exclusive=input.analysis_begin_sample;
      }
      runtime.setInhouseAudio(board,input);
    }
    if(changed&&frame==1458)mode(6);
    if(changed&&frame==1459)mode(32);
    assert(runtime.step(now,nullptr));
    if(frame>=600&&frame<650)assert(producer.latest().silence&&!runtime.inhouseAdmissionOpen(0)&&!runtime.inhouseAdmissionOpen(1));
    if(frame>=650&&frame<850){
      assert(producer.latest().silence); // unchanged SPH classifier cannot wake
      assert(runtime.inhouseAdmissionOpen(0)&&runtime.inhouseAdmissionOpen(1));
    }
    if(frame>=850&&frame<1450)assert(!runtime.inhouseAdmissionOpen(0)&&!runtime.inhouseAdmissionOpen(1));
    if(changed&&(frame==1452||frame==1453||frame==1455||frame==1456||frame==1459||frame==1461||frame==1462||frame==1464||frame==1465||frame==1467||frame==1468))
      assert(!runtime.inhouseAdmissionOpen(0)&&runtime.inhouseAdmissionOpen(1));
    if(frame>=1473&&frame<1515){
      assert(producer.latest().valid); // Donor stale/rewarm closes even with fresh Inhouse.
      assert(!runtime.inhouseAdmissionOpen(0)&&!runtime.inhouseAdmissionOpen(1));
    }
    if(frame==1515)assert(runtime.inhouseAdmissionOpen(0)&&runtime.inhouseAdmissionOpen(1));
    char status[8192];assert(runtime.statusJson(status,sizeof(status)));
    auto* section=std::strstr(status,"\"inhouse_frames\"");assert(section);
    unsigned long long injections[2]{};
    for(unsigned board=0;board<2;++board){
      section=std::strstr(section,"\"injection_count\":");assert(section);
      assert(std::sscanf(section,"\"injection_count\":%llu",&injections[board])==1);++section;
      unsigned char bytes[960];for(unsigned lane=0;lane<2;++lane)assert(runtime.packBoardGrb48Lane(bytes+480*lane,480,board,lane)==480);
      std::fwrite(bytes,1,sizeof(bytes),pixels);
      if(frame>=1200&&frame<1450)for(auto b:bytes)assert(b==0);
    }
    std::fprintf(trace,"%u,%u,%u,%u,%.9g,%.9g,%u,%llu,%llu\n",frame,unsigned(runtime.inhouseSourceSilence(0)),unsigned(runtime.inhouseAdmissionOpen(0)),unsigned(runtime.inhouseAdmissionOpen(1)),double(producer.rawRms()),double(runtime.inhouseDonorPeakSquared()),runtime.inhouseDonorSequence(),injections[0],injections[1]);
    if(frame==700)std::puts(status);
  }
  std::fclose(trace);std::fclose(pixels);
}
