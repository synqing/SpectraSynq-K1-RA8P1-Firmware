// Emotiscope.HIL v1.1_build, source 7f86939bd7c16c4a6dd5789813e9d551246ed134.
// The donor AP and complete musical VP share one translation unit, as in Arduino.
// GPLv3 donor attribution and source notices are retained in emotiscope_donor/.
// Platform substitutions: deterministic clock/RNG, scalar ESP-DSP primitives,
// no network/NVS/physical touch, captured RGB8 output in place of ESP32 RMT.
// Native geometry remains 128. The Titan's 128->160/GRB48 mapping is external.
#include "emotiscope_port.h"
#include "core/visual/product_palette.h"
#include "emotiscope_edge_response.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace emotiscope {
#define IRAM_ATTR
#define PI 3.14159265358979323846
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define SAMPLE_RATE 12800
#define CHUNK_SIZE 64
template<class A,class B> inline auto min(A a,B b) {return a<b?a:b;}
template<class A,class B> inline auto max(A a,B b) {return a>b?a:b;}
template<class F> inline void profile_function(F f,const char*) {f();}
inline unsigned bitRead(uint32_t v,unsigned b){return (v>>b)&1u;}
struct PsychicWebSocketHandler {void sendAll(const char*){}};
PsychicWebSocketHandler websocket_handler;
struct SerialShim {void println(const char*){}};
SerialShim Serial;
static uint32_t port_clock_us=0, rng_state=1;
static uint64_t port_extended_us=0;
static bool port_clock_seen=false;
static uint32_t clock_regressions=0;
bool advance_port_clock(uint32_t now_us){
  if(!port_clock_seen){port_extended_us=now_us;port_clock_seen=true;}
  else {
    const uint32_t elapsed=now_us-port_clock_us;
    // Ordinary calls are at 5/10 ms. A backward timestamp must not become a
    // fictitious 71-minute advance; caller service intervals are below 2^31 us.
    if(elapsed>=0x80000000u){++clock_regressions;return false;}
    port_extended_us+=elapsed;
  }
  port_clock_us=now_us;return true;
}
uint32_t micros(){return port_clock_us;}
uint32_t millis(){return static_cast<uint32_t>(port_extended_us/1000u);}
uint32_t esp_random(){
  uint32_t x=rng_state;x^=x<<13;x^=x>>17;x^=x<<5;
  rng_state=x;return x;
}
long random(long lo,long hi){return lo+long(esp_random()%uint32_t(hi-lo));}
inline int dsps_mulc_f32(const float* in,float* out,int n,float gain,int si,int so){
  for(int i=0;i<n;++i)out[i*so]=in[i*si]*gain;return 0;
}
inline int dsps_mulc_f32_ae32(const float*a,float*b,int n,float g,int si,int so){return dsps_mulc_f32(a,b,n,g,si,so);}
inline int dsps_mulc_f32_ansi(const float*a,float*b,int n,float g,int si,int so){return dsps_mulc_f32(a,b,n,g,si,so);}
inline int dsps_add_f32(const float*a,const float*b,float*out,int n,int sa,int sb,int so){
  for(int i=0;i<n;++i)out[i*so]=a[i*sa]+b[i*sb];return 0;
}
inline int dsps_mul_f32_ae32(const float*a,const float*b,float*out,int n,int sa,int sb,int so){
  for(int i=0;i<n;++i)out[i*so]=a[i*sa]*b[i*sb];return 0;
}
#include "emotiscope_donor/global_defines.h"
#include "emotiscope_donor/types.h"
config configuration;
extern light_mode light_modes[];
float noise_spectrum[NUM_FREQS]={};
float sample_history[SAMPLE_HISTORY_LENGTH]={};
uint32_t t_now_us=0,t_now_ms=0;
// Declared target scheduling rate, not a fabricated HIL measurement.
float FPS_GPU=K1_EMOTISCOPE_RENDER_HZ;
bool EMOTISCOPE_ACTIVE=true;
volatile bool device_touch_active=false,app_touch_active=false,slider_touch_active=false;
void save_config(){}
void save_config_delayed(){}
void save_noise_spectrum(){}
void watch_gpu_fps(){}
#include "emotiscope_donor/utilities.h"
// All musical colour paths use the existing K1 catalogue, including boot and
// missing-runtime-sampler fallback. There is no selectable native HSV path.
static void k1_builtin_palette_sample(uint32_t id,float phase,float rgb[3]){
  const auto c=k1::core::visual::sampleProductPaletteHd(static_cast<uint16_t>(id),phase,1.0f);
  rgb[0]=c.red;rgb[1]=c.green;rgb[2]=c.blue;
}
static volatile uint32_t k1_palette_select=K1_EMOTISCOPE_DEFAULT_PALETTE;
static K1PaletteSampler k1_palette_sampler=&k1_builtin_palette_sample;
#define hsv donor_hsv
#define get_color_range_hue donor_get_color_range_hue
#define apply_gamma_correction donor_apply_gamma_correction
#define clear_display donor_clear_display
#define apply_background donor_apply_background
#define apply_tonemapping donor_apply_tonemapping
#include "emotiscope_donor/leds.h"
#undef apply_tonemapping
#undef apply_background
#undef clear_display
#undef apply_gamma_correction
#undef get_color_range_hue
#undef hsv
// Palettes 1-44 are the only colour source, drawn the way K1 drew them (K1
// Spectrum, palette mode): palette position = strip position (bass = palette
// start), reversed by Reverse; auto colour scrolls it (K1 auto shift, using the
// donor's drifting colour). Colour, Colour range and Saturation do not touch it.
// Level 0.65v^2+0.35v (K1 SQUARE_ITER 1); no final x^2 gamma (K1
// ENABLE_OUTPUT_GAMMA 0). Background and screensaver draw palette colours; peak
// soft-clip scales whole pixels so hues hold. Silence is never filled for colour.
float get_color_range_hue(float progress){
  const float base=configuration.auto_color_cycle?configuration.color:0.0f;
  return configuration.reverse_color_range?base+1.0f-progress:base+progress;
}
void apply_gamma_correction(){}
// Hype offsets its second dot by 0.5*color_range. With a palette that offset is
// pinned to half the palette for the duration of the mode draw only (same render
// thread; restored before background, before any control read).
static bool k1_draw_scope=false;
static float k1_saved_color_range=0.0f;
static void k1_draw_scope_end(){
  if(k1_draw_scope){configuration.color_range=k1_saved_color_range;k1_draw_scope=false;}
}
void clear_display(float keep=0.0f){
  donor_clear_display(keep);
  if(!k1_draw_scope){
    k1_saved_color_range=configuration.color_range;configuration.color_range=1.0f;k1_draw_scope=true;
  }
}
static CRGBF k1_palette_colour(float position){
  float c[3]={0.0f,0.0f,0.0f};
  k1_palette_sampler(k1_palette_select-1u,position,c);
  return CRGBF{c[0],c[1],c[2]};
}
void apply_background(float background_level){
  k1_draw_scope_end();
  if(light_modes[configuration.current_mode].type == LIGHT_MODE_TYPE_SYSTEM)return;
  // Same light as the donor's background at the same setting: its (0.25b) was
  // squared by the final gamma, which palettes skip.
  const float level=(0.25f*background_level)*(0.25f*background_level);
  if(!(level>0.0f))return;
  if(!configuration.mirror_mode){
    for(uint16_t i=0;i<NUM_LEDS;i++){
      const CRGBF c=k1_palette_colour(get_color_range_hue(num_leds_float_lookup[i]));
      leds[i].r+=c.r*level;leds[i].g+=c.g*level;leds[i].b+=c.b*level;
    }
  }else{
    for(uint16_t i=0;i<(NUM_LEDS>>1);i++){
      const CRGBF c=k1_palette_colour(get_color_range_hue(num_leds_float_lookup[i<<1]));
      const uint16_t l=((NUM_LEDS>>1)-1)-i,r=(NUM_LEDS>>1)+i;
      leds[l].r+=c.r*level;leds[l].g+=c.g*level;leds[l].b+=c.b*level;
      leds[r].r+=c.r*level;leds[r].g+=c.g*level;leds[r].b+=c.b*level;
    }
  }
}
void apply_tonemapping(){
  k1_draw_scope_end();
  for(uint16_t i=0;i<NUM_LEDS;i++){
    const float m=fmaxf(leds[i].r,fmaxf(leds[i].g,leds[i].b));
    if(m>0.75f){
      const float k=soft_clip_hdr(m)/m;
      leds[i].r*=k;leds[i].g*=k;leds[i].b*=k;
    }
  }
}
CRGBF hsv(float h,float s,float v){
  const uint32_t p=k1_palette_select;
  const K1PaletteSampler sample=k1_palette_sampler;
  // Selector and sampler are validated at their owner-only setters.
  (void)s;
  float c[3]={0.0f,0.0f,0.0f};
  sample(p-1u,h,c);
  const float level=0.65f*v*v+0.35f*v;
  return CRGBF{c[0]*level,c[1]*level,c[2]*level};
}
#include "emotiscope_donor/ui.h"
#include "emotiscope_donor/goertzel.h"
#include "emotiscope_donor/vu.h"
#include "emotiscope_donor/tempo.h"
#define run_screensaver donor_run_screensaver
#include "emotiscope_donor/screensaver.h"
#undef run_screensaver
// Donor screensaver with its four fixed colours replaced by palette colours.
static CRGBF k1_screensaver_last[4];
void run_screensaver(){
  if(configuration.screensaver == false){ return; }
  if(light_modes[configuration.current_mode].type != LIGHT_MODE_TYPE_ACTIVE){ return; }
  float mag_sum=0;
  for(uint16_t i=0;i<NUM_FREQS;i++)mag_sum+=frequencies_musical[i].magnitude;
  if(mag_sum<screensaver_threshold){
    if(inactive==false){inactive=true;inactive_start=t_now_ms;}
  }else if(mag_sum>screensaver_threshold){inactive=false;}
  if(device_touch_active==true||app_touch_active==true||slider_touch_active==true)inactive=false;
  if(inactive==false){ if(screensaver_mix>0.0)screensaver_mix-=0.01; }
  if(inactive==true){
    if(t_now_ms-inactive_start>=SCREENSAVER_WAIT_MS){ if(screensaver_mix<1.0)screensaver_mix+=0.001; }
  }
  if(screensaver_mix>0.001){
    scale_CRGBF_array_by_constant(leds,1.0-(screensaver_mix*screensaver_mix),NUM_LEDS);
    const float push_val=0.005;
    for(uint16_t i=0;i<4;i++){
      k1_screensaver_last[i]=k1_palette_colour(0.125f+0.25f*i);
      sine_positions[i]+=push_val+(0.0001*i);
      draw_dot(leds,SCREENSAVER_1+i,k1_screensaver_last[i],sin(sine_positions[i])*(0.5*screensaver_mix)+0.5,screensaver_mix*screensaver_mix);
    }
  }
}
#include "emotiscope_donor/standby.h"
// Arduino accepts aggregate parenthesized initialization in the unused presets
// helper; this syntax bridge preserves the exact RGB values on C++17 compilers.
#define CRGBF(...) CRGBF{__VA_ARGS__}
#include "emotiscope_donor/light_modes.h"
#undef CRGBF
CRGBF dither_error[NUM_LEDS];
static uint8_t raw_led_data[NUM_LEDS*3];
static uint16_t raw_led_data16[NUM_LEDS*3]; // RGB order, one per float in leds
#include "emotiscope_quantize.inc"
static bool frame_output_valid=true;
static uint32_t nonfinite_output_frames=0;
#ifdef K1_EMOTISCOPE_TEST_SEAM
static bool test_fail_next_frame=false;
#endif
void transmit_leds(){
#ifdef K1_EMOTISCOPE_TEST_SEAM
  if(test_fail_next_frame){test_fail_next_frame=false;leds[0].r=std::nanf("");}
#endif
  // Validate before the donor float-to-integer conversion. A numerical fault
  // retains the last complete hardware frame; it is not converted to white/black.
  const float* p=reinterpret_cast<const float*>(leds);
  for(unsigned i=0;i<NUM_LEDS*3;++i){
    if(!std::isfinite(p[i]) || p[i]<0.0f || p[i]>1.0f){
      frame_output_valid=false;++nonfinite_output_frames;return;
    }
  }
  std::memset(raw_led_data,0,sizeof(raw_led_data));
  quantize_color_error(configuration.temporal_dithering);
  // Precision candidate from the identical validated floats; the reference
  // quantiser above (and its dither history) runs regardless of selection.
  // Single-precision (the M85 FPU is SP-only); v in [0,1] so +0.5 then truncate
  // is round-half-up. -ffp-contract=off keeps the multiply and add unfused.
  for(unsigned i=0;i<NUM_LEDS*3;++i)
    raw_led_data16[i]=static_cast<uint16_t>(p[i]*65535.0f+0.5f);
  frame_output_valid=true;
}
void apply_edge_response(float delta);
#define K1_EMOTISCOPE_SHARED_MUSICAL_TICK 1
#include "emotiscope_donor/gpu_core.h"
#undef K1_EMOTISCOPE_SHARED_MUSICAL_TICK
static bool initialized=false;
static uint32_t audio_hops=0,render_frames=0,rejected_audio_chunks=0;
static uint32_t audio_gaps=0,fresh_hops=0;
static bool gap_rewarming=false;
static float response_gain=1.0f;
static EmotiscopeEdgeResponse edge_response;
static K1EmotiscopeAudioPublication musical_publication{};
static bool publication_seen=false;
static uint32_t musical_tick_us=0;
static void capture_musical_publication(){
  auto& p=musical_publication;
  ++p.sequence;p.audio_hops=audio_hops;p.now_us=port_clock_us;p.valid=fresh_hops>=64u&&!gap_rewarming;
  p.vu_level_raw=vu_level_raw;p.vu_level=vu_level;p.vu_max=vu_max;p.vu_floor=vu_floor;
  p.tempo_confidence=tempo_confidence;p.tempi_power_sum=tempi_power_sum;
  std::memcpy(p.spectrum,spectrogram,sizeof(p.spectrum));
  std::memcpy(p.spectrum_smooth,spectrogram_smooth,sizeof(p.spectrum_smooth));
  std::memcpy(p.chromagram,chromagram,sizeof(p.chromagram));
  p.novelty_normalised=novelty_curve_normalized[NOVELTY_HISTORY_LENGTH-1];
  std::memcpy(p.tempi_smooth,tempi_smooth,sizeof(p.tempi_smooth));
  for(unsigned i=0;i<NUM_FREQS;++i)p.frequency_magnitude[i]=frequencies_musical[i].magnitude;
  for(unsigned i=0;i<NUM_TEMPI;++i){p.tempi_phase[i]=tempi[i].phase;p.tempi_beat[i]=tempi[i].beat;}
}
static void import_musical_publication(float gain){
  const auto& p=musical_publication;
  vu_level_raw=p.vu_level_raw;vu_level=clip_float(p.vu_level*gain);vu_max=p.vu_max;vu_floor=p.vu_floor;
  tempo_confidence=clip_float(p.tempo_confidence*gain);tempi_power_sum=fmaxf(p.tempi_power_sum*gain,0.00000001f);
  for(unsigned i=0;i<NUM_FREQS;++i){spectrogram[i]=p.spectrum[i]*gain;spectrogram_smooth[i]=p.spectrum_smooth[i]*gain;frequencies_musical[i].magnitude=p.frequency_magnitude[i]*gain;}
  for(unsigned i=0;i<12;++i)chromagram[i]=p.chromagram[i]*gain;
  novelty_curve_normalized[NOVELTY_HISTORY_LENGTH-1]=clip_float(p.novelty_normalised*gain);
  for(unsigned i=0;i<NUM_TEMPI;++i)tempi_smooth[i]=p.tempi_smooth[i]*gain;
  for(unsigned i=0;i<NUM_TEMPI;++i){tempi[i].phase=p.tempi_phase[i];tempi[i].beat=p.tempi_beat[i];}
}

