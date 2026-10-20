#include "core/visual/paired_wave_scene.h"
#include <cassert>
#include <cstring>
#include <cmath>
#include <array>
#include <limits>
#include <cstdlib>
#include <new>
using namespace k1::core::visual;
static bool allocation_forbidden=false;
void* operator new(std::size_t n) {assert(!allocation_forbidden);if(void* p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}

float energy(const RibbonFrame16& f){float s=0;for(auto p:f)s+=p.red+p.green+p.blue;return s;}
PairedFrame16 replay(unsigned hz) {
 PairedWaveScene scene; k1::contract::AudioFeaturesV1 audio{};
 k1::core::audio::TempoTrackerEvent tempo{};VisualWaveformHistory wave{};
 audio.validity_flags=k1::contract::kValidBaseSnapshot;audio.event_flags=0;audio.vu_level=.25F;
 unsigned publication=0,frame=0;
 while(frame<=hz*2) {
  const auto render=std::uint64_t(frame)*1000000/hz;
  while(std::uint64_t(publication)*25000<=render) {
   audio.sequence=publication+1;
   const auto now=std::uint64_t(publication)*25000;
   scene.consume({audio,tempo,wave,now,nullptr},now);++publication;
  }
  scene.render(render);++frame;
 }
 return scene.render(2000000);
}
int main() {
  allocation_forbidden=true;
  const auto cadence_reference=replay(120);
  for(unsigned hz:{30,60,120,240}) {
    const auto frame=replay(hz);
    for(unsigned edge=0;edge<2;++edge)for(unsigned i=0;i<160;++i) {
      const auto a=(edge?frame.bottom:frame.top)[i],b=(edge?cadence_reference.bottom:cadence_reference.top)[i];
      assert(std::abs(int(a.red)-int(b.red))<=1);
      assert(std::abs(int(a.green)-int(b.green))<=1);
      assert(std::abs(int(a.blue)-int(b.blue))<=1);
    }
  }
  // A loud attack must still obey articulation. The previous max(impact, energy)
  // made 0 and 0.7 identical at full input, hiding the authored control.
  float attack_output[3]{};
  for(unsigned j=0;j<3;++j) {
    PairedWaveScene controlled;PairedWaveSettings setting;
    setting.articulation=j==0?0.F:j==1?.7F:1.F;
    assert(controlled.configure(setting,0));
    k1::contract::AudioFeaturesV1 input{};
    input.validity_flags=k1::contract::kValidBaseSnapshot|k1::contract::kValidOnsetV2;
    input.event_flags=0;input.sequence=1;input.onset_event_id=1;input.vu_level=1.F;
    k1::core::audio::TempoTrackerEvent rhythm{};VisualWaveformHistory history{};
    controlled.consume({input,rhythm,history,0,nullptr},0);
    const auto& f=controlled.render(0);attack_output[j]=energy(f.top)+energy(f.bottom);
  }
  assert(attack_output[0]>0); // Immediate music response survives zero accent.
  assert(attack_output[1]>attack_output[0]*2);
  assert(attack_output[2]>attack_output[1]);

  PairedWaveScene scene;
  PairedWaveSettings settings;assert(scene.configure(settings,0));
  assert(energy(scene.render(0).top)==0);
  k1::contract::AudioFeaturesV1 a{};
  a.validity_flags=k1::contract::kValidBaseSnapshot|k1::contract::kValidOnsetV2;
  a.vu_level=.5F;a.event_flags=0;a.sequence=1;a.onset_event_id=1;
  k1::core::audio::TempoTrackerEvent tempo{};VisualWaveformHistory wave{};
  const auto saved=a;
  scene.consume({a,tempo,wave,0,nullptr},0);
  const auto first=scene.render(0);assert(energy(first.top)>0);
  assert(std::memcmp(&saved,&a,sizeof(a))==0);
  scene.render(150000);const float phase=scene.telemetry().transport_px;
  settings.velocity_px_s=35;assert(scene.configure(settings,150000));
  assert(scene.telemetry().transport_px==phase);
  assert(!scene.configure(settings,149999));
  settings.velocity_px_s=std::numeric_limits<float>::quiet_NaN();assert(!scene.configure(settings,150000));
  for(unsigned i=1;i<=600;++i) {
    const auto now=150000+std::uint64_t(i)*8333;
    a.sequence=i+1;a.onset_event_id=i%15==0?i+1:a.onset_event_id;
    scene.consume({a,tempo,wave,now,nullptr},now);
    const auto& out=scene.render(now);
    for(const auto* edge:{&out.top,&out.bottom})for(unsigned j=0;j<80;++j) {
      const auto x=(*edge)[j],y=(*edge)[159-j];
      assert(x.red==y.red&&x.green==y.green&&x.blue==y.blue);
      assert(x.red<=6168&&x.green<=6168&&x.blue<=6168);
    }
    assert(std::memcmp(out.top.data(),out.bottom.data(),sizeof(out.top))!=0);
  }
  assert(scene.telemetry().attacks>1);
  assert(energy(scene.render(15000000).top)==0); // No stale held glow.

  // Asymmetric sentinel catches a reversal or cross-edge copy that mirrored
  // artistic frames cannot reveal. Exercise every swap/reverse combination.
  PairedFrame16 sentinels;
  for(unsigned i=0;i<160;++i){sentinels.top[i]={std::uint16_t(i+1),0x1234,0x5678};sentinels.bottom[i]={0x9876,std::uint16_t(i+200),0x4321};}
  std::array<std::uint8_t,960> top{},bottom{};
  for(unsigned flags=0;flags<8;++flags) {
    PairedOrientation o{bool(flags&1),bool(flags&2),bool(flags&4)};
    assert(stagePaired(sentinels,o,top.data(),top.size(),bottom.data(),bottom.size()));
    for(unsigned i=0;i<160;++i)for(unsigned edge=0;edge<2;++edge) {
      const auto& src=(edge==0)!=o.swap_edges?sentinels.top:sentinels.bottom;
      const unsigned at=(edge==0?o.reverse_top:o.reverse_bottom)?159-i:i;
      const auto p=src[at];const auto& bytes=edge==0?top:bottom;
      assert((unsigned(bytes[i*6])<<8|bytes[i*6+1])==p.green);
      assert((unsigned(bytes[i*6+2])<<8|bytes[i*6+3])==p.red);
      assert((unsigned(bytes[i*6+4])<<8|bytes[i*6+5])==p.blue);
    }
  }
  const auto before=top;
  assert(!stagePaired(sentinels,{},top.data(),959,bottom.data(),960));
  assert(!stagePaired(sentinels,{},top.data(),960,top.data(),960));
  assert(top==before);
  assert(sizeof(PairedWaveScene)<16000);

  // Isolated musical attack: actual final pairs leave a quiet interval before
  // the weaker return. Silence stops NEW deposits; retained material travels.
  PairedWaveScene impulse;
  a.sequence=1;a.onset_event_id=1;a.vu_level=.8F;a.event_flags=0;
  impulse.consume({a,tempo,wave,0,nullptr},0);
  float initial_peak=0,return_peak=0;unsigned dark=0;bool returned=false,departed=false;
  for(unsigned frame=0;frame<960;++frame) {
    const std::uint64_t now=std::uint64_t(frame)*8333;
    if(frame){a.sequence=frame+1;a.vu_level=0;a.event_flags=k1::contract::kEventSilence;impulse.consume({a,tempo,wave,now,nullptr},now);}
    const auto& f=impulse.render(now);const float e=energy(f.top)+energy(f.bottom);
    if(!departed&&e>0)initial_peak=std::max(initial_peak,e);
    if(e==0&&initial_peak>0){departed=true;++dark;}
    if(departed&&e>0){returned=true;return_peak=std::max(return_peak,e);}
  }
  assert(departed&&dark>6&&returned&&return_peak<initial_peak*.35F);

  PairedWaveScene edited;
  a.event_flags=0;a.vu_level=.5F;a.sequence=1;a.onset_event_id=1;
  edited.consume({a,tempo,wave,0,nullptr},0);
  const auto before_edit=edited.render(300000);
  PairedWaveSettings edit;edit.palette_id=7;edit.width_px=12;edit.velocity_px_s=60;
  assert(edited.configure(edit,300000));
  const auto after_edit=edited.render(300000);
  assert(std::memcmp(&before_edit,&after_edit,sizeof(before_edit))==0);
}
