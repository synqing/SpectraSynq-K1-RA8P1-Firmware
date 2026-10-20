// Paired radial field inspired by Lightwave CrossStripWaveInterferenceEffect,
// pin60009324. Authored musical adaptation; not a donor-parity implementation.
#include "core/visual/paired_wave_scene.h"
#include <algorithm>
#include <cmath>

namespace k1::core::visual {
namespace {
float unit(float x) noexcept {return std::clamp(x,0.F,1.F);}
bool bounded(float x,float lo,float hi) noexcept {return std::isfinite(x)&&x>=lo&&x<=hi;}
float peak(RibbonRgb c) noexcept {return std::max({c.red,c.green,c.blue});}
RibbonRgb scaled(RibbonRgb c,float x) noexcept {return {c.red*x,c.green*x,c.blue*x};}
void add(RibbonRgb& a,RibbonRgb b) noexcept {a.red+=b.red;a.green+=b.green;a.blue+=b.blue;}
float band(float cycle,float width) noexcept {
  // Rounded triangular crests with real zero-valued troughs. Both edges use
  // the same spatial period; the authored offset makes them complementary.
  const float f=cycle-std::floor(cycle);
  const float tooth=1-std::abs(2*f-1);
  const float x=unit((tooth-(1-width))/width);
  return x*x*(3-2*x);
}
std::uint16_t code(float x) noexcept {return std::uint16_t(std::floor(unit(x)*65535.F+.5F));}
}

bool stagePaired(const PairedFrame16& f,PairedOrientation o,std::uint8_t* a,std::size_t na,
                 std::uint8_t* b,std::size_t nb) noexcept {
  constexpr std::size_t bytes=kPixelsPerChannel*kPackedBytesPerPixel;
  if(!a||!b||na!=bytes||nb!=bytes) return false;
  const auto pa=reinterpret_cast<std::uintptr_t>(a),pb=reinterpret_cast<std::uintptr_t>(b);
  if(pa<=pb?pb-pa<na:pa-pb<nb) return false;
  const auto& top=o.swap_edges?f.bottom:f.top;
  const auto& bottom=o.swap_edges?f.top:f.bottom;
  for(unsigned i=0;i<kPixelsPerChannel;++i) {
    packPixel(top[o.reverse_top?kPixelsPerChannel-1-i:i],a+i*6);
    packPixel(bottom[o.reverse_bottom?kPixelsPerChannel-1-i:i],b+i*6);
  }
  return true;
}

void PairedWaveScene::reset(std::uint64_t time) noexcept {
  time_us_=publication_us_=last_attack_us_=time;sequence_=onset_=0;
  phase_=0;energy_=envelope_=timbre_=previous_energy_=0;colour_position_=.3F;
  accents_={};frame_={};telemetry_={};initialised_=true;have_sequence_=have_attack_=false;
}

bool PairedWaveScene::configure(const PairedWaveSettings& s,std::uint64_t time) noexcept {
  if(!bounded(s.wavelength_px,8,32)||!bounded(s.velocity_px_s,4,80)||
     !bounded(s.edge_offset_cycles,0,.5F)||!bounded(s.articulation,0,1)||
     s.palette_id>=kProductPaletteCount||!advance(time)) return false;
  settings_=s;return true;
}

bool PairedWaveScene::advance(std::uint64_t time) noexcept {
  if(!initialised_)reset(time);
  if(time<time_us_)return false;
  const double dt=double(time-time_us_)*1e-6;time_us_=time;
  // Stale input cannot keep depositing material or freeze a bright field.
  const float target=(have_sequence_ && time>=publication_us_ && time-publication_us_<=200000)?energy_:0;
  const double tau=target>envelope_?.035:.38;
  const double retention=std::exp(-dt/tau);
  // Exact integral for the piecewise-constant envelope target. No frame-count
  // drift, no phase reset, and velocity remains positive (centre outward).
  const double area=target*dt+(envelope_-target)*tau*(1-retention);
  phase_+=(settings_.velocity_px_s/settings_.wavelength_px)*(.45*dt+.55*area);
  phase_-=std::floor(phase_);
  envelope_=float(target+(envelope_-target)*retention);
  for(auto& a:accents_)if(a.active){a.age+=dt;if(a.age>2.6)a.active=false;}
  return true;
}

void PairedWaveScene::consume(const VisualAudioFrameView& view,std::uint64_t time) noexcept {
  if(!advance(time))return;
  const auto& a=view.audio;
  telemetry_.audio_available=(a.validity_flags&contract::kValidBaseSnapshot)&&
    time>=view.source_publication_us&&time-view.source_publication_us<=200000;
  if(!telemetry_.audio_available||(have_sequence_&&sequence_==a.sequence))return;
  if(!std::isfinite(a.vu_level)){telemetry_.audio_available=false;return;}
  const float dt=have_sequence_?float(time-publication_us_)*1e-6F:0;
  sequence_=a.sequence;publication_us_=view.source_publication_us;have_sequence_=true;
  energy_=(a.event_flags&contract::kEventSilence)?0:std::sqrt(unit(a.vu_level));
  const float lo=std::isfinite(a.low_energy)?std::max(0.F,a.low_energy):0;
  const float hi=std::isfinite(a.high_energy)?std::max(0.F,a.high_energy):0;
  const float target=lo+hi>1e-6F?hi/(lo+hi):.5F;
  timbre_+=(target-timbre_)*(1-std::exp(-dt/.7F));
  // Bounded selected-palette movement, never a hue-wheel oscillator.
  const float colour_target=.18F+.26F*timbre_;
  colour_position_+=std::clamp(colour_target-colour_position_,-.08F*dt,.08F*dt);
  const bool onset=(a.validity_flags&contract::kValidOnsetV2)&&a.onset_event_id&&
    a.onset_event_id!=onset_&&a.onset_event_age_ms<=200;
  const bool energy_attack=energy_>previous_energy_+.12F;
  previous_energy_=energy_;
  if(onset)onset_=a.onset_event_id;
  if(energy_<=0||(!onset&&!energy_attack))return;
  if(have_attack_&&time-last_attack_us_<140000){++telemetry_.omitted;return;}
  for(auto& accent:accents_)if(!accent.active) {
    accent={};accent.active=true;accent.top_leads=(telemetry_.attacks%2)==0;
    accent.strength=settings_.articulation*(.35F+.65F*energy_);
    accent.top=ribbonPalette(settings_.palette_id,colour_position_);
    accent.bottom=ribbonPalette(settings_.palette_id,std::min(.88F,colour_position_+.4F));
    last_attack_us_=time;have_attack_=true;++telemetry_.attacks;return;
  }
  ++telemetry_.omitted;
}

const PairedFrame16& PairedWaveScene::render(std::uint64_t time) noexcept {
  frame_={};telemetry_.frame_valid=advance(time);if(!telemetry_.frame_valid)return frame_;
  telemetry_.envelope=envelope_;telemetry_.phase_cycles=float(phase_);
  // Immediate body response plus a slower envelope: attacks have their own
  // moving articulation; held/unmetrical music keeps the field flowing.
  const bool fresh=have_sequence_&&time>=publication_us_&&time-publication_us_<=200000;
  const float body=.48F*(.22F*(fresh?energy_:0)+.78F*envelope_);
  const float width=.30F+.16F*timbre_;
  const auto top_colour=ribbonPalette(settings_.palette_id,colour_position_);
  const auto bottom_colour=ribbonPalette(settings_.palette_id,std::min(.88F,colour_position_+.4F));
  std::array<RibbonRgb,kPixelsPerHalf> top{},bottom{};float maximum=1;
  for(unsigned d=0;d<kPixelsPerHalf;++d) {
    const float cycle=d/settings_.wavelength_px-float(phase_);
    top[d]=scaled(top_colour,body*band(cycle,width));
    bottom[d]=scaled(bottom_colour,body*.82F*band(cycle+settings_.edge_offset_cycles,width));
    for(const auto& a:accents_)if(a.active)for(unsigned edge=0;edge<2;++edge) {
      const bool lead=(edge==0)==a.top_leads;
      const double age=a.age-(lead?0:.13);if(age<0)continue;
      const float radius=float(age)*62.F;
      const float x=(float(d)-radius)/(2.F+float(age)*1.5F);
      if(std::abs(x)>=3)continue;
      const float weight=a.strength*(lead?1.F:.42F)*std::exp(-float(age)*1.4F)*
        std::exp(-.5F*x*x)*std::pow(1-x*x/9,2.F);
      add(edge==0?top[d]:bottom[d],scaled(edge==0?a.top:a.bottom,weight));
    }
    maximum=std::max({maximum,peak(top[d]),peak(bottom[d])});
  }
  telemetry_.limiter_gain=1/maximum;
  const float gain=telemetry_.limiter_gain*24.F/255.F;
  for(unsigned d=0;d<kPixelsPerHalf;++d) {
    const auto p=mirroredPair(d);
    const auto a=scaled(top[d],gain),b=scaled(bottom[d],gain);
    frame_.top[p.left]=frame_.top[p.right]={code(a.red),code(a.green),code(a.blue)};
    frame_.bottom[p.left]=frame_.bottom[p.right]={code(b.red),code(b.green),code(b.blue)};
  }
  return frame_;
}
} // namespace k1::core::visual
