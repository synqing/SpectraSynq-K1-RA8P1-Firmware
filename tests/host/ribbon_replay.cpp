// THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Offline, never opens hardware.
#include "k1_live_runtime.h"
#include "core/visual/product_effect_renderer.h"
#include "core/visual/ribbon_profile.h"
#include "core/visual/tempo_reference_scene.h"
#include "../reference/bloom_donors.h"
#include "../reference/cross_strip_reference.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>

namespace v=k1::core::visual;
using Frame=v::RibbonFrame16;
constexpr unsigned lanes=5, fps=120;
Frame encode(const v::RibbonFrame& f) {
  float peak=1; for(auto p:f) peak=std::max({peak,p.red,p.green,p.blue});
  Frame out{}; for(unsigned i=0;i<160;++i) {
    auto q=[&](float x){return static_cast<std::uint16_t>(std::lround(std::clamp(x/peak,0.F,1.F)*65535.F*24.F/255.F));};
    out[i]={q(f[i].red),q(f[i].green),q(f[i].blue)};
  } return out;
}
double total(const Frame& f) {double s=0;for(auto p:f)s+=p.red+p.green+p.blue;return s;}
void write(FILE* file,const Frame& f) {
  for(auto p:f) for(auto c:{p.red,p.green,p.blue}) {
    unsigned char b[]{static_cast<unsigned char>(c),static_cast<unsigned char>(c>>8)};
    if(std::fwrite(b,1,2,file)!=2) throw std::runtime_error("Frame write failed");
  }
}
int main(int argc,char** argv) try {
  std::string input,output; v::RibbonSettings settings; bool synthetic=false; float liveiness=.5F; v::PairedWaveSettings paired_settings;
  for(int i=1;i<argc;++i) {
    const std::string key=argv[i]; if(i+1>=argc)throw std::runtime_error("Missing argument");
    const std::string value=argv[++i];
    if(key=="--pcm") input=value; else if(key=="--output") output=value;
    else if(key=="--synthetic") synthetic=value=="1";
    else { const float x=std::stof(value);
      if(key=="--pair_spread")paired_settings.spread_px2_s=x; else if(key=="--pair_lifetime")paired_settings.half_life_s=x; else if(key=="--pair_gap")paired_settings.gap_s=x; else if(key=="--pair_return")paired_settings.return_strength=x; else if(key=="--pair_width")paired_settings.width_px=x; else if(key=="--pair_velocity")paired_settings.velocity_px_s=x; else if(key=="--pair_delay")paired_settings.edge_delay_s=x; else if(key=="--pair_articulation")paired_settings.articulation=x; else if(key=="--liveiness")liveiness=x; else if(key=="--velocity")settings.velocity_px_s=x; else if(key=="--spread")settings.spread_px2_s=x;
      else if(key=="--lifetime")settings.half_life_s=x; else if(key=="--width")settings.width_px=x;
      else if(key=="--spacing")settings.launch_spacing_s=x; else if(key=="--gap")settings.gap_s=x;
      else if(key=="--return")settings.return_strength=x; else if(key=="--injection")settings.injection_response=x;
      else if(key=="--persistence")settings.persistence_s=x; else if(key=="--palette") {if(!std::isfinite(x)||x<0||x>=v::kProductPaletteCount||std::floor(x)!=x)throw std::runtime_error("Palette must be a catalogue integer");settings.palette_id=static_cast<std::uint16_t>(x);}
      else if(key=="--recolour")settings.recolour_history=x!=0; else if(key=="--knee")settings.knee=x!=0;
      else throw std::runtime_error("Unknown option");
    }
  }
  if(output.empty() || (!synthetic && input.empty()) || !v::validRibbonSettings(settings))throw std::runtime_error("Invalid configuration");
  for(const char* suffix:{".rgb16le",".matched.rgb16le",".json",".native-grb16",".paired.rgb16le",".paired.matched.rgb16le",".paired.native-grb16"})
    if(std::filesystem::exists(output+suffix))throw std::runtime_error("Refusing to overwrite export");
  std::filesystem::create_directories(std::filesystem::path(output).parent_path());
  std::vector<std::int16_t> pcm;
  if(!synthetic) {
    FILE* f=std::fopen(input.c_str(),"rb");if(!f)throw std::runtime_error("PCM open failed");
    unsigned char b[2];std::size_t n;
    while((n=std::fread(b,1,2,f))) {if(n!=2)throw std::runtime_error("Odd PCM bytes");pcm.push_back(static_cast<std::int16_t>(b[0]|unsigned(b[1])<<8));}
    std::fclose(f); if(pcm.empty() || pcm.size()%180)throw std::runtime_error("Whole 180-sample hops required");
  }
  FILE* frames=std::fopen((output+".rgb16le").c_str(),"wb");
  FILE* matched=std::fopen((output+".matched.rgb16le").c_str(),"wb");
  FILE* wire=std::fopen((output+".native-grb16").c_str(),"wb");
  FILE* meta=std::fopen((output+".json").c_str(),"w");
  if(!frames||!matched||!wire||!meta)throw std::runtime_error("Output open failed");
  FILE* paired_file=std::fopen((output+".paired.rgb16le").c_str(),"wb");
  FILE* paired_match=std::fopen((output+".paired.matched.rgb16le").c_str(),"wb");
  FILE* paired_wire=std::fopen((output+".paired.native-grb16").c_str(),"wb");
  if(!paired_file||!paired_match||!paired_wire)throw std::runtime_error("Paired output open failed");
  paired_settings.palette_id=settings.palette_id;
  v::PairedWaveScene paired; if(!paired.configure(paired_settings,0))throw std::runtime_error("Paired settings rejected");
  k1::reference::CrossStripReference cross_strip;
  double pair_energy[3]{},pair_occupancy[3]{},pair_peak[3]{}; unsigned pair_limited=0;
  const std::uint64_t duration=synthetic?60000000ULL:pcm.size()*1000000ULL/24000;
  const auto count=(duration*fps+999999)/1000000+1;
  k1::titan::LiveAudioRuntime audio; audio.initialise(1000000000U);
  v::ChannelRenderState incumbent(v::PixelChannelId::kChannelA);
  auto& controls=incumbent.controls();controls.mode_id=3;controls.palette_id=settings.palette_id;
  controls.palette_mode_enabled=true;controls.brightness=255;controls.photons_id=65535;controls.mood=.2F;
  v::RibbonProfile ribbon; if(!ribbon.configure(settings,0) || !ribbon.setLiveiness(liveiness,0))throw std::runtime_error("Settings rejected");
  k1::reference::SensoryRgbBloom sb; k1::reference::EmotiscopeScalarBloom emo;
  k1::core::audio::AudioPipelineOutput synthetic_output{};
  std::uint64_t seq=0,donor_tick=0;std::size_t offset=0;unsigned previous_case=99;
  double energies[lanes]{}, occupied[lanes]{}, peaks[lanes]{};unsigned limited=0;
  std::fprintf(meta,"{\"schema\":\"k1-ribbon-comparison.v1\",\"fps\":120,\"count\":%llu,\"pixels\":160,\"display_ceiling\":6168,\"lanes\":[\"SB 4.1.1 RGB-history kernel / K1 input adapter\",\"Emotiscope 1.1 scalar-history kernel / K1 input adapter\",\"Incumbent K1 Bloom / native Pixel8\",\"K1 ribbon / float to native16\",\"AP diagnostic / unchanged\"],\"donor_cadence_hz\":100,\"cadence_note\":\"Chosen fixed 100 calls/s; Emotiscope reference FPS, not a claim of measured donor cadence\",\"frames\":[",static_cast<unsigned long long>(count));
  for(std::uint64_t frame=0;frame<count;++frame) {
    const std::uint64_t now=frame*1000000ULL/fps;
    float signal=0;unsigned scenario=std::min(5U,unsigned(now/10000000)), tick=unsigned((now%10000000)*fps/1000000);
    if(synthetic) {
      if(scenario!=previous_case) {sb={};emo={};cross_strip={};paired.reset(now);ribbon.reset(now);ribbon.configure(settings,now);const auto saved_controls=incumbent.controls();incumbent=v::ChannelRenderState(v::PixelChannelId::kChannelA);incumbent.controls()=saved_controls;previous_case=scenario;}
      signal=(tick>=60 && tick<72)?1.F:0.F;
      if(scenario==1 && tick>=84 && tick<96)signal=.75F;
      if(scenario==2)signal=tick>60 && tick<540?std::sin((tick-60)*3.14159265F/480):0;
      if(scenario==3)signal=tick>=60 && tick<420?.6F:0;
      if(scenario==4)signal=tick>=60 && tick<120?.8F:0;
      synthetic_output.features={}; auto& a=synthetic_output.features;a.sequence=frame+1;a.publish_time_us=now;
      a.validity_flags=k1::contract::kValidBaseSnapshot|k1::contract::kValidChroma|k1::contract::kValidOnsetV2;
      a.vu_level=signal;a.chroma_a_origin[3]=signal;
      a.event_flags=signal==0?k1::contract::kEventSilence:0;
      // Declared feature sequence drives the same material adapter as PCM.
      if(tick==60 || (scenario==1 && tick==84)) {a.onset_event_id=frame+1;a.onset_strength=signal;}
      if(scenario==5 && tick==120) {auto edited=settings;edited.velocity_px_s*=1.5F;ribbon.configure(edited,now);}
    } else while(offset<pcm.size() && (seq+1)*7500<=now) {
      k1_audio_hop_t hop{};++seq;if(k1_audio_hop_init_interval(&hop,1,seq)!=0)throw std::runtime_error("Hop init");
      std::memcpy(hop.pcm,pcm.data()+offset,360);offset+=180;
      hop.supporting_dma_receipt_us=hop.newest_sample_capture_estimate_us=seq*7500;hop.capture_estimate_valid=1;
      audio.advanceHostCycles(7500000U);if(audio.consume(hop)!=k1::titan::LiveHopStatus::Ok)throw std::runtime_error("AP consume");
    }
    const k1::core::audio::AudioPipelineOutput empty{};
    const auto& published=synthetic?synthetic_output:audio.latest()?*audio.latest():empty;
    const auto saved=published;
    const v::VisualAudioFrameView view{published.features,published.tempo,audio.waveform(),published.features.publish_time_us,&published.tempo_field};
    if(view.source_publication_us>now)throw std::runtime_error("Future publication");
    ribbon.consume(view,now); paired.consume(view,now);
    if(!synthetic)signal=(view.audio.event_flags&k1::contract::kEventSilence)?0:std::clamp(view.audio.vu_level,0.F,1.F);
    const auto palette=(synthetic && scenario==4 && tick>=144)?std::uint16_t(7):settings.palette_id;
    if(palette!=ribbon.settings().palette_id) {auto edit=ribbon.settings();edit.palette_id=palette;ribbon.configure(edit,now);}
    // Mapping explicitly lives outside the donor kernels: K1 VU -> donor VU;
    // palette sample times VU -> donor-ready deposited RGB. No donor AP claim.
    while(donor_tick*10000<=now) {
      const auto colour=v::ribbonPalette(palette,.35F);
      sb.step(k1::reference::scaled(colour,signal),.2F);
      std::array<v::RibbonRgb,64> colours{};colours.fill(colour);emo.step(signal,.2F,colours);++donor_tick;
    }
    incumbent.controls().mode_id=3;incumbent.controls().palette_id=palette;incumbent.controls().palette_mode_enabled=true;incumbent.controls().brightness=255;incumbent.controls().photons_id=65535;
    incumbent.prepareAudio(view.audio);incumbent.clearFrame();
    if(!v::renderProductChannel(incumbent,view,1.F/fps).rendered)throw std::runtime_error("Incumbent rejected");
    v::applyProductOutputTreatment(incumbent.frame(),incumbent.controls(),incumbent.outputTreatmentState());
    std::array<Frame,lanes> images{};images[0]=encode(k1::reference::toK1(sb.frame));images[1]=encode(k1::reference::toK1(emo.frame));
    for(unsigned i=0;i<160;++i) {auto p=incumbent.frame()[i];images[2][i]={std::uint16_t(p.red*257U*24U/255U),std::uint16_t(p.green*257U*24U/255U),std::uint16_t(p.blue*257U*24U/255U)};}
    const auto native=ribbon.render(now); images[3]=native.pixels();
    const auto diagnostic=v::renderTempoReferencePair(view,{1,now*48000/1000000},now);
    for(unsigned i=0;i<160;++i) {float x=diagnostic.candidate[i]*257;images[4][i]={std::uint16_t(x*.12F),std::uint16_t(x),std::uint16_t(x*.72F)};}
    if(std::memcmp(&saved,&published,sizeof(saved)))throw std::runtime_error("AP mutation");
    std::uint8_t a[v::kPackedBytesPerLane]{},b[v::kPackedBytesPerLane]{};
    if(!native.stage(a,sizeof(a),b,sizeof(b)))throw std::runtime_error("Native staging");
    if(std::fwrite(a,1,sizeof(a),wire)!=sizeof(a)||std::fwrite(b,1,sizeof(b),wire)!=sizeof(b))throw std::runtime_error("Native write");
    const std::array<v::PairedFrame16,3> pairs{cross_strip.step(1.F/fps),v::PairedFrame16{images[3],images[3]},paired.render(now)};
    pair_limited+=paired.telemetry().limiter_gain<1;
    double pair_target=1e30;
    for(const auto& p:pairs)if(total(p.top)+total(p.bottom)>0)pair_target=std::min(pair_target,total(p.top)+total(p.bottom));
    if(pair_target==1e30)pair_target=0;
    for(unsigned scene=0;scene<3;++scene) {
      const auto& pair=pairs[scene]; const double energy=total(pair.top)+total(pair.bottom);
      pair_energy[scene]+=energy/65535/fps;
      const double matching=energy>0?std::min(1.,pair_target/energy):0;
      for(const auto* edge:{&pair.top,&pair.bottom}) {
        write(paired_file,*edge); auto adjusted=*edge;
        for(unsigned i=0;i<160;++i) {
          const auto p=(*edge)[i];const auto high=std::max({p.red,p.green,p.blue});
          pair_peak[scene]=std::max(pair_peak[scene],double(high));pair_occupancy[scene]+=high>=62?1:0;
          adjusted[i]={std::uint16_t(p.red*matching),std::uint16_t(p.green*matching),std::uint16_t(p.blue*matching)};
        }
        write(paired_match,adjusted);
      }
    }
    std::uint8_t top[960]{},bottom[960]{};
    if(!v::stagePaired(pairs[2],{},top,sizeof(top),bottom,sizeof(bottom)))throw std::runtime_error("Paired staging");
    if(std::fwrite(top,1,960,paired_wire)!=960||std::fwrite(bottom,1,960,paired_wire)!=960)throw std::runtime_error("Paired native write");
    double target=1e30;for(unsigned l=0;l<4;++l)if(total(images[l])>0)target=std::min(target,total(images[l]));
    if(target==1e30)target=0;
    for(unsigned l=0;l<lanes;++l) {
      write(frames,images[l]);energies[l]+=total(images[l])/65535/fps;
      const double gain=total(images[l])>0?std::min(1.0,target/total(images[l])):0;auto adjusted=images[l];
      for(unsigned i=0;i<160;++i) {
        auto p=images[l][i];const auto peak=std::max({p.red,p.green,p.blue});peaks[l]=std::max(peaks[l],double(peak));occupied[l]+=peak>=62?1:0;
        adjusted[i]={std::uint16_t(p.red*gain),std::uint16_t(p.green*gain),std::uint16_t(p.blue*gain)};
      }write(matched,adjusted);
    }
    const auto& t=ribbon.telemetry();limited+=t.limiter_gain<1;
    std::fprintf(meta,"%s{\"us\":%llu,\"vu\":%.6g,\"phase\":%s,\"outgoing\":%u,\"returning\":%u,\"active\":%u,\"energy\":%.9g}",frame?",":"",(unsigned long long)now,double(signal),published.tempo_field.phase_authority?"true":"false",t.outgoing,t.returning,t.active,total(images[3])/65535);
  }
  std::fprintf(meta,"],\"metrics\":[");for(unsigned l=0;l<lanes;++l)std::fprintf(meta,"%s{\"channel_sum_seconds\":%.12g,\"occupancy_at_1pct_ceiling\":%.8g,\"peak_code\":%.0f}",l?",":"",energies[l],occupied[l]/count/160,peaks[l]);
  std::fprintf(meta,"],\"paired_metrics\":[");
  for(unsigned i=0;i<3;++i)std::fprintf(meta,"%s{\"channel_sum_seconds\":%.12g,\"occupancy_at_1pct_ceiling\":%.8g,\"peak_code\":%.0f}",i?",":"",pair_energy[i],pair_occupancy[i]/count/320,pair_peak[i]);
  const auto& pt=paired.telemetry();
  std::fprintf(meta,"],\"paired_admission\":{\"attempted\":%u,\"admitted\":%u,\"omitted\":%u,\"refractory\":%u,\"bodies_admitted\":%u,\"bodies_omitted\":%u}",pt.attacks_attempted,pt.attacks_admitted,pt.attacks_omitted,pt.attacks_refractory,pt.bodies_admitted,pt.bodies_omitted);
  std::fprintf(meta,",\"paired_candidate_limited_frames\":%u,\"paired_scenes\":[\"Cross-Strip Wave · original kernel\",\"Ribbon · intentionally identical edges\",\"Retained paired material · musical adaptation\"],\"paired_edge_pixels\":160,\"paired_scene_count\":3,\"paired_orientation\":\"logical top and bottom left-to-right; no target pin binding\",\"paired_reference\":{\"phase_offset_cycles\":0.375,\"wavelength_px\":12,\"drift_hz\":0.12,\"speed\":50,\"intensity\":255,\"brightness\":24},\"candidate_limited_frames\":%u,\"ap_nonmutation\":true,\"hops\":%llu,\"physical_execution\":false}\n",pair_limited,limited,(unsigned long long)seq);
  if(std::fclose(frames)||std::fclose(matched)||std::fclose(meta)||std::fclose(wire)||std::fclose(paired_file)||std::fclose(paired_match)||std::fclose(paired_wire))throw std::runtime_error("Close failed");
  std::printf("Exported %llu frames, %llu PCM hops\n",(unsigned long long)count,(unsigned long long)seq);return 0;
} catch(const std::exception& e){std::fprintf(stderr,"ribbon_replay: %s\n",e.what());return 2;}
