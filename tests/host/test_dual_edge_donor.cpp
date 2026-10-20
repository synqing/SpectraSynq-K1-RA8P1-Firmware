#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "palette_runtime.h"
#include "k1_live_protocol.h"
#include "k1_dual_edge_donor.h"
using namespace k1::core::visual;
using namespace k1::titan;
static unsigned word(const unsigned char* p){return unsigned(p[0])*256+p[1];}
static unsigned long long sum(const Pixel16* p){unsigned long long s=0;for(unsigned i=0;i<160;++i)s+=p[i].red+p[i].green+p[i].blue;return s;}
static unsigned energy(const Pixel16& p){return unsigned(p.red)+p.green+p.blue;}
static unsigned dist(const Pixel16& x, const Pixel16& y){
 auto d=[](unsigned a,unsigned b){return a>b?a-b:b-a;};
 return d(x.red,y.red)+d(x.green,y.green)+d(x.blue,y.blue);
}
static unsigned toward(const Pixel16& before, const Pixel16& after, const Pixel16& target){
 const auto was=dist(before,target), now=dist(after,target);
 return was>now?was-now:0U;
}
static void pitch(k1::contract::AudioFeaturesV1& features, unsigned strong, unsigned second){
 for(unsigned i=0;i<12;++i) features.chroma_a_origin[i]=0;
 features.chroma_a_origin[strong]=.9F; features.chroma_a_origin[second]=.7F;
 for(unsigned i=0;i<60;++i) features.spectrum[i]=(i%12==strong?.9F:i%12==second?.7F:0.0F);
}
static void drive(unsigned n, std::uint64_t& t, VisualAudioFrameView& view, ChannelRenderState& ca, ChannelRenderState& cb, Pixel16* a, Pixel16* b){
 // Each output frame carries a publication from that frame. A frozen stamp ages out of the 30000 µs window.
 for(unsigned i=0;i<n;++i){ view.source_publication_us=t; renderDualEdgeDonor(t,&view,ca,cb,a,b); t+=16667ULL; }
}
int main(){
 assert(k1::titan::LiveConfigBlob{}.brightness==100);
 PaletteRuntime runtime;PaletteConfig cfg;cfg.mode_a=cfg.mode_b=8;cfg.flags=5;assert(cfg.brightness==100);
 assert(runtime.configure(cfg,0));
 Pixel16 a[160]{},b[160]{};unsigned char wa[482]{},wb[482]{};
 for(unsigned i=0;i<160;++i){a[i]={std::uint16_t(256+i*113),std::uint16_t(1024+i*37),std::uint16_t(301+i*71)};b[i]={std::uint16_t(65000-i*83),std::uint16_t(8192-i*17),std::uint16_t(20000+i*11)};}
 // DIN identity is a half-frame offset, never a visual-channel selector.
 for(unsigned selected=0;selected<2;++selected){
  cfg.output_channel=selected;assert(runtime.configure(cfg,0));
  assert(runtime.stageWideFrame(0,a,160)&&runtime.stageWideFrame(1,b,160));
  std::memset(wa,0xA5,sizeof wa);std::memset(wb,0x5A,sizeof wb);
  assert(runtime.packBenchGrb48Lane(wa,479,0)==0 && wa[0]==0xA5);
  assert(runtime.packBenchGrb48Lane(wa,480,0)==480);
  assert(runtime.packBenchGrb48Lane(wb,480,1)==480);
  assert(wa[480]==0xA5&&wa[481]==0xA5&&wb[480]==0x5A&&wb[481]==0x5A);
  const auto* scene=selected?b:a;
  for(unsigned i=0;i<160;++i){
   const auto* pixel=(i<80?wa:wb)+(i%80)*6;
   assert(word(pixel)==unsigned(scene[i].green)*100/255);
   assert(word(pixel+2)==unsigned(scene[i].red)*100/255);
   assert(word(pixel+4)==unsigned(scene[i].blue)*100/255);
  }
  assert(runtime.packBenchGrb48Lane(wa,480,2)==0);
 }
 k1::contract::AudioFeaturesV1 features;features.sequence=1;features.event_flags=0;features.validity_flags=k1::contract::kValidBaseSnapshot|k1::contract::kValidChroma;
 features.peak_scaled=.6F;features.vu_level=.4F;features.chroma_a_origin[3]=.9F;features.chroma_a_origin[7]=.7F;
 for(unsigned i=0;i<60;++i) features.spectrum[i]=(i%12==3?.9F:i%12==7?.7F:0.0F);
 k1::core::audio::TempoTrackerEvent tempo{};VisualWaveformHistory wave;wave.raw_maximum=2000;wave.peak_scaled=.55F;
 VisualAudioFrameView view{features,tempo,wave,1000,nullptr};
 ChannelRenderState ca(PixelChannelId::kChannelA),cb(PixelChannelId::kChannelB);
 ca.controls().mood=.35F;ca.controls().vp_bloom_alpha=.99F;cb.controls().chroma=.033325F;
 resetDualEdgeDonor();renderDualEdgeDonor(1000,&view,ca,cb,a,b);
 assert(sum(a)>0&&sum(b)>0);assert(a[79].red==a[80].red&&a[30].red==0&&a[129].red==0);
 for(unsigned frame=1;frame<240;++frame){
  const auto now=1000ULL+frame*16667ULL; view.source_publication_us=now;
  renderDualEdgeDonor(now,&view,ca,cb,a,b);
 }
 assert(sum(a)>0&&sum(b)>0);assert(std::memcmp(a,b,sizeof a)!=0);
 assert(a[120].red+a[120].green+a[120].blue>0);
 const auto prior=a[80];
 // Raw pitch-class sums above1 must not flatten every pitch to white.
 for(unsigned i=0;i<12;++i) features.chroma_a_origin[i]=3.0F;
 for(unsigned i=0;i<60;++i) features.spectrum[i]=(i%12==9?.9F:i%12==1?.7F:0.0F);
 {
  const auto now=1000ULL+239ULL*16667ULL+10000ULL; view.source_publication_us=now;
  renderDualEdgeDonor(now,&view,ca,cb,a,b);
 }
 assert(a[80].red!=prior.red || a[80].green!=prior.green || a[80].blue!=prior.blue);
 auto lit=sum(a)+sum(b);
 for(unsigned frame=240;frame<1440;++frame)renderDualEdgeDonor(1000+frame*16667ULL,nullptr,ca,cb,a,b);
 assert(sum(a)+sum(b)<lit/100);
 // Leg A. 80 wire frames are about 155 internal ticks of 8631 µs. At mood 0.35 and
 // shift scale 1, Aurora travels 1.525 px/tick, so pixel 149 is filled. Bloom's
 // 40-pixel quadratic fade leaves that pixel near (10/39)^2 of pixel 109, and the
 // mirror copies it to pixel 10. energy(10)*2 > energy(50) fails if that fade remains.
 {
  resetDualEdgeDonor();
  pitch(features, 3, 7);
  view.source_publication_us = 5000000;
  std::uint64_t t = 5000000;
  ca.controls().mood = .35F;
  ca.controls().vp_bloom_shift_scale = 1.0F;
  ca.controls().vp_waveform_shift_rate = 120.0F;
  drive(80, t, view, ca, cb, a, b);
  const unsigned e10 = energy(a[10]);
  const unsigned e50 = energy(a[50]);
  const unsigned e149 = energy(a[149]);
  if (!(e10 > 500 && e149 > 500 && e10 * 2 > e50 && a[79].red == a[80].red && energy(a[80]) > 500)) {
   std::fprintf(stderr, "aurora fill e10=%u e50=%u e149=%u c80=%u,%u,%u\n",
                e10, e50, e149, a[80].red, a[80].green, a[80].blue);
   assert(false);
  }
  const Pixel16 old_end = a[149];
  const Pixel16 old_centre = a[80];
  pitch(features, 9, 1);
  view.source_publication_us = t;
  drive(3, t, view, ca, cb, a, b);
  if (!(energy(a[80]) > 500 && dist(a[80], old_centre) > 0 && dist(a[149], old_end) < dist(a[149], a[80]))) {
   std::fprintf(stderr, "aurora retain end=%u,%u,%u centre=%u,%u,%u old=%u,%u,%u\n",
                a[149].red, a[149].green, a[149].blue, a[80].red, a[80].green, a[80].blue,
                old_end.red, old_end.green, old_end.blue);
   assert(false);
  }
 }
 // 16 wire frames are 29 internal ticks. Scale 1 travels about 44 px and misses
 // pixel 149. Rate 240 clamps the existing scale to 2 and travels about 88 px.
 {
  auto reach = [&](float rate) {
   resetDualEdgeDonor();
   pitch(features, 3, 7);
   view.source_publication_us = 8000000;
   std::uint64_t t = 8000000;
   ca.controls().mood = .35F;
   ca.controls().vp_bloom_shift_scale = 1.0F;
   ca.controls().vp_waveform_shift_rate = rate;
   drive(16, t, view, ca, cb, a, b);
   return energy(a[149]);
  };
  const unsigned slow = reach(120.0F);
  const unsigned fast = reach(240.0F);
  if (!(fast > 500 && slow * 8 < fast)) {
   std::fprintf(stderr, "shift scale slow=%u fast=%u\n", slow, fast);
   assert(false);
  }
 }
 // Leg B. A repeated stamp inside 30000 µs still deposits. Past that window it does not.
 // No audio fault in this block, so the reject flag cannot hide the window.
 {
  auto colour_of=[&](unsigned strong, unsigned second){
   resetDualEdgeDonor();
   pitch(features, strong, second);
   view.visual_magnitudes=nullptr;
   std::uint64_t u=20000000;
   ca.controls().mood=.35F;
   ca.controls().vp_bloom_shift_scale=1.0F;
   ca.controls().vp_waveform_shift_rate=120.0F;
   drive(12, u, view, ca, cb, a, b);
   return a[80];
  };
  const Pixel16 pitch_a=colour_of(3,7), pitch_b=colour_of(9,1), pitch_c=colour_of(5,11);
  if(!(dist(pitch_a,pitch_b)>1500&&dist(pitch_b,pitch_c)>1500&&dist(pitch_a,pitch_c)>1500)){
   std::fprintf(stderr,"pitch separation a-b=%u b-c=%u a-c=%u\n",dist(pitch_a,pitch_b),dist(pitch_b,pitch_c),dist(pitch_a,pitch_c));
   assert(false);
  }
  resetDualEdgeDonor();
  pitch(features,3,7);
  view.visual_magnitudes=nullptr;
  std::uint64_t t=30000000;
  ca.controls().mood=.35F;
  drive(40, t, view, ca, cb, a, b);
  const auto window_stamp=t-16667ULL;
  const Pixel16 held_centre=a[80];
  const auto held_sum=sum(a);
  pitch(features,9,1);
  view.source_publication_us=window_stamp;
  renderDualEdgeDonor(window_stamp+20000ULL,&view,ca,cb,a,b);
  const auto young_close=toward(held_centre,a[80],pitch_b);
  const Pixel16 young_centre=a[80];
  pitch(features,5,11);
  view.source_publication_us=window_stamp;
  renderDualEdgeDonor(window_stamp+45000ULL,&view,ca,cb,a,b);
  const auto stale_close=toward(young_centre,a[80],pitch_c);
  if(!(young_close>500&&stale_close*4<young_close&&sum(a)>held_sum/2&&energy(a[149])>200)){
   std::fprintf(stderr,"window young_close=%u stale_close=%u sum=%llu e149=%u centre=%u,%u,%u\n",
    young_close,stale_close,sum(a),energy(a[149]),a[80].red,a[80].green,a[80].blue);
   assert(false);
  }
  // Loud frames move the gain follower off 1. A fault must not rebuild it or the picture.
  resetDualEdgeDonor();
  std::array<float,k1::contract::kSpectrumBinCount> mags{};
  // One loud pitch class. A flat field is rejected by the sparseness gate and would paint black.
  for(unsigned i=0;i<mags.size();++i) if(i%12U==3U) mags[i]=12.0F;
  view.visual_magnitudes=&mags;
  pitch(features,3,7);
  t=40000000;
  drive(80, t, view, ca, cb, a, b);
  view.visual_magnitudes=nullptr;
  const auto seeded=dualEdgeDonorContinuity();
  if(!(seeded.visual_gain<0.7F&&seeded.visual_gain>0.2F&&seeded.donor_next_us!=0&&sum(a)>100000ULL)){
   std::fprintf(stderr,"gain seed %g next %llu sum %llu\n",seeded.visual_gain,(unsigned long long)seeded.donor_next_us,sum(a));
   assert(false);
  }
  const auto jump=t+40000ULL;
  view.source_publication_us=jump;
  renderDualEdgeDonor(jump,&view,ca,cb,a,b);
  const auto jumped=dualEdgeDonorContinuity();
  if(!(jumped.donor_next_us==seeded.donor_next_us+4ULL*8631ULL&&jumped.donor_next_us!=jump&&jumped.visual_gain==seeded.visual_gain)){
   std::fprintf(stderr,"jump next %llu expected %llu now %llu gain %g seeded %g\n",
    (unsigned long long)jumped.donor_next_us,(unsigned long long)(seeded.donor_next_us+4ULL*8631ULL),
    (unsigned long long)jump,jumped.visual_gain,seeded.visual_gain);
   assert(false);
  }
  const auto before_fault=dualEdgeDonorContinuity();
  const auto picture=sum(a);
  const Pixel16 fault_centre=a[80];
  runtime.resetAudioState();
  const auto faulted=dualEdgeDonorContinuity();
  if(!(faulted.donor_next_us==before_fault.donor_next_us&&faulted.phase_next_us==before_fault.phase_next_us
       &&faulted.hue_position==before_fault.hue_position&&faulted.hue_shift_speed==before_fault.hue_shift_speed
       &&faulted.visual_gain==before_fault.visual_gain&&faulted.donor_next_us!=0&&faulted.visual_gain!=1.0F)){
   std::fprintf(stderr,"fault rebuilt next %llu/%llu phase %llu/%llu hue %g/%g speed %g/%g gain %g/%g\n",
    (unsigned long long)faulted.donor_next_us,(unsigned long long)before_fault.donor_next_us,
    (unsigned long long)faulted.phase_next_us,(unsigned long long)before_fault.phase_next_us,
    faulted.hue_position,before_fault.hue_position,faulted.hue_shift_speed,before_fault.hue_shift_speed,
    faulted.visual_gain,before_fault.visual_gain);
   assert(false);
  }
  // Same stamp inside the window is invalid after the fault, so it must not deposit.
  pitch(features,9,1);
  view.source_publication_us=jump;
  const auto rejected_now=jump+20000ULL;
  renderDualEdgeDonor(rejected_now,&view,ca,cb,a,b);
  const Pixel16 after_reject=a[80];
  // Stopping the stamp lets the centre go dark. That hole can sit closer to the
  // new pitch without being a deposit. A real stamp would stay bright and near it.
  const auto reject_far=dist(after_reject,pitch_b);
  const auto reject_energy=energy(after_reject);
  // A different stamp older than 30000 µs is stale even though it is not the rejected one.
  pitch(features,5,11);
  const auto stale_now=rejected_now+45000ULL;
  view.source_publication_us=stale_now-45000ULL;
  renderDualEdgeDonor(stale_now,&view,ca,cb,a,b);
  const auto fault_stale_close=toward(after_reject,a[80],pitch_c);
  for(unsigned i=0;i<4;++i){ renderDualEdgeDonor(stale_now+16667ULL*(i+1ULL),nullptr,ca,cb,a,b); }
  const auto suspended=dualEdgeDonorContinuity();
  if(!(reject_far*2>dist(fault_centre,pitch_b)&&reject_energy*2<energy(fault_centre)
       &&fault_stale_close<250&&sum(a)>picture/2&&energy(a[120])>500
       &&suspended.visual_gain==before_fault.visual_gain&&suspended.hue_position==before_fault.hue_position
       &&suspended.hue_shift_speed==before_fault.hue_shift_speed&&suspended.phase_next_us==before_fault.phase_next_us
       &&suspended.donor_next_us!=0&&suspended.donor_next_us!=stale_now)){
   std::fprintf(stderr,"suspend far=%u from=%u e=%u/%u stale_close=%u trail=%u sum=%llu/%llu gain %g/%g hue %g/%g next %llu\n",
    reject_far,dist(fault_centre,pitch_b),reject_energy,energy(fault_centre),fault_stale_close,energy(a[120]),
    sum(a),picture,suspended.visual_gain,before_fault.visual_gain,
    suspended.hue_position,before_fault.hue_position,(unsigned long long)suspended.donor_next_us);
   assert(false);
  }
  const Pixel16 return_centre=a[80];
  const Pixel16 return_end=a[149];
  pitch(features,9,1);
  t=stale_now+16667ULL*5ULL;
  drive(3, t, view, ca, cb, a, b);
  const auto returned=dualEdgeDonorContinuity();
  if(!(dist(a[80],return_centre)>800&&dist(a[149],return_end)<dist(a[149],a[80])
       &&returned.visual_gain==before_fault.visual_gain&&returned.visual_gain<0.85F
       &&energy(a[80])>500&&sum(a)>picture/5)){
   std::fprintf(stderr,"return move=%u end_keep=%u end_vs_centre=%u gain %g e=%u sum=%llu\n",
    dist(a[80],return_centre),dist(a[149],return_end),dist(a[149],a[80]),returned.visual_gain,energy(a[80]),sum(a));
   assert(false);
  }
 }
 std::puts("DONOR_PASS continuous160 split80_80 selected_scene GRB48 gain_once centre_history silence_decay aurora_fill aurora_retain shift_scale freshness");
}
