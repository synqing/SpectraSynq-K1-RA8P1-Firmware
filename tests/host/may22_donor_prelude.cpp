// Host prelude for pinned donor e78b6f686ee99fa4463d6fd4773d813fa0e8b319.
// Effect, palette, chromagram and waveform functions below this prelude are
// copied from that commit. This file supplies the types those functions call.
// SQ15x16 is binary64 here, not the FixedPoints 16.16 type.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

struct SQ15x16 {
  double v = 0;
  SQ15x16() = default;
  SQ15x16(double x) : v(x) {}
  SQ15x16(int x) : v(x) {}
  SQ15x16(unsigned x) : v(x) {}
  explicit operator float() const { return float(v); }
  explicit operator double() const { return v; }
  explicit operator int() const { return int(v); }
  explicit operator uint8_t() const { return uint8_t(int(v)); }
  int getInteger() const { return int(v); }
  SQ15x16 operator+(SQ15x16 o) const { return v + o.v; }
  SQ15x16 operator-(SQ15x16 o) const { return v - o.v; }
  SQ15x16 operator*(SQ15x16 o) const { return v * o.v; }
  SQ15x16 operator/(SQ15x16 o) const { return v / o.v; }
  SQ15x16 operator+(double o) const { return v + o; }
  SQ15x16 operator-(double o) const { return v - o; }
  SQ15x16 operator*(double o) const { return v * o; }
  SQ15x16 operator/(double o) const { return v / o; }
  SQ15x16& operator+=(SQ15x16 o) { v += o.v; return *this; }
  SQ15x16& operator-=(SQ15x16 o) { v -= o.v; return *this; }
  SQ15x16& operator*=(SQ15x16 o) { v *= o.v; return *this; }
  SQ15x16& operator/=(SQ15x16 o) { v /= o.v; return *this; }
  SQ15x16& operator+=(double o) { v += o; return *this; }
  SQ15x16& operator-=(double o) { v -= o; return *this; }
  SQ15x16& operator*=(double o) { v *= o; return *this; }
  bool operator<(SQ15x16 o) const { return v < o.v; }
  bool operator>(SQ15x16 o) const { return v > o.v; }
  bool operator<=(SQ15x16 o) const { return v <= o.v; }
  bool operator>=(SQ15x16 o) const { return v >= o.v; }
  bool operator<(double o) const { return v < o; }
  bool operator>(double o) const { return v > o; }
  bool operator<=(double o) const { return v <= o; }
  bool operator>=(double o) const { return v >= o; }
  bool operator==(SQ15x16 o) const { return v == o.v; }
};
inline SQ15x16 operator-(double a, SQ15x16 b) { return a - b.v; }
inline SQ15x16 operator+(double a, SQ15x16 b) { return a + b.v; }
inline SQ15x16 operator*(double a, SQ15x16 b) { return a * b.v; }
inline SQ15x16 operator/(double a, SQ15x16 b) { return a / b.v; }
enum { NATIVE_RESOLUTION = 160, NUM_FREQS = 80, NUM_ZONES = 2, NUM_AGC_BANDS = 4,
       SAMPLE_HISTORY_LENGTH = 4096, SPECTRAL_HISTORY_LENGTH = 5 };
