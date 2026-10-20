// Paired material after the rejected Cross-Strip field. Lightwave Lorenz Ribbon
// retained distance kernels and Rose Bloom multi-timescale response inform this
// centre-transport adaptation. Pin60009324; no donor-frame parity is claimed.
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
  lorenz_x_=-8;lorenz_y_=-9;lorenz_z_=25;formation_=deposit_clock_=impact_=0;
  knots_={};frame_={};telemetry_={};initialised_=true;have_sequence_=have_attack_=false;
}

bool PairedWaveScene::configure(const PairedWaveSettings& s,std::uint64_t time) noexcept {
  if(!bounded(s.width_px,3,14)||!bounded(s.velocity_px_s,4,80)||
     !bounded(s.edge_delay_s,0,.5F)||!bounded(s.articulation,0,1)||
     !bounded(s.spread_px2_s,0,10)||!bounded(s.half_life_s,.2F,8)||
     !bounded(s.gap_s,.05F,2)||!bounded(s.return_strength,0,.45F)||
     s.palette_id>=kProductPaletteCount||!advance(time)) return false;
  settings_=s;return true;
}

void PairedWaveScene::deposit(float strength) noexcept {
  if(strength<.006F)return;
  for(auto& knot:knots_)if(!knot.active) {
    knot=Knot{};knot.active=true;knot.amplitude=unit(strength);
    knot.width=settings_.width_px*(.75F+.45F*timbre_+.25F*formation_);
    knot.delay=settings_.edge_delay_s;
    knot.top_leads=lorenz_x_<0;
    knot.edge_balance=unit(.5F+lorenz_x_/36.F);
    knot.top=ribbonPalette(settings_.palette_id,colour_position_);
    knot.bottom=ribbonPalette(settings_.palette_id,std::min(.86F,colour_position_+.30F));
    return;
  }
  ++telemetry_.omitted; // Preserve already travelling material when full.
}

void PairedWaveScene::step(float dt,bool fresh) noexcept {
  const float target=fresh?energy_:0;
  envelope_+=(target-envelope_)*(1-std::exp(-dt/(target>envelope_?.04F:.42F)));
  impact_*=std::exp(-dt/.14F);
  // Lorenz derivative from the named Lightwave material, integrated in real
  // seconds with bounded substeps. Its trajectory shapes NEW material only.
  // Unlike the source, the trajectory never sets an on-strip position.
  const float h=dt*(.65F+.55F*timbre_);
  const float dx=10*(lorenz_y_-lorenz_x_);
  const float dy=lorenz_x_*(28-lorenz_z_)-lorenz_y_;
  const float dz=lorenz_x_*lorenz_y_-(8.F/3.F)*lorenz_z_;
  const float mx=lorenz_x_+.5F*h*dx,my=lorenz_y_+.5F*h*dy,mz=lorenz_z_+.5F*h*dz;
  lorenz_x_+=h*10*(my-mx);lorenz_y_+=h*(mx*(28-mz)-my);lorenz_z_+=h*(mx*my-(8.F/3.F)*mz);
  const float opening=unit((std::abs(lorenz_x_)-3.F)/10.F);
  const float contour=opening*opening*(3-2*opening);
  formation_+=(contour-formation_)*(1-std::exp(-dt/.16F));
  const float velocity=settings_.velocity_px_s*(.65F+.35F*envelope_+.15F*formation_);
  phase_+=velocity*dt; // Accumulated physical transport, never reset by attacks.
  const float retention=std::exp(-dt*.69314718056F/settings_.half_life_s);
  for(auto& knot:knots_)if(knot.active) {
    const float age=knot.age;knot.age+=dt;
    knot.amplitude*=retention;
    if(settings_.spread_px2_s>0)knot.width=std::sqrt(knot.width*knot.width+2*settings_.spread_px2_s*dt);
    for(unsigned edge=0;edge<2;++edge) {
      auto& state=knot.edges[edge];if(state.done)continue;
      const float delay=((edge==0)==knot.top_leads)?0:knot.delay;
      float remaining=std::max(0.F,knot.age-delay)-std::max(0.F,age-delay);
      for(unsigned part=0;part<3&&remaining>0&&!state.done;++part) {
        if(state.waiting) {
          const float spent=std::min(remaining,state.quiet);remaining-=spent;state.quiet-=spent;
          if(state.quiet<=1e-6F){state.waiting=false;state.returning=true;state.radius=79;}
        } else {
          const float support=2.5F*knot.width;
          const float distance=state.returning?state.radius+support:79+support-state.radius;
          const float spent=std::min(remaining,std::max(0.F,distance)/velocity);
          state.radius+=(state.returning?-1:1)*velocity*spent;remaining-=spent;
          if(spent>=std::max(0.F,distance)/velocity) {
            if(state.returning)state.done=true;
            else {state.waiting=true;state.quiet=settings_.gap_s;}
          }
        }
      }
    }
    if(knot.age>5.8F||(knot.edges[0].done&&knot.edges[1].done))knot.active=false;
  }
  deposit_clock_+=dt;
  if(deposit_clock_>=1.F/24.F-1e-7F) {
    deposit_clock_=0;
    if(fresh && energy_>0)deposit(2.2F*envelope_*formation_);
  }
}

