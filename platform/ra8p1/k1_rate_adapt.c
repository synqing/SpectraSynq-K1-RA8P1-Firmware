#include "k1_rate_adapt.h"

#include <string.h>

void k1_rate_adapt_reset(k1_rate_adapt_t *st)
{
    if (st == 0) {
        return;
    }
    memset(st, 0, sizeof(*st));
    st->delay_samples = K1_RATE_DELAY_INPUT_SAMPLES;
}

static int16_t sat16(int32_t v)
{
    if (v > 32767) {
        return 32767;
    }
    if (v < -32768) {
        return (int16_t)-32768;
    }
    return (int16_t)v;
}

int k1_rate_adapt_block(k1_rate_adapt_t *st, const int16_t *in120_stereo,
                        int16_t *out96_stereo)
{
    int16_t filtered[K1_RATE_IN_FRAMES * K1_RATE_CHANNELS];
    uint32_t i;
    uint32_t o;
    if (st == 0 || in120_stereo == 0 || out96_stereo == 0) {
        return 0;
    }
    for (i = 0; i < K1_RATE_IN_FRAMES; ++i) {
        uint32_t ch;
        for (ch = 0; ch < K1_RATE_CHANNELS; ++ch) {
            int32_t x = in120_stereo[i * 2u + ch] << 15;
            st->lpf[ch] +=
                (int32_t)(((int64_t)(x - st->lpf[ch]) * K1_RATE_POLE_Q15) >> 15);
            filtered[i * 2u + ch] = sat16(st->lpf[ch] >> 15);
        }
    }
    for (o = 0; o < K1_RATE_OUT_FRAMES; ++o) {
        uint32_t num = o * 5u;
        uint32_t idx = num / 4u;
        uint32_t frac = num % 4u;
        uint32_t ch;
        uint32_t next = idx + 1u;
        if (next >= K1_RATE_IN_FRAMES) {
            next = K1_RATE_IN_FRAMES - 1u;
            frac = 0u;
        }
        for (ch = 0; ch < K1_RATE_CHANNELS; ++ch) {
            int32_t a = filtered[idx * 2u + ch];
            int32_t b = filtered[next * 2u + ch];
            int32_t y = a * (int32_t)(4u - frac) + b * (int32_t)frac;
            out96_stereo[o * 2u + ch] = sat16((y + 2) / 4);
        }
    }
    st->started = 1u;
    st->blocks += 1u;
    return 1;
}
