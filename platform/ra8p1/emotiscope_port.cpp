// Emotiscope.HIL v1.1_build, source 7f86939bd7c16c4a6dd5789813e9d551246ed134.
// The donor AP and complete musical VP share one translation unit, as in Arduino.
// GPLv3 donor attribution and source notices are retained in emotiscope_donor/.
// Platform substitutions: deterministic clock/RNG, scalar ESP-DSP primitives,
// no network/NVS/physical touch, captured RGB8 output in place of ESP32 RMT.
// Native geometry remains 128. The Titan's 128->160/GRB48 mapping is external.
#include "emotiscope_port.h"
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
// Palette adaptation: every donor mode colours through hsv(). The donor definition is
// renamed (source untouched) and wrapped: palette 0 = native donor HSV; 1..44 = K1
// product palette (id-1). See the K1 drawing rules below.
static volatile uint32_t k1_palette_select=0;
static K1PaletteSampler k1_palette_sampler=nullptr;  // registered by the runtime that links the K1 palettes
#define hsv donor_hsv
#define get_color_range_hue donor_get_color_range_hue
#define apply_gamma_correction donor_apply_gamma_correction
#include "emotiscope_donor/leds.h"
#undef apply_gamma_correction
#undef get_color_range_hue
#undef hsv
// Palettes 1-44 are drawn the way K1 drew them (K1 Spectrum, palette mode):
// the whole palette spans the strip (Colour slides it, Colour range unused),
// no saturation blend, level shaped 0.65v^2+0.35v (K1 SQUARE_ITER 1) and no
// final x^2 gamma (K1 ENABLE_OUTPUT_GAMMA 0). Palette 0 is the donor, unchanged.
// leds.h background keeps the donor hue slice (background defaults to 0).
static inline bool k1_palette_active(){
  const uint32_t p=k1_palette_select;
  return p!=0u&&p<=K1_EMOTISCOPE_PALETTES&&k1_palette_sampler!=nullptr;
}
float get_color_range_hue(float progress){
  if(!k1_palette_active())return donor_get_color_range_hue(progress);
  return configuration.reverse_color_range?(1.0f-configuration.color)-progress
                                          :configuration.color+progress;
}
void apply_gamma_correction(){
  if(!k1_palette_active())donor_apply_gamma_correction();
}
CRGBF hsv(float h,float s,float v){
  const uint32_t p=k1_palette_select;
  const K1PaletteSampler sample=k1_palette_sampler;
  if(p==0u||p>K1_EMOTISCOPE_PALETTES||!sample)return donor_hsv(h,s,v);
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
#include "emotiscope_donor/screensaver.h"
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
void transmit_leds(){
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
#include "emotiscope_donor/gpu_core.h"
static bool initialized=false;
static uint32_t audio_hops=0,render_frames=0,rejected_audio_chunks=0;
static uint32_t audio_gaps=0,fresh_hops=0;
static bool gap_rewarming=false;
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
  init_window_lookup();
  init_goertzel_constants_musical();
  init_tempo_goertzel_constants();
  init_noise_samples();
  init_floating_point_lookups();
  init_vu();
  init_random_dither_error();
  initialized=true;
  return true;
}
extern "C" bool k1_emotiscope_set_config(const K1EmotiscopeConfig* input){
  using namespace emotiscope;
  if(!initialized || !input || input->current_mode<0 || input->current_mode>7)return false;
  const float values[]={input->brightness,input->softness,input->color,input->warmth,
    input->color_range,input->speed,input->saturation,input->background};
  for(float value:values)if(!std::isfinite(value)||value<0.0f||value>1.0f)return false;
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
  run_gpu();
  ++render_frames;
  if(!frame_output_valid)return false;
  for(unsigned i=0;i<NUM_LEDS;++i){
    rgb[3*i]=raw_led_data[3*i+1];
    rgb[3*i+1]=raw_led_data[3*i];
    rgb[3*i+2]=raw_led_data[3*i+2];
  }
  return true;
}
extern "C" bool k1_emotiscope_set_palette(uint32_t id){
  if(id>K1_EMOTISCOPE_PALETTES)return false;
  emotiscope::k1_palette_select=id;return true;
}
extern "C" void k1_emotiscope_set_palette_sampler(K1PaletteSampler sampler){
  emotiscope::k1_palette_sampler=sampler;
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
