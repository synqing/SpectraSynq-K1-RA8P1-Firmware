#include "inhouse_audio.h"
#include "k1_poly24.h"
#include "core/visual/inhouse_waveform.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
using namespace k1::core::visual;
static k1::titan::InhouseAudio paired, guarded;
static k1_poly24_t fir;
static InhouseWaveformRenderer renderer;
static k1_audio_hop_t hop(unsigned sequence, float raw_peak) {
  k1_audio_hop_t h{};
  h.contract_version=2; h.samples=180; h.analysis_rate_hz=24000;
  h.stream_epoch=1; h.hop_sequence=sequence;
  h.analysis_begin=(sequence-1)*180; h.analysis_end_exclusive=sequence*180;
  for(unsigned i=0;i<180;++i) h.pcm[i]=int16_t(28000*std::sin((i+sequence*180)*.1));
  h.paired_level={2,1,1,sequence,raw_peak*.7F,raw_peak};
  return h;
}
int main() {
  // Independent DC oracle: a unity-gain FIR must retain the raw constant,
  // even though the protected stream contains a different constant.
  for(int raw_value:{-12000,0,1000}) {
    k1_poly24_reset(&fir,41422);
    int16_t raw[296], protected_pcm[296];
    std::fill_n(raw,296,int16_t(raw_value)); std::fill_n(protected_pcm,296,int16_t(29000));
    unsigned checked=0;
    for(unsigned slot=0;slot<12;++slot) {
      assert(k1_poly24_push_paired(&fir,protected_pcm,raw,296,1)==k1_poly24_ok);
      k1_poly24_paired_hop_t p{};
      while(k1_poly24_pull_paired(&fir,&p)==k1_poly24_ok) if(slot>2) {
        const float expected=std::fabs(float(raw_value))/32768.0F;
        assert(std::fabs(p.pre_guard_peak-expected)<2e-6F);
        assert(std::fabs(p.pre_guard_rms-expected)<2e-6F); ++checked;
      }
    }
    assert(checked>5);
  }
  // The protected PCM is identical throughout. Only the independently carried
  // source peak falls. The musical envelope must preserve that fall.
  k1::titan::InhouseAudioConfig cfg{}; cfg.analysis_rate_hz=24000;
  assert(paired.configure(cfg)); assert(guarded.configure(cfg));
  float low_peak=0, old_low=0;
  for(unsigned seq=1;seq<=320;++seq) {
    auto h=hop(seq,seq<=300?.8F:.16F); auto old=h; old.paired_level.version=1;
    assert(k1::titan::InhouseAudio::pairedLevelValid(h));
    assert(paired.consume(h,1000000+seq*7500,true));
    assert(guarded.consume(old,1000000+seq*7500,true));
    assert(paired.latest().raw_peak_pcm==guarded.latest().raw_peak_pcm);
    assert(paired.latest().raw_rms==guarded.latest().raw_rms);
    if(seq==320) { low_peak=paired.latest().peak_scaled; old_low=guarded.latest().peak_scaled; }
  }
  assert(low_peak<.4F && old_low>.9F);
  assert(paired.configure(cfg)); assert(guarded.configure(cfg));
  for(unsigned seq=1;seq<=50;++seq) {
    auto h=hop(seq,.1F); h.paired_level.guarded_support=0;
    auto old=h; old.paired_level.version=1;
    assert(paired.consume(h,1000000+seq*7500,true));
    assert(guarded.consume(old,1000000+seq*7500,true));
    assert(paired.latest().peak_scaled==guarded.latest().peak_scaled);
  }
  for(unsigned fault=0;fault<6;++fault) {
    auto h=hop(321,.2F);
    switch(fault) {
      case 0:h.paired_level.pre_guard_peak=-.1F;break;
      case 1:h.paired_level.pre_guard_peak=.01F;break;
      case 2:h.paired_level.pre_guard_peak=std::numeric_limits<float>::quiet_NaN();break;
      case 3:h.paired_level.pre_guard_peak=std::numeric_limits<float>::infinity();break;
      case 4:h.paired_level.pre_guard_peak=std::numeric_limits<float>::max();break;
      case 5:h.paired_level.version=3;break;
    }
    assert(!k1::titan::InhouseAudio::pairedLevelValid(h));
  }
  // June Hybrid's independent source rule: seed the centre, then scroll out.
  // Loudness must not consume that travel. Exercise quiet and strong input,
  // actual colour output, symmetry, full-width transport and quiet drainage.
  InhouseWaveformControls controls{}; controls.mode=32; controls.palette_id=2;
  float totals[2]{};
  for(unsigned strength=0;strength<2;++strength) {
    renderer.reset(); Pixel16 pixels[160]{};
    for(unsigned frame=1;frame<=500;++frame) {
      InhouseWaveformAudio a{}; a.valid=true; a.epoch=1; a.generation=frame;
      a.capture_us=a.publish_us=frame*10000; a.silence=frame>200;
      a.analysis_begin_sample=(frame-1)*180; a.analysis_end_sample_exclusive=frame*180;
      a.peak_scaled=1; a.vu_level=strength?.65F:.1F;
      a.unguarded_vu_valid=true; a.unguarded_vu=strength?2.0F:.1F;
      a.linear_shape[20]=.4F; a.linear_shape[24]=.2F;
      assert(renderer.render(a,controls,frame*10000,pixels,160));
      if(frame<=200) assert(renderer.observation().position==80.0F);
      for(unsigned i=0;i<80;++i) {
        assert(pixels[i].red==pixels[159-i].red);
        assert(pixels[i].green==pixels[159-i].green);
        assert(pixels[i].blue==pixels[159-i].blue);
      }
      if(frame==200) {
        assert(pixels[79].red+pixels[79].green+pixels[79].blue>0);
        assert(pixels[150].red+pixels[150].green+pixels[150].blue>0);
        for(const auto& px:pixels) totals[strength]+=px.red+px.green+px.blue;
      }
    }
    for(const auto& px:pixels) assert(px.red==0 && px.green==0 && px.blue==0);
  }
  // June Hybrid uses relative peak/chroma brightness. These two cases have
  // identical peak and zero chroma: changing admitted absolute level alone
  // must not introduce the retired triangular-kernel attenuation.
  assert(totals[1]==totals[0] && totals[0]>0);
  std::printf("source_peak_fall=%.6f guarded_peak=%.6f weak_light=%.0f strong_light=%.0f\n",low_peak,old_low,totals[0],totals[1]);
}