struct Freq { float target_freq=0; uint16_t block_size=0; float inv_block_size_half=0, block_size_recip=0, window_mult=0, zone=0; int32_t coeff_q14=0; };
Freq frequencies[NUM_FREQS];
int32_t magnitudes[NUM_FREQS] = {};
float magnitudes_normalized[NUM_FREQS] = {};
float magnitudes_normalized_avg[NUM_FREQS] = {};
float magnitudes_final[NUM_FREQS] = {};
float magnitudes_last[NUM_FREQS] = {};
float noise_samples[NUM_FREQS] = {};
float max_mags[NUM_ZONES] = {};
short sample_window[SAMPLE_HISTORY_LENGTH] = {};
SQ15x16 spectrogram[NUM_FREQS];
SQ15x16 spectrogram_smooth[NUM_FREQS];
SQ15x16 chromagram_smooth[12];
SQ15x16 spectral_tilt_lut[NUM_FREQS];
SQ15x16 spectral_history[SPECTRAL_HISTORY_LENGTH][NUM_FREQS];
SQ15x16 novelty_curve[SPECTRAL_HISTORY_LENGTH];
int16_t spectral_history_index = 0;
uint8_t spectrogram_history_index = 0;
const uint8_t spectrogram_history_length = 3;
struct AgcBand { SQ15x16 gain, target_gain, energy; };
AgcBand agc_bands[NUM_AGC_BANDS];
uint32_t vp_dbg_chroma_seq = 0;
uint8_t vp_dbg_chroma_range = 0;
SQ15x16 vp_dbg_chroma_pre_max, vp_dbg_chroma_pre_mean, vp_dbg_chroma_norm_max, vp_dbg_chroma_norm_mean;
SQ15x16 vp_dbg_chroma_final_max, vp_dbg_chroma_final_mean, vp_dbg_chroma_flatness;
const float notes[] = {
  55,58.27047f,61.73541f,65.40639f,69.29566f,73.41619f,77.78175f,82.40689f,87.30706f,92.49861f,97.99886f,103.8262f,
  110,116.5409f,123.4708f,130.8128f,138.5913f,146.8324f,155.5635f,164.8138f,174.6141f,184.9972f,195.9977f,207.6523f,
  220,233.0819f,246.9417f,261.6256f,277.1826f,293.6648f,311.1270f,329.6276f,349.2282f,369.9944f,391.9954f,415.3047f,
  440,466.1638f,493.8833f,523.2511f,554.3653f,587.3295f,622.2540f,659.2551f,698.4565f,739.9888f,783.9909f,830.6094f,
  880,932.3275f,987.7666f,1046.502f,1108.731f,1174.659f,1244.508f,1318.510f,1396.913f,1479.978f,1567.982f,1661.219f,
  1760,1864.655f,1975.533f,2093.005f,2217.461f,2349.318f,2489.016f,2637.020f,2793.825f,2959.956f,3135.964f,3322.437f,
  3520,3729.310f,3951.065f,4186.009f,4434.922f,4698.636f,4978.032f,5274.041f,5587.652f,5919.911f,6271.927f,6644.875f,
  7040,7458.620f,7902.130f,8372.018f,8869.844f,9397.272f,9956.064f,10548.08f,11175.30f,11839.82f,12543.85f,13289.75f
};

enum { LIGHT_MODE_BLOOM = 3, LIGHT_MODE_WAVEFORM_FAST = 7 };
#define ENABLE_HSV_SOFT_CLIP 1
#define SOFT_CLIP_KNEE 0.85f
#define SOFT_CLIP_ROLLOFF 0.4f
#define PHOTONS_CURVE_MODE 0
#define ENABLE_WAVEFORM_CHROMAGRAM_COLOR 1
#define WAVEFORM_REACTIVE_RAW_MARGIN 1.10f
#define WAVEFORM_REACTIVE_PEAK_FLOOR 0.08f
#define WAVEFORM_IDLE_FADE 0.85f
#define MAGNITUDES_AVG_ATTACK 0.3f
#define MAGNITUDES_AVG_RELEASE 0.3f
#define SPECTROGRAM_SMOOTH_ATTACK 0.75f
#define SPECTROGRAM_SMOOTH_RELEASE 0.75f
#define ENABLE_OUTPUT_GAMMA 0
#define IRAM_ATTR
#define isfinite std::isfinite
static inline uint8_t apply_gamma8(uint8_t v) { return v; }

