#include "core/visual/ribbon_material.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <cstdlib>
#include <new>
using namespace k1::core::visual;
static bool allocation_forbidden=false;
void* operator new(std::size_t n) { assert(!allocation_forbidden);if(void* p=std::malloc(n))return p;throw std::bad_alloc(); }
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
float sum(const RibbonFrame16& f) { float s=0; for(auto p:f) s+=p.red+p.green+p.blue; return s; }
int main() {
  allocation_forbidden=true;
  RibbonMaterial a,b;
  RibbonSettings s; s.spread_px2_s=0;s.launch_spacing_s=.44F;
  assert(a.configure(s,0)); assert(b.configure(s,0));
  assert(a.deposit(1,.3F,0)); assert(b.deposit(1,.3F,0));
  const float first=sum(a.render(0)); assert(first>0);
  a.advance(250000); const auto old=a.objects()[0];
  assert(a.deposit(.8F,.7F,250000)); // Departed material cannot absorb a centre attack.
  assert(a.objects()[0].radius==old.radius && a.objects()[0].born_us==0);
  assert(a.telemetry().omitted==1 && a.objects()[0].amplitude==old.amplitude);
  a.advance(500000); assert(a.deposit(.6F,.7F,500000));
  assert(a.objects()[0].radius>old.radius && a.objects()[1].active);
  const auto before=a.objects()[0]; s.velocity_px_s=96;
  assert(a.configure(s,500000)); assert(a.objects()[0].radius==before.radius);
  a.advance(750000); assert(std::fabs(a.objects()[0].radius-before.radius-24)<.001F);
  const auto f=a.render(750000);
  for(unsigned i=0;i<80;++i) {
    const auto x=f[i],y=f[159-i]; assert(x.red==y.red && x.green==y.green && x.blue==y.blue);
    assert(x.red<=6168 && x.green<=6168 && x.blue<=6168);
  }
  assert(sum(a.render(750000,0))==0);
  s.velocity_px_s=std::numeric_limits<float>::quiet_NaN(); assert(!a.configure(s,750000));
  assert(!a.advance(740000));
  // A single complete launch has a genuinely black interval and weaker return.
  bool gap=false,echo=false; float echo_max=0;
  for(unsigned i=0;i<=720;++i) {
    auto& out=b.render(static_cast<std::uint64_t>(i)*10000);
    if(b.objects()[0].waiting) { assert(sum(out)==0); gap=true; }
    if(b.telemetry().returning) { echo=true; echo_max=std::max(echo_max,sum(out)); }
  }
  assert(gap && echo && echo_max<first*.5F);
  assert(sum(b.render(10000000))==0);
  // Different render cadences reach the same material state, including the
  // support-boundary transition. Comparison tolerance covers float recurrence.
  for(unsigned hz:{30U,60U,120U,240U}) {
    RibbonMaterial stepped,direct; RibbonSettings law;
    stepped.configure(law,0);direct.configure(law,0);
    stepped.deposit(1,.3F,0);direct.deposit(1,.3F,0);
    for(unsigned i=1;i<=hz*4;++i)stepped.advance(std::uint64_t(i)*1000000/hz);
    direct.advance(4000000);
    const auto x=stepped.objects()[0],y=direct.objects()[0];
    assert(x.active==y.active && x.returning==y.returning && x.waiting==y.waiting);
    assert(std::fabs(x.radius-y.radius)<.002F && std::fabs(x.amplitude-y.amplitude)<.0001F);
  }
  RibbonMaterial colour; RibbonSettings palette;colour.configure(palette,0);colour.deposit(1,.3F,0);
  const auto owned=colour.objects()[0].colour;palette.palette_id=7;colour.configure(palette,200000);
  assert(colour.objects()[0].colour.red==owned.red && colour.objects()[0].radius>0);
  const float radius=colour.objects()[0].radius;palette.recolour_history=true;colour.configure(palette,200000);
  assert(colour.objects()[0].active && colour.objects()[0].radius==radius);
  // No tempo/phase evidence is needed for immediate useful music response.
  k1::contract::AudioFeaturesV1 audio{};k1::core::audio::TempoTrackerEvent tempo{};
  VisualWaveformHistory wave{};audio.validity_flags=k1::contract::kValidBaseSnapshot;
  audio.event_flags=0;audio.vu_level=.8F;audio.sequence=1;
  RibbonMaterial fallback;VisualAudioFrameView view{audio,tempo,wave,0,nullptr};
  fallback.consume(view,0);assert(sum(fallback.render(0))>0);
  fallback.consume(view,1000000);assert(!fallback.telemetry().audio_available);
  assert(fallback.telemetry().launches==1); // Missing publication cannot invent attacks.
  RibbonMaterial quiet_music;
  for(unsigned i=0;i<=50;++i) {
    audio.sequence=i+1;audio.vu_level=.01F;
    VisualAudioFrameView fresh{audio,tempo,wave,std::uint64_t(i)*100000,nullptr};
    quiet_music.consume(fresh,std::uint64_t(i)*100000);
    quiet_music.render(std::uint64_t(i)*100000);
  }
  assert(quiet_music.telemetry().launches>=5 && sum(quiet_music.render(5000000))>0);
  RibbonSettings treatment;treatment.persistence_s=.15F;treatment.knee=true;
  RibbonMaterial treated;treated.configure(treatment,0);treated.deposit(1,.3F,0);
  float first_treated=sum(treated.render(0)),quiet_max=0;unsigned quiet_frames=0;
  for(unsigned i=1;i<700;++i) {
    const auto& image=treated.render(std::uint64_t(i)*10000);
    if(treated.objects()[0].waiting && treated.objects()[0].quiet_remaining_s<.16F) {
      quiet_max=std::max(quiet_max,sum(image));++quiet_frames;
    }
  }
  assert(quiet_frames>5 && quiet_max<first_treated*.01F);
  assert(sizeof(RibbonMaterial)<20000); // Explicit per-channel resource budget.
  std::puts("ribbon: persistence, dense attacks, edits, symmetry, cap, quiet interval, weak return, invalid inputs PASS");
}
