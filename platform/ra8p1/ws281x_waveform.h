#pragma once

#include "ws281x_diag.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HOST waveform / cost layer. SenseGlow timer/DMAC cost method adapted to
   K1 WS2812/WS2816 profiles. Not SK6812 RGBW. Not a GPIO-PODR drop-in.
   GPT duty tables are the selected transmitter encoding; PODR cost is the
   comparison, not the product backend.
   Profile bounds: WS2812 128 pixels (24-bit, P601 bench strip);
   WS2816 80 pixels per GPT lane (48-bit). Duty cap is the larger table. */

#define K1_WS281X_WAVE_MAX_PIXELS_WS2812 128u
#define K1_WS281X_WAVE_MAX_PIXELS_WS2816 80u
#define K1_WS281X_WAVE_MAX_PIXELS K1_WS281X_WAVE_MAX_PIXELS_WS2812
#define K1_WS281X_GPT_DUTY_CAP                                             \
    (K1_WS281X_WAVE_MAX_PIXELS_WS2816 * 48u)

typedef struct {
    uint32_t lanes;
    uint32_t pixels;
    uint32_t bits;
    uint32_t slots;
    uint32_t data_bytes;
    uint32_t reset_bytes;
    uint32_t bytes;
    uint32_t payload_bytes_per_s;
    uint32_t data_ns;
    uint32_t reset_ns;
    uint32_t emit_ns;
} k1_ws281x_cost_t;

typedef struct {
    uint32_t clock_hz;
    uint32_t period_counts;
    uint32_t t0h_counts;
    uint32_t t1h_counts;
    uint32_t period_reg; /* period_counts - 1 */
    uint32_t bits;
} k1_ws281x_duty_meta_t;

int k1_ws281x_pack_grb24(uint8_t r, uint8_t g, uint8_t b, uint8_t *out);
uint32_t k1_ws281x_profile_max_pixels(uint32_t profile);
uint32_t k1_ws281x_ns_to_counts_ceil(uint32_t clock_hz, uint32_t ns);
int k1_ws281x_gpt_quantise(uint32_t profile, uint32_t clock_hz,
                           k1_ws281x_duty_meta_t *meta);
int k1_ws281x_build_gpt_duty(const uint8_t *bytes, size_t nbytes,
                             uint32_t profile, uint32_t clock_hz,
                             uint32_t *duty, size_t capacity,
                             k1_ws281x_duty_meta_t *meta);
int k1_ws281x_gpio_podr_cost(uint32_t pixels, uint32_t profile,
                             uint32_t slot_ns, k1_ws281x_cost_t *cost);
int k1_ws281x_gpt_cost(uint32_t pixels, uint32_t profile,
                       k1_ws281x_cost_t *cost);
int k1_ws281x_gpio_podr_cost_lanes(uint32_t lanes, uint32_t pixels,
                                   uint32_t profile, uint32_t slot_ns,
                                   k1_ws281x_cost_t *cost);
int k1_ws281x_gpt_cost_lanes(uint32_t lanes, uint32_t pixels, uint32_t profile,
                             k1_ws281x_cost_t *cost);
uint32_t k1_ws281x_gpt_buffer_bytes(uint32_t lanes, uint32_t pixels,
                                    uint32_t profile, uint32_t double_buffered);

#ifdef __cplusplus
}
#endif