// Legacy EdgeMixer VALUE adaptation; the envelope belongs to this VP instance.
void apply_edge_response(float delta){
  (void)edge_response.apply(reinterpret_cast<float*>(leds),musical_publication,
      delta*(1000000/REFERENCE_FPS)/1000000.0f);
}
void mute_audio_publication(){
  std::memset(spectrogram,0,sizeof(spectrogram));
  std::memset(spectrogram_smooth,0,sizeof(spectrogram_smooth));
  std::memset(chromagram,0,sizeof(chromagram));
  for(unsigned i=0;i<NUM_FREQS;++i)frequencies_musical[i].magnitude=0.0f;
  vu_level_raw=0.0f;vu_level=0.0f;vu_max=0.000001f;
}

static_assert(sizeof(config)==sizeof(K1EmotiscopeConfig),"configuration ABI");
static_assert(NUM_LEDS==128 && NUM_FREQS==64 && NUM_TEMPI==96,"native donor dimensions");
} // namespace emotiscope

extern "C" void k1_emotiscope_default_config(K1EmotiscopeConfig* out){
  if(!out)return;
  // Exact load_config() fallback values, without importing credentials or NVS.
  *out={1.00f,0.25f,0.33f,0.00f,0.00f,0.50f,0.75f,0.25f,1,
        true,true,true,false,false};
}
extern "C" bool k1_emotiscope_init(uint32_t seed){
  using namespace emotiscope;
  if(initialized)return false;
  rng_state=seed?seed:1u;
  K1EmotiscopeConfig defaults;k1_emotiscope_default_config(&defaults);
  std::memcpy(&configuration,&defaults,sizeof(configuration));
  init_light_mode_list();
#ifndef K1_EMOTISCOPE_SECONDARY_INSTANCE
  init_window_lookup();
  init_goertzel_constants_musical();
  init_tempo_goertzel_constants();
  init_noise_samples();
#endif
  init_floating_point_lookups();
#ifndef K1_EMOTISCOPE_SECONDARY_INSTANCE
  init_vu();
#endif
  init_random_dither_error();
  edge_response.initialise_bins();

  initialized=true;
  return true;
}
extern "C" bool k1_emotiscope_set_config(const K1EmotiscopeConfig* input){
  using namespace emotiscope;
  // Neutral7 is an inactive full-strip lamp, not a musical visualisation.
  if(!initialized || !input || input->current_mode<0 || input->current_mode>6)return false;
  // Fixed HSV controls are obsolete in the permanent catalogue path. A full
  // read-modify-write may preserve their current values, including a drifting
  // auto-colour phase above1; it may not pretend an inert field is configurable.
  if(input->color!=configuration.color || input->color_range!=configuration.color_range ||
      input->saturation!=configuration.saturation)return false;
  const float values[]={input->brightness,input->softness,input->color,input->warmth,
    input->color_range,input->speed,input->saturation,input->background};
  for(unsigned i=0;i<8;++i){
    if(!std::isfinite(values[i]))return false;
    if(i==2u)continue; // unchanged internally evolving palette phase
    if(values[i]<0.0f||values[i]>1.0f)return false;
  }
  const bool mode_changed=configuration.current_mode!=input->current_mode;
  std::memcpy(&configuration,input,sizeof(configuration));
  if(mode_changed)lpf_drag=1.0f; // donor mode transition
  return true;
}
extern "C" void k1_emotiscope_get_config(K1EmotiscopeConfig* out){
  if(out)std::memcpy(out,&emotiscope::configuration,sizeof(*out));
}
static bool emotiscope_unit_float(float value){
  return std::isfinite(value) && value>=0.0f && value<=1.0f;
}
extern "C" bool k1_emotiscope_patch(const K1EmotiscopePatch* patch,uint32_t* changed,uint32_t* mode_transition){
  using namespace emotiscope;
  if(changed)*changed=0;
  if(mode_transition)*mode_transition=0;
  if(!initialized || !patch || (patch->mask & ~0x3FFFu))return false;
  K1EmotiscopeConfig next;
  std::memcpy(&next,&configuration,sizeof(next));
  const uint32_t mask=patch->mask;
  if(mask&((1u<<2)|(1u<<4)|(1u<<6)))return false;
  if(mask&1u && !emotiscope_unit_float(patch->brightness))return false;
  if(mask&2u && !emotiscope_unit_float(patch->softness))return false;
  if(mask&4u && !emotiscope_unit_float(patch->color))return false;
  if(mask&8u && !emotiscope_unit_float(patch->warmth))return false;
  if(mask&16u && !emotiscope_unit_float(patch->color_range))return false;
  if(mask&32u && !emotiscope_unit_float(patch->speed))return false;
  if(mask&64u && !emotiscope_unit_float(patch->saturation))return false;
  if(mask&128u && !emotiscope_unit_float(patch->background))return false;
  if(mask&256u && (patch->current_mode<0 || patch->current_mode>6))return false;
  const uint8_t flags[]={patch->mirror_mode,patch->screensaver,patch->temporal_dithering,
    patch->auto_color_cycle,patch->reverse_color_range};
  for(unsigned i=0;i<5;++i) if((mask&(1u<<(9+i))) && flags[i]>1u) return false;
  if(mask&1u) next.brightness=patch->brightness;
  if(mask&2u) next.softness=patch->softness;
  if(mask&4u) next.color=patch->color;
  if(mask&8u) next.warmth=patch->warmth;
  if(mask&16u) next.color_range=patch->color_range;
  if(mask&32u) next.speed=patch->speed;
  if(mask&64u) next.saturation=patch->saturation;
  if(mask&128u) next.background=patch->background;
  if(mask&256u) next.current_mode=patch->current_mode;
  if(mask&512u) next.mirror_mode=patch->mirror_mode!=0;
  if(mask&1024u) next.screensaver=patch->screensaver!=0;
  if(mask&2048u) next.temporal_dithering=patch->temporal_dithering!=0;
  if(mask&4096u) next.auto_color_cycle=patch->auto_color_cycle!=0;
  if(mask&8192u) next.reverse_color_range=patch->reverse_color_range!=0;
  const bool mode_changed=(mask&256u) && configuration.current_mode!=next.current_mode;
  const bool any=std::memcmp(&configuration,&next,sizeof(next))!=0;
  if(!any)return true;
  std::memcpy(&configuration,&next,sizeof(configuration));
  if(mode_changed)lpf_drag=1.0f;
  if(changed)*changed=1u;
  if(mode_transition)*mode_transition=mode_changed?1u:0u;
  return true;
}
extern "C" float k1_emotiscope_effective_hue(void){
  const float color=emotiscope::configuration.color;
  if(!std::isfinite(color))return 0.0f;
  float wrapped=color-std::floor(color);
  if(wrapped<0.0f)wrapped+=1.0f;
  if(wrapped>=1.0f)wrapped=0.0f;
  return wrapped;
}
#ifdef K1_EMOTISCOPE_TEST_SEAM
extern "C" void k1_emotiscope_test_fail_next_frame(){emotiscope::test_fail_next_frame=true;}
extern "C" void k1_emotiscope_test_set_raw_color(float color){
  emotiscope::configuration.color=color;
}
extern "C" float k1_emotiscope_test_lpf_drag(void){return emotiscope::lpf_drag;}
extern "C" void k1_emotiscope_test_set_novelty_sample(float value){
  emotiscope::novelty_curve[10]=value;
}
extern "C" float k1_emotiscope_test_novelty_sample(void){return emotiscope::novelty_curve[10];}
// One colour through the production colour path: hsv(get_color_range_hue(progress),
// saturation, v), then the final gamma stage. pre = before gamma, post = after.
extern "C" void k1_emotiscope_test_colour(float progress,float v,float pre[3],float post[3]){
  using namespace emotiscope;
  const CRGBF c=hsv(get_color_range_hue(progress),configuration.saturation,v);
  pre[0]=c.r;pre[1]=c.g;pre[2]=c.b;
  for(unsigned i=0;i<NUM_LEDS;++i)leds[i]=c;
  apply_gamma_correction();
  post[0]=leds[0].r;post[1]=leds[0].g;post[2]=leds[0].b;
}
// Frame stages for palette checks. stage 0: background onto a black frame;
// 1: tonemap the given frame; 2: screensaver at full mix, returns its 4 colours
// in rgbf[0..11]; 3: colour range seen during a mode draw, then after background.
extern "C" void k1_emotiscope_test_stage(int stage,float arg,float* rgbf){
  using namespace emotiscope;
  if(stage==0){
    std::memset(leds,0,sizeof(leds));apply_background(arg);std::memcpy(rgbf,leds,sizeof(leds));
  }else if(stage==1){
    std::memcpy(leds,rgbf,sizeof(leds));apply_tonemapping();std::memcpy(rgbf,leds,sizeof(leds));
  }else if(stage==2){
    std::memset(leds,0,sizeof(leds));
    for(auto& c:k1_screensaver_last)c=CRGBF{-1.0f,-1.0f,-1.0f};
    const bool was=configuration.screensaver;configuration.screensaver=true;
    screensaver_mix=1.0f;inactive=true;inactive_start=t_now_ms;
    run_screensaver();configuration.screensaver=was;
    for(unsigned i=0;i<4;++i){rgbf[3*i]=k1_screensaver_last[i].r;rgbf[3*i+1]=k1_screensaver_last[i].g;rgbf[3*i+2]=k1_screensaver_last[i].b;}
  }else if(stage==3){
    clear_display();rgbf[0]=configuration.color_range;
    apply_background(0.0f);rgbf[1]=configuration.color_range;
  }else if(stage==4){
    // Deterministic audio level through the actual Analog renderer, without
    // publishing fabricated audio into the production analyser.
    const float saved_vu=vu_level,saved_smooth=vu_level_smooth;
    vu_level=arg;vu_level_smooth=arg;
    clear_display();draw_analog();apply_background(0.0f);apply_tonemapping();
    std::memcpy(rgbf,leds,sizeof(leds));
    vu_level=saved_vu;vu_level_smooth=saved_smooth;
  }
}
// Push a known post-gamma float frame through the production output stage
// (validation, RGB8 quantiser with its dither history, 16-bit candidate).
extern "C" bool k1_emotiscope_test_transmit(const float* rgbf,uint8_t* rgb8,uint16_t* rgb16){
  using namespace emotiscope;
  std::memcpy(leds,rgbf,sizeof(leds));
  transmit_leds();
  if(!frame_output_valid)return false;
  for(unsigned i=0;i<NUM_LEDS;++i){
    rgb8[3*i]=raw_led_data[3*i+1];rgb8[3*i+1]=raw_led_data[3*i];rgb8[3*i+2]=raw_led_data[3*i+2];
  }
  std::memcpy(rgb16,raw_led_data16,sizeof(raw_led_data16));
  return true;
}
#endif
extern "C" bool k1_emotiscope_push_pcm64(const float* samples,uint32_t now_us){
  using namespace emotiscope;
  if(!initialized||!samples){++rejected_audio_chunks;return false;}
  for(unsigned i=0;i<CHUNK_SIZE;++i)
    if(!std::isfinite(samples[i])||samples[i]<-1.0f||samples[i]>1.0f){
      ++rejected_audio_chunks;return false;
    }
  if(!advance_port_clock(now_us)){++rejected_audio_chunks;return false;}
  // The donor's shared t_now_ms is refreshed by run_gpu, not by its audio loop.
  // Preserve that chronology. PCM acquisition is the only replaced AP ingress.
  shift_and_copy_arrays(sample_history,SAMPLE_HISTORY_LENGTH,samples,CHUNK_SIZE);
  calculate_magnitudes();
  get_chromagram();
  run_vu();
  update_tempo();
  ++audio_hops;
  if(fresh_hops<64u)++fresh_hops;
  if(gap_rewarming){
    if(fresh_hops<64u)mute_audio_publication();
    else gap_rewarming=false;
  }
  return true;
}
extern "C" bool k1_emotiscope_render(uint32_t now_us,uint8_t* rgb){
  using namespace emotiscope;
  if(!initialized||!rgb)return false;
  if(!advance_port_clock(now_us))return false;
#ifndef K1_EMOTISCOPE_SECONDARY_INSTANCE
  // Audio/tempo publication advances once per release, independent of either
  // board's mode, controls or output validity. VP function statics are per TU.
  if(!publication_seen || now_us!=musical_tick_us){
    t_now_us=micros();t_now_ms=millis();
    const float delta=publication_seen?float(now_us-musical_tick_us)/(1000000/REFERENCE_FPS):0.0f;
    update_novelty();update_tempi_phase(delta);
    musical_tick_us=now_us;publication_seen=true;capture_musical_publication();
  }
#else
  if(!publication_seen || musical_publication.now_us!=now_us)return false;
#endif
  import_musical_publication(response_gain);
  run_gpu();
  // Restore the immutable raw observation: visual response must not enter the
  // next analyser hop or another board's response.
  import_musical_publication(1.0f);
  ++render_frames;
  if(!frame_output_valid)return false;
  for(unsigned i=0;i<NUM_LEDS;++i){
    rgb[3*i]=raw_led_data[3*i+1];
    rgb[3*i+1]=raw_led_data[3*i];
    rgb[3*i+2]=raw_led_data[3*i+2];
  }
  return true;
}
extern "C" void k1_emotiscope_audio_publication(K1EmotiscopeAudioPublication* out){
  if(out)std::memcpy(out,&emotiscope::musical_publication,sizeof(*out));
}
extern "C" bool k1_emotiscope_set_response_gain(float gain){
  if(!std::isfinite(gain)||gain<0.0f||gain>2.0f)return false;
  emotiscope::response_gain=gain;return true;
}
extern "C" float k1_emotiscope_response_gain(){return emotiscope::response_gain;}
extern "C" bool k1_emotiscope_set_edge_response(uint32_t mode,float depth){
  return emotiscope::edge_response.configure(mode,depth);
}
extern "C" uint32_t k1_emotiscope_edge_response(){return emotiscope::edge_response.mode();}
extern "C" float k1_emotiscope_edge_depth(){return emotiscope::edge_response.depth();}
#ifdef K1_EMOTISCOPE_SECONDARY_INSTANCE
extern "C" bool k1_secondary_emotiscope_accept_audio(const K1EmotiscopeAudioPublication* publication){
  using namespace emotiscope;
  if(!initialized||!publication||publication->sequence==0)return false;
  musical_publication=*publication;publication_seen=true;audio_hops=publication->audio_hops;
  return true;
}
#endif
extern "C" bool k1_emotiscope_set_palette(uint32_t id){
  if(id==0u||id>K1_EMOTISCOPE_PALETTES)return false;
  emotiscope::k1_palette_select=id;return true;
}
extern "C" void k1_emotiscope_set_palette_sampler(K1PaletteSampler sampler){
  emotiscope::k1_palette_sampler=sampler?sampler:&emotiscope::k1_builtin_palette_sample;
}
extern "C" uint32_t k1_emotiscope_palette(void){return emotiscope::k1_palette_select;}
extern "C" void k1_emotiscope_last_rgb16(uint16_t* rgb16){
  if(rgb16)std::memcpy(rgb16,emotiscope::raw_led_data16,sizeof(emotiscope::raw_led_data16));
}
extern "C" void k1_emotiscope_snapshot(K1EmotiscopeSnapshot* out){
  using namespace emotiscope;
  if(!out)return;
  std::memset(out,0,sizeof(*out));
  out->audio_hops=audio_hops;out->render_frames=render_frames;
  out->rejected_audio_chunks=rejected_audio_chunks;
  out->nonfinite_output_frames=nonfinite_output_frames;
  out->clock_regressions=clock_regressions;
  out->now_us=port_clock_us;out->now_ms=t_now_ms;out->current_mode=configuration.current_mode;
  out->vu_level_raw=vu_level_raw;out->vu_level=vu_level;
  out->vu_max=vu_max;out->vu_floor=vu_floor;
  out->tempo_confidence=tempo_confidence;out->tempi_power_sum=tempi_power_sum;
  std::memcpy(out->spectrogram,spectrogram,sizeof(spectrogram));
  std::memcpy(out->spectrogram_smooth,spectrogram_smooth,sizeof(spectrogram_smooth));
  std::memcpy(out->chromagram,chromagram,sizeof(chromagram));
  std::memcpy(out->novelty,novelty_curve,sizeof(novelty_curve));
  std::memcpy(out->novelty_normalized,novelty_curve_normalized,sizeof(novelty_curve_normalized));
  std::memcpy(out->vu_curve,vu_curve,sizeof(vu_curve));
  std::memcpy(out->tempi_smooth,tempi_smooth,sizeof(tempi_smooth));
  for(unsigned i=0;i<NUM_TEMPI;++i){
    out->tempi_magnitude[i]=tempi[i].magnitude;
    out->tempi_full_scale[i]=tempi[i].magnitude_full_scale;
    out->tempi_phase[i]=tempi[i].phase;out->tempi_beat[i]=tempi[i].beat;
    out->tempi_target_hz[i]=tempi[i].target_tempo_hz;
    out->tempi_block_size[i]=tempi[i].block_size;
  }
  std::memcpy(out->post_gamma_rgb,leds,sizeof(leds));
}
extern "C" const char* k1_emotiscope_mode_name(uint32_t mode){
  return mode<9?emotiscope::get_light_mode_name(static_cast<int16_t>(mode)):"";
}

