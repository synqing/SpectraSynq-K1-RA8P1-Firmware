#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Causal 5:4 stereo: 16 kHz / 120 frames → 12.8 kHz / 96 frames / 7.5 ms.
   One-pole lowpass at 16 kHz, then linear 5:4. Pole state persists across
   blocks. Delay is one input sample (62.5 µs). IM1 and IM2 are separate
   channels in the same stereo frame; callers must not mix lanes. */

#define K1_RATE_IN_HZ 16000u
#define K1_RATE_OUT_HZ 12800u
#define K1_RATE_IN_FRAMES 120u
#define K1_RATE_OUT_FRAMES 96u
#define K1_RATE_CHANNELS 2u
#define K1_RATE_DELAY_INPUT_SAMPLES 1u
#define K1_RATE_DELAY_US 62u
#define K1_RATE_POLE_Q15 16384 /* 0.5 */

typedef struct {
    int32_t lpf[K1_RATE_CHANNELS];
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
