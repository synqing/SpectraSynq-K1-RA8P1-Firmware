#ifndef K1_EMOTISCOPE_PORT_H
#define K1_EMOTISCOPE_PORT_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { K1_EMOTISCOPE_PIXELS=128, K1_EMOTISCOPE_CHUNK=64,
       K1_EMOTISCOPE_SAMPLE_RATE=12800, K1_EMOTISCOPE_RENDER_HZ=100,
       K1_EMOTISCOPE_FREQS=64, K1_EMOTISCOPE_TEMPI=96,
       K1_EMOTISCOPE_NOVELTY=1024 };
typedef struct {
  float brightness, softness, color, warmth, color_range, speed, saturation, background;
  int32_t current_mode;
  bool mirror_mode, screensaver, temporal_dithering, auto_color_cycle, reverse_color_range;
} K1EmotiscopeConfig;
typedef struct {
  uint32_t audio_hops, render_frames, rejected_audio_chunks, nonfinite_output_frames;
  uint32_t clock_regressions;
  uint32_t now_us, now_ms;
  int32_t current_mode;
  float vu_level_raw, vu_level, vu_max, vu_floor, tempo_confidence, tempi_power_sum;
  float spectrogram[64], spectrogram_smooth[64], chromagram[12];
  float novelty[1024], novelty_normalized[1024], vu_curve[1024];
  float tempi_magnitude[96], tempi_full_scale[96], tempi_phase[96], tempi_beat[96];
  float tempi_smooth[96], tempi_target_hz[96];
  uint32_t tempi_block_size[96];
  float post_gamma_rgb[384];
} K1EmotiscopeSnapshot;
typedef struct {
  bool valid;
  uint32_t audio_hops, render_frames, audio_gaps, fresh_hops;
  uint32_t rejected_audio_chunks, nonfinite_output_frames;
  uint32_t clock_regressions;
  float vu_level_raw, vu_level;
  float spectrum[64], spectrum_smooth[64], chromagram[12];
  float dominant_tempo_bpm, tempo_confidence;
} K1EmotiscopeStatus;
/* Capture loss is an explicit port adaptation: zero only PCM capture history,
 * rewarm 64 fresh chunks (320 ms), and mute AP deposits while rewarming.
 * Existing VP/tempo/LPF/dither/configuration histories are retained. */
void k1_emotiscope_audio_gap(void);
void k1_emotiscope_status(K1EmotiscopeStatus *out);
/* Single-threaded owner; init is boot-once because original function-local histories
 * are retained. No ordinary audio discontinuity invokes init or clears history. */
bool k1_emotiscope_init(uint32_t random_seed);
void k1_emotiscope_default_config(K1EmotiscopeConfig *out);
bool k1_emotiscope_set_config(const K1EmotiscopeConfig *configuration);
void k1_emotiscope_get_config(K1EmotiscopeConfig *out);
/* Masked update. Bits are field_id-1 for fields 1..14 only. Validates supplied
 * fields and leaves every clear field untouched, including an auto-color phase
 * that has walked above 1. An explicit color write stores a normalized hue and
 * replaces that phase. A real mode change sets the donor fade once. */
typedef struct {
  uint32_t mask;
  float brightness, softness, color, warmth, color_range, speed, saturation, background;
  int32_t current_mode;
  uint8_t mirror_mode, screensaver, temporal_dithering, auto_color_cycle, reverse_color_range;
} K1EmotiscopePatch;
bool k1_emotiscope_patch(const K1EmotiscopePatch *patch, uint32_t *changed, uint32_t *mode_transition);
/* Fractional part of the stored color phase, in [0,1). Not written back. */
float k1_emotiscope_effective_hue(void);
#ifdef K1_EMOTISCOPE_TEST_SEAM
void k1_emotiscope_test_set_raw_color(float color);
float k1_emotiscope_test_lpf_drag(void);
void k1_emotiscope_test_set_novelty_sample(float value);
float k1_emotiscope_test_novelty_sample(void);
#endif
/* Exactly 64 normalized mono samples at 12800 Hz. Rejects an entire invalid chunk.
 * now_us is the deterministic clock supplied to the hardware-independent engine. */
bool k1_emotiscope_push_pcm64(const float samples[64], uint32_t now_us);
/* Called at the explicit port schedule of 100 Hz. This is not a recovered original
 * GPU frame rate. Returns RGB8 (not GRB) after the donor's gamma and dithering.
 * No Titan palette, gamma, or global hardware brightness is applied here. */
bool k1_emotiscope_render(uint32_t now_us, uint8_t rgb[384]);
void k1_emotiscope_snapshot(K1EmotiscopeSnapshot *out);
const char *k1_emotiscope_mode_name(uint32_t mode);
#ifdef __cplusplus
}
#endif
#endif
