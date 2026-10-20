#include "core/visual/ribbon_material.h"
#include <algorithm>
#include <cmath>

namespace k1::core::visual {
namespace {
constexpr float kLn2 = 0.6931471805599453F;
float unit(float x) noexcept { return std::clamp(x, 0.0F, 1.0F); }
bool range(float x, float lo, float hi) noexcept { return std::isfinite(x) && x>=lo && x<=hi; }
RibbonRgb scale(RibbonRgb a, float b) noexcept { return {a.red*b,a.green*b,a.blue*b}; }
void add(RibbonRgb& a, RibbonRgb b) noexcept { a.red+=b.red; a.green+=b.green; a.blue+=b.blue; }
float peak(RibbonRgb a) noexcept { return std::max({a.red,a.green,a.blue}); }
RibbonRgb persist(RibbonRgb x, RibbonRgb p, float retention) noexcept {
  return {std::max(x.red,p.red*retention),std::max(x.green,p.green*retention),std::max(x.blue,p.blue*retention)};
}
std::uint16_t quantise(float x) noexcept { return static_cast<std::uint16_t>(std::floor(unit(x)*65535.0F+0.5F)); }
}

RibbonRgb ribbonPalette(std::uint16_t id, float position) noexcept {
  const auto& p=productPalette(id);
  if (!std::isfinite(position) || p.stop_count==0) return {};
  const float x=unit(position)*255.0F;
  unsigned i=0;
  while (i+1<p.stop_count && x>p.stops[i+1].position) ++i;
  const auto a=p.stops[i];
  if (i+1==p.stop_count || x<=a.position) return {a.colour.red/255.0F,a.colour.green/255.0F,a.colour.blue/255.0F};
  const auto b=p.stops[i+1];
  const float f=(x-a.position)/(b.position-a.position);
  return {(a.colour.red+(b.colour.red-a.colour.red)*f)/255.0F,
          (a.colour.green+(b.colour.green-a.colour.green)*f)/255.0F,
          (a.colour.blue+(b.colour.blue-a.colour.blue)*f)/255.0F};
}

bool validRibbonSettings(const RibbonSettings& s) noexcept {
  return range(s.velocity_px_s,4,240) && range(s.spread_px2_s,0,30) &&
    range(s.half_life_s,.1F,12) && range(s.width_px,.5F,10) &&
    range(s.tail_ratio,1,3) && range(s.launch_spacing_s,.08F,3) &&
    range(s.gap_s,.05F,2) && range(s.return_strength,0,.45F) &&
    range(s.injection_response,0,3) && range(s.persistence_s,0,.15F) &&
    range(s.saturation,0,1) && range(s.warmth,-1,1) && range(s.contrast,.25F,3) &&
    s.palette_id<kProductPaletteCount;
}

void RibbonMaterial::reset(std::uint64_t time) noexcept {
  objects_={}; frame_={}; body_history_={}; return_history_={}; output_={}; telemetry_={};
  time_us_=render_us_=last_launch_us_=last_audio_us_=time; last_onset_=last_sequence_=0;
  colour_position_=.5F; prior_energy_=material_energy_=0; initialised_=true; have_launch_=have_sequence_=false;
}

bool RibbonMaterial::configure(const RibbonSettings& s, std::uint64_t time) noexcept {
  if (!validRibbonSettings(s) || !advance(time)) return false;
  // Width changes affect future deposits. Velocity/diffusion/lifetime affect
  // future segments of existing objects, after the old law reaches edit time.
  // Every object retains both its deposited colour and palette coordinate.
  // Selecting live colour changes mapping, never position or object identity.
  if (s.persistence_s==0) { body_history_={}; return_history_={}; }
  settings_=s;
  return true;
}

bool RibbonMaterial::advance(std::uint64_t time) noexcept {
  if (!initialised_) { reset(time); return true; }
  if (time<time_us_) return false;
  const double elapsed=static_cast<double>(time-time_us_)*1e-6;
  time_us_=time;
  for (auto& o:objects_) {
    if (!o.active) continue;
    double remaining=elapsed;
    // Three bounded segments: outgoing, quiet, returning. No frame-size dt cap.
    for (unsigned segment=0;segment<3 && remaining>0 && o.active;++segment) {
      if (o.waiting) {
        const double dt=std::min(remaining,static_cast<double>(o.quiet_remaining_s));
        o.quiet_remaining_s-=static_cast<float>(dt); remaining-=dt;
        o.amplitude*=std::exp(-kLn2*static_cast<float>(dt)/settings_.half_life_s);
        if (o.quiet_remaining_s<=1e-6F) {
          o.waiting=false; o.returning=true; o.radius=static_cast<float>(kPixelsPerHalf-1);
          o.amplitude*=settings_.return_strength;
        }
        continue;
      }
      // Solve the moving compact-support boundary analytically, including
      // diffusion during this segment. Recomputing a frozen support per frame
      // would make the disappearance/return time cadence-dependent.
      const double k=3.0*settings_.tail_ratio, v=settings_.velocity_px_s;
      const float ribbon_length=o.sample_count ? o.offsets[o.sample_count-1]:0;
      const double x=o.returning ? -o.radius-ribbon_length : o.radius-(kPixelsPerHalf-1)-ribbon_length;
      const double A=v*v, B=2*x*v-2*k*k*settings_.spread_px2_s;
      const double C=x*x-k*k*o.variance;
      const double until=x>=k*std::sqrt(o.variance) ? 0.0 :
        std::max(0.0,(-B+std::sqrt(std::max(0.0,B*B-4*A*C)))/(2*A));
      const double dt=std::min(remaining,until);
      o.radius+=(o.returning?-1.0F:1.0F)*settings_.velocity_px_s*static_cast<float>(dt);
      o.variance+=2.0F*settings_.spread_px2_s*static_cast<float>(dt);
      o.amplitude*=std::exp(-kLn2*static_cast<float>(dt)/settings_.half_life_s);
      remaining-=dt;
      if (dt>=until) {
        if(o.returning) o.active=false;
        else { o.waiting=true; o.quiet_remaining_s=settings_.gap_s; }
      }
      if(o.amplitude<1e-5F) o.active=false;
    }
  }
  return true;
}

bool RibbonMaterial::deposit(float strength, float position, std::uint64_t time) noexcept {
  if (!range(strength,0,4) || !range(position,0,1) || !advance(time)) return false;
  const float amount=std::min(1.0F,strength*settings_.injection_response);
  if (amount<=0) return true;
  if (have_launch_ && static_cast<double>(time-last_launch_us_)*1e-6<settings_.launch_spacing_s) {
    // Only material still at the injection site may absorb another attack.
    // Never brighten an entire travelling ribbon on a later transient.
    for(auto& o:objects_) if(o.active && !o.returning && !o.waiting && o.born_us==last_launch_us_) {
      const unsigned last=o.sample_count-1;
      if(o.radius-o.offsets[last]<2.F) {
        const auto colour=ribbonPalette(settings_.palette_id,position);
        const float gain=.18F*amount;
        o.samples[last]=scale(o.samples[last],1-gain);add(o.samples[last],scale(colour,gain));
        ++telemetry_.merged;
      } else ++telemetry_.omitted;
      return true;
    }
    ++telemetry_.omitted; return true;
  }
  for(auto& o:objects_) if(!o.active) {
    o={}; o.active=true; o.amplitude=amount; o.variance=settings_.width_px*settings_.width_px;
    o.palette_position=position; o.colour=ribbonPalette(settings_.palette_id,position); o.born_us=time;
    o.sample_count=1;o.samples[0]=o.colour;o.offsets[0]=0;
    last_launch_us_=time; have_launch_=true; ++telemetry_.launches; return true;
  }
  ++telemetry_.omitted; return true;
}

void RibbonMaterial::consume(const VisualAudioFrameView& v, std::uint64_t time) noexcept {
  if (!advance(time)) return;
  const auto& a=v.audio;
  telemetry_.audio_available=(a.validity_flags&contract::kValidBaseSnapshot)!=0 &&
    time>=v.source_publication_us && time-v.source_publication_us<=200000;
  if (!telemetry_.audio_available || (have_sequence_ && a.sequence==last_sequence_)) return;
  const float colour_dt=static_cast<float>(static_cast<double>(time-last_audio_us_)*1e-6);
  last_audio_us_=time;
  last_sequence_=a.sequence; have_sequence_=true;
  if ((a.event_flags&contract::kEventSilence)!=0) { prior_energy_=0; return; }
  if (!std::isfinite(a.vu_level)) { telemetry_.audio_available=false; return; }
  const float energy=unit(a.vu_level);
  const float follow=1-std::exp(-colour_dt/(energy>material_energy_?.015F:.08F));
  material_energy_+=follow*(energy-material_energy_);
  if ((a.validity_flags&contract::kValidChroma)!=0) {
    float sum=0, moment=0, strongest=0;unsigned dominant=0;
    for(unsigned i=0;i<contract::kChromaBinCount;++i) if(std::isfinite(a.chroma_a_origin[i]) && a.chroma_a_origin[i]>0) {
      sum+=a.chroma_a_origin[i]; moment+=a.chroma_a_origin[i]*i;
      if(a.chroma_a_origin[i]>strongest){strongest=a.chroma_a_origin[i];dominant=i;}
    }
    if(sum>1e-6F) {
      const float target=.12F+.76F*(.35F*moment/sum+.65F*dominant)/11.0F;
      colour_position_+=std::clamp(target-colour_position_,-.2F*colour_dt,.2F*colour_dt);
    }
  }
  bool attack=(a.validity_flags&contract::kValidOnsetV2)!=0 && a.onset_event_id!=0 &&
    a.onset_event_id!=last_onset_ && a.onset_event_age_ms<=200;
  if(attack) {
    last_onset_=a.onset_event_id;
    const float strength=std::max(a.onset_strength,a.bass_onset_strength);
    if(std::isfinite(strength)) deposit(.8F*std::sqrt(energy)+.2F*unit(strength),colour_position_,time);
  } else if (energy>.001F && (!have_launch_ || time-last_launch_us_>=850000)) {
    // Declared energy-driven gesture, never labelled a detected beat. Works
    // immediately and in held/unmetrical music without any phase authority.
    // The AP silence flag owns silence: quiet sustained music must not lose
    // its response merely because it never crosses an extra VP loudness gate.
    deposit(.5F*std::sqrt(energy),colour_position_,time);
  }
  prior_energy_=energy;
  // Feed the young ribbon while the sound develops. Each cross-section owns
  // its colour and energy; later attacks never recolour earlier deposits.
  // The finite formation window leaves actual separation between ribbons.
  for(auto& o:objects_) if(o.active && !o.returning && !o.waiting &&
      time-o.born_us<=350000 && o.sample_count<RibbonObject::kSamples &&
      o.radius-o.offsets[o.sample_count-1]>=.85F) {
    const float age=static_cast<float>(time-o.born_us)*1e-6F;
    const float taper=std::pow(std::max(0.F,1-age/.35F),.65F);
    const float density=(.45F+.55F*std::sqrt(material_energy_))*taper;
    o.offsets[o.sample_count]=o.radius;
    o.samples[o.sample_count]=scale(ribbonPalette(settings_.palette_id,colour_position_),density);
    ++o.sample_count;
  }
}

const RibbonFrame16& RibbonMaterial::render(std::uint64_t time, float master) noexcept {
  output_={}; frame_={};
  if(!range(master,0,24.0F/255.0F) || !advance(time)) { telemetry_.frame_valid=false; return output_; }
  telemetry_.frame_valid=true; telemetry_.active=telemetry_.outgoing=telemetry_.returning=0;
  for(const auto& o:objects_) if(o.active) {
    ++telemetry_.active;
    if(!o.waiting) { if(o.returning) ++telemetry_.returning; else ++telemetry_.outgoing; }
  }
  const float dt=static_cast<float>(static_cast<double>(time-render_us_)*1e-6);
  render_us_=time;
  const float retention=settings_.persistence_s>0 ? std::exp(-kLn2*dt/settings_.persistence_s):0;
  telemetry_.pre_limit_peak=0;
  for(std::size_t d=0;d<kPixelsPerHalf;++d) {
    RibbonRgb body{}, echo{};
    for(const auto& o:objects_) if(o.active && !o.waiting) {
      const float behind=o.returning ? static_cast<float>(d)-o.radius:o.radius-static_cast<float>(d);
      const float length=o.offsets[o.sample_count-1];
      const float distance=behind<0 ? -behind:std::max(0.F,behind-length);
      const float sigma=std::sqrt(o.variance)*(behind>length?settings_.tail_ratio:1.F);
      const float x=distance/sigma;
      if(x>=3) continue;
      // Compact smooth front/tail with exact support; peak-normalised width.
      const float shape=std::exp(-.5F*x*x)*std::pow(1-x*x/9.0F,2.0F);
      RibbonRgb colour=o.samples[0];
      if(behind>=length)colour=o.samples[o.sample_count-1];
      else if(behind>0) {
        unsigned i=1;while(i+1<o.sample_count && o.offsets[i]<behind)++i;
        float mix=unit((behind-o.offsets[i-1])/(o.offsets[i]-o.offsets[i-1]));
        mix=mix*mix*(3-2*mix);
        colour=scale(o.samples[i-1],1-mix);add(colour,scale(o.samples[i],mix));
      }
      if(settings_.recolour_history)colour=scale(ribbonPalette(settings_.palette_id,o.palette_position),peak(colour)/std::max(.001F,peak(o.colour)));
      add(o.returning?echo:body,scale(colour,o.amplitude*shape));
    }
    const auto pair=mirroredPair(d);
    // Persistence belongs to its material layer. Once every front in a layer
    // has left the logical strip, extinguish its residue; it cannot fill the
    // authored quiet interval or outlive the final return.
    body=telemetry_.outgoing ? persist(body,body_history_[pair.right],retention):RibbonRgb{};
    echo=telemetry_.returning ? persist(echo,return_history_[pair.right],retention):RibbonRgb{};
    if(peak(body)<1e-6F) body={};
    if(peak(echo)<1e-6F) echo={};
    body_history_[pair.left]=body_history_[pair.right]=body;
    return_history_[pair.left]=return_history_[pair.right]=echo;
    // Outgoing owns foreground. Dense return traffic stays subordinate.
    RibbonRgb c=body; add(c,scale(echo,telemetry_.outgoing? .35F:1.0F));
    const float y=.2126F*c.red+.7152F*c.green+.0722F*c.blue;
    c={y+settings_.saturation*(c.red-y),y+settings_.saturation*(c.green-y),y+settings_.saturation*(c.blue-y)};
    c.red*=1+.2F*settings_.warmth; c.blue*=1-.2F*settings_.warmth;
    const float luminance=.2126F*c.red+.7152F*c.green+.0722F*c.blue;
    if(luminance>0 && settings_.contrast!=1) c=scale(c,std::pow(luminance,settings_.contrast)/luminance);
    if(settings_.knee) { const float p=peak(c); c=scale(c,1/(1+p)); }
    if(!std::isfinite(c.red)||!std::isfinite(c.green)||!std::isfinite(c.blue)) {
      telemetry_.frame_valid=false; frame_={}; output_={}; return output_;
    }
    frame_[pair.left]=frame_[pair.right]=c;
    telemetry_.pre_limit_peak=std::max(telemetry_.pre_limit_peak,peak(c));
  }
  telemetry_.limiter_gain=1/std::max(1.0F,telemetry_.pre_limit_peak);
  for(std::size_t i=0;i<output_.size();++i) {
    const auto c=scale(frame_[i],master*telemetry_.limiter_gain);
    output_[i]={quantise(c.red),quantise(c.green),quantise(c.blue)};
  }
  return output_;
}
} // namespace k1::core::visual