struct Conf {
  float PHOTONS = 0.200195f, CHROMA = 0, MOOD = 0.116638f, SATURATION = 1;
  float SQUARE_ITER = 1, SENSITIVITY = 2.4f, INCANDESCENT_FILTER = 0.50f, BULB_OPACITY = 0.13f;
  float SWEET_SPOT_MIN_LEVEL = 302, SWEET_SPOT_MAX_LEVEL = 30000, DC_OFFSET = -8084;
  uint32_t SAMPLE_RATE = 12800;
  uint16_t SAMPLES_PER_CHUNK = 96, LED_COUNT = 160, CHROMAGRAM_RANGE = 60;
  uint8_t NOTE_OFFSET = 12, LIGHTSHOW_MODE = LIGHT_MODE_BLOOM, PALETTE_INDEX = 0;
  bool MIRROR_ENABLED = true, AUTO_COLOR_SHIFT = true, PALETTE_MODE_ENABLED = false;
  bool INCANDESCENT_MODE = false, BASE_COAT = false, TEMPORAL_DITHERING = true;
  bool REVERSE_ORDER = false, STANDBY_DIMMING = false;
};
Conf CONFIG;
float SECONDARY_PHOTONS = 1, SECONDARY_CHROMA = 0.033325f, SECONDARY_MOOD = 0.216614f, SECONDARY_SATURATION = 1;
bool SECONDARY_MIRROR_ENABLED = true, SECONDARY_AUTO_COLOR_SHIFT = true;
bool SECONDARY_PALETTE_MODE_ENABLED = true, SECONDARY_INCANDESCENT_MODE = false;
uint8_t SECONDARY_PALETTE_INDEX = 10, SECONDARY_LIGHTSHOW_MODE = LIGHT_MODE_WAVEFORM_FAST;
float SECONDARY_INCANDESCENT_FILTER = 0.5f;
bool VP_FIX_AGC_SOFT_KNEE = true, VP_FIX_CHROMAGRAM_SPARSENESS = true, VP_FIX_BLOOM_DECAY = false;
bool VP_FIX_SECONDARY_CLEAN = true, VP_FIX_PRISM_DEFAULT_OFF = true, VP_BLOOM_FORCE_SATURATION = true;
float VP_BLOOM_ALPHA = 0.99f, VP_BLOOM_SHIFT_SCALE = 1;
bool vp_render_secondary_channel = false, chromatic_mode = true, debug_mode = false;
bool palette_owns_render_colour_source() {
  return vp_render_secondary_channel ? SECONDARY_PALETTE_MODE_ENABLED : CONFIG.PALETTE_MODE_ENABLED;
}
bool noise_complete = true, noise_transition_queued = false, mode_transition_queued = false;
int noise_iterations = 256;
float SYSTEM_FPS = 48.26f, LED_FPS = 115.86f, MASTER_BRIGHTNESS = 1, silent_scale = 1;
float waveform_peak_scaled = 0, max_waveform_val = 0, max_waveform_val_follower = 302;
uint32_t max_waveform_val_raw = 0;
SQ15x16 hue_position = 0, hue_shift_speed = 0, hue_destination = 0.2, hue_shifting_mix = -0.35;
SQ15x16 hue_shifting_mix_target = 1, hue_push_direction = 1, chroma_val = 0;
SQ15x16 agc_envelope = 0, agc_noise_floor = 0.001;
bool agc_gated = true;
struct CRGB16 { SQ15x16 r, g, b; };
CRGB16 leds_16[NATIVE_RESOLUTION];
CRGB16 leds_16_prev[NATIVE_RESOLUTION];
CRGB16 leds_16_prev_secondary[NATIVE_RESOLUTION];
CRGB16 leds_16_temp[NATIVE_RESOLUTION];
CRGB16 leds_scaled[NATIVE_RESOLUTION];
CRGB16 leds_16_secondary[NATIVE_RESOLUTION];
CRGB16 incandescent_lookup = {1.0000, 0.4453, 0.1562};
const SQ15x16 dither_table[4] = {SQ15x16(0.25), SQ15x16(0.50), SQ15x16(0.75), SQ15x16(1.00)};
uint8_t dither_step = 0;
struct CHSV;
struct CRGB { uint8_t r=0,g=0,b=0; CRGB()=default; CRGB(uint8_t R,uint8_t G,uint8_t B):r(R),g(G),b(B){} CRGB(const CHSV& hsv); };
struct CHSV {
  union { struct { uint8_t hue, sat, val; }; struct { uint8_t h, s, v; }; };
  CHSV() : hue(0), sat(0), val(0) {}
  CHSV(uint8_t H, uint8_t S, uint8_t V) : hue(H), sat(S), val(V) {}
  void setHSV(uint8_t H, uint8_t S, uint8_t V) { hue=H; sat=S; val=V; }
};
CRGB leds_out[NATIVE_RESOLUTION];
void hsv2rgb_rainbow(const CHSV&, CRGB&);
CHSV rgb2hsv_approximate(const CRGB&);
inline CRGB from_hsv(CHSV h){ CRGB c; hsv2rgb_rainbow(h,c); return c; }

