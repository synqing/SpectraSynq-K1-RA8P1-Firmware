#include "emotiscope_port.h"
#include "emotiscope_secondary.h"
#include "emotiscope_control.h"
#include "palette_runtime.h"
#include "k1_live_protocol.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
int main(int argc,char** argv){
  assert(argc==3 || argc==5 || argc==6);const unsigned selected=std::atoi(argv[1]);const bool change=std::atoi(argv[2]);
  const bool peak_overload=argc==6 && std::strcmp(argv[5],"--peak-overload")==0;
  const unsigned modes[2]={argc>=5?unsigned(std::atoi(argv[3])):(selected==0?32u:6u),argc>=5?unsigned(std::atoi(argv[4])):(selected==1?32u:6u)};
  assert(selected<2);assert(k1_emotiscope_init(1));assert(k1_secondary_emotiscope_init(2));
  K1EmotiscopeConfig cfg{};k1_emotiscope_get_config(&cfg);cfg.current_mode=6;
  cfg.background=0;cfg.screensaver=0;cfg.auto_color_cycle=0;cfg.temporal_dithering=0;cfg.brightness=1;
  assert(k1_emotiscope_set_config(&cfg));assert(k1_secondary_emotiscope_set_config(&cfg));
  k1::titan::PaletteRuntime runtime;k1::titan::PaletteConfig pc;pc.palette_a=pc.palette_b=0;pc.mode_a=pc.mode_b=6;pc.brightness=255;pc.flags=5;
  assert(runtime.configure(pc,0));assert(runtime.setBoardIntensity(0,.25f));assert(runtime.setBoardIntensity(1,.25f));runtime.setSecondaryOutputEnabled(true);
  auto set=[&](K1EmotiscopeConfig c){return selected?k1_secondary_emotiscope_set_config(&c):k1_emotiscope_set_config(&c);};
  for(int unsupported:{9,10,12,31,33}){auto c=cfg;c.current_mode=unsupported;assert(!set(c));}
  for(unsigned board=0;board<2;++board){auto c=cfg;c.current_mode=modes[board];assert(board?k1_secondary_emotiscope_set_config(&c):k1_emotiscope_set_config(&c));runtime.noteDonorMode(modes[board],board);}
  cfg.current_mode=modes[selected];
  using namespace k1::emotiscope;
  assert(donorResponseBytes(kDonorCfgGetExtended)==424);assert(donorResponseBytes(kDonorCfgGetInhouse)==428);
  DonorDedup dedup{};DonorObserved observed{};observed.boot_id=42;observed.board_capabilities=511;observed.board=selected;observed.inhouse_mood=.05f;
  DonorIntent intent{};intent.sub=(cfg.current_mode==32?kDonorCfgPatchInhouse:kDonorCfgPatchWaveforms);intent.boot_id=42;intent.request_id=7;intent.board=selected;intent.mask=std::uint64_t{1}<<63u;intent.inhouse_mood=.25f;
  DonorApplyResult applied{};
  if(k1_emotiscope_is_inhouse_mode(cfg.current_mode)){
  donorApply(&dedup,intent,observed,&applied);assert(applied.status==0&&applied.inhouse_mood_changed&&applied.inhouse_mood==.25f);assert(applied.affected_boards==(1u<<selected));assert(donorGetU32(applied.response+4)==(cfg.current_mode==32?7u:8u));assert(donorGetF32(applied.response+424)==.25f);
  DonorDedup legacy{};intent.sub=kDonorCfgPatchExtended;intent.request_id=8;donorApply(&legacy,intent,observed,&applied);assert(applied.status==kDonorStatusInvalid&&!applied.changed);
  intent.sub=kDonorCfgPatchInhouse;intent.board=2;intent.request_id=9;DonorDedup other{};donorApply(&other,intent,observed,&applied);assert(applied.status==kDonorStatusInvalid&&!applied.changed);
  }
  auto scene=k1_composed_scene_defaults();scene.scene=3;scene.quiet_scope=2;scene.choreography=1;
  assert(k1_emotiscope_set_scene_config(&scene));assert(k1_secondary_emotiscope_set_scene_config(&scene));
  std::uint8_t dummy[384];if(k1_emotiscope_is_inhouse_mode(cfg.current_mode))assert(!(selected?k1_secondary_emotiscope_render(1,dummy):k1_emotiscope_render(1,dummy)));
  std::uint32_t hashes[2]={2166136261u,2166136261u};unsigned lit[2]={},lowbits=0;
  char overload_status[8192]{},recovery_status[8192]{};
  for(unsigned frame=0;frame<(peak_overload?160u:600u);++frame){
    const auto now=std::uint64_t(frame+1)*10000;
    float pcm[64];for(unsigned half=0;half<2;++half){for(unsigned i=0;i<64;++i)pcm[i]=.35f*std::sin(float((frame*128+half*64+i)*6.283185307179586*220/12800));assert(k1_emotiscope_push_pcm64(pcm,std::uint32_t(now-5000+half*5000)));}
    k1::core::visual::InhouseWaveformAudio a{};a.valid=true;a.analysis_begin_sample=frame*128;a.analysis_end_sample_exclusive=frame*128+96;a.silence=false;a.epoch=1;a.generation=frame+1;a.capture_us=now-5000;a.publish_us=now;a.peak_scaled=.3f+.2f*std::sin(frame*.07f);a.vu_level=.3f;a.raw_peak_pcm=12000;
    for(unsigned i=0;i<80;++i)a.spectrum[i]=.08f+.3f*float((i+frame/30)%12==0);
    a.vu_average=a.vu_level;a.spectral_energy=.4f;a.novelty=.1f;
    a.pcm_history_valid=true;a.tempo_valid=true;a.tempo_bpm=128.f;
    a.tempo_phase01=std::fmod(float(frame)*.01f*128.f/60.f,1.f);a.tempo_confidence=.8f;a.tempo_capture_us=a.capture_us;
    for(unsigned h=0;h<4;++h)for(unsigned i=0;i<96;++i)a.pcm_history[h][i]=std::int16_t(12000.f*std::sin((frame*128+h*96+i)*.08f));
    for(unsigned board=0;board<2;++board){
      auto input=a;
      // Paired production runs differ only in this edge's original input peak.
      // `change` supplies the deliberately clipped control trace, not a change
      // to the donor audio or peer publication.
      if(peak_overload&&board==selected)input.peak_scaled=frame<60?(change?1.f:2.f):.2f;
      runtime.setInhouseAudio(board,input);
    }
    if(!peak_overload&&change&&frame==200){cfg.brightness=.3f;assert(set(cfg));}
    assert(runtime.frameDue(now));assert(runtime.step(now,nullptr));
    assert(!runtime.frameDue(now+1));assert(!runtime.step(now+1,nullptr));
    for(unsigned board=0;board<2;++board){std::uint8_t payload[480];for(unsigned lane=0;lane<2;++lane){assert(runtime.packBoardGrb48Lane(payload,sizeof(payload),board,lane)==480);for(auto byte:payload){hashes[board]=(hashes[board]^byte)*16777619u;lit[board]+=byte!=0;}if(board==selected)for(unsigned i=0;i<240;++i)lowbits+=payload[i*2]!=payload[i*2+1];}}
    K1ComposedSceneStatus st{};if(selected)k1_secondary_emotiscope_scene_status(&st);else k1_emotiscope_scene_status(&st);if(k1_emotiscope_is_inhouse_mode(cfg.current_mode))assert(st.quiet==0);
    if(peak_overload&&frame==59)assert(runtime.statusJson(overload_status,sizeof(overload_status)));
    if(peak_overload&&frame==70)assert(runtime.statusJson(recovery_status,sizeof(recovery_status)));
  }
  if(argc==3)assert(lit[0]>100&&lit[1]>100&&lowbits>100);char catalogue[2048];assert(runtime.catalogueJson(catalogue,sizeof(catalogue)));assert(std::strstr(catalogue,"32"));
  char status[8192];assert(runtime.statusJson(status,sizeof(status)));assert(std::strstr(status,"inhouse_frames"));assert(std::strstr(status,"injection_count"));assert(k1::titan::livePathConsumesAcceptedRevision(runtime.visualPath()));
  k1::titan::LiveRenderReceipt receipt{};k1::titan::liveNotePatchAccepted(&receipt,23,7);
  if(k1::titan::livePathConsumesAcceptedRevision(runtime.visualPath()))k1::titan::liveNoteRendererConsumed(&receipt,600,600,runtime.lastBoardFrameUs(selected));
  assert(receipt.rendered_request_id==23&&receipt.rendered_revision==7);
  assert(!k1::titan::liveNotePairCompleted(&receipt,22,23,23));assert(receipt.emitted_request_id==0);
  assert(k1::titan::liveNotePairCompleted(&receipt,23,23,23));assert(receipt.emitted_request_id==23);
  std::printf("{\"selected\":%u,\"change\":%u,\"crc\":[%u,%u],\"lit_bytes\":[%u,%u],\"wide_precision_samples\":%u",selected,unsigned(change),hashes[0],hashes[1],lit[0],lit[1],lowbits);
  if(peak_overload)std::printf(",\"overload_status\":%s,\"recovery_status\":%s",overload_status,recovery_status);
  std::puts("}");
}
