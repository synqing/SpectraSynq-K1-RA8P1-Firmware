#include "bloom_donors.h"
#include <cassert>
#include <cstring>
#include <cstdio>
namespace original_sb {
using CRGB16=k1::reference::SensoryRgbBloom::FixedRgb;
constexpr int NATIVE_RESOLUTION=128;
CRGB16 leds_16[128]{},leds_16_prev[128]{};
struct {float MOOD=0;} CONFIG;
struct RGB8 {unsigned char r,g,b;};
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
void step(RGB8 temp_col){
  // Clear output
  memset(leds_16, 0, sizeof(CRGB16) * NATIVE_RESOLUTION);

  draw_sprite(leds_16, leds_16_prev, 128, 128, 0.250 + 1.750 * CONFIG.MOOD, 0.99);

    leds_16[63] = { temp_col.r / 255.0, temp_col.g / 255.0, temp_col.b / 255.0 };
  leds_16[64] = leds_16[63];

  //-------------------------------------------------------

  // Copy last frame to temp
  memcpy(leds_16_prev, leds_16, sizeof(CRGB16) * NATIVE_RESOLUTION);

  for(uint8_t i = 0; i < 32; i++){
    float prog = i / 31.0;
    leds_16[128-1-i].r *= (prog*prog);
    leds_16[128-1-i].g *= (prog*prog);
    leds_16[128-1-i].b *= (prog*prog);
  }

  for (uint8_t i = 0; i < 64; i++) {
    leds_16[i] = leds_16[128 - 1 - i];
  }
}
}
namespace original_emo {
using std::min;
constexpr int NUM_LEDS=128;
struct CRGBF {float r,g,b;};
CRGBF leds[128]{};float vu_level=0,num_leds_float_lookup[128]{};
struct {float speed=0,saturation=1;bool mirror_mode=true;} configuration;
float clip_float(float x){return std::clamp(x,0.F,1.F);}
float get_color_range_hue(float){return 0;}
CRGBF hsv(float,float,float value){return {value,0,0};} // Declared red mapping boundary.
void draw_sprite(float dest[], float sprite[], uint32_t dest_length, uint32_t sprite_length, float position, float alpha) {
  int32_t position_whole = position;  // Downcast to integer accuracy
  float position_fract = position - position_whole;
  float mix_right = position_fract;
  float mix_left = 1.0 - mix_right;

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
      dest[pos_left] += sprite[i] * mix_left * alpha;
    }

    if (skip_right == false) {
      dest[pos_right] += sprite[i] * mix_right * alpha;
    }
  }
}
float novelty_image_prev[NUM_LEDS] = { 0.0 };

void draw_bloom() {
	float novelty_image[NUM_LEDS] = { 0.0 };

	float spread_speed = 0.125 + 0.875*configuration.speed;
	draw_sprite(novelty_image, novelty_image_prev, NUM_LEDS, NUM_LEDS, spread_speed, 0.99);

	novelty_image[0] = (vu_level);
	novelty_image[0] = min( 1.0f, novelty_image[0] );

	if(configuration.mirror_mode == true){
		for(uint16_t i = 0; i < NUM_LEDS>>1; i++){
			float progress = num_leds_float_lookup[i<<1];
			float novelty_pixel = clip_float(novelty_image[i]*1.0);
			CRGBF color = hsv(
				get_color_range_hue(progress),
				configuration.saturation,
				novelty_pixel
			);
			leds[ (NUM_LEDS>>1)    + i] = color;
			leds[((NUM_LEDS>>1)-1) - i] = color;
		}
	}
	else{
		for(uint16_t i = 0; i < NUM_LEDS; i++){
			float progress = num_leds_float_lookup[i];
			float novelty_pixel = clip_float(novelty_image[i]*2.0);
			CRGBF color = hsv(
				get_color_range_hue(progress),
				configuration.saturation,
				novelty_pixel
			);
			leds[i] = color;
		}
	}

	memcpy(novelty_image_prev, novelty_image, sizeof(float)*NUM_LEDS);
}
}
int main(){
 k1::reference::SensoryRgbBloom sb; k1::reference::EmotiscopeScalarBloom emo;
 std::array<k1::reference::Rgb,64> red{};red.fill({1,0,0});
 for(unsigned frame=0;frame<1800;++frame){
  const unsigned char r=frame%83<9?static_cast<unsigned char>((frame*37)%256):0;
  const unsigned char g=frame%71<5?static_cast<unsigned char>((frame*19)%256):0;
  const float mood=frame<900?.2F:.73F,vu=r/255.F;
  original_sb::CONFIG.MOOD=mood;original_sb::step({r,g,0});sb.step({r/255.F,g/255.F,0},mood);
  original_emo::configuration.speed=mood;original_emo::vu_level=vu;original_emo::draw_bloom();emo.step(vu,mood,red);
  for(unsigned i=0;i<128;++i){
   assert(float(sb.fixed_history[i].r)==float(original_sb::leds_16_prev[i].r));
   assert(float(sb.fixed_history[i].g)==float(original_sb::leds_16_prev[i].g));
   assert(sb.frame[i].red==float(original_sb::leds_16[i].r));
   assert(sb.frame[i].green==float(original_sb::leds_16[i].g));
   assert(emo.history[i]==original_emo::novelty_image_prev[i]);
   assert(emo.frame[i].red==original_emo::leds[i].r);
  }
 }
 std::puts("1800 frames: original retained statements and both adapters match exactly at declared boundaries");
}
