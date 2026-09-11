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

static int16_t fir_push(k1_rate_adapt_t *st, uint32_t ch, int16_t x)
{
    int32_t acc;
    uint32_t t;
    for (t = K1_RATE_FIR_TAPS - 1u; t > 0u; --t) {
        st->z[t][ch] = st->z[t - 1u][ch];
    }
    st->z[0][ch] = x;
    /* Causal 4-tap [1,2,2,1]/6. Group delay 1.5 input samples; delay_samples
       reports the 3-sample state depth. */
    acc = st->z[0][ch] + 2 * st->z[1][ch] + 2 * st->z[2][ch] + st->z[3][ch];
    return sat16((acc + 3) / 6);
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
        filtered[i * 2u] = fir_push(st, 0u, in120_stereo[i * 2u]);
        filtered[i * 2u + 1u] = fir_push(st, 1u, in120_stereo[i * 2u + 1u]);
    }
    for (o = 0; o < K1_RATE_OUT_FRAMES; ++o) {
        uint32_t num = o * 5u;
        uint32_t idx = num / 4u;
        uint32_t frac = num % 4u;
        uint32_t ch;
        if (idx >= K1_RATE_IN_FRAMES - 1u) {
            idx = K1_RATE_IN_FRAMES - 2u;
            frac = 4u;
        }
        for (ch = 0; ch < K1_RATE_CHANNELS; ++ch) {
            int32_t a = filtered[idx * 2u + ch];
            int32_t b = filtered[(idx + 1u) * 2u + ch];
            int32_t y = a * (int32_t)(4u - frac) + b * (int32_t)frac;
            out96_stereo[o * 2u + ch] = sat16((y + 2) / 4);
        }
    }
    st->started = 1u;
    st->blocks += 1u;
    return 1;
}
