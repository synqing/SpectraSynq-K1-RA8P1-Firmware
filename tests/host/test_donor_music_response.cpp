// Actual permitted trance PCM -> production AP -> donor -> split GRB48 packer.
// Host response regression only: this cannot establish optical/aesthetic success.
// Usage: test_donor_music_response pcm_s16le_24000 label [--poison-spectrum]
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include "core/audio/audio_pipeline.h"
#include "core/visual/visual_audio_frame.h"
#include "k1_dual_edge_donor.h"
#include "palette_runtime.h"

using namespace k1::core::audio;
using namespace k1::core::visual;
using namespace k1::titan;
namespace {
constexpr std::uint32_t kRate=24000, kHop=180;
constexpr std::uint64_t kHopUs=7500, kFrameUs=16667;
std::uint64_t hashBytes(std::uint64_t h, const void* p, std::size_t n) {
  const auto* b=static_cast<const unsigned char*>(p);
  while(n--) { h^=*b++; h*=1099511628211ULL; }
  return h;
}
unsigned word(const unsigned char* p) { return (unsigned(p[0])<<8)|p[1]; }
struct Metrics {
  unsigned frames=0, dark=0, maximum_span=0, max_channel=0;
  unsigned clipped_bins=0, ap_hops=0;
  std::uint64_t occupied_pixels=0, peak_frame_sum=0, last_sum=0;
  std::uint64_t output_hash=1469598103934665603ULL;
  std::uint64_t ap_hash=1469598103934665603ULL;
  std::set<std::uint64_t> frame_hashes;
  std::set<unsigned> colour_bins;
  void observe(const unsigned char* wire) {
    unsigned first=160,last=0,occupied=0;
    std::uint64_t channels[3]={0,0,0};
    std::uint64_t total=0;
    for(unsigned i=0;i<160;++i) {
      const auto* p=wire+i*6;
      const unsigned g=word(p),r=word(p+2),b=word(p+4);
      const unsigned mx=std::max({r,g,b});
      max_channel=std::max(max_channel,mx);
      channels[0]+=r;channels[1]+=g;channels[2]+=b;
      total+=r+g+b;
      // 1% of mandated100/255 ceiling; a visible-drive descriptor, not photons.
      if(mx>=257) {first=std::min(first,i);last=i;++occupied;}
    }
    ++frames; if(total==0)++dark;
    occupied_pixels+=occupied;
    if(occupied)maximum_span=std::max(maximum_span,last-first+1);
    peak_frame_sum=std::max(peak_frame_sum,total);last_sum=total;
    const auto h=hashBytes(1469598103934665603ULL,wire,960);
    frame_hashes.insert(h);output_hash=hashBytes(output_hash,wire,960);
    // Chromaticity bins ignore brightness: uniform gain cannot pass variation.
    if(total>0) {
      const unsigned r=unsigned(channels[0]*32/total);
      const unsigned g=unsigned(channels[1]*32/total);
      colour_bins.insert(r*33+g);
    }
  }
};
}
int main(int argc,char** argv) {
  if(argc<3) {std::fprintf(stderr,"usage: %s pcm24000 label [--poison-spectrum]\n",argv[0]);return 2;}
  const bool poison=argc>3 && std::string(argv[3])=="--poison-spectrum";
  std::ifstream f(argv[1],std::ios::binary|std::ios::ate);
  if(!f){std::fprintf(stderr,"cannot open PCM\n");return 2;}
  const auto bytes=f.tellg();
  if(bytes<=0||std::uint64_t(bytes)%2){std::fprintf(stderr,"invalid PCM\n");return 2;}
  std::vector<std::int16_t> pcm(std::size_t(bytes)/2);f.seekg(0);
  f.read(reinterpret_cast<char*>(pcm.data()),bytes);
  unsigned input_peak=0;for(auto s:pcm)input_peak=std::max(input_peak,unsigned(std::abs(int(s))));
  if(input_peak<26000||input_peak>26400){std::fprintf(stderr,"fixture must have0.8 peak\n");return 2;}
  AudioPipeline pipeline;
  AudioPipelineOutput latest;
  GdftSampleWindow window{};
  VisualWaveformHistory waveform{};
  PaletteRuntime runtime;
  PaletteConfig config;config.mode_a=config.mode_b=8;config.flags=5;
  config.brightness=100;config.output_channel=0;
  if(!runtime.configure(config,0))return 2;
  runtime.channel(0).controls().mood=.35F;
  runtime.channel(0).controls().vp_bloom_alpha=.99F;
  runtime.channel(0).controls().brightness=255;
  runtime.channel(1).controls().chroma=.033325F;
  runtime.channel(1).controls().brightness=255;
  resetDualEdgeDonor();
  std::array<unsigned char,960> wire{};
  const std::uint64_t end_us=pcm.size()*1000000ULL/kRate;
  std::uint64_t ap_us=kHopUs,frame_us=kFrameUs;
  std::size_t cursor=0;std::uint32_t sequence=0;
  Metrics m;
  while(frame_us<=end_us) {
    if(ap_us<=frame_us && cursor+kHop<=pcm.size()) {
      std::memmove(window.data(),window.data()+kHop,(window.size()-kHop)*sizeof(window[0]));
      std::memcpy(window.data()+window.size()-kHop,pcm.data()+cursor,kHop*sizeof(window[0]));
      float peak=0,energy=0;
      for(unsigned i=0;i<kHop;++i) {const float v=float(pcm[cursor+i])/32768.0F;peak=std::max(peak,std::fabs(v));energy+=v*v;}
      AudioPipelineInput input;
      input.samples=&window;input.sequence=++sequence;
      input.frame_ms=std::uint32_t(ap_us/1000);input.capture_time_us=ap_us;input.publish_time_us=ap_us;
      input.peak_scaled=peak;input.vu_level=std::sqrt(energy/kHop);input.silence=peak==0;
      input.media_time={1,std::uint64_t(cursor+kHop)*2};input.media_time_valid=true;
      latest=pipeline.process(input);
      pushVisualWaveform(waveform,pcm.data()+cursor,kHop,peak*32768.0F,peak,sequence);
      if(ap_us>=2000000) {
        ++m.ap_hops;
        for(unsigned i=0;i<60;++i)m.clipped_bins+=latest.features.spectrum[i]>=1.0F;
        m.ap_hash=hashBytes(m.ap_hash,&latest.features,sizeof latest.features);
      }
      if(poison)std::fill(std::begin(latest.features.spectrum),std::end(latest.features.spectrum),1.0F);
      cursor+=kHop;ap_us+=kHopUs;
    } else {
      VisualAudioFrameView view{latest.features,latest.tempo,waveform,
        latest.features.publish_time_us,&latest.tempo_field,&latest.visual_magnitudes};
      if(!runtime.step(frame_us,&view)) {std::fprintf(stderr,"render schedule failed\n");return 2;}
      if(runtime.packBenchGrb48Lane(wire.data(),480,0)!=480 ||
         runtime.packBenchGrb48Lane(wire.data()+480,480,1)!=480)return 2;
      if(frame_us>=2000000)m.observe(wire.data());
      frame_us+=kFrameUs;
    }
  }
  // A music-conditioned field must release during silence; no constant body.
  for(unsigned i=0;i<1200;++i) {
    runtime.step(frame_us,nullptr);
    frame_us+=kFrameUs;
  }
  runtime.packBenchGrb48Lane(wire.data(),480,0);
  runtime.packBenchGrb48Lane(wire.data()+480,480,1);
  std::uint64_t silence_sum=0;for(unsigned i=0;i<480;++i)silence_sum+=word(wire.data()+i*2);
  const double dark_fraction=m.frames?double(m.dark)/m.frames:1;
  const bool pass=m.frames>=100 && dark_fraction<.10 && m.frame_hashes.size()>100 &&
    m.colour_bins.size()>=2 && m.maximum_span>=80 && m.max_channel<=25700 &&
    m.peak_frame_sum>0 && silence_sum<m.peak_frame_sum/100;
  std::printf("{\"track\":\"%s\",\"poison_spectrum\":%s,\"frames\":%u,\"dark_frames\":%u,\"dark_fraction\":%.6f,\"unique_frames\":%zu,\"chromaticity_bins\":%zu,\"mean_occupied_pixels\":%.3f,\"maximum_span\":%u,\"maximum_channel\":%u,\"peak_frame_sum\":%llu,\"silence_sum\":%llu,\"legacy_clipped60_fraction\":%.6f,\"output_hash\":\"%016llx\",\"ap_hash\":\"%016llx\",\"pass\":%s}\n",
    argv[2],poison?"true":"false",m.frames,m.dark,dark_fraction,m.frame_hashes.size(),m.colour_bins.size(),
    m.frames?double(m.occupied_pixels)/m.frames:0,m.maximum_span,m.max_channel,
    (unsigned long long)m.peak_frame_sum,(unsigned long long)silence_sum,
    m.ap_hops?double(m.clipped_bins)/(m.ap_hops*60):0,
    (unsigned long long)m.output_hash,(unsigned long long)m.ap_hash,pass?"true":"false");
  return pass?0:1;
}
