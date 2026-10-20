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
int main(int argc,char** argv) {
  assert(argc==4);const uint8_t uid[16]{};k1_fixture_initialise(uid,1000000000U,0);
  assert(!palettes.emitEnabled() && live_cfg.emit_on==0);
  assert(palettes.config().brightness==24 && palettes.config().mode_a==8);
  // A completed generation is queued for promotion. Any hidden poll is hostile.
  queued_generation=true;
  for(unsigned advancing: {0U,10000000U}) {
    cycle_increment=advancing;
    for(unsigned op: {1U,6U,15U,17U,21U,23U,24U,26U}) {
      live_runtime.advanceHostCycles(10000);
      std::vector<unsigned char> runtime_before(sizeof(live_runtime));
      std::memcpy(runtime_before.data(), &live_runtime, sizeof(live_runtime));
      auto before=pair_polls;send(op);retire_reply();
      assert(std::memcmp(runtime_before.data(), &live_runtime, sizeof(live_runtime))==0);
      assert(pair_polls==before && pair_starts==0 && queued_generation);
    }
    for(unsigned sub: {1U,2U}) {
      std::vector<uint8_t> p(4);put32(p.data(),sub);send(25,p);retire_reply();
      assert(pair_polls==0 && pair_starts==0 && queued_generation);
    }
    // Frame/wire reads and cursor-based history requests are observers too.
    for(unsigned channel: {0U,1U}) {
      std::vector<uint8_t> body(4);put32(body.data(),channel);
      send(18,body);retire_reply();
      assert(pair_polls==0 && pair_starts==0 && queued_generation);
    }
    palette_wire_sequence=0;
    send(19);retire_reply(8); // No published wire frame yet: read fails, never starts one.
    palette_wire_sequence=1;
    send(19);retire_reply();
    for(unsigned op: {24U,26U}) {
      send(op,std::vector<uint8_t>(8,0));retire_reply();
      assert(pair_polls==0 && pair_starts==0 && queued_generation);
    }
  }
  // Intrusive live diagnostics reject before a busy()/poll()/submit route.
  for(unsigned op: {9U,10U,11U,22U,27U}) {send(op);retire_reply(10);assert(pair_polls==0);}
  // Python's actual packer supplies the scalar wire request.
  FILE* f=std::fopen(argv[1],"rb");assert(f);std::vector<uint8_t> patch(32);
  const auto count=std::fread(patch.data(),1,patch.size(),f);std::fclose(f);patch.resize(count);
  const auto initial=palettes.channel(0).controls().vp_waveform_shift_rate;
  send(25,patch);assert(k1_fixture_control_pending());assert(pair_polls==0);
  assert(palettes.channel(0).controls().vp_waveform_shift_rate==initial);
  const auto immutable=pending_control.config;
  send(16,std::vector<uint8_t>(32));assert(pending_control.config.revision==immutable.revision);
  inject_reentry=true;fake_cycles=10000000;cycle_increment=1;
  pair_active=true;k1_fixture_poll(9);
  assert(pending_control.phase==ControlPhase::Queued && live_cfg_revision==0);
  pair_active=false;queued_generation=true;pair_starts=0;inject_reentry=true;
  k1_fixture_poll(10);
  assert(pair_starts==1 && !queued_generation && !inject_reentry);
  assert(live_cfg_revision==1 && palettes.channel(0).controls().vp_waveform_shift_rate==200.0F);
  assert(pending_control.phase==ControlPhase::Applied);
  size_t n=0;k1_fixture_reply(&n);assert(n==0); // No response construction in RT.
  pair_active=false;k1_fixture_control_service();retire_reply();
  assert(!k1_fixture_control_pending());
  assert(owner_reentry_rejected >= 1U);
  // Stale and wrong-boot commands never publish an intent or touch controls.
  send(25,patch);retire_reply(5);assert(!k1_fixture_control_pending());
  put32(patch.data()+4,1);put32(patch.data()+16,99);
  send(25,patch);retire_reply(6);assert(!k1_fixture_control_pending());
  put32(patch.data()+16,1);
  send(25,patch);assert(k1_fixture_control_pending());
  k1_fixture_disconnect();assert(!k1_fixture_control_pending());
  assert(live_cfg_revision==1);send(1);retire_reply();
  // Palette off request likewise cannot submit until the RT owner runs.
  std::vector<uint8_t> config(32);const unsigned words[]{1,33,43,8,8,1,24,0};
  for(unsigned i=0;i<8;++i)put32(config.data()+4*i,words[i]);
  auto starts=pair_starts;send(16,config);assert(pair_starts==starts);
  k1_fixture_poll(20);pair_active=false;k1_fixture_control_service();retire_reply();
  assert(!palettes.emitEnabled());
  // A stale timeout witness cannot produce a second response for a completed
  // transaction whose owner application is pending.
  control_timeout_pending=true;
  send(16,config);assert(fill==0 && k1_fixture_control_pending());
  k1_fixture_control_service();k1_fixture_reply(&n);assert(n==0);
  k1_fixture_poll(30);pair_active=false;k1_fixture_control_service();retire_reply();
  k1_fixture_control_service();k1_fixture_reply(&n);assert(n==0);

  // Python wire bytes pass through the actual command decoder and RT owner.
  // Advancing clocks and read-only traffic cannot apply or ramp a target.
  auto read_bytes=[](const char* path) {
    FILE* f=std::fopen(path,"rb");assert(f);std::vector<uint8_t> b(128);
    b.resize(std::fread(b.data(),1,b.size(),f));std::fclose(f);return b;
  };
  auto perf=read_bytes(argv[2]);put32(perf.data()+4,live_cfg_revision);
  const auto old_focus=palettes.channel(0).audioFocus().level_gain;
  const auto old_motion=palettes.channel(0).controls().vp_waveform_shift_rate;
  auto polls=pair_polls;starts=pair_starts;
  send(25,perf);assert(k1_fixture_control_pending());
  assert(pair_polls==polls && pair_starts==starts);
  assert(palettes.channel(0).audioFocus().level_gain==old_focus);
  pair_active=true;k1_fixture_poll(40);
  assert(pending_control.phase==ControlPhase::Queued);
  pair_active=false;k1_fixture_poll(41);
  assert(pending_control.phase==ControlPhase::Applied);
  assert(live_cfg.focus_a[0]==2.F && live_cfg.visual_a.vp_waveform_shift_rate==20.F);
  assert(palettes.channel(0).controls().vp_waveform_shift_rate==old_motion);
  pair_active=false;k1_fixture_control_service();retire_reply();
  fake_cycles+=100000000U;send(23);retire_reply();
  assert(palettes.channel(0).audioFocus().level_gain==old_focus);
  // Even advancing owner time cannot modify controls during transmission.
  pair_active=true;k1_fixture_poll(141);
  assert(palettes.channel(0).audioFocus().level_gain==old_focus);
  pair_active=false;k1_fixture_poll(142);
  assert(palettes.channel(0).audioFocus().level_gain>old_focus);
  assert(palettes.channel(0).audioFocus().level_gain<2.F);
  assert(palettes.channel(0).controls().vp_waveform_shift_rate<old_motion);
  // Legacy visual patches do not cancel an unfinished focus target.
  put32(patch.data()+4,live_cfg_revision);put32(patch.data()+8,83);put32(patch.data()+16,1);
  send(25,patch);k1_fixture_poll(143);pair_active=false;k1_fixture_control_service();retire_reply();
  assert(performance_ramp.active);
  for(unsigned i=0;i<20;++i){fake_cycles+=100000000U;pair_active=false;k1_fixture_poll(200+i*100);}
  assert(std::fabs(palettes.channel(0).audioFocus().level_gain-2.F)<1e-5F);
  // A same-mode scene changes palette without clearing audio state/counters.
  auto scene=read_bytes(argv[3]);put32(scene.data()+8,live_cfg_revision);
  const auto frames=palettes.frames();const auto last_pub=palettes.lastPublicationUs();
  std::vector<uint8_t> audio_before(sizeof(live_runtime));
  std::memcpy(audio_before.data(), &live_runtime, sizeof(live_runtime));
  polls=pair_polls;send(25,scene);assert(pair_polls==polls);
  assert(std::memcmp(audio_before.data(),&live_runtime,sizeof(live_runtime))==0);
  assert(palettes.config().palette_a!=36);
  k1_fixture_poll(2500);pair_active=false;k1_fixture_control_service();retire_reply();
  assert(palettes.config().palette_a==36 && live_cfg.palette_a==36);
  assert(palettes.frames()>=frames && palettes.lastPublicationUs()==last_pub);
  // Invalid mixed batch, duplicate, wrong boot, mode and palette are atomic rejects.
  auto revision=live_cfg_revision;put32(perf.data()+4,revision);
  perf[28]=0xff;perf[29]=0xff;send(25,perf);retire_reply(3);
  assert(live_cfg_revision==revision && !k1_fixture_control_pending());
  scene=read_bytes(argv[3]);put32(scene.data()+8,revision);put32(scene.data()+4,0);
  send(25,scene);retire_reply(3);assert(live_cfg_revision==revision);
  perf=read_bytes(argv[2]);put32(perf.data()+4,revision);put32(perf.data()+8,0);
  send(25,perf);retire_reply(3);assert(live_cfg_revision==revision);

  {
    using namespace k1::titan;
    LiveConfigBlob base=live_cfg; base.mode_a=base.mode_b=8;base.output_channel=0;
    const auto before=base;
    LivePatchEntry invalid[]{ {kPerfLevel,kLivePatchKindF32,1.4F}, {0xffff,kLivePatchKindF32,1.F} };
    assert(liveApplyPerformance(&base,base.revision,base.boot_id,0,invalid,2)==LivePatchStatus::Forbidden);
    assert(std::memcmp(&base,&before,sizeof(base))==0);
    invalid[1]=invalid[0];
    assert(liveApplyPerformance(&base,base.revision,base.boot_id,0,invalid,2)==LivePatchStatus::Invalid);
    invalid[0].value=NAN;
    assert(liveApplyPerformance(&base,base.revision,base.boot_id,0,invalid,1)==LivePatchStatus::Invalid);
    invalid[0].value=1.4F;
    assert(liveApplyPerformance(&base,base.revision,base.boot_id+1,0,invalid,1)==LivePatchStatus::WrongBoot);
    base.mode_a=7;
    assert(liveApplyPerformance(&base,base.revision,base.boot_id,0,invalid,1)==LivePatchStatus::Forbidden);
    // dt-correct response is invariant to frame subdivision, within float error.
    LiveConfigBlob target=before,a=before,b=before;
    for(auto id:kPerformanceFields)*performanceField(target,id)=id==kPatchVisShiftRate?240.F:id==kPatchVisIdleFade?.999F:1.F;
    a.visual_a.vp_waveform_shift_rate=b.visual_a.vp_waveform_shift_rate=0.F;
    PerformanceRamp one,many;one.start(0);many.start(0);
    one.step(a,target,100000);
    for(unsigned t=1000;t<=100000;t+=1000)many.step(b,target,t);
    for(auto id:kPerformanceFields)assert(std::fabs(*performanceField(a,id)-*performanceField(b,id))<.001F);
  }


  // Simulate process loss: no host packets after a leased momentary boost.
  perf=read_bytes(argv[2]);perf.resize(28);k1::titan::livePutU16(perf.data()+12,1);put32(perf.data(),11);put32(perf.data()+4,live_cfg_revision);
  const auto saved_level=live_cfg.focus_a[0];
  send(25,perf);k1_fixture_poll(3000);pair_active=false;k1_fixture_control_service();retire_reply();
  assert(performance_audition.active && performance_audition.original==saved_level);
  const auto lease_revision=live_cfg_revision;
  fake_cycles+=800000000U;pair_active=true;k1_fixture_poll(3800);
  assert(performance_audition.active && live_cfg_revision==lease_revision);
  pair_active=false;k1_fixture_poll(3801);
  assert(!performance_audition.active && live_cfg.focus_a[0]==saved_level);
  assert(live_cfg_revision==lease_revision+1 && live_cfg.last_patch_request_id==0);
  // A newer explicit level target cancels the lease; expiry cannot overwrite it.
  perf=read_bytes(argv[2]);perf.resize(28);k1::titan::livePutU16(perf.data()+12,1);put32(perf.data(),11);put32(perf.data()+4,live_cfg_revision);
  send(25,perf);k1_fixture_poll(3802);pair_active=false;k1_fixture_control_service();retire_reply();
  assert(performance_audition.active);
  put32(perf.data(),9);put32(perf.data()+4,live_cfg_revision);
  send(25,perf);k1_fixture_poll(3803);pair_active=false;k1_fixture_control_service();retire_reply();
  assert(!performance_audition.active);const auto explicit_revision=live_cfg_revision;
  fake_cycles+=800000000U;k1_fixture_poll(4603);
  assert(live_cfg_revision==explicit_revision && live_cfg.focus_a[0]==2.F);
  // Finite effect/palette selection is inert until the idle owner consumes it.
  for (unsigned mode : {8U,7U,11U,3U,9U,12U,13U,14U,15U,21U,24U,26U,22U,23U,8U}) {
    std::vector<uint8_t> selection(24);
    put32(selection.data(),12);put32(selection.data()+4,live_cfg_revision);
    put32(selection.data()+8,1000+mode);put32(selection.data()+12,live_cfg.boot_id);
    put32(selection.data()+16,mode);put32(selection.data()+20,37);
    const auto before_mode=palettes.config().mode_a;
    const auto polls=pair_polls, starts=pair_starts;
    const auto hops=live_runtime.hopsConsumed();
    send(25,selection);
    assert(pair_polls==polls && pair_starts==starts);
    pair_active=true;k1_fixture_poll(4700);
    assert(palettes.config().mode_a==before_mode && pending_control.phase==ControlPhase::Queued);
    pair_active=false;k1_fixture_poll(4701);
    assert(palettes.config().mode_a==mode && palettes.config().mode_b==mode);
    assert(palettes.config().palette_a==37 && palettes.config().brightness==24);
    assert(live_runtime.hopsConsumed()==hops);
    pair_active=false;k1_fixture_control_service();retire_reply();
  }
  {
    std::vector<uint8_t> selection(24);put32(selection.data(),12);
    put32(selection.data()+4,live_cfg_revision);put32(selection.data()+8,1999);
    put32(selection.data()+12,live_cfg.boot_id);put32(selection.data()+16,0);put32(selection.data()+20,37);
    const auto revision=live_cfg_revision;send(25,selection);retire_reply(3);
    assert(live_cfg_revision==revision && !k1_fixture_control_pending());
  }

  // A cross-effect scene carries its scalars in one immutable selection.
  {
    using namespace k1::titan;
    std::vector<uint8_t> selection(40);put32(selection.data(),12);
    put32(selection.data()+4,live_cfg_revision);put32(selection.data()+8,2000);
    put32(selection.data()+12,live_cfg.boot_id);put32(selection.data()+16,26);put32(selection.data()+20,36);
    livePutU16(selection.data()+24,kPerfKick);livePutU16(selection.data()+26,1);livePutF32(selection.data()+28,1.75F);
    livePutU16(selection.data()+32,kPerfSnare);livePutU16(selection.data()+34,1);livePutF32(selection.data()+36,.5F);
    const auto revision=live_cfg_revision;send(25,selection);pair_active=true;k1_fixture_poll(4800);
    assert(live_cfg_revision==revision && live_cfg.mode_a==8);
    pair_active=false;k1_fixture_poll(4801);k1_fixture_control_service();retire_reply();
    assert(live_cfg_revision==revision+1 && live_cfg.mode_a==26 && live_cfg.palette_a==36);
    assert(live_cfg.focus_a[10]==1.75F && live_cfg.focus_a[11]==.5F);
    // Two independent leases: a subsequent explicit kick edit cancels only kick.
    std::vector<uint8_t> lease(36);put32(lease.data(),11);put32(lease.data()+4,live_cfg_revision);
    put32(lease.data()+8,2001);livePutU16(lease.data()+12,2);put32(lease.data()+16,live_cfg.boot_id);
    livePutU16(lease.data()+20,kPerfKick);livePutU16(lease.data()+22,1);livePutF32(lease.data()+24,2.F);
    livePutU16(lease.data()+28,kPerfSnare);livePutU16(lease.data()+30,1);livePutF32(lease.data()+32,.75F);
    send(25,lease);pair_active=false;k1_fixture_poll(4802);k1_fixture_control_service();retire_reply();
    assert(performance_auditions[10].active && performance_auditions[11].active);
    lease.resize(28);put32(lease.data(),9);put32(lease.data()+4,live_cfg_revision);livePutU16(lease.data()+12,1);livePutF32(lease.data()+24,1.25F);
    send(25,lease);pair_active=false;k1_fixture_poll(4803);k1_fixture_control_service();retire_reply();
    assert(!performance_auditions[10].active && performance_auditions[11].active);
    fake_cycles+=800000000U;pair_active=false;k1_fixture_poll(5603);
    assert(live_cfg.focus_a[10]==1.25F && live_cfg.focus_a[11]==.5F && !performance_auditions[11].active);
    // Unsupported field in a new mode cannot partially change either identity or values.
    put32(selection.data()+4,live_cfg_revision);put32(selection.data()+16,13); // kick is inactive here
    const auto before=live_cfg;send(25,selection);retire_reply(3);
    assert(std::memcmp(&before,&live_cfg,sizeof(before))==0);
  }
  // Actual staged product renderer: each admitted scalar must change pixels
  // for a declared input. This is a consumer witness, not optical qualification.
  for(auto id:k1::titan::kPerformanceFields) {
    if(!k1::titan::performanceConsumes(8,id))continue;
    using namespace k1::titan;using namespace k1::core::visual;
    PaletteRuntime low,high;PaletteConfig cfg;
    cfg.mode_a=cfg.mode_b=8;cfg.palette_a=33;cfg.palette_b=43;cfg.brightness=24;cfg.flags=1;
    assert(low.configure(cfg,0) && high.configure(cfg,0));
    auto a=live_cfg,b=live_cfg;
    a.focus_a[0]=b.focus_a[0]=1.F;a.focus_a[5]=b.focus_a[5]=1.F;
    a.visual_a.vp_waveform_shift_rate=b.visual_a.vp_waveform_shift_rate=120.F;
    a.visual_a.vp_waveform_idle_fade=b.visual_a.vp_waveform_idle_fade=.985F;
    a.visual_a.vp_waveform_fallback_brightness=b.visual_a.vp_waveform_fallback_brightness=.5F;
    a.visual_a.chroma=b.visual_a.chroma=0.F;
    a.visual_a.incandescent_filter=b.visual_a.incandescent_filter=0.F;
    a.visual_a.bulb_opacity=b.visual_a.bulb_opacity=0.F;
    a.visual_a.flags&=~(kVisFixSecondary|kVisIncandescent);b.visual_a.flags=a.visual_a.flags;
    *performanceField(a,id)=id==kPatchVisIdleFade?.90F:0.F;
    *performanceField(b,id)=id==kPatchVisShiftRate?240.F:id==kPatchVisIdleFade?.999F:(id==kPerfLevel||id==kPerfTonal)?2.F:1.F;
    live_copy_visual(low.channel(0).controls(),a.visual_a);live_copy_focus(low.channel(0).audioFocus(),a.focus_a);
    live_copy_visual(high.channel(0).controls(),b.visual_a);live_copy_focus(high.channel(0).audioFocus(),b.focus_a);
    k1::contract::AudioFeaturesV1 audio{};k1::core::audio::TempoTrackerEvent tempo{};VisualWaveformHistory wave{};
    audio.validity_flags=k1::contract::kValidBaseSnapshot|k1::contract::kValidChroma|k1::contract::kValidSpectrum;
    audio.peak_scaled=.65F;audio.vu_level=.65F;audio.chroma_strength=.45F;audio.chroma_a_origin[0]=.9F;audio.chroma_a_origin[4]=.4F;
    wave.peak_scaled=.65F;wave.raw_maximum=4000.F;
    bool differs=false;
    for(unsigned i=1;i<=180;++i) {
      audio.sequence=i;audio.publish_time_us=i*kPalettePeriodUs;
      VisualAudioFrameView view{audio,tempo,wave,audio.publish_time_us};
      assert(low.step(audio.publish_time_us,&view));assert(high.step(audio.publish_time_us,&view));
      differs |= std::memcmp(low.channel(0).frame().data(),high.channel(0).frame().data(),sizeof(k1::core::Pixel8)*160)!=0;
    }
    std::printf("PERFORMANCE_CONSUMER field=%u differs=%d\n",id,differs);std::fflush(stdout);assert(differs);
    const auto count=low.frames();const auto publication=low.lastPublicationUs();
    const auto effect=low.channel(0).effectState();
    low.selectPerformancePalette(36,200*kPalettePeriodUs);
    assert(low.frames()==count && low.lastPublicationUs()==publication);
    assert(std::memcmp(&effect,&low.channel(0).effectState(),sizeof(effect))==0);
  }
  std::puts("PERFORMANCE_OWNER_PASS wire_decode idle_only_ramp atomic_scene no_audio_reset");
  std::puts("CONTROL_OWNER_PASS advancing_clock observations mailbox reentry forbidden_diagnostics");
}