extern "C" void k1_emotiscope_audio_gap(){
  using namespace emotiscope;
  if(!initialized)return;
  ++audio_gaps;fresh_hops=0;gap_rewarming=true;
  std::memset(sample_history,0,sizeof(sample_history));
  mute_audio_publication();
}
extern "C" void k1_emotiscope_status(K1EmotiscopeStatus* out){
  using namespace emotiscope;
  if(!out)return;
  std::memset(out,0,sizeof(*out));
  out->valid=initialized && fresh_hops>=64u && !gap_rewarming;
  out->audio_hops=audio_hops;out->render_frames=render_frames;
  out->audio_gaps=audio_gaps;out->fresh_hops=fresh_hops;
  out->rejected_audio_chunks=rejected_audio_chunks;
  out->nonfinite_output_frames=nonfinite_output_frames;
  out->clock_regressions=clock_regressions;
  out->vu_level_raw=vu_level_raw;out->vu_level=vu_level;
  std::memcpy(out->spectrum,spectrogram,sizeof(spectrogram));
  std::memcpy(out->spectrum_smooth,spectrogram_smooth,sizeof(spectrogram_smooth));
  std::memcpy(out->chromagram,chromagram,sizeof(chromagram));
  // A maximum donor candidate is not a lock. Unrefreshed tempo is never fresh.
  if(out->valid){
    unsigned best=0;
    for(unsigned i=1;i<NUM_TEMPI;++i)if(tempi_smooth[i]>tempi_smooth[best])best=i;
    if(tempi_smooth[best]>0.0f)out->dominant_tempo_bpm=tempi[best].target_tempo_hz*60.0f;
    out->tempo_confidence=tempo_confidence;
  }
}
