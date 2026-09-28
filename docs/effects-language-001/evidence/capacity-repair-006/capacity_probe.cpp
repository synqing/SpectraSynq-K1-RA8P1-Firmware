#include "core/visual/paired_wave_scene.h"
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
using namespace k1::core::visual;
static unsigned differences(const PairedFrame16& a,const PairedFrame16& b) {
 unsigned n=0;
 for(unsigned edge=0;edge<2;++edge)for(unsigned i=0;i<160;++i) {
  const auto x=(edge?a.bottom:a.top)[i],y=(edge?b.bottom:b.top)[i];
  n+=(x.red!=y.red)+(x.green!=y.green)+(x.blue!=y.blue);
 }
 return n;
}
static unsigned long long sum(const PairedFrame16& a) {
 unsigned long long s=0;
 for(unsigned e=0;e<2;++e)for(auto p:(e?a.bottom:a.top))s+=p.red+p.green+p.blue;
 return s;
}
static void save(const std::string& path,const PairedFrame16& a) {
 std::ofstream o(path,std::ios::binary);
 for(unsigned e=0;e<2;++e)for(auto p:(e?a.bottom:a.top))for(std::uint16_t c:{p.red,p.green,p.blue}) {
  const char bytes[]={char(c&255),char(c>>8)};o.write(bytes,2);
 }
}
static bool probe(float velocity,const std::string& folder) {
 PairedWaveScene scene;PairedWaveSettings cfg;cfg.velocity_px_s=velocity;
 if(!scene.configure(cfg,0))return false;
 k1::contract::AudioFeaturesV1 audio{};
 audio.validity_flags=k1::contract::kValidBaseSnapshot|k1::contract::kValidOnsetV2;
 audio.event_flags=0;audio.vu_level=1.F;audio.low_energy=.5F;audio.high_energy=.5F;
 k1::core::audio::TempoTrackerEvent tempo{};VisualWaveformHistory wave{};
 std::uint32_t onset=0;
 for(unsigned hop=0;hop<=800;++hop) {
  const std::uint64_t now=std::uint64_t(hop)*25000;
  audio.sequence=hop+1;audio.onset_event_age_ms=0;
  if(hop%6==0) {
   auto with=scene,without=scene;
   const auto before=scene.telemetry();
   audio.onset_event_id=onset;
   without.consume({audio,tempo,wave,now,nullptr},now);
   const auto f0=without.render(now);
   audio.onset_event_id=++onset;
   with.consume({audio,tempo,wave,now,nullptr},now);
   const auto f1=with.render(now);
   const auto ta=with.telemetry(),tb=without.telemetry();
   if(now==4800000) {
    save(folder+"/attack.rgb16le",f1);save(folder+"/no_attack.rgb16le",f0);
    std::printf("{\"witness\":true,\"velocity_px_s\":%.1f,\"time_us\":%llu,\"hop_us\":25000,\"onset_period_us\":150000,\"vu_level\":1,\"before_attacks\":%u,\"before_omitted\":%u,\"attack_branch_attacks\":%u,\"control_branch_attacks\":%u,\"attack_branch_omitted\":%u,\"control_branch_omitted\":%u,\"different_rgb_components\":%u,\"attack_channel_sum\":%llu,\"control_channel_sum\":%llu}\n",velocity,(unsigned long long)now,before.attacks,before.omitted,ta.attacks,tb.attacks,ta.omitted,tb.omitted,differences(f1,f0),sum(f1),sum(f0));
    return differences(f1,f0)>0 && ta.attacks_admitted==tb.attacks_admitted+1 && ta.attacks_omitted==0;
   }
   scene=with;
  } else {
   audio.onset_event_id=onset;scene.consume({audio,tempo,wave,now,nullptr},now);scene.render(now);
  }
 }
 std::printf("{\"witness\":false,\"velocity_px_s\":%.1f,\"duration_us\":20000000,\"attacks\":%u,\"omitted\":%u}\n",velocity,scene.telemetry().attacks,scene.telemetry().omitted);
 return false;
}
int main(int argc,char** argv) {
 if(argc!=2)return 2;
 if(probe(40.F,argv[1]))return 0;
 return probe(4.F,argv[1])?0:1;
}
