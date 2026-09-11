#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Causal 5:4 stereo adapter: 16 kHz / 120 frames → 12.8 kHz / 96 frames
   per 7.5 ms. Filter state persists across blocks. IM1 programme and IM2
   measurement stay separate callers. */

#define K1_RATE_IN_HZ 16000u
#define K1_RATE_OUT_HZ 12800u
#define K1_RATE_IN_FRAMES 120u
#define K1_RATE_OUT_FRAMES 96u
#define K1_RATE_CHANNELS 2u
#define K1_RATE_FIR_TAPS 4u
#define K1_RATE_DELAY_INPUT_SAMPLES 3u
#define K1_RATE_DELAY_US 188u /* 3 / 16000 s, truncated */

typedef struct {
    int32_t z[K1_RATE_FIR_TAPS][K1_RATE_CHANNELS];
    uint32_t started;
    uint32_t blocks;
    uint32_t delay_samples;
} k1_rate_adapt_t;

void k1_rate_adapt_reset(k1_rate_adapt_t *st);
int k1_rate_adapt_block(k1_rate_adapt_t *st, const int16_t *in120_stereo,
                        int16_t *out96_stereo);

#ifdef __cplusplus
}
#endif
