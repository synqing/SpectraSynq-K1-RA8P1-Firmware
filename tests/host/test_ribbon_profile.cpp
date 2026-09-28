#include "core/visual/ribbon_profile.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>

using namespace k1::core::visual;
int main() {
  RibbonProfile a,b;
  RibbonSettings base; base.velocity_px_s=40;
  assert(a.configure(base,0) && b.configure(base,0));
  k1::contract::AudioFeaturesV1 audio{};
  k1::core::audio::TempoTrackerEvent tempo{}; VisualWaveformHistory wave{};
  audio.validity_flags=k1::contract::kValidBaseSnapshot;
  audio.vu_level=.8F; audio.sequence=1; audio.event_flags=0;
  const auto saved=audio;
  const VisualAudioFrameView view{audio,tempo,wave,0,nullptr};
  a.consume(view,0); b.consume(view,0);
  assert(a.effectiveVelocity()==40);
  assert(a.setLiveiness(0,0) && a.effectiveVelocity()==20);
  assert(a.settings().velocity_px_s==40);
  a.render(250000);
  const auto old=a.material().objects()[0];
  assert(std::abs(old.radius-5)<1e-6F);
  assert(a.setLiveiness(1,250000) && a.effectiveVelocity()==80);
  assert(a.material().objects()[0].radius==old.radius);
  assert(a.material().objects()[0].amplitude==old.amplitude);
  assert(a.material().objects()[0].variance==old.variance);
  assert(a.settings().half_life_s==base.half_life_s);
  a.render(500000); b.render(500000);
  assert(std::abs(a.material().objects()[0].radius-25)<1e-6F);
  assert(std::abs(b.material().objects()[0].radius-20)<1e-6F);
  assert(std::abs(a.material().objects()[0].amplitude-b.material().objects()[0].amplitude)<2e-7F);
  assert(std::memcmp(&saved,&audio,sizeof(audio))==0);
  assert(!a.setLiveiness(std::numeric_limits<float>::quiet_NaN(),500000));
  assert(!a.setLiveiness(.5F,499999));
  assert(a.liveiness()==1 && a.settings().velocity_px_s==40);
  base.velocity_px_s=240; assert(a.configure(base,500000));
  assert(a.effectiveVelocity()==240 && a.settings().velocity_px_s==240);

  // Actual profile producer -> post-master frame -> native wire. No second
  // gain, Pixel8 expansion, palette remap or gamma is permitted at staging.
  std::array<std::uint8_t,kPackedBytesPerLane> first{},second{};
  const auto frame=b.render(500000,255); // Titan maximum remains 24/255.
  assert(frame.valid() && frame.stage(first.data(),first.size(),second.data(),second.size()));
  bool fractional=false,lit=false;
  for(unsigned i=0;i<160;++i) {
    const auto p=frame.pixels()[i];
    assert(p.red<=6168 && p.green<=6168 && p.blue<=6168);
    const auto& bytes=i<80?first:second; const unsigned n=(i%80)*6;
    assert((unsigned(bytes[n])<<8|bytes[n+1])==p.green);
    assert((unsigned(bytes[n+2])<<8|bytes[n+3])==p.red);
    assert((unsigned(bytes[n+4])<<8|bytes[n+5])==p.blue);
    fractional|=(p.red%257 || p.green%257 || p.blue%257); lit|=(p.red || p.green || p.blue);
  }
  assert(lit && fractional);
  const auto before=first;
  assert(!frame.stage(first.data(),first.size()-1,second.data(),second.size()));
  assert(!frame.stage(first.data(),first.size(),first.data()+1,second.size()));
  assert(!frame.stage(nullptr,first.size(),second.data(),second.size()));
  assert(first==before);
  const auto invalid=b.render(499999);
  assert(!invalid.valid() && !invalid.stage(first.data(),first.size(),second.data(),second.size()));
  const auto zero=b.render(500000,0);
  for(auto p:zero.pixels()) assert(p.red==0 && p.green==0 && p.blue==0);
}
