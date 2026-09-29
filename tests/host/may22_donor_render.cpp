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

void hsv2rgb_rainbow( const CHSV& hsv, CRGB& rgb)
{
    // Yellow has a higher inherent brightness than
    // any other color; 'pure' yellow is perceived to
    // be 93% as bright as white.  In order to make
    // yellow appear the correct relative brightness,
    // it has to be rendered brighter than all other
    // colors.
    // Level Y1 is a moderate boost, the default.
    // Level Y2 is a strong boost.
    const uint8_t Y1 = 1;
    const uint8_t Y2 = 0;
    
    // G2: Whether to divide all greens by two.
    // Depends GREATLY on your particular LEDs
    const uint8_t G2 = 0;
    
    // Gscale: what to scale green down by.
    // Depends GREATLY on your particular LEDs
    const uint8_t Gscale = 0;
    
    
    uint8_t hue = hsv.hue;
    uint8_t sat = hsv.sat;
    uint8_t val = hsv.val;
    
    uint8_t offset = hue & 0x1F; // 0..31
    
    // offset8 = offset * 8
    uint8_t offset8 = offset;
    {
#if defined(__AVR__)
        // Left to its own devices, gcc turns "x <<= 3" into a loop
        // It's much faster and smaller to just do three single-bit shifts
        // So this business is to force that.
        offset8 <<= 1;
        
        offset8 <<= 1;
        
        offset8 <<= 1;
#else
        // On ARM and other non-AVR platforms, we just shift 3.
        offset8 <<= 3;
#endif
    }
    
    uint8_t third = scale8( offset8, (256 / 3)); // max = 85
    
    uint8_t r, g, b;
    
    if( ! (hue & 0x80) ) {
        // 0XX
        if( ! (hue & 0x40) ) {
            // 00X
            //section 0-1
            if( ! (hue & 0x20) ) {
                // 000
                //case 0: // R -> O
                r = K255 - third;
                g = third;
                b = 0;
                FORCE_REFERENCE(b);
            } else {
                // 001
                //case 1: // O -> Y
                if( Y1 ) {
                    r = K171;
                    g = K85 + third ;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
                if( Y2 ) {
                    r = K170 + third;
                    //uint8_t twothirds = (third << 1);
                    uint8_t twothirds = scale8( offset8, ((256 * 2) / 3)); // max=170
                    g = K85 + twothirds;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
            }
        } else {
            //01X
            // section 2-3
            if( !  (hue & 0x20) ) {
                // 010
                //case 2: // Y -> G
                if( Y1 ) {
                    //uint8_t twothirds = (third << 1);
                    uint8_t twothirds = scale8( offset8, ((256 * 2) / 3)); // max=170
                    r = K171 - twothirds;
                    g = K170 + third;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
                if( Y2 ) {
                    r = K255 - offset8;
                    g = K255;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
            } else {
                // 011
                // case 3: // G -> A
                r = 0;
                FORCE_REFERENCE(r);
                g = K255 - third;
                b = third;
            }
        }
    } else {
        // section 4-7
        // 1XX
        if( ! (hue & 0x40) ) {
            // 10X
            if( ! ( hue & 0x20) ) {
                // 100
                //case 4: // A -> B
                r = 0;
                FORCE_REFERENCE(r);
                //uint8_t twothirds = (third << 1);
                uint8_t twothirds = scale8( offset8, ((256 * 2) / 3)); // max=170
                g = K171 - twothirds; //K170?
                b = K85  + twothirds;
                
            } else {
                // 101
                //case 5: // B -> P
                r = third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K255 - third;
                
            }
        } else {
            if( !  (hue & 0x20)  ) {
                // 110
                //case 6: // P -- K
                r = K85 + third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K171 - third;
                
            } else {
                // 111
                //case 7: // K -> R
                r = K170 + third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K85 - third;
                
            }
        }
    }
    
    // This is one of the good places to scale the green down,
    // although the client can scale green down as well.
    if( G2 ) g = g >> 1;
    if( Gscale ) g = scale8_video_LEAVING_R1_DIRTY( g, Gscale);
    
    // Scale down colors if we're desaturated at all
    // and add the brightness_floor to r, g, and b.
    if( sat != 255 ) {
        if( sat == 0) {
            r = 255; b = 255; g = 255;
        } else {
            uint8_t desat = 255 - sat;
            desat = scale8_video( desat, desat);

            uint8_t satscale = 255 - desat;
            //satscale = sat; // uncomment to revert to pre-2021 saturation behavior

            //nscale8x3_video( r, g, b, sat);
#if (FASTLED_SCALE8_FIXED==1)
            r = scale8_LEAVING_R1_DIRTY( r, satscale);
              // Fixes jumping red pixel: https://github.com/FastLED/FastLED/pull/943
            g = scale8_LEAVING_R1_DIRTY( g, satscale);
            
            b = scale8_LEAVING_R1_DIRTY( b, satscale);
            
            cleanup_R1();
#else
            if( r ) r = scale8( r, satscale) + 1;
            if( g ) g = scale8( g, satscale) + 1;
            if( b ) b = scale8( b, satscale) + 1;
#endif
            uint8_t brightness_floor = desat;
            r += brightness_floor;
            g += brightness_floor;
            b += brightness_floor;
        }
    }
    
    // Now scale everything down if we're at value < 255.
    if( val != 255 ) {
        
        val = scale8_video_LEAVING_R1_DIRTY( val, val);
        if( val == 0 ) {
            r=0; g=0; b=0;
        } else {
            // nscale8x3_video( r, g, b, val);
#if (FASTLED_SCALE8_FIXED==1)
            r = scale8_LEAVING_R1_DIRTY( r, val);
              // Fixes jumping red pixel: https://github.com/FastLED/FastLED/pull/943
            g = scale8_LEAVING_R1_DIRTY( g, val);
            
            b = scale8_LEAVING_R1_DIRTY( b, val);
            
            cleanup_R1();
#else
            if( r ) r = scale8( r, val) + 1;
            if( g ) g = scale8( g, val) + 1;
            if( b ) b = scale8( b, val) + 1;
#endif
        }
    }
    
    // Here we have the old AVR "missing std X+n" problem again
    // It turns out that fixing it winds up costing more than
    // not fixing it.
    // To paraphrase Dr Bronner, profile! profile! profile!
    //asm volatile(  ""  :  :  : "r26", "r27" );
    //asm volatile (" movw r30, r26 \n" : : : "r30", "r31");
    rgb.r = r;
    rgb.g = g;
    rgb.b = b;
}



CHSV rgb2hsv_approximate( const CRGB& rgb)
{
    uint8_t r = rgb.r;
    uint8_t g = rgb.g;
    uint8_t b = rgb.b;
    uint8_t h, s, v;
    
    // find desaturation
    uint8_t desat = 255;
    if( r < desat) desat = r;
    if( g < desat) desat = g;
    if( b < desat) desat = b;
    
    // remove saturation from all channels
    r -= desat;
    g -= desat;
    b -= desat;
    
    //Serial.print("desat="); Serial.print(desat); Serial.println("");
    
    //uint8_t orig_desat = sqrt16( desat * 256);
    //Serial.print("orig_desat="); Serial.print(orig_desat); Serial.println("");
    
    // saturation is opposite of desaturation
    s = 255 - desat;
    //Serial.print("s.1="); Serial.print(s); Serial.println("");
    
    if( s != 255 ) {
        // undo 'dimming' of saturation
        s = 255 - sqrt16( (255-s) * 256);
    }
    // without lib8tion: float ... ew ... sqrt... double ew, or rather, ew ^ 0.5
    // if( s != 255 ) s = (255 - (256.0 * sqrt( (float)(255-s) / 256.0)));
    //Serial.print("s.2="); Serial.print(s); Serial.println("");
    
    
    // at least one channel is now zero
    // if all three channels are zero, we had a
    // shade of gray.
    if( (r + g + b) == 0) {
        // we pick hue zero for no special reason
        return CHSV( 0, 0, 255 - s);
    }
    
    // scale all channels up to compensate for desaturation
    if( s < 255) {
        if( s == 0) s = 1;
        uint32_t scaleup = 65535 / (s);
        r = ((uint32_t)(r) * scaleup) / 256;
        g = ((uint32_t)(g) * scaleup) / 256;
        b = ((uint32_t)(b) * scaleup) / 256;
    }
    //Serial.print("r.2="); Serial.print(r); Serial.println("");
    //Serial.print("g.2="); Serial.print(g); Serial.println("");
    //Serial.print("b.2="); Serial.print(b); Serial.println("");
    
    uint16_t total = r + g + b;
    
    //Serial.print("total="); Serial.print(total); Serial.println("");
    
    // scale all channels up to compensate for low values
    if( total < 255) {
        if( total == 0) total = 1;
        uint32_t scaleup = 65535 / (total);
        r = ((uint32_t)(r) * scaleup) / 256;
        g = ((uint32_t)(g) * scaleup) / 256;
        b = ((uint32_t)(b) * scaleup) / 256;
    }
    //Serial.print("r.3="); Serial.print(r); Serial.println("");
    //Serial.print("g.3="); Serial.print(g); Serial.println("");
    //Serial.print("b.3="); Serial.print(b); Serial.println("");
    
    if( total > 255 ) {
        v = 255;
    } else {
        v = qadd8(desat,total);
        // undo 'dimming' of brightness
        if( v != 255) v = sqrt16( v * 256);
        // without lib8tion: float ... ew ... sqrt... double ew, or rather, ew ^ 0.5
        // if( v != 255) v = (256.0 * sqrt( (float)(v) / 256.0));
        
    }
    
    //Serial.print("v="); Serial.print(v); Serial.println("");
    
    
#if 0
    
    //#else
    if( v != 255) {
        // this part could probably use refinement/rethinking,
        // (but it doesn't overflow & wrap anymore)
        uint16_t s16;
        s16 = (s * 256);
        s16 /= v;
        //Serial.print("s16="); Serial.print(s16); Serial.println("");
        if( s16 < 256) {
            s = s16;
        } else {
            s = 255; // clamp to prevent overflow
        }
    }
#endif
    
    //Serial.print("s.3="); Serial.print(s); Serial.println("");
    
    
    // since this wasn't a pure shade of gray,
    // the interesting question is what hue is it
    
    
    
    // start with which channel is highest
    // (ties don't matter)
    uint8_t highest = r;
    if( g > highest) highest = g;
    if( b > highest) highest = b;
    
    if( highest == r ) {
        // Red is highest.
        // Hue could be Purple/Pink-Red,Red-Orange,Orange-Yellow
        if( g == 0 ) {
            // if green is zero, we're in Purple/Pink-Red
            h = (HUE_PURPLE + HUE_PINK) / 2;
            h += scale8( qsub8(r, 128), FIXFRAC8(48,128));
        } else if ( (r - g) > g) {
            // if R-G > G then we're in Red-Orange
            h = HUE_RED;
            h += scale8( g, FIXFRAC8(32,85));
        } else {
            // R-G < G, we're in Orange-Yellow
            h = HUE_ORANGE;
            h += scale8( qsub8((g - 85) + (171 - r), 4), FIXFRAC8(32,85)); //221
        }
        
    } else if ( highest == g) {
        // Green is highest
        // Hue could be Yellow-Green, Green-Aqua
        if( b == 0) {
            // if Blue is zero, we're in Yellow-Green
            //   G = 171..255
            //   R = 171..  0
            h = HUE_YELLOW;
            uint8_t radj = scale8( qsub8(171,r),   47); //171..0 -> 0..171 -> 0..31
            uint8_t gadj = scale8( qsub8(g,171),   96); //171..255 -> 0..84 -> 0..31;
            uint8_t rgadj = radj + gadj;
            uint8_t hueadv = rgadj / 2;
            h += hueadv;
            //h += scale8( qadd8( 4, qadd8((g - 128), (128 - r))),
            //             FIXFRAC8(32,255)); //
        } else {
            // if Blue is nonzero we're in Green-Aqua
            if( (g-b) > b) {
                h = HUE_GREEN;
                h += scale8( b, FIXFRAC8(32,85));
            } else {
                h = HUE_AQUA;
                h += scale8( qsub8(b, 85), FIXFRAC8(8,42));
            }
        }
        
    } else /* highest == b */ {
        // Blue is highest
        // Hue could be Aqua/Blue-Blue, Blue-Purple, Purple-Pink
        if( r == 0) {
            // if red is zero, we're in Aqua/Blue-Blue
            h = HUE_AQUA + ((HUE_BLUE - HUE_AQUA) / 4);
            h += scale8( qsub8(b, 128), FIXFRAC8(24,128));
        } else if ( (b-r) > r) {
            // B-R > R, we're in Blue-Purple
            h = HUE_BLUE;
            h += scale8( r, FIXFRAC8(32,85));
        } else {
            // B-R < R, we're in Purple-Pink
            h = HUE_PURPLE;
            h += scale8( qsub8(r, 85), FIXFRAC8(32,85));
        }
    }
    
    h += 1;
    return CHSV( h, s, v);
}

float low_pass_filter(float new_data, float last_data, uint32_t sample_rate, float cutoff_freq) {
    float alpha = 1.0f - expf(-2.0f * float(M_PI) * cutoff_freq / float(sample_rate));
    return (1.0f - alpha) * last_data + alpha * new_data;
}
void low_pass_array(float* new_frame, float* last_frame, uint16_t length, uint32_t sample_rate, float cutoff_freq){
    for(uint16_t i = 0; i < length; i++) new_frame[i] = low_pass_filter(new_frame[i], last_frame[i], sample_rate, cutoff_freq);
}
CRGB::CRGB(const CHSV& hsv){ hsv2rgb_rainbow(hsv, *this); }

CRGB16 hsv(SQ15x16 h, SQ15x16 s, SQ15x16 v) {
  while (h > 1.0) { h -= 1.0; }
  while (h < 0.0) { h += 1.0; }

  CRGB base_color = CHSV(uint8_t(h * 255.0), uint8_t(s * 255.0), 255);

  CRGB16 col = { base_color.r / 255.0, base_color.g / 255.0, base_color.b / 255.0 };
  //col = desaturate(col, SQ15x16(1.0) - s);

  col.r *= v;
  col.g *= v;
  col.b *= v;

  return col;
}

void clip_led_values(CRGB16* buffer) { // Modified to accept buffer pointer
#if ENABLE_HSV_SOFT_CLIP
  // Phase 2 Change 9 (2026-05-20): hue-preserving soft-clip.
  // When any channel exceeds KNEE, ALL channels scale down so max approaches
  // but never quite reaches 1.0. This preserves the hue ratio at peaks instead
  // of letting per-channel clipping desaturate toward white.
  static const SQ15x16 KNEE = SQ15x16(SOFT_CLIP_KNEE);
  static const SQ15x16 ROLLOFF = SQ15x16(SOFT_CLIP_ROLLOFF);
  static const SQ15x16 ONE = SQ15x16(1.0);
  static const SQ15x16 ZERO = SQ15x16(0.0);
  static const SQ15x16 EPSILON = SQ15x16(0.001);

  for (uint16_t i = 0; i < NATIVE_RESOLUTION; i++) {
    // Floor at zero (same as original)
    if (buffer[i].r < ZERO) buffer[i].r = ZERO;
    if (buffer[i].g < ZERO) buffer[i].g = ZERO;
    if (buffer[i].b < ZERO) buffer[i].b = ZERO;

    // Find max channel
    SQ15x16 m = buffer[i].r;
    if (buffer[i].g > m) m = buffer[i].g;
    if (buffer[i].b > m) m = buffer[i].b;

    if (m > KNEE) {
      // Compress: above knee, compressed_m = KNEE + (m - KNEE) * ROLLOFF
      SQ15x16 compressed_m = KNEE + (m - KNEE) * ROLLOFF;
      if (compressed_m > ONE) compressed_m = ONE;
      // Scale all channels by ratio (guarded against div-by-near-zero)
      if (m > EPSILON) {
        SQ15x16 scale = compressed_m / m;
        buffer[i].r *= scale;
        buffer[i].g *= scale;
        buffer[i].b *= scale;
      }
    }
  }
#else
  // Original hard-clamp (prior behavior)
  for (uint16_t i = 0; i < NATIVE_RESOLUTION; i++) {
    if (buffer[i].r < 0.0) { buffer[i].r = 0.0; }
    if (buffer[i].g < 0.0) { buffer[i].g = 0.0; }
    if (buffer[i].b < 0.0) { buffer[i].b = 0.0; }

    if (buffer[i].r > 1.0) { buffer[i].r = 1.0; }
    if (buffer[i].g > 1.0) { buffer[i].g = 1.0; }
    if (buffer[i].b > 1.0) { buffer[i].b = 1.0; }
  }
#endif
}

void apply_brightness() {
  // This is only used to fade in when booting!
  if (millis() >= 1000 && noise_transition_queued == false && mode_transition_queued == false) {
    if (MASTER_BRIGHTNESS < 1.0) {
      MASTER_BRIGHTNESS += 0.005;
    }
    if (MASTER_BRIGHTNESS > 1.0) {
      MASTER_BRIGHTNESS = 1.00;
    }
  }

  // Phase 1 2026-05-20: PHOTONS knob curve (Captain default Mode 2 = sqrt, perceptual).
  // Per-frame call, not per-pixel — soft-float cost on S2 (~1µs) is acceptable.
#if PHOTONS_CURVE_MODE == 0
  SQ15x16 photons_curve = CONFIG.PHOTONS * CONFIG.PHOTONS;                       // quadratic (prior)
#elif PHOTONS_CURVE_MODE == 1
  SQ15x16 photons_curve = CONFIG.PHOTONS;                                        // linear
#elif PHOTONS_CURVE_MODE == 2
  SQ15x16 photons_curve = SQ15x16(sqrtf(CONFIG.PHOTONS));                        // sqrt (perceptual; CONFIG.PHOTONS is already float)
#endif
  SQ15x16 brightness = MASTER_BRIGHTNESS * photons_curve * silent_scale;
  
  if (debug_mode && (millis() % 5000 == 0)) {
    USBSerial.print("DEBUG: Brightness components - MASTER_BRIGHTNESS: ");
    USBSerial.print(float(MASTER_BRIGHTNESS));
    USBSerial.print(" PHOTONS²: ");
    USBSerial.print(float(CONFIG.PHOTONS * CONFIG.PHOTONS));
    USBSerial.print(" silent_scale: ");
    USBSerial.print(float(silent_scale));
    USBSerial.print(" Final brightness: ");
    USBSerial.println(float(brightness));
  }

  for (uint16_t i = 0; i < NATIVE_RESOLUTION; i++) {
    leds_16[i].r *= brightness;
    leds_16[i].g *= brightness;
    leds_16[i].b *= brightness;
  }

  clip_led_values(leds_16);
}

void quantize_color(bool temporal_dithering) {
  if (temporal_dithering) {
    dither_step++;
    if (dither_step >= 4) {
      dither_step = 0;
    }

    static uint8_t noise_origin_r = 0;  // 0
    static uint8_t noise_origin_g = 0;  // 2
    static uint8_t noise_origin_b = 0;  // 4

    noise_origin_r += 1;
    noise_origin_g += 1;
    noise_origin_b += 1;

    for (uint16_t i = 0; i < CONFIG.LED_COUNT; i += 1) {
      // RED #####################################################
      SQ15x16 decimal_r = leds_scaled[i].r * SQ15x16(254);
      SQ15x16 whole_r = decimal_r.getInteger();
      SQ15x16 fract_r = decimal_r - whole_r;

      if (fract_r >= dither_table[(noise_origin_r + i) % 4]) {
        whole_r += SQ15x16(1);
      }

      // Phase 1 2026-05-20: apply output gamma at the final uint8 write.
      leds_out[i].r = apply_gamma8(whole_r.getInteger());

      // GREEN ###################################################
      SQ15x16 decimal_g = leds_scaled[i].g * SQ15x16(254);
      SQ15x16 whole_g = decimal_g.getInteger();
      SQ15x16 fract_g = decimal_g - whole_g;

      if (fract_g >= dither_table[(noise_origin_g + i) % 4]) {
        whole_g += SQ15x16(1);
      }

      leds_out[i].g = apply_gamma8(whole_g.getInteger());

      // BLUE ####################################################
      SQ15x16 decimal_b = leds_scaled[i].b * SQ15x16(254);
      SQ15x16 whole_b = decimal_b.getInteger();
      SQ15x16 fract_b = decimal_b - whole_b;

      if (fract_b >= dither_table[(noise_origin_b + i) % 4]) {
        whole_b += SQ15x16(1);
      }

      leds_out[i].b = apply_gamma8(whole_b.getInteger());
    }
  } else {
    for (uint16_t i = 0; i < CONFIG.LED_COUNT; i += 1) {
      // Phase 1 2026-05-20: gamma at non-dither final write too.
      leds_out[i].r = apply_gamma8(uint8_t(leds_scaled[i].r * 255));
      leds_out[i].g = apply_gamma8(uint8_t(leds_scaled[i].g * 255));
      leds_out[i].b = apply_gamma8(uint8_t(leds_scaled[i].b * 255));
    }
  }
}

void apply_incandescent_filter() {
  SQ15x16 mix = CONFIG.INCANDESCENT_FILTER;
  SQ15x16 inv_mix = 1.0 - mix;

  for (uint8_t i = 0; i < NATIVE_RESOLUTION; i++) {
    SQ15x16 filtered_r = leds_16[i].r * incandescent_lookup.r;
    SQ15x16 filtered_g = leds_16[i].g * incandescent_lookup.g;
    SQ15x16 filtered_b = leds_16[i].b * incandescent_lookup.b;

    leds_16[i].r = (leds_16[i].r * inv_mix) + (filtered_r * mix);
    leds_16[i].g = (leds_16[i].g * inv_mix) + (filtered_g * mix);
    leds_16[i].b = (leds_16[i].b * inv_mix) + (filtered_b * mix);

    /*
    SQ15x16 max_val = 0;
    if (leds_16[i].r > max_val) { max_val = leds_16[i].r; }
    if (leds_16[i].g > max_val) { max_val = leds_16[i].g; }
    if (leds_16[i].b > max_val) { max_val = leds_16[i].b; }

    SQ15x16 leakage = (max_val >> 2) * mix;

    CRGB base_col = CRGB(
      (uint16_t(leds[i].r * (255 - leakage)) >> 8),
      (uint16_t(leds[i].g * (255 - leakage)) >> 8),
      (uint16_t(leds[i].b * (255 - leakage)) >> 8));

    CRGB leak_col = CRGB(
      uint16_t((uint16_t(incandescent_lookup.r * (leakage)) >> 8) * max_val) >> 8,
      uint16_t((uint16_t(incandescent_lookup.g * (leakage)) >> 8) * max_val) >> 8,
      uint16_t((uint16_t(incandescent_lookup.b * (leakage)) >> 8) * max_val) >> 8);

    leds[i].r = base_col.r + leak_col.r;
    leds[i].g = base_col.g + leak_col.g;
    leds[i].b = base_col.b + leak_col.b;
    */
  }
}

void shift_leds_up(CRGB16* led_array, uint16_t offset) {
  memcpy(leds_16_temp, led_array, sizeof(CRGB16) * NATIVE_RESOLUTION);
  memcpy(led_array + offset, leds_16_temp, (NATIVE_RESOLUTION - offset) * sizeof(CRGB16));
  memset(led_array, 0, offset * sizeof(CRGB16));
}

void mirror_image_downwards(CRGB16* led_array) {
  uint16_t half_res = NATIVE_RESOLUTION >> 1;
  for (uint16_t i = 0; i < half_res; i++) { // Loop up to half resolution
    // Copy the pixel from the second half
    leds_16_temp[half_res + i] = led_array[half_res + i];
    // Mirror it to the first half (e.g., index 159 mirrors to 0, 158 to 1, etc.)
    leds_16_temp[half_res - 1 - i] = led_array[half_res + i];
  }

  memcpy(led_array, leds_16_temp, sizeof(CRGB16) * NATIVE_RESOLUTION);
}

void render_bulb_cover() {
  SQ15x16 cover[4] = { 0.25, 1.00, 0.25, 0.00 };

  for (uint8_t i = 0; i < NATIVE_RESOLUTION; i++) {
    CRGB16 covered_color = {
      leds_16[i].r * cover[i % 4],
      leds_16[i].g * cover[i % 4],
      leds_16[i].b * cover[i % 4],
    };

    SQ15x16 bulb_opacity = CONFIG.BULB_OPACITY;
    SQ15x16 bulb_opacity_inv = 1.0 - bulb_opacity;

    leds_16[i].r = leds_16[i].r * bulb_opacity_inv + covered_color.r * bulb_opacity;
    leds_16[i].g = leds_16[i].g * bulb_opacity_inv + covered_color.g * bulb_opacity;
    leds_16[i].b = leds_16[i].b * bulb_opacity_inv + covered_color.b * bulb_opacity;
  }
}

CRGB force_saturation(CRGB input, uint8_t saturation) {
  CHSV out_col_hsv = rgb2hsv_approximate(input);
  CHSV out_col_hsv_sat = out_col_hsv;
  out_col_hsv_sat.setHSV(out_col_hsv.h, saturation, out_col_hsv.v);

  return CRGB(out_col_hsv_sat);
}

CRGB force_hue(CRGB input, uint8_t hue) {
  CHSV out_col_hsv = rgb2hsv_approximate(input);
  CHSV out_col_hsv_hue = out_col_hsv;
  out_col_hsv_hue.setHSV(hue, out_col_hsv.s, out_col_hsv.v);

  return CRGB(out_col_hsv_hue);
}

void process_color_shift() {
  int16_t rounded_index = spectral_history_index - 1;
  while (rounded_index < 0) {
    rounded_index += SPECTRAL_HISTORY_LENGTH;
  }

  SQ15x16 novelty_now = novelty_curve[rounded_index];

  // Remove bottom 10%, stretch values to still occupy full 0.0-1.0 range
  novelty_now -= SQ15x16(0.10);
  if (novelty_now < 0.0) {
    novelty_now = 0.0;
  }
  novelty_now *= SQ15x16(1.111111);  // ---- 1.0 / (1.0 - 0.10)

  novelty_now = novelty_now * novelty_now * novelty_now;  // Square the novelty value
  //novelty_now = (novelty_now)*0.5 + (novelty_now*novelty_now)*0.5; // "half-square" it again

  if (novelty_now > 0.05) {
    novelty_now = 0.05;
  }

  if (novelty_now > hue_shift_speed) {
    hue_shift_speed = novelty_now * SQ15x16(0.25); // Reduced factor from 0.75 to slow down shift
  } else {
    hue_shift_speed *= SQ15x16(0.99);
  }

  // Add and wrap
  hue_position += (hue_shift_speed * hue_push_direction);
  while (hue_position < 0.0) {
    hue_position += 1.0;
  }
  while (hue_position >= 1.0) {
    hue_position -= 1.0;
  }

  if (fabs_fixed(hue_position - hue_destination) <= 0.01) {
    hue_push_direction *= -1.0;
    hue_shifting_mix_target *= -1.0;
    hue_destination = random_float();
    //printf("###################################### NEW DEST: %f\n", hue_destination);
  }

  SQ15x16 hue_shifting_mix_distance = fabs_fixed(hue_shifting_mix - hue_shifting_mix_target);
  if (hue_shifting_mix < hue_shifting_mix_target) {
    hue_shifting_mix += hue_shifting_mix_distance * 0.01;
  } else if (hue_shifting_mix > hue_shifting_mix_target) {
    hue_shifting_mix -= hue_shifting_mix_distance * 0.01;
  }

  /*
    if(chance(0.2) == true){ //0.2% of loops
        hue_push_direction *= -1.0;
        printf("###################################### SWITCH DIR: %f\n", hue_push_direction);
    }
    */
}

void make_smooth_chromagram() {
  memset(chromagram_smooth, 0, sizeof(SQ15x16) * 12);

  uint8_t chroma_range = CONFIG.CHROMAGRAM_RANGE;
  if (chroma_range < 1) chroma_range = 1;
  if (chroma_range > NUM_FREQS) chroma_range = NUM_FREQS;
  SQ15x16 chroma_bin_divisor = SQ15x16(float(chroma_range) / 12.0f);

  for (uint8_t i = 0; i < chroma_range; i++) {
    SQ15x16 note_magnitude = spectrogram_smooth[i];

    if (note_magnitude > 1.0) {
      note_magnitude = 1.0;
    } else if (note_magnitude < 0.0) {
      note_magnitude = 0.0;
    }

    uint8_t chroma_bin = i % 12;
    chromagram_smooth[chroma_bin] += note_magnitude / chroma_bin_divisor;
  }

  SQ15x16 pre_max = SQ15x16(0.0);
  SQ15x16 pre_mean = SQ15x16(0.0);
  for (uint8_t i = 0; i < 12; i++) {
    pre_mean += chromagram_smooth[i];
    if (chromagram_smooth[i] > pre_max) pre_max = chromagram_smooth[i];
  }
  pre_mean /= SQ15x16(12.0);

  static SQ15x16 max_peak = 0.001;

  max_peak *= 0.999;
  if (max_peak < 0.01) {
    max_peak = 0.01;
  }

  for (uint16_t i = 0; i < 12; i++) {
    if (chromagram_smooth[i] > max_peak) {
      SQ15x16 distance = chromagram_smooth[i] - max_peak;
      max_peak += distance *= SQ15x16(0.05);
    }
  }

  SQ15x16 multiplier = 1.0 / max_peak;

  for (uint8_t i = 0; i < 12; i++) {
    chromagram_smooth[i] *= multiplier;
  }

  SQ15x16 norm_max = SQ15x16(0.0);
  SQ15x16 norm_mean = SQ15x16(0.0);
  for (uint8_t i = 0; i < 12; i++) {
    norm_mean += chromagram_smooth[i];
    if (chromagram_smooth[i] > norm_max) norm_max = chromagram_smooth[i];
  }
  norm_mean /= SQ15x16(12.0);
  SQ15x16 flatness = norm_max - norm_mean;

  if (VP_FIX_CHROMAGRAM_SPARSENESS) {
    SQ15x16 gate_gain = SQ15x16(0.0);
    const SQ15x16 gate_floor = SQ15x16(0.08);
    const SQ15x16 gate_full = SQ15x16(0.28);
    const SQ15x16 quiet_max = SQ15x16(0.08);

    if (norm_max <= quiet_max || flatness <= gate_floor) {
      gate_gain = SQ15x16(0.0);
    } else if (flatness >= gate_full) {
      gate_gain = SQ15x16(1.0);
    } else {
      gate_gain = (flatness - gate_floor) / (gate_full - gate_floor);
    }

    for (uint8_t i = 0; i < 12; i++) {
      chromagram_smooth[i] -= SQ15x16(0.1);
      if (chromagram_smooth[i] < SQ15x16(0.0)) chromagram_smooth[i] = SQ15x16(0.0);
      chromagram_smooth[i] *= gate_gain;
      if (chromagram_smooth[i] > SQ15x16(1.0)) chromagram_smooth[i] = SQ15x16(1.0);
    }
  }

  SQ15x16 final_max = SQ15x16(0.0);
  SQ15x16 final_mean = SQ15x16(0.0);
  for (uint8_t i = 0; i < 12; i++) {
    final_mean += chromagram_smooth[i];
    if (chromagram_smooth[i] > final_max) final_max = chromagram_smooth[i];
  }
  final_mean /= SQ15x16(12.0);

  vp_dbg_chroma_range = chroma_range;
  vp_dbg_chroma_pre_max = pre_max;
  vp_dbg_chroma_pre_mean = pre_mean;
  vp_dbg_chroma_norm_max = norm_max;
  vp_dbg_chroma_norm_mean = norm_mean;
  vp_dbg_chroma_final_max = final_max;
  vp_dbg_chroma_final_mean = final_mean;
  vp_dbg_chroma_flatness = flatness;
  vp_dbg_chroma_seq++;
}

void draw_sprite(CRGB16 dest[], CRGB16 sprite[], uint32_t dest_length, uint32_t sprite_length, float position, SQ15x16 alpha) {
  int32_t position_whole = position;  // Downcast to integer accuracy
  float position_fract = position - position_whole;
  SQ15x16 mix_right = position_fract;
  SQ15x16 mix_left = 1.0 - mix_right;

  for (uint16_t i = 0; i < sprite_length; i++) {
    int32_t pos_left = i + position_whole;
    int32_t pos_right = i + position_whole + 1;

    bool skip_left = false;
    bool skip_right = false;

    if (pos_left < 0) {
      pos_left = 0;
      skip_left = true;
    }
    if (pos_left > dest_length - 1) {
      pos_left = dest_length - 1;
      skip_left = true;
    }

    if (pos_right < 0) {
      pos_right = 0;
      skip_right = true;
    }
    if (pos_right > dest_length - 1) {
      pos_right = dest_length - 1;
      skip_right = true;
    }

    if (skip_left == false) {
      dest[pos_left].r += sprite[i].r * mix_left * alpha;
      dest[pos_left].g += sprite[i].g * mix_left * alpha;
      dest[pos_left].b += sprite[i].b * mix_left * alpha;
    }

    if (skip_right == false) {
      dest[pos_right].r += sprite[i].r * mix_right * alpha;
      dest[pos_right].g += sprite[i].g * mix_right * alpha;
      dest[pos_right].b += sprite[i].b * mix_right * alpha;
    }
  }
}

void get_smooth_spectrogram() {
  static SQ15x16 spectrogram_smooth_last[NUM_FREQS];

  for (uint8_t bin = 0; bin < NUM_FREQS; bin++) {
    SQ15x16 note_brightness = spectrogram[bin];

    // Phase 1 2026-05-20: asymmetric attack/release (was symmetric 0.75/0.75 — flattened transients).
    if (spectrogram_smooth[bin] < note_brightness) {
      SQ15x16 distance = note_brightness - spectrogram_smooth[bin];
      spectrogram_smooth[bin] += distance * SQ15x16(SPECTROGRAM_SMOOTH_ATTACK);

    } else if (spectrogram_smooth[bin] > note_brightness) {
      SQ15x16 distance = spectrogram_smooth[bin] - note_brightness;
      spectrogram_smooth[bin] -= distance * SQ15x16(SPECTROGRAM_SMOOTH_RELEASE);
    }
  }
}

CRGB16 crgb_to_crgb16(CRGB rgb_color) {
  return { SQ15x16(rgb_color.r / 255.0f), SQ15x16(rgb_color.g / 255.0f), SQ15x16(rgb_color.b / 255.0f) };
}

SQ15x16 clamp01_fixed(SQ15x16 value) {
  if (value < SQ15x16(0.0)) return SQ15x16(0.0);
  if (value > SQ15x16(1.0)) return SQ15x16(1.0);
  return value;
}

CRGB16 clamp_crgb16(CRGB16 color) {
  color.r = clamp01_fixed(color.r);
  color.g = clamp01_fixed(color.g);
  color.b = clamp01_fixed(color.b);
  return color;
}

const CRGBPalette16& cached_gradient_palette(uint8_t palette_index, bool render_secondary) {
  static CRGBPalette16 primary_palette;
  static CRGBPalette16 secondary_palette;
  static uint8_t primary_index = 255;
  static uint8_t secondary_index = 255;

  if (palette_index >= gGradientPaletteCount) {
    palette_index = 0;
  }

  if (render_secondary) {
    if (secondary_index != palette_index) {
      secondary_palette = CRGBPalette16(gGradientPalettes[palette_index]);
      secondary_index = palette_index;
    }
    return secondary_palette;
  }

  if (primary_index != palette_index) {
    primary_palette = CRGBPalette16(gGradientPalettes[palette_index]);
    primary_index = palette_index;
  }
  return primary_palette;
}

uint8_t palette_index_with_phase(uint8_t palette_index) {
  bool auto_shift = vp_render_secondary_channel ? SECONDARY_AUTO_COLOR_SHIFT : CONFIG.AUTO_COLOR_SHIFT;
  if (!auto_shift) {
    return palette_index;
  }
  return palette_index + uint8_t(float(hue_position) * 255.0f);
}

CRGB16 palette_manual_colour(const CRGBPalette16& pal, SQ15x16 hue, SQ15x16 brightness) {
  float h = float(hue);
  if (!isfinite(h)) h = 0.0f;
  h -= floorf(h);
  if (h < 0.0f) h += 1.0f;

  CRGB rgb_color = ColorFromPalette(pal, palette_index_with_phase(uint8_t(h * 255.0f)), 255, LINEARBLEND);
  CRGB16 color = crgb_to_crgb16(rgb_color);
  SQ15x16 level = clamp01_fixed(brightness);
  color.r *= level;
  color.g *= level;
  color.b *= level;
  return clamp_crgb16(color);
}

CRGB16 palette_chroma_colour(const CRGBPalette16& pal, SQ15x16 fallback_brightness) {
  // Palette colour must be sampled once per rendered colour. Summing multiple
  // palette RGB samples collapses gradients toward grey/white.  Use the raw
  // chromagram to choose the palette position, and use brightness separately.
  float total_weight = 0.0f;
  float max_weight = 0.0f;
  float x = 0.0f;
  float y = 0.0f;
  static const float TWO_PI_F = 6.2831853071795864769f;

  for (uint8_t c = 0; c < 12; c++) {
    float raw_bin = float(chromagram_smooth[c]);
    if (!isfinite(raw_bin) || raw_bin < 0.0f) raw_bin = 0.0f;
    if (raw_bin > 1.0f) raw_bin = 1.0f;

    float angle = (float(c) / 12.0f) * TWO_PI_F;
    x += cosf(angle) * raw_bin;
    y += sinf(angle) * raw_bin;
    total_weight += raw_bin;
    if (raw_bin > max_weight) max_weight = raw_bin;
  }

  SQ15x16 brightness = clamp01_fixed(fallback_brightness);

  if (total_weight <= 0.0001f) {
    return palette_manual_colour(pal, SQ15x16(CONFIG.CHROMA), brightness);
  }

  float hue = atan2f(y, x) / TWO_PI_F;
  if (!isfinite(hue)) hue = float(CONFIG.CHROMA);
  if (hue < 0.0f) hue += 1.0f;

  // Use both peak and average chroma energy. Peak preserves responsiveness for
  // single-note content; average prevents dense chords from immediately pinning
  // every palette lookup to full value.
  float avg_weight = total_weight / 12.0f;
  float energy = (max_weight * 0.75f) + (avg_weight * 0.25f);
  if (energy > 1.0f) energy = 1.0f;
  if (energy < 0.0f) energy = 0.0f;

  SQ15x16 energy_fixed = SQ15x16(energy);
  if (energy_fixed > brightness) brightness = energy_fixed;
  brightness = clamp01_fixed(brightness);

  return palette_manual_colour(pal, SQ15x16(hue), brightness);
}

void light_mode_bloom(CRGB16* leds_prev_buffer, SQ15x16 shift_multiplier = SQ15x16(1.0)) { // Accept previous buffer as argument
  // Clear output
  memset(leds_16, 0, sizeof(CRGB16) * NATIVE_RESOLUTION);

  // Draw previous frame shifted with mood scaling.
  // Shift speed is in *pixels per frame*; the original 0.250..2.000 range was tuned for
  // NATIVE_RESOLUTION=128. Scale by NR/128.0 so the bloom propagates at the same visual
  // speed regardless of strip length (otherwise wider strips look like a static sheet).
  const float bloom_scale = (float(NATIVE_RESOLUTION) / 128.0f) * VP_BLOOM_SHIFT_SCALE;
  SQ15x16 vp_bloom_alpha = SQ15x16(VP_FIX_BLOOM_DECAY ? 0.88f : VP_BLOOM_ALPHA);
  draw_sprite(leds_16, leds_prev_buffer, NATIVE_RESOLUTION, NATIVE_RESOLUTION,
              (0.250 + 1.750 * CONFIG.MOOD) * bloom_scale * float(shift_multiplier), vp_bloom_alpha);

  //-------------------------------------------------------
  // Calculate new color input based on chromagram
  CRGB16 sum_color;
  SQ15x16 share = 1 / 6.0; // Normalization factor if summing all 12 bins
  memset(&sum_color, 0, sizeof(CRGB16)); // Initialize sum_color

  // Phase 1 2026-05-20: hoist palette construction OUT of the per-bin loop (was 12x redundant).
  bool render_secondary = vp_render_secondary_channel;
  bool palette_owns_colour = palette_owns_render_colour_source();
  uint8_t palette_to_use_bl = render_secondary ? SECONDARY_PALETTE_INDEX : CONFIG.PALETTE_INDEX;
  const CRGBPalette16& pal_bl = cached_gradient_palette(palette_to_use_bl, render_secondary);

  for (uint8_t i = 0; i < 12; i++) {
    float prog = i / 12.0;
    SQ15x16 bin = chromagram_smooth[i];

    CRGB16 add_color;
    if (palette_owns_colour) {
        add_color = {0, 0, 0};
    } else {
         // Bloom Classic: sum every note colour before applying global contrast.
         add_color = hsv(SQ15x16(prog), CONFIG.SATURATION, bin * bin * share);
    }

    sum_color.r += add_color.r;
    sum_color.g += add_color.g;
    sum_color.b += add_color.b;
  }

  // Clamp color values
  if (sum_color.r > 1.0) { sum_color.r = 1.0; };
  if (sum_color.g > 1.0) { sum_color.g = 1.0; };
  if (sum_color.b > 1.0) { sum_color.b = 1.0; };

  if (!palette_owns_colour) {
    for (uint8_t i = 0; i < CONFIG.SQUARE_ITER; i++) {
      sum_color.r *= sum_color.r;
      sum_color.g *= sum_color.g;
      sum_color.b *= sum_color.b;
    }
  } else {
    sum_color = palette_chroma_colour(pal_bl, SQ15x16(0.0));
  }

  // Apply saturation and hue adjustments (similar to original logic but using fixed point)
  CRGB temp_col_rgb = { uint8_t(sum_color.r * 255), uint8_t(sum_color.g * 255), uint8_t(sum_color.b * 255) };
  if (!palette_owns_colour || (VP_BLOOM_FORCE_SATURATION && !VP_FIX_BLOOM_DECAY)) {
    temp_col_rgb = force_saturation(temp_col_rgb, 255 * float(CONFIG.SATURATION));
  }

  if (!palette_owns_colour && chromatic_mode == false) {
    SQ15x16 led_hue = chroma_val + hue_position + SQ15x16(0.05);
    temp_col_rgb = force_hue(temp_col_rgb, 255*float(led_hue));
  }

  CRGB16 final_insert_color = { temp_col_rgb.r / 255.0, temp_col_rgb.g / 255.0, temp_col_rgb.b / 255.0 };

  // If in palette mode (non-chromatic), override final_insert_color with the palette sum
  if (palette_owns_colour) {
      // Clamp the summed palette color before assigning
      if (sum_color.r > 1.0) sum_color.r = 1.0; else if (sum_color.r < 0.0) sum_color.r = 0.0;
      if (sum_color.g > 1.0) sum_color.g = 1.0; else if (sum_color.g < 0.0) sum_color.g = 0.0;
      if (sum_color.b > 1.0) sum_color.b = 1.0; else if (sum_color.b < 0.0) sum_color.b = 0.0;
      final_insert_color = sum_color; 
  }

  // Insert the new color at the center of the strip
  uint16_t center_idx1 = (NATIVE_RESOLUTION / 2) - 1;
  uint16_t center_idx2 = NATIVE_RESOLUTION / 2;
  leds_16[center_idx1] = final_insert_color;
  leds_16[center_idx2] = final_insert_color; // Insert in two center pixels for symmetry

  // Snapshot transport history before display-only edge fade and mirror.
  memcpy(leds_prev_buffer, leds_16, sizeof(CRGB16) * NATIVE_RESOLUTION);

  //-------------------------------------------------------

  // Apply fade towards the ends of the strip (adjust fade range if needed)
  uint16_t fade_width = NATIVE_RESOLUTION / 4; // Fade over the outer quarters
  for(uint16_t i = 0; i < fade_width; i++) {
    float prog = (float)i / (fade_width - 1);
    SQ15x16 fade_amount = SQ15x16(prog * prog); // Quadratic fade, ensure SQ15x16

    // Fade right end
    leds_16[NATIVE_RESOLUTION - 1 - i].r *= fade_amount;
    leds_16[NATIVE_RESOLUTION - 1 - i].g *= fade_amount;
    leds_16[NATIVE_RESOLUTION - 1 - i].b *= fade_amount;

    // Fade left end
    leds_16[i].r *= fade_amount;
    leds_16[i].g *= fade_amount;
    leds_16[i].b *= fade_amount;
  }

  // Mirroring is implicitly handled by the structure? Or apply explicitly if needed.
  // If the sprite shift + center insert doesn't create symmetry, uncomment:
   mirror_image_downwards(leds_16); // Re-enabled mirroring
}

uint16_t waveform_full_strip_position(float amp) {
  if (amp > 1.0f) amp = 1.0f;
  else if (amp < -1.0f) amp = -1.0f;

  const int center = NATIVE_RESOLUTION / 2;
  float pos_f = center + amp * (NATIVE_RESOLUTION / 2.0f);
  int pos = int(pos_f + (pos_f >= 0 ? 0.5f : -0.5f));
  if (pos < 0) pos = 0;
  if (pos >= NATIVE_RESOLUTION) pos = NATIVE_RESOLUTION - 1;
  return uint16_t(pos);
}

uint16_t waveform_upper_half_source_position(float amp) {
  uint16_t pos = waveform_full_strip_position(amp);
  const uint16_t half_res = NATIVE_RESOLUTION >> 1;
  if (pos < half_res) pos = half_res;
  return pos;
}

void waveform_shift_upper_half_up(CRGB16* led_array, uint8_t steps) {
  const uint16_t half_res = NATIVE_RESOLUTION >> 1;
  if (steps > half_res) steps = half_res;

  for (uint8_t step = 0; step < steps; step++) {
    for (int16_t i = NATIVE_RESOLUTION - 1; i > int16_t(half_res); i--) {
      led_array[i] = led_array[i - 1];
    }
    led_array[half_res] = {0,0,0};
  }
}

void light_mode_waveform_fast(CRGB16* leds_previous, CRGB16& last_color, float& waveform_peak_scaled_last) {
  const float led_share = 1.0 / 12.0; // Corrected share calculation
  // static CRGB16 sum_color_last = {0,0,0}; // REMOVED internal static color
  // Use the passed 'last_color' reference instead

  // --- Waveform Debugging Start ---
  bool is_secondary = (leds_previous == leds_16_prev_secondary);
  static uint32_t last_waveform_debug = 0;
  if (debug_mode && (millis() - last_waveform_debug > 500)) { // Rate limit debug output
    // Check if the previous buffer seems valid (e.g., not all zeros if it shouldn't be)
    SQ15x16 prev_sum = 0;
    for(int k=0; k<NATIVE_RESOLUTION; ++k) { prev_sum += leds_previous[k].r + leds_previous[k].g + leds_previous[k].b; }
    
    USBSerial.print("[DBG WFM] Ch: "); USBSerial.print(is_secondary ? 2 : 1);
    USBSerial.print(" | PrevSum: "); USBSerial.print(float(prev_sum));
    USBSerial.print(" | LastClr R: "); USBSerial.print(float(last_color.r));
    USBSerial.print(" G: "); USBSerial.print(float(last_color.g));
    USBSerial.print(" B: "); USBSerial.println(float(last_color.b));
    last_waveform_debug = millis();
  }
  // --- Waveform Debugging End ---

  // --- Color Calculation (Mostly kept as is, using CRGB16) ---
  // ... (rest of color calculation logic using 'last_color') ...
  SQ15x16 smoothed_peak_fixed = SQ15x16(waveform_peak_scaled) * 0.05 + SQ15x16(waveform_peak_scaled_last) * 0.95;
  waveform_peak_scaled_last = float(smoothed_peak_fixed);
  float abs_amp = fabsf(waveform_peak_scaled);
  if (abs_amp > 1.0f) abs_amp = 1.0f;
  float waveform_reactive_raw_floor = float(CONFIG.SWEET_SPOT_MIN_LEVEL) * WAVEFORM_REACTIVE_RAW_MARGIN;
  bool waveform_reactive = (max_waveform_val_raw > waveform_reactive_raw_floor) && (abs_amp >= WAVEFORM_REACTIVE_PEAK_FLOOR);

  CRGB16 current_sum_color = {0,0,0};
  float brightness_sum = 0.0;
  bool palette_owns_colour = palette_owns_render_colour_source();
  uint8_t palette_to_use = vp_render_secondary_channel ? SECONDARY_PALETTE_INDEX : CONFIG.PALETTE_INDEX;
  const CRGBPalette16& pal = cached_gradient_palette(palette_to_use, vp_render_secondary_channel);
  for (uint8_t c = 0; c < 12; c++) {
    float prog = c / 12.0f;
    float bin = float(chromagram_smooth[c]);

    float bright = bin;
    // Apply full iterations
    for (uint8_t s = 0; s < floor(CONFIG.SQUARE_ITER); s++) {
      bright *= bright;
    }
    // Apply fractional iteration
    float fract_iter = CONFIG.SQUARE_ITER - floor(CONFIG.SQUARE_ITER);
    if (fract_iter > 0.01) {
      float squared = bright * bright;
      bright = bright * (1.0f - fract_iter) + squared * fract_iter;
    }
    
    bright *= 1.5; 
    if (bright > 1.0) { bright = 1.0; }

    if (palette_owns_colour) {
        brightness_sum += bright * led_share;
    } else if (chromatic_mode == true) {
        CRGB16 note_col = hsv(SQ15x16(prog), CONFIG.SATURATION, SQ15x16(bright));
        current_sum_color.r += note_col.r * led_share;
        current_sum_color.g += note_col.g * led_share;
        current_sum_color.b += note_col.b * led_share;
    } else {
        brightness_sum += bright * led_share;
    }
  }

  if (palette_owns_colour) {
    current_sum_color = palette_chroma_colour(pal, SQ15x16(brightness_sum));
  } else if (chromatic_mode == false) {
    current_sum_color = hsv(chroma_val + hue_position, CONFIG.SATURATION, SQ15x16(brightness_sum));
  }

#if ENABLE_WAVEFORM_CHROMAGRAM_COLOR
  // Phase 2 Change 11 (2026-05-20): restore chromagram-driven trail color.
  // Prior code hardcoded last_color = hsv(CONFIG.CHROMA, SAT, 1.0), discarding
  // the chromagram math above. Per observation 53369, naive use of current_sum_color
  // collapses to black when chromagram is silent — so we blend with CHROMA fallback
  // based on chromagram total energy.
  {
    float chromagram_energy = 0.0f;
    for (uint8_t c = 0; c < 12; c++) chromagram_energy += float(chromagram_smooth[c]);
    chromagram_energy /= 12.0f;  // 0 to ~1
    SQ15x16 blend = SQ15x16(chromagram_energy * 2.0f); // ramp aggressively toward chromagram
    if (blend > SQ15x16(1.0)) blend = SQ15x16(1.0);
    SQ15x16 inv_blend = SQ15x16(1.0) - blend;
    SQ15x16 fallback_brightness = waveform_reactive ? SQ15x16(abs_amp) : SQ15x16(0.0);
    CRGB16 fallback;
    if (palette_owns_colour) {
      fallback = palette_manual_colour(pal, SQ15x16(CONFIG.CHROMA), fallback_brightness);
    } else {
      fallback = hsv(SQ15x16(CONFIG.CHROMA), CONFIG.SATURATION, fallback_brightness);
    }
    last_color.r = current_sum_color.r * blend + fallback.r * inv_blend;
    last_color.g = current_sum_color.g * blend + fallback.g * inv_blend;
    last_color.b = current_sum_color.b * blend + fallback.b * inv_blend;
    last_color = clamp_crgb16(last_color);
  }
#else
  // Original (prior firmware): hardcoded CHROMA-driven color, chromagram math wasted
  if (palette_owns_colour) {
    last_color = palette_manual_colour(pal, SQ15x16(CONFIG.CHROMA), SQ15x16(1.0));
  } else {
    last_color = hsv(SQ15x16(CONFIG.CHROMA), CONFIG.SATURATION, SQ15x16(1.0));
  }
  last_color = clamp_crgb16(last_color);
#endif
  // --- End Color Calculation ---

  // --- Dynamic Fading for Trails ---
  // Operate directly on the global leds_16 buffer, assuming it holds the *target* state from previous frame
  float max_fade_reduction = 0.10; 
  SQ15x16 dynamic_fade_amount = waveform_reactive ? SQ15x16(1.0 - (max_fade_reduction * abs_amp)) : SQ15x16(WAVEFORM_IDLE_FADE);

  // Apply the dynamic fade TO THE GLOBAL leds_16 buffer
  for (uint16_t i = 0; i < NATIVE_RESOLUTION; i++) {
      leds_16[i].r *= dynamic_fade_amount;
      leds_16[i].g *= dynamic_fade_amount;
      leds_16[i].b *= dynamic_fade_amount;
  }

  // --- Waveform Display ---
  float amp = waveform_peak_scaled;

  if (CONFIG.MIRROR_ENABLED) {
    waveform_shift_upper_half_up(leds_16, 1);
  } else {
    shift_leds_up(leds_16, 1); // Shift the global leds_16 buffer
  }

  // Set the new dot only once the calibrated audio floor has been crossed.
  if (waveform_reactive) {
    uint16_t pos = CONFIG.MIRROR_ENABLED
      ? waveform_upper_half_source_position(amp)
      : waveform_full_strip_position(amp);
    leds_16[pos] = last_color; // Draw onto the global leds_16 buffer
  }

  if (CONFIG.MIRROR_ENABLED) {
    // Upper half is the intentional source trace; mirror only after constraining the dot there.
    mirror_image_downwards(leds_16);
  }
}

void IRAM_ATTR process_GDFT() {
  float MOOD_VAL = CONFIG.MOOD;
  if (CONFIG.LIGHTSHOW_MODE == LIGHT_MODE_BLOOM) {
    MOOD_VAL = 1.0;
  }

  static bool interlace_flip = false;
  interlace_flip = !interlace_flip;  // Switch field every frame on lower notes to save execution time

  // Reset magnitude caps every frame
  for (uint8_t i = 0; i < NUM_ZONES; i++) {
    max_mags[i] = 0.0;  // Higher than the average noise floor
  }

  // Increment spectrogram history index
  spectrogram_history_index++;
  if (spectrogram_history_index >= spectrogram_history_length) {
    spectrogram_history_index = 0;  // wrap to index zero at end
  }

  // Run GDFT (Goertzel-based Discrete Fourier Transform) with NUM_FREQS frequencies
  // Fixed-point code adapted from example here: https://sourceforge.net/p/freetel/code/HEAD/tree/misc/goertzal/goertzal.c
  for (uint16_t i = 0; i < NUM_FREQS; i++) {  // Run NUM_FREQS times
    int32_t q0, q1, q2;
    int64_t mult;
    
    // Cache these values to avoid repeated structure access
    int32_t coeff_q14 = frequencies[i].coeff_q14;
    uint16_t block_size = frequencies[i].block_size;
    float block_size_half = frequencies[i].block_size / 2.0;

    q1 = 0;
    q2 = 0;
    
    // Pre-calculate starting index
    uint16_t sample_idx = SAMPLE_HISTORY_LENGTH - 1;
    
    // Optimized inner loop - eliminates repeated index calculation
    for (uint16_t n = 0; n < block_size; n++) {
      int32_t sample = (int32_t)sample_window[sample_idx--];
      mult = coeff_q14 * (int32_t)q1;
      q0 = (sample >> 6) + (mult >> 14) - q2;
      q2 = q1;
      q1 = q0;
    }

    mult = coeff_q14 * (int32_t)q1;
    magnitudes[i] = q2 * q2 + q1 * q1 - ((int32_t)(mult >> 14)) * q2;

    if (magnitudes[i] < 0) {
      magnitudes[i] = 0;
    }

    magnitudes[i] = sqrtf((float)magnitudes[i]); // Phase 1 2026-05-20: was sqrt() (soft-double on S2)

    // Normalizing the magnitude
    float normalized_magnitude = magnitudes[i] * frequencies[i].inv_block_size_half;
    magnitudes_normalized[i] = normalized_magnitude;

    if (frequencies[i].target_freq == 440.0) {
      //USBSerial.println(magnitudes_normalized[i]);
    }

    // Phase 1 2026-05-20: asymmetric attack/release for transient snap (was symmetric 0.3 EMA).
    // Arrays are float (see globals.h); coefficients kept float to match (no new soft-float introduced).
    {
      float coeff = (magnitudes_normalized[i] > magnitudes_normalized_avg[i])
                      ? MAGNITUDES_AVG_ATTACK
                      : MAGNITUDES_AVG_RELEASE;
      magnitudes_normalized_avg[i] = (magnitudes_normalized[i] * coeff)
                                   + (magnitudes_normalized_avg[i] * (1.0f - coeff));
    }
  }

  // Gather noise data if noise_complete == false
  if (noise_complete == false) {
    for (uint8_t i = 0; i < NUM_FREQS; i += 1) {
      if (magnitudes_normalized_avg[i] > noise_samples[i]) {
        noise_samples[i] = magnitudes_normalized_avg[i];
      }
    }
    noise_iterations++;
    if (noise_iterations >= 256) {  // Calibration complete
      noise_complete = true;
      USBSerial.println("NOISE CAL COMPLETE");

      // SINGLE-DOMAIN CAL (2026-05-20): DC_OFFSET was stamped at iter 128 in
      // i2s_audio.h. SSL was sampled in Phase B (iters 129..240) from AC-corrected
      // max_waveform_val_raw. Both values are already in the AC domain. No
      // post-hoc DC-correction needed — the previous correction at this site
      // (subtract DC_OFFSET from SSL) is removed because it would now double-correct.

      // Reset broadband AGC v2 state. The new design has a hysteretic silence gate that should
      // already prevent the runaway-during-cal pathology, but for belt-and-braces we reset all
      // adaptive state at cal completion so the system starts fresh from a known baseline.
      for (uint8_t b = 0; b < NUM_AGC_BANDS; b++) {
        agc_bands[b].gain = SQ15x16(1.0);
        agc_bands[b].target_gain = SQ15x16(1.0);
      }
      agc_envelope = SQ15x16(0.0);
      agc_noise_floor = SQ15x16(0.001);
      agc_gated = true;

      save_ambient_noise_calibration();           // Save results to noise_cal.bin
      save_config();                              // Save config to config.bin
    }
  }

  // Apply noise reduction data
  for (uint8_t i = 0; i < NUM_FREQS; i += 1) {
    if (noise_complete == true) {
      magnitudes_normalized_avg[i] -= float(noise_samples[i] * SQ15x16(1.5));  // Treat noise 1.5x louder than calibration
      if (magnitudes_normalized_avg[i] < 0.0) {
        magnitudes_normalized_avg[i] = 0.0;
      }
    }
  }

  memcpy(magnitudes_final, magnitudes_normalized_avg, sizeof(float) * NUM_FREQS);
  low_pass_array(magnitudes_final, magnitudes_last, NUM_FREQS, SYSTEM_FPS, 1.0 + (10.0 * MOOD_VAL));
  memcpy(magnitudes_last, magnitudes_final, sizeof(float) * NUM_FREQS);

  /*
  // When enabled, streams magnitudes[] array over Serial
  if (stream_magnitudes == true) {
    if (serial_iter >= 2) {  // Don't print every frame
      serial_iter = 0;
      USBSerial.print("sbs((magnitudes=");
      for (uint16_t i = 0; i < NUM_FREQS; i++) {
        USBSerial.print(uint32_t(magnitudes[i]));
        if (i < NUM_FREQS - 1) {
          USBSerial.print(',');
        }
      }
      USBSerial.println("))");
    }
  }
  */

  // --- BROADBAND AGC v2 (re-engaged 2026-05-21 after mutex confirmed dead) ---
  //
  // Single broadband AGC stage replaces the per-band cochlear AGC (which had
  // pole-instability bugs in TREBLE, dead AGC_MAX_BAND_DIVERGENCE code, and a
  // silence→max_gain ramp pathology — flagged by 5-SSA root-cause analysis 2026-05-20).
  //
  // Pipeline:
  //   1. signal_level = average of magnitudes_final[] across all NUM_FREQS bins
  //   2. Envelope follower (asymmetric: 30 ms attack, 500 ms release at 100 Hz frame rate)
  //   3. Slow noise-floor tracker (10 s time constant, only updates when envelope is
  //      near current floor — rejects loud transients from contaminating the estimate)
  //   4. Hysteretic silence gate (open when envelope ≥ 4× noise_floor, close when ≤ 2.5×)
  //   5. Target gain = AGC_TARGET / envelope (capped at AGC_MAX_GAIN, smoothed by GAIN_SMOOTH)
  //   6. Gain FREEZES while gated (no runaway during silence)
  //   7. Output: spectrogram[i] = magnitudes_final[i] * gain * spectral_tilt_lut[i], clamped ≤ 1.0
  //
  // agc_bands[] is mirrored with the broadband values for telemetry compat.
  // agc_envelope / agc_noise_floor / agc_gated are reset at noise_cal completion
  // (see end-of-cal block ~30 lines below) so they start fresh on every cal cycle.

  // Time constants at SYSTEM_FPS ≈ 100 Hz
  const SQ15x16 ATTACK_ALPHA  = SQ15x16(0.28);   // 30 ms
  const SQ15x16 RELEASE_ALPHA = SQ15x16(0.02);   // 500 ms
  const SQ15x16 NOISE_ALPHA   = SQ15x16(0.001);  // 10 s
  const SQ15x16 GAIN_SMOOTH   = SQ15x16(0.05);   // ~200 ms gain settle
  const SQ15x16 AGC_TARGET    = SQ15x16(0.4);    // ~40 % headroom under saturation
  const SQ15x16 AGC_MAX_GAIN  = SQ15x16(10.0);
  const SQ15x16 AGC_EPS           = SQ15x16(0.001);
  const SQ15x16 effective_target = VP_FIX_AGC_SOFT_KNEE ? SQ15x16(0.25) : AGC_TARGET;

  // 1. Broadband signal level (average magnitude across bins)
  float signal_level = 0.0f;
  for (uint16_t i = 0; i < NUM_FREQS; i++) {
    signal_level += magnitudes_final[i];
  }
  signal_level /= (float)NUM_FREQS;
  SQ15x16 sig_q = SQ15x16(signal_level);

  // 2. Envelope follower (asymmetric attack/release)
  if (sig_q > agc_envelope) {
    agc_envelope += (sig_q - agc_envelope) * ATTACK_ALPHA;
  } else {
    agc_envelope += (sig_q - agc_envelope) * RELEASE_ALPHA;
  }

  // 3. Slow noise-floor tracker — only tracks when envelope is near current floor
  //    (prevents contamination by loud transients).
  if (agc_envelope < agc_noise_floor * SQ15x16(2.0)) {
    agc_noise_floor += (agc_envelope - agc_noise_floor) * NOISE_ALPHA;
  }
  if (agc_noise_floor < AGC_EPS) agc_noise_floor = AGC_EPS;  // floor at 0.001

  // 4. Hysteretic silence gate
  SQ15x16 gate_open_th  = agc_noise_floor * SQ15x16(4.0);
  SQ15x16 gate_close_th = agc_noise_floor * SQ15x16(2.5);
  if (agc_gated && agc_envelope > gate_open_th)  agc_gated = false;
  if (!agc_gated && agc_envelope < gate_close_th) agc_gated = true;

  // 5+6. Target gain (only adapts when ungated; frozen during silence)
  static SQ15x16 agc_gain = SQ15x16(1.0);
  if (!agc_gated) {
    SQ15x16 target_gain = effective_target / (agc_envelope + AGC_EPS);
    if (target_gain > AGC_MAX_GAIN) target_gain = AGC_MAX_GAIN;
    if (target_gain < SQ15x16(0.1)) target_gain = SQ15x16(0.1);  // floor at 10× attenuation
    agc_gain += (target_gain - agc_gain) * GAIN_SMOOTH;
  }

  // 7. Apply gain × tilt to spectrogram, clamp to [0, 1]
  for (uint16_t i = 0; i < NUM_FREQS; i++) {
    SQ15x16 out = SQ15x16(magnitudes_final[i]) * agc_gain * spectral_tilt_lut[i];
    if (VP_FIX_AGC_SOFT_KNEE && out > SQ15x16(0.5)) {
      SQ15x16 excess = out - SQ15x16(0.5);
      out = SQ15x16(0.5) + (excess / (SQ15x16(1.0) + excess));
    }
    if (out > SQ15x16(1.0)) out = SQ15x16(1.0);
    if (out < SQ15x16(0.0)) out = SQ15x16(0.0);
    spectrogram[i] = out;
  }

  // Telemetry mirror — broadband gain/energy across all band slots
  for (uint8_t b = 0; b < NUM_AGC_BANDS; b++) {
    agc_bands[b].gain        = agc_gain;
    agc_bands[b].target_gain = agc_gain;
    agc_bands[b].energy      = sig_q;
  }
  // --- END BROADBAND AGC v2 ---
}

void calculate_novelty(uint32_t t_now) {
  static uint32_t iter = 0;
  iter++;

  // Calculate "novelty" (positive change) in this moment by marking the positive changes from the previous frame
  // Sum in a column-wise fashion into novelty_now
  SQ15x16 novelty_now = 0.0;
  for (uint16_t i = 0; i < NUM_FREQS; i++) {
    int16_t rounded_index = spectral_history_index - 1;
    while (rounded_index < 0) {
      rounded_index += SPECTRAL_HISTORY_LENGTH;
    }
    SQ15x16 novelty_bin = spectrogram[i] - spectral_history[rounded_index][i];

    if (novelty_bin < 0.0) {
      novelty_bin = 0.0;
    }

    novelty_now += novelty_bin;
  }
  novelty_now /= NUM_FREQS;  // Normalize result

  // Append current spectrogram to last place in history:
  for (uint16_t b = 0; b < NUM_FREQS; b += 8) {
    spectral_history[spectral_history_index][b + 0] = spectrogram[b + 0];
    spectral_history[spectral_history_index][b + 1] = spectrogram[b + 1];
    spectral_history[spectral_history_index][b + 2] = spectrogram[b + 2];
    spectral_history[spectral_history_index][b + 3] = spectrogram[b + 3];
    spectral_history[spectral_history_index][b + 4] = spectrogram[b + 4];
    spectral_history[spectral_history_index][b + 5] = spectrogram[b + 5];
    spectral_history[spectral_history_index][b + 6] = spectrogram[b + 6];
    spectral_history[spectral_history_index][b + 7] = spectrogram[b + 7];
  }

  // Append new novelty measurement to novelty curve history
  novelty_curve[spectral_history_index] = sqrtf((float)novelty_now); // Phase 1 2026-05-20: sqrt→sqrtf for S2 soft-float

  spectral_history_index++;
  if (spectral_history_index >= SPECTRAL_HISTORY_LENGTH) {
    spectral_history_index -= SPECTRAL_HISTORY_LENGTH;
  }
}

static void precompute_goertzel_constants() {
  for (uint16_t i = 0; i < NUM_FREQS; i++) {
    frequencies[i].target_freq = notes[i + CONFIG.NOTE_OFFSET];
    float neighbor_left, neighbor_right;
    if (i == 0) { neighbor_left = notes[i + CONFIG.NOTE_OFFSET]; neighbor_right = notes[i + CONFIG.NOTE_OFFSET + 1]; }
    else if (i == NUM_FREQS - 1) { neighbor_left = notes[i + CONFIG.NOTE_OFFSET - 1]; neighbor_right = notes[i + CONFIG.NOTE_OFFSET]; }
    else { neighbor_left = notes[i + CONFIG.NOTE_OFFSET - 1]; neighbor_right = notes[i + CONFIG.NOTE_OFFSET + 1]; }
    float max_distance_hz = std::max(std::fabs(neighbor_left - frequencies[i].target_freq), std::fabs(neighbor_right - frequencies[i].target_freq));
    frequencies[i].block_size = uint16_t(CONFIG.SAMPLE_RATE / (max_distance_hz * 2.0f));
    if (frequencies[i].block_size > 2000) frequencies[i].block_size = 2000;
    frequencies[i].inv_block_size_half = frequencies[i].block_size ? 2.0f / frequencies[i].block_size : 0;
    float k = int(0.5f + ((frequencies[i].block_size * frequencies[i].target_freq) / CONFIG.SAMPLE_RATE));
    float w = (2.0f * float(M_PI) * k) / frequencies[i].block_size;
    frequencies[i].coeff_q14 = int32_t((1 << 14) * (2.0f * std::cos(w)));
    float freq = frequencies[i].target_freq;
    float tilt = freq < 200.f ? 1.30f : freq > 3000.f ? 0.85f : 1.0f;
    spectral_tilt_lut[i] = SQ15x16(tilt);
  }
}

static int16_t sens_sample(int16_t in) {
  // Full-scale music PCM is not the SPH0645 integer domain. Sensitivity 2.4 is
  // kept, and full scale lands at 2400, above the stored sweet-spot floor of 302
  // and inside the range the Goertzel shift (>> 6) was written for.
  int32_t x = int32_t(in) * 2400 / 32767;
  if (x > 32767) x = 32767;
  if (x < -32767) x = -32767;
  return int16_t(x);
}

static void condition_window(const int16_t* pcm, int pcm_n, int end) {
  for (int i = 0; i < SAMPLE_HISTORY_LENGTH; i++) {
    int src = end - SAMPLE_HISTORY_LENGTH + i;
    sample_window[i] = (src >= 0 && src < pcm_n) ? sens_sample(pcm[src]) : 0;
  }
  max_waveform_val_raw = 0;
  for (int i = SAMPLE_HISTORY_LENGTH - CONFIG.SAMPLES_PER_CHUNK; i < SAMPLE_HISTORY_LENGTH; i++) {
    uint32_t a = uint32_t(std::abs(int(sample_window[i])));
    if (a > max_waveform_val_raw) max_waveform_val_raw = a;
  }
  max_waveform_val = float(max_waveform_val_raw) - CONFIG.SWEET_SPOT_MIN_LEVEL;
  if (max_waveform_val > max_waveform_val_follower) {
    float delta = max_waveform_val - max_waveform_val_follower;
    max_waveform_val_follower += delta * 0.25f;
  } else if (max_waveform_val < max_waveform_val_follower) {
    float delta = max_waveform_val_follower - max_waveform_val;
    max_waveform_val_follower -= delta * 0.005f;
    if (max_waveform_val_follower < CONFIG.SWEET_SPOT_MIN_LEVEL) max_waveform_val_follower = CONFIG.SWEET_SPOT_MIN_LEVEL;
  }
  float waveform_peak_scaled_raw = max_waveform_val / max_waveform_val_follower;
  if (waveform_peak_scaled_raw > waveform_peak_scaled) waveform_peak_scaled += (waveform_peak_scaled_raw - waveform_peak_scaled) * 0.25f;
  else waveform_peak_scaled -= (waveform_peak_scaled - waveform_peak_scaled_raw) * 0.25f;
}

static void write_edge(FILE* f, const CRGB16* edge, float gain) {
  for (int i = 0; i < NATIVE_RESOLUTION; i++) {
    for (double c : {double(edge[i].r), double(edge[i].g), double(edge[i].b)}) {
      if (c < 0) c = 0;
      if (c > 1) c = 1;
      uint16_t q = uint16_t(c * gain * 65535.0 + 0.5);
      std::fwrite(&q, 2, 1, f);
    }
  }
}

static void prove_history() {
  Conf saved = CONFIG;
  std::memset(chromagram_smooth, 0, sizeof chromagram_smooth);
  chromagram_smooth[0] = SQ15x16(1);
  std::memset(leds_16_prev, 0, sizeof leds_16_prev);
  for (int i = 0; i < 40; i++) light_mode_bloom(leds_16_prev);
  double carried = 0;
  for (int i = 96; i < 112; i++) carried += double(leds_16[i].r) + double(leds_16[i].g) + double(leds_16[i].b);
  std::memset(leds_16_prev, 0, sizeof leds_16_prev);
  light_mode_bloom(leds_16_prev);
  double once = 0;
  for (int i = 96; i < 112; i++) once += double(leds_16[i].r) + double(leds_16[i].g) + double(leds_16[i].b);
  CONFIG = saved;
  if (!(carried > 0.05 && once < 0.01)) {
    std::fprintf(stderr, "history check failed carried %.4f once %.4f\n", carried, once);
    std::exit(1);
  }
}

int main(int argc, char** argv) {
  if (argc < 3) { std::fprintf(stderr, "usage: may22_donor pcm12800 prefix\n"); return 2; }
  precompute_goertzel_constants();
  prove_history();
  std::memset(leds_16_prev, 0, sizeof leds_16_prev);
  std::memset(leds_16_prev_secondary, 0, sizeof leds_16_prev_secondary);
  std::ifstream in(argv[1], std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(in)), {});
  if (bytes.size() < 4 || bytes.size() % 2) { std::fprintf(stderr, "pcm\n"); return 2; }
  std::vector<int16_t> pcm(bytes.size() / 2);
  std::memcpy(pcm.data(), bytes.data(), bytes.size());
  const double sys_fps = 48.26, led_fps = 115.86;
  const double seconds = double(pcm.size()) / 12800.0;
  const int frames = int(seconds * led_fps);
  FILE* donor = std::fopen((std::string(argv[2]) + ".rgb16le").c_str(), "wb");
  FILE* titan = std::fopen((std::string(argv[2]) + ".titan.rgb16le").c_str(), "wb");
  if (!donor || !titan) return 2;
  CRGB16 wave_last{};
  float wave_peak_last = 0;
  int sys_i = 0;
  Conf primary = CONFIG;
  for (int frame = 0; frame < frames; frame++) {
    double t = frame / led_fps;
    int need = int(t * sys_fps);
    while (sys_i <= need) {
      int end = int((sys_i / sys_fps) * 12800.0);
      if (end > int(pcm.size())) end = int(pcm.size());
      condition_window(pcm.data(), int(pcm.size()), end);
      CONFIG = primary;
      CONFIG.LIGHTSHOW_MODE = LIGHT_MODE_BLOOM;
      process_GDFT();
      calculate_novelty(uint32_t(t * 1000.0));
      if (primary.AUTO_COLOR_SHIFT) process_color_shift();
      sys_i++;
    }
    CONFIG = primary;
    vp_render_secondary_channel = false;
    if (frame == 2000) {
      std::fprintf(stderr, "t %.2f chroma_final_max %.4f waveform_peak %.3f raw %u spec40 %.4f\n",
        t, float(vp_dbg_chroma_final_max), waveform_peak_scaled, max_waveform_val_raw, float(spectrogram[40]));
    }
    get_smooth_spectrogram();
    make_smooth_chromagram();
    light_mode_bloom(leds_16_prev);
    render_bulb_cover();
    apply_brightness();
    apply_incandescent_filter();
    clip_led_values(leds_16);
    CRGB16 top[NATIVE_RESOLUTION];
    std::memcpy(top, leds_16, sizeof top);
    CONFIG.LIGHTSHOW_MODE = SECONDARY_LIGHTSHOW_MODE;
    CONFIG.PHOTONS = SECONDARY_PHOTONS;
    CONFIG.CHROMA = SECONDARY_CHROMA;
    CONFIG.MOOD = SECONDARY_MOOD;
    CONFIG.MIRROR_ENABLED = SECONDARY_MIRROR_ENABLED;
    CONFIG.SATURATION = SECONDARY_SATURATION;
    CONFIG.AUTO_COLOR_SHIFT = SECONDARY_AUTO_COLOR_SHIFT;
    vp_render_secondary_channel = true;
    std::memcpy(leds_16, leds_16_prev_secondary, sizeof leds_16);
    light_mode_waveform_fast(leds_16_prev_secondary, wave_last, wave_peak_last);
    std::memcpy(leds_16_prev_secondary, leds_16, sizeof leds_16);
    float secondary_gain = SECONDARY_PHOTONS * SECONDARY_PHOTONS * silent_scale;
    for (int i = 0; i < NATIVE_RESOLUTION; i++) {
      leds_16[i].r = leds_16[i].r * secondary_gain;
      leds_16[i].g = leds_16[i].g * secondary_gain;
      leds_16[i].b = leds_16[i].b * secondary_gain;
    }
    clip_led_values(leds_16);
    write_edge(donor, top, 1.f);
    write_edge(donor, leds_16, 1.f);
    write_edge(titan, top, 24.f / 255.f);
    write_edge(titan, leds_16, 24.f / 255.f);
    CONFIG = primary;
    vp_render_secondary_channel = false;
  }
  std::fclose(donor);
  std::fclose(titan);
  std::printf("frames %d led_fps %.2f system_fps %.2f seconds %.2f\n", frames, led_fps, sys_fps, seconds);
  return 0;
}