struct StubSerial {
  template<class T> StubSerial& print(T){return *this;}
  template<class T> StubSerial& println(T){return *this;}
  StubSerial& println(){return *this;}
} USBSerial;
uint32_t millis(){ return 10000; }
SQ15x16 fabs_fixed(SQ15x16 x){ return x.v < 0 ? SQ15x16(-x.v) : x; }
float random_float(){ static uint32_t s=0x12345678u; s=s*1664525u+1013904223u; return (s>>8) / 16777216.f; }
void save_ambient_noise_calibration(){}
void save_config(){}

static inline uint8_t scale8(uint8_t i, uint16_t scale){ return uint8_t((uint16_t(i) * scale) >> 8); }
static inline uint8_t qadd8(uint8_t i, uint8_t j){ unsigned s=unsigned(i)+j; return s>255?255:uint8_t(s); }
static inline uint8_t qsub8(uint8_t i, uint8_t j){ return i<j?0:uint8_t(i-j); }
static inline uint8_t sqrt16(uint16_t x){ return uint8_t(sqrtf(float(x))); }
#define K255 255
#define K171 171
#define K170 170
#define K85 85
#define FORCE_REFERENCE(x) ((void)(x))
#define HUE_RED 0
#define HUE_ORANGE 32
#define HUE_YELLOW 64
#define HUE_GREEN 96
#define HUE_AQUA 128
#define HUE_BLUE 160
#define HUE_PURPLE 192
#define HUE_PINK 224
#define FIXFRAC8(N,D) (((N)*256)/(D))
#define FASTLED_SCALE8_FIXED 1
#define scale8_video scale8
#define scale8_LEAVING_R1_DIRTY scale8
#define scale8_video_LEAVING_R1_DIRTY scale8
#define cleanup_R1() ((void)0)