bool PairedWaveScene::advance(std::uint64_t time) noexcept {
  if(!initialised_)reset(time);
  if(time<time_us_)return false;
  double remaining=double(time-time_us_)*1e-6;
  // Beyond the entire material lifetime all old knots are extinct. Missing
  // observations cannot create replacement material during the gap.
  if(remaining>6) {knots_={};envelope_=impact_=0;remaining=.25;time_us_=time-250000;}
  while(remaining>1e-9) {
    // Split at formation events as well as the integration bound: a display
    // cadence change must not shift deposits to the next presentation frame.
    const float dt=float(std::min({remaining,1.0/240,double(1.F/24.F-deposit_clock_)}));
    const bool fresh=have_sequence_&&time_us_>=publication_us_&&time_us_-publication_us_<=200000;
    step(dt,fresh);remaining-=dt;time_us_+=std::uint64_t(std::llround(dt*1e6));
  }
  time_us_=time;return true;
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
  const float colour_target=.22F+.20F*timbre_;
  colour_position_+=std::clamp(colour_target-colour_position_,-.08F*dt,.08F*dt);
  const bool onset=(a.validity_flags&contract::kValidOnsetV2)&&a.onset_event_id&&
    a.onset_event_id!=onset_&&a.onset_event_age_ms<=200;
  const bool energy_attack=energy_>previous_energy_+.12F;
  previous_energy_=energy_;if(onset)onset_=a.onset_event_id;
  if(energy_<=0||(!onset&&!energy_attack))return;
  if(have_attack_&&time-last_attack_us_<140000){++telemetry_.omitted;return;}
  impact_=settings_.articulation*(.35F+.65F*energy_);
  deposit(std::max(impact_,energy_)); // Immediate centre response; no sampler wait.
  last_attack_us_=time;have_attack_=true;++telemetry_.attacks;
}

const PairedFrame16& PairedWaveScene::render(std::uint64_t time) noexcept {
  frame_={};telemetry_.frame_valid=advance(time);if(!telemetry_.frame_valid)return frame_;
  telemetry_.envelope=envelope_;telemetry_.transport_px=float(phase_);
  std::array<RibbonRgb,80> fields[2]{};float maximum=1;
  for(unsigned edge=0;edge<2;++edge)for(unsigned d=0;d<80;++d) {
    RibbonRgb rgb[2]{};float weights[2]{},strongest[2]{};
    for(const auto& knot:knots_)if(knot.active) {
      const auto& state=knot.edges[edge];
      const bool lead=(edge==0)==knot.top_leads;
      if(state.done||state.waiting||(!lead&&knot.age<knot.delay))continue;
      const float x=std::abs(float(d)-state.radius)/(2.5F*knot.width);
      if(x>=1)continue;
      // Retained distance kernels and a narrower same-colour core, from the
      // Lorenz Ribbon material idea. Compact polynomial support keeps true
      // negative space and avoids thousands of transcendental calls per frame.
      const float q=1-x*x,body=q*q*q;
      const float shape=.65F*body+.35F*body*body*body;
      const float balance=edge==0?.55F+.45F*knot.edge_balance:1.F-.45F*knot.edge_balance;
      const float retirement=unit(5.8F-knot.age);
      const float weight=knot.amplitude*shape*balance*retirement*retirement*(3-2*retirement);
      const unsigned layer=state.returning?1:0;
      add(rgb[layer],scaled(edge==0?knot.top:knot.bottom,weight));
      weights[layer]+=weight;strongest[layer]=std::max(strongest[layer],weight);
    }
    // Age-weighted maxima retain broad material without making deposit cadence
    // a brightness multiplier. Weighted colour preserves deposited ownership.
    for(unsigned layer=0;layer<2;++layer)if(weights[layer]>0) {
      const float scale=strongest[layer]/weights[layer]*(layer==0?1.F:settings_.return_strength);
      add(fields[edge][d],scaled(rgb[layer],scale));
    }
    maximum=std::max(maximum,peak(fields[edge][d]));
  }
  telemetry_.limiter_gain=1/maximum;const float gain=24.F/255.F/maximum;
  for(unsigned d=0;d<80;++d) {
    const auto p=mirroredPair(d);const auto a=scaled(fields[0][d],gain),b=scaled(fields[1][d],gain);
    frame_.top[p.left]=frame_.top[p.right]={code(a.red),code(a.green),code(a.blue)};
    frame_.bottom[p.left]=frame_.bottom[p.right]={code(b.red),code(b.green),code(b.blue)};
  }
  return frame_;
}
} // namespace k1::core::visual
