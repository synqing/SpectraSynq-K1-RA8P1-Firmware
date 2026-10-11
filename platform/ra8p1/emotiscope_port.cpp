// Emotiscope.HIL v1.1_build, source 7f86939bd7c16c4a6dd5789813e9d551246ed134.
// The donor AP and complete musical VP share one translation unit, as in Arduino.
// GPLv3 donor attribution and source notices are retained in emotiscope_donor/.
// Platform substitutions: deterministic clock/RNG, scalar ESP-DSP primitives,
// no network/NVS/physical touch, captured RGB8 output in place of ESP32 RMT.
// Native geometry remains 128. The Titan's 128->160/GRB48 mapping is external.
#include "emotiscope_port.h"
#include "core/visual/product_palette.h"
#include "emotiscope_edge_response.h"
#include "emotiscope_edge_colour.h"
#ifdef K1_TITAN_TOP_OUTPUT
#include "musical_conditioning.h"
#include "emotiscope_ml.h"
#include "composed_scene_runtime.h"
#include "emotiscope_secondary.h"
#endif
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if defined(K1_TITAN_TOP_OUTPUT) && !defined(K1_EMOTISCOPE_SECONDARY_INSTANCE)
static EmotiscopeMl shared_ml;
static bool shared_ml_demand[2]={false,false};
static EmotiscopeMl::Clock ml_cycles=nullptr,ml_clock=nullptr;
extern "C" void k1_emotiscope_ml_enable_board(unsigned board,bool enabled){
  if(board<2u)shared_ml_demand[board]=enabled;
  if(!shared_ml_demand[0]&&!shared_ml_demand[1])shared_ml.reset();
}
extern "C" void k1_emotiscope_set_ml_clocks(uint32_t (*cycles)(),uint32_t (*clock)()){
  ml_cycles=cycles;ml_clock=clock;
}
#endif
#ifdef K1_TITAN_TOP_OUTPUT
#ifndef K1_EMOTISCOPE_SECONDARY_INSTANCE
composed_scene::Board* k1_scene_boards[2]={nullptr,nullptr};
static composed_scene::Pair k1_scene_pair;
#else
extern composed_scene::Board* k1_scene_boards[2];
#endif
#endif
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
static K1EmotiscopeEdgeColourConfig edge_colour_config{0u,0u,0u,0.0f,30.0f};
static K1EmotiscopeEdgePairConfig edge_pair_config{0u,{0u,0u,0u,0.0f,30.0f}};
static K1EmotiscopeMusicalConfig musical_config{0u,0.0f,40.0f,160.0f};
static K1EmotiscopeMlConfig ml_config{0u,0.0f};
static float k1_frame_ml_offset=0.0f;
static volatile uint32_t k1_palette_select=K1_EMOTISCOPE_DEFAULT_PALETTE;
static K1PaletteSampler k1_palette_sampler=&k1_builtin_palette_sample;
// One complete colour/material snapshot for all contributions in this VP frame.
static EmotiscopeEdgeColourConfig k1_frame_edge;
static uint32_t k1_frame_pair=0, k1_frame_palette=K1_EMOTISCOPE_DEFAULT_PALETTE;
static K1PaletteSampler k1_frame_sampler=&k1_builtin_palette_sample;
static void k1_edge_begin_frame(){
  const auto pair=edge_pair_config;
  const auto c=pair.link?pair.colour:edge_colour_config;
  k1_frame_edge={c.enabled,c.mode,c.spatial,c.strength,c.spread_degrees};
  k1_frame_pair=pair.link;k1_frame_palette=k1_palette_select;
  k1_frame_sampler=k1_palette_sampler;
}

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
// Destination is a native128 pixel; retain the existing nearest-source physical
// geometry. Duplicated physical pixels share their source's representative mask.
static CRGBF k1_palette_colour_at(float u,unsigned destination){
  u+=k1_frame_ml_offset;
  float original[3]={},colour[3]={};
  k1_frame_sampler(k1_frame_palette-1u,u,original);
  std::memcpy(colour,original,sizeof(colour));
  const unsigned physical=(destination*159u+63u)/127u;
#ifdef K1_EMOTISCOPE_SECONDARY_INSTANCE
  constexpr bool primary=false;
#else
  constexpr bool primary=true;
#endif
  EmotiscopeEdgeColour::apply(k1_frame_edge,k1_frame_pair,primary,u,
      EmotiscopeEdgeColour::physical_centre_weight(physical),original,
      k1_frame_sampler,k1_frame_palette,colour);
  return CRGBF{colour[0],colour[1],colour[2]};
}
static CRGBF hsv_at(float u,float saturation,float value,unsigned destination){
  (void)saturation;
  const CRGBF c=k1_palette_colour_at(u,destination);
  const float level=0.65f*value*value+0.35f*value;
  return CRGBF{c.r*level,c.g*level,c.b*level};
}
// Keep authored u through each swept dot contribution, applying the mask at
// every covered destination before intensity/coverage and RGB composition.
static void draw_palette_dot(CRGBF* layer,uint16_t slot,float u,float value,
                             float position,float opacity=1.0f){
  float x1=fx_dots[slot].position*float(NUM_LEDS-1);
  float x2=position*float(NUM_LEDS-1);
  fx_dots[slot].position=position;
  if(x2<x1){const float swap=x1;x1=x2;x2=swap;}
  const int first=int(std::floor(x1)),last=int(std::ceil(x2));
  const float start=1.0f-(x1-first),end=x2-std::floor(x2);
  for(int i=first;i<=last;++i){
    if(i<0||i>=NUM_LEDS)continue;
    float mix=opacity;
    if(i==first)mix*=start;else if(i==last)mix*=end;
    mix=std::sqrt(mix);
    const CRGBF c=hsv_at(u,1.0f,value,static_cast<unsigned>(i));
    if(c.r!=0.0f||c.g!=0.0f||c.b!=0.0f){
      layer[i].r+=c.r*mix;layer[i].g+=c.g*mix;layer[i].b+=c.b*mix;
    }else{
      layer[i].r=layer[i].r*(1.0f-mix)+c.r*mix;
      layer[i].g=layer[i].g*(1.0f-mix)+c.g*mix;
      layer[i].b=layer[i].b*(1.0f-mix)+c.b*mix;
    }
  }
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
      const CRGBF c=k1_palette_colour_at(get_color_range_hue(num_leds_float_lookup[i]),i);
      leds[i].r+=c.r*level;leds[i].g+=c.g*level;leds[i].b+=c.b*level;
    }
  }else{
    for(uint16_t i=0;i<(NUM_LEDS>>1);i++){
      const CRGBF c=k1_palette_colour_at(get_color_range_hue(num_leds_float_lookup[i<<1]),(NUM_LEDS>>1)+i);
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
void run_screensaver(float elapsed_seconds){
  if(configuration.screensaver == false || !std::isfinite(elapsed_seconds) || elapsed_seconds < 0.0f){ return; }
  if(light_modes[configuration.current_mode].type != LIGHT_MODE_TYPE_ACTIVE){ return; }
  float mag_sum=0;
  for(uint16_t i=0;i<NUM_FREQS;i++)mag_sum+=frequencies_musical[i].magnitude;
  if(mag_sum<screensaver_threshold){
    if(inactive==false){inactive=true;inactive_start=t_now_ms;}
  }else if(mag_sum>screensaver_threshold){inactive=false;}
  if(device_touch_active==true||app_touch_active==true||slider_touch_active==true)inactive=false;
  if(inactive==false){ if(screensaver_mix>0.0)screensaver_mix=fmaxf(0.0f,screensaver_mix-elapsed_seconds); }
  if(inactive==true){
    if(t_now_ms-inactive_start>=SCREENSAVER_WAIT_MS){ if(screensaver_mix<1.0)screensaver_mix=fminf(1.0f,screensaver_mix+0.1f*elapsed_seconds); }
  }
  if(screensaver_mix>0.001){
    scale_CRGBF_array_by_constant(leds,1.0-(screensaver_mix*screensaver_mix),NUM_LEDS);
    const float push_val=0.005;
    for(uint16_t i=0;i<4;i++){
      k1_screensaver_last[i]=k1_palette_colour_at(0.125f+0.25f*i,64u);
      sine_positions[i]+=(push_val+(0.0001*i))*elapsed_seconds*100.0f;
      sine_positions[i]=fmodf(sine_positions[i],6.283185307179586f);
      draw_palette_dot(leds,SCREENSAVER_1+i,0.125f+0.25f*i,1.0f,sin(sine_positions[i])*(0.5*screensaver_mix)+0.5,screensaver_mix*screensaver_mix);
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
static uint32_t last_pcm_us=0;
#ifdef K1_TITAN_TOP_OUTPUT
static K1MusicalConditioning musical_conditioning;
static composed_scene::Board composed_board;
static K1EmotiscopeAudioPublication model_publication{},board_publication{};
static K1EmotiscopeMlResult ml_observation{};
static uint32_t conditioning_frame_us=0;
static bool conditioning_frame_seen=false;
#endif
static K1EmotiscopeAudioPublication musical_publication{};
static bool publication_seen=false;
static uint32_t musical_tick_us=0;
static void capture_musical_publication(){
  auto& p=musical_publication;
  ++p.sequence;p.audio_hops=audio_hops;p.now_us=port_clock_us;p.valid=fresh_hops>=64u&&!gap_rewarming&&(port_clock_us-last_pcm_us<=30000u);
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
static void import_musical_publication(const K1EmotiscopeAudioPublication& p,float gain){
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
  (void)edge_response.apply(reinterpret_cast<float*>(leds),
#ifdef K1_TITAN_TOP_OUTPUT
      board_publication,
#else
      musical_publication,
#endif
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
#ifdef K1_TITAN_TOP_OUTPUT
  float musical_hz[NUM_FREQS];
  for(unsigned i=0;i<NUM_FREQS;++i)musical_hz[i]=notes[BOTTOM_NOTE+NOTE_STEP*i];
  if(!musical_conditioning.initialise_frequencies(musical_hz))return false;
  composed_board.initialise(musical_hz);
#ifdef K1_EMOTISCOPE_SECONDARY_INSTANCE
  k1_scene_boards[1]=&composed_board;
#else
  k1_scene_boards[0]=&composed_board;
  float timing_hz[NUM_TEMPI];for(unsigned i=0;i<NUM_TEMPI;++i)timing_hz[i]=tempi[i].target_tempo_hz;
  if(!k1_scene_pair.configureTiming(timing_hz))return false;
#endif
#endif

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
  k1_edge_begin_frame();
  if(stage==0){
    std::memset(leds,0,sizeof(leds));apply_background(arg);std::memcpy(rgbf,leds,sizeof(leds));
  }else if(stage==1){
    std::memcpy(leds,rgbf,sizeof(leds));apply_tonemapping();std::memcpy(rgbf,leds,sizeof(leds));
  }else if(stage==2){
    std::memset(leds,0,sizeof(leds));
    for(auto& c:k1_screensaver_last)c=CRGBF{-1.0f,-1.0f,-1.0f};
    const bool was=configuration.screensaver;configuration.screensaver=true;
    screensaver_mix=1.0f;inactive=true;inactive_start=t_now_ms;
    run_screensaver(0.01f);configuration.screensaver=was;
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
  last_pcm_us=now_us;
  ++audio_hops;
  if(fresh_hops<64u)++fresh_hops;
  if(gap_rewarming){
    if(fresh_hops<64u)mute_audio_publication();
    else gap_rewarming=false;
  }
  return true;
}
extern "C" bool k1_emotiscope_prepare_frame(uint32_t now_us){
  using namespace emotiscope;
  if(!initialized || !advance_port_clock(now_us))return false;
#ifndef K1_EMOTISCOPE_SECONDARY_INSTANCE
  // Shared observation is prepared before either VP. Repeated preparation at
  // the same timestamp is idempotent; no renderer advances musical analysis.
  if(!publication_seen || now_us!=musical_tick_us){
    t_now_us=micros();t_now_ms=millis();
    const float delta=publication_seen?float(now_us-musical_tick_us)/(1000000/REFERENCE_FPS):0.0f;
    update_novelty();update_tempi_phase(delta);
    musical_tick_us=now_us;publication_seen=true;capture_musical_publication();
#ifdef K1_TITAN_TOP_OUTPUT
    shared_ml.prepare(musical_publication,now_us,shared_ml_demand[0]||shared_ml_demand[1],ml_cycles,ml_clock);
    ml_observation=shared_ml.result();
#endif
  }
#ifdef K1_TITAN_TOP_OUTPUT
  // A same-clock disable/reset must invalidate the copied result immediately,
  // without admitting another model execution or advancing the shared tick.
  ml_observation=shared_ml.result();
#endif
#else
  if(!publication_seen || musical_publication.now_us!=now_us)return false;
#endif
  return true;
}
#ifdef K1_TITAN_TOP_OUTPUT
namespace emotiscope {
static composed_motion::Rgb composed_palette(float u,void*){
  const auto c=k1_palette_colour_at(get_color_range_hue(u),64u+unsigned(u*63.0f));
  return {c.r,c.g,c.b};
}
static void run_composed(){
  t_now_us=micros();t_now_ms=millis();
  const auto state=composed_board.status();
  const auto scope=composed_board.active.quiet_scope;
  const bool protected_quiet=composed_board.forced_quiet || (state.quiet && (scope==1u||scope==2u));
  for(unsigned i=0;i<NUM_LEDS;++i){
    const auto c=composed_board.pixels[i];leds[i]={c.r,c.g,c.b};
  }
  // These scenes own their history: donor image LPF would leak old pixels into
  // the protected plateau and recolour/blur the authored travelling material.
  if(!protected_quiet){
    const float support=composed_board.motion.active()?.25f:1.0f;
    if(!state.quiet || scope==0u || scope==3u)apply_background(configuration.background*support);
    if(scope==0u)run_screensaver(.01f);
    apply_brightness();apply_tonemapping();apply_warmth(configuration.warmth);
    multiply_CRGBF_array_by_LUT(leds,WHITE_BALANCE,NUM_LEDS);
  }else{
    std::memset(leds,0,sizeof(leds));std::memset(dither_error,0,sizeof(dither_error));
  }
  transmit_leds();
}
}
extern "C" void k1_emotiscope_scene_prepare(uint32_t now_us){
  using namespace emotiscope;
  if(composed_board.observed && composed_board.now_us==now_us)return;
  if(!k1_emotiscope_prepare_frame(now_us))return;
  const auto frame_ml=ml_config;
  EmotiscopeMl::apply(musical_publication,ml_observation,frame_ml,now_us,model_publication);
  const float dt=conditioning_frame_seen?float(now_us-conditioning_frame_us)*.000001f:.01f;
  musical_conditioning.apply(model_publication,board_publication,dt,now_us);
  conditioning_frame_us=now_us;conditioning_frame_seen=true;
  k1_frame_ml_offset=(composed_board.requested.scene && composed_board.requested.ml_target!=2u)?0.0f:
    EmotiscopeMl::palette_offset(musical_publication,ml_observation,frame_ml,now_us);
  k1_edge_begin_frame();
  composed_board.palette=&composed_palette;
  composed_board.observe(musical_publication,ml_observation,frame_ml,now_us,millis());
  // Existing per-board musical interpretation and response remain real scene consumers.
  composed_board.level=musical_publication.valid?composed_motion::unit(board_publication.vu_level*response_gain):0.0f;
  composed_board.strength=composed_scene::Board::gestureStrength(composed_board.level,composed_board.admission.value());
}
#ifndef K1_EMOTISCOPE_SECONDARY_INSTANCE
extern "C" void k1_emotiscope_scene_pair_frame(uint32_t now_us){
  k1_emotiscope_scene_prepare(now_us);
  k1_secondary_emotiscope_scene_prepare(now_us);
  k1_scene_pair.updateTiming(emotiscope::musical_publication,emotiscope::millis());
  k1_scene_pair.render(k1_scene_boards[0],k1_scene_boards[1],now_us);
}
#endif
extern "C" bool k1_emotiscope_set_scene_config(const K1ComposedSceneConfig* c){
  if(!c || !emotiscope::composed_board.configure(*c))return false;
  // Pair ownership is separate from the selected board's motion parameters.
  for(auto* b:k1_scene_boards)if(b){
    b->requested.choreography=c->choreography;b->requested.initiator=c->initiator;
    b->requested.answer_delay_ms=c->answer_delay_ms;
  }
  return true;
}
extern "C" void k1_emotiscope_get_scene_config(K1ComposedSceneConfig* c){if(c)*c=emotiscope::composed_board.requested;}
extern "C" void k1_emotiscope_scene_status(K1ComposedSceneStatus* s){if(s)*s=emotiscope::composed_board.status();}
#endif
extern "C" bool k1_emotiscope_render(uint32_t now_us,uint8_t* rgb){
  using namespace emotiscope;
  if(!rgb || !k1_emotiscope_prepare_frame(now_us))return false;
#ifdef K1_TITAN_TOP_OUTPUT
  k1_emotiscope_scene_prepare(now_us);
  import_musical_publication(board_publication,response_gain);
#else
  import_musical_publication(musical_publication,response_gain);
#endif
  k1_edge_begin_frame();
#ifdef K1_TITAN_TOP_OUTPUT
  if(composed_board.active.scene||composed_board.forced_quiet){
    if(!composed_board.rendered || composed_board.rendered_us!=now_us){
      const bool launch=composed_board.eligible()&&composed_board.fresh_event;
      composed_board.render(launch?composed_board.event:0u,composed_board.strength);
    }
    run_composed();
  }else
#endif
  run_gpu();
  // Restore the immutable raw observation: visual response must not enter the
  // next analyser hop or another board's response.
  import_musical_publication(musical_publication,1.0f);
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
static EmotiscopeEdgeColourConfig emotiscope_colour_helper(const K1EmotiscopeEdgeColourConfig& c){
  return {c.enabled,c.mode,c.spatial,c.strength,c.spread_degrees};
}
extern "C" bool k1_emotiscope_set_edge_colour(const K1EmotiscopeEdgeColourConfig* c){
  if(!c || !EmotiscopeEdgeColour::valid(emotiscope_colour_helper(*c)))return false;
  emotiscope::edge_colour_config=*c;return true;
}
extern "C" void k1_emotiscope_get_edge_colour(K1EmotiscopeEdgeColourConfig* c){if(c)*c=emotiscope::edge_colour_config;}
extern "C" bool k1_emotiscope_set_edge_pair(const K1EmotiscopeEdgePairConfig* c){
  if(!c || c->link>3u || !EmotiscopeEdgeColour::valid(emotiscope_colour_helper(c->colour)))return false;
  emotiscope::edge_pair_config=*c;return true;
}
extern "C" void k1_emotiscope_get_edge_pair(K1EmotiscopeEdgePairConfig* c){if(c)*c=emotiscope::edge_pair_config;}
extern "C" uint32_t k1_emotiscope_effective_edge_pair(){
  const auto& c=emotiscope::edge_pair_config;return EmotiscopeEdgeColour::effective_pair(c.colour.mode,c.link);
}
extern "C" bool k1_emotiscope_set_musical_config(const K1EmotiscopeMusicalConfig* c){
  if(!c || c->role>2u || !emotiscope_unit_float(c->strength) ||
      !std::isfinite(c->attack_ms) || c->attack_ms<1.0f || c->attack_ms>1000.0f ||
      !std::isfinite(c->release_ms) || c->release_ms<1.0f || c->release_ms>3000.0f)return false;
#ifdef K1_TITAN_TOP_OUTPUT
  if(!emotiscope::musical_conditioning.configure(*c))return false;
#else
  if(c->role!=0u||c->strength!=0.0f)return false;
#endif
  emotiscope::musical_config=*c;return true;
}
extern "C" void k1_emotiscope_get_musical_config(K1EmotiscopeMusicalConfig* c){if(c)*c=emotiscope::musical_config;}
extern "C" bool k1_emotiscope_set_ml_config(const K1EmotiscopeMlConfig* c){
  if(!c || c->enabled>1u || !emotiscope_unit_float(c->strength))return false;
#ifdef K1_TITAN_TOP_OUTPUT
#ifdef K1_EMOTISCOPE_SECONDARY_INSTANCE
  constexpr unsigned board=1u;
#else
  constexpr unsigned board=0u;
#endif
  if(emotiscope::ml_config.enabled!=c->enabled)emotiscope::ml_observation.valid=0u;
  k1_emotiscope_ml_enable_board(board,c->enabled!=0u);
#else
  if(c->enabled!=0u||c->strength!=0.0f)return false;
#endif
  emotiscope::ml_config=*c;return true;
}
extern "C" void k1_emotiscope_get_ml_config(K1EmotiscopeMlConfig* c){if(c)*c=emotiscope::ml_config;}
extern "C" void k1_emotiscope_get_ml_result(K1EmotiscopeMlResult* out){
  if(!out)return;
#ifdef K1_TITAN_TOP_OUTPUT
  *out=emotiscope::ml_observation;
  if(!emotiscope::musical_publication.valid ||
      emotiscope::port_clock_us-out->now_us>100000u)out->valid=0u;
#else
  std::memset(out,0,sizeof(*out));
#endif
}
extern "C" void k1_emotiscope_musical_state(uint32_t* cursor,float* value){
#ifdef K1_TITAN_TOP_OUTPUT
  if(cursor)*cursor=emotiscope::musical_conditioning.event_cursor();
  if(value)*value=emotiscope::musical_conditioning.value();
#else
  if(cursor)*cursor=0u;if(value)*value=0.0f;
#endif
}
#if defined(K1_TITAN_TOP_OUTPUT) && defined(K1_EMOTISCOPE_SECONDARY_INSTANCE)
extern "C" void k1_secondary_emotiscope_accept_ml(const K1EmotiscopeMlResult* result){
  if(result)emotiscope::ml_observation=*result;
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
  ++audio_gaps;fresh_hops=0;gap_rewarming=true;musical_publication.valid=0u;
#ifdef K1_TITAN_TOP_OUTPUT
  musical_conditioning.reset();ml_observation.valid=0u;
#ifndef K1_EMOTISCOPE_SECONDARY_INSTANCE
  shared_ml.reset();
#endif
#endif
  std::memset(sample_history,0,sizeof(sample_history));
  mute_audio_publication();
}
extern "C" void k1_emotiscope_status(K1EmotiscopeStatus* out){
  using namespace emotiscope;
  if(!out)return;
  std::memset(out,0,sizeof(*out));
#ifdef K1_EMOTISCOPE_SECONDARY_INSTANCE
  out->valid=initialized && musical_publication.valid && port_clock_us-musical_publication.now_us<=30000u;
#else
  out->valid=initialized && fresh_hops>=64u && !gap_rewarming && port_clock_us-last_pcm_us<=30000u;
#endif
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

#if defined(K1_TITAN_TOP_OUTPUT) && !defined(K1_EMOTISCOPE_SECONDARY_INSTANCE)
extern "C" bool k1_emotiscope_ml_witness(K1EmotiscopeMlResult* result,float input[64],uint32_t now_us){
  if(!result||!input)return false;
  *result=shared_ml.result();
  const bool valid=emotiscope::musical_publication.valid &&
    now_us-emotiscope::last_pcm_us<=30000u && shared_ml.inference_input(input,now_us);
  if(!valid){result->valid=0u;std::memset(input,0,64u*sizeof(float));
    std::memset(result->latent,0,sizeof(result->latent));
    std::memset(result->reconstruction,0,sizeof(result->reconstruction));}
  return valid;
}
#endif