enum TBlendType { NOBLEND=0, LINEARBLEND=1, LINEARBLEND_NOWRAP=2 };
struct CRGBPalette16 { CRGB entries[16]{}; CRGBPalette16()=default; CRGBPalette16(const uint8_t* progpal); CRGB& operator[](int i){return entries[i];} const CRGB& operator[](int i) const {return entries[i];} };
using saccum87 = int16_t; using accum88 = uint16_t;
void fill_gradient_RGB(CRGB* leds, uint16_t startpos, CRGB startcolor, uint16_t endpos, CRGB endcolor) {
  if (endpos < startpos) { std::swap(startpos,endpos); std::swap(startcolor,endcolor); }
  saccum87 rdistance87 = (int16_t(endcolor.r) - int16_t(startcolor.r)) << 7;
  saccum87 gdistance87 = (int16_t(endcolor.g) - int16_t(startcolor.g)) << 7;
  saccum87 bdistance87 = (int16_t(endcolor.b) - int16_t(startcolor.b)) << 7;
  uint16_t pixeldistance = endpos - startpos;
  int16_t divisor = pixeldistance ? pixeldistance : 1;
  saccum87 rdelta87 = rdistance87 / divisor; rdelta87 = saccum87(rdelta87 * 2);
  saccum87 gdelta87 = gdistance87 / divisor; gdelta87 = saccum87(gdelta87 * 2);
  saccum87 bdelta87 = bdistance87 / divisor; bdelta87 = saccum87(bdistance87 * 2);
  accum88 r88 = accum88(startcolor.r) << 8, g88 = accum88(startcolor.g) << 8, b88 = accum88(startcolor.b) << 8;
  for (uint16_t i = startpos; i <= endpos; ++i) {
    leds[i] = CRGB(uint8_t(r88 >> 8), uint8_t(g88 >> 8), uint8_t(b88 >> 8));
    r88 = accum88(int(r88) + rdelta87); g88 = accum88(int(g88) + gdelta87); b88 = accum88(int(b88) + bdelta87);
  }
}
union TRGBGradientPaletteEntryUnion { uint32_t dword; struct { uint8_t index,r,g,b; }; };
#define FL_PGM_READ_DWORD_NEAR(p) ((p)->dword)
const uint8_t es_vintage_01_gp[] = {0,4,1,1, 51,16,0,1, 76,97,104,3, 101,255,131,19, 127,67,9,4, 153,16,0,1, 229,4,1,1, 255,4,1,1};
const uint8_t unused_gp[] = {0,0,0,0, 255,0,0,0};
const uint8_t* gGradientPalettes[11] = {unused_gp,unused_gp,unused_gp,unused_gp,unused_gp,unused_gp,unused_gp,unused_gp,unused_gp,unused_gp, es_vintage_01_gp};
const int gGradientPaletteCount = 11;
CRGBPalette16 load_palette(const uint8_t* progpal) {
  CRGBPalette16 pal{};
  auto* progent = reinterpret_cast<const TRGBGradientPaletteEntryUnion*>(progpal);
  TRGBGradientPaletteEntryUnion u; uint16_t count=0;
  do { u.dword = FL_PGM_READ_DWORD_NEAR(progent + count); ++count; } while (u.index != 255);
  int8_t lastSlotUsed = -1;
  u.dword = FL_PGM_READ_DWORD_NEAR(progent);
  CRGB rgbstart(u.r,u.g,u.b); int indexstart=0;
  while (indexstart < 255) {
    ++progent; u.dword = FL_PGM_READ_DWORD_NEAR(progent);
    int indexend=u.index; CRGB rgbend(u.r,u.g,u.b);
    int istart8=indexstart/16, iend8=indexend/16;
    if (count < 16) {
      if (istart8 <= lastSlotUsed && lastSlotUsed < 15) { istart8 = lastSlotUsed+1; if (iend8 < istart8) iend8 = istart8; }
      lastSlotUsed = int8_t(iend8);
    }
    fill_gradient_RGB(pal.entries, uint16_t(istart8), rgbstart, uint16_t(iend8), rgbend);
    indexstart = indexend; rgbstart = rgbend;
  }
  return pal;
}
CRGBPalette16::CRGBPalette16(const uint8_t* progpal) { *this = load_palette(progpal); }
CRGB ColorFromPalette(const CRGBPalette16& pal, uint8_t index, uint8_t brightness, TBlendType blendType) {
  uint8_t hi4 = index >> 4, lo4 = index & 0x0F;
  const CRGB* entry = &pal.entries[hi4];
  uint8_t red1=entry->r, green1=entry->g, blue1=entry->b;
  if (lo4 && blendType != NOBLEND) {
    entry = hi4==15 ? &pal.entries[0] : entry+1;
    uint8_t f2 = lo4 << 4, f1 = 255 - f2;
    uint8_t red2=entry->r; red1=scale8(red1,f1); red2=scale8(red2,f2); red1=uint8_t(red1+red2);
    uint8_t green2=entry->g; green1=scale8(green1,f1); green2=scale8(green2,f2); green1=uint8_t(green1+green2);
    uint8_t blue2=entry->b; blue1=scale8(blue1,f1); blue2=scale8(blue2,f2); blue1=uint8_t(blue1+blue2);
  }
  if (brightness != 255) {
    if (!brightness) return {};
    ++brightness;
    if (red1) red1=scale8(red1,brightness);
    if (green1) green1=scale8(green1,brightness);
    if (blue1) blue1=scale8(blue1,brightness);
  }
  return CRGB(red1,green1,blue1);
}
