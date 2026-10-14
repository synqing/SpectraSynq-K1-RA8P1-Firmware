/* Lane P4 correction 2 host oracle: the PREVIOUS on-device k1_poly24 design,
   copied verbatim from k1_poly24.c at 7e1d69b (renamed). It is the H1 reference
   for the offline-window design; it is not built into any image. */
#include "k1_poly24.h"
#include <math.h>

/* Zeroth-order modified Bessel function (series), float only. */
static float ref_bessel_i0(float x)
{
    const float half = 0.5f * x;
    float term = 1.0f;
    float sum = 1.0f;
    for (uint32_t k = 1u; k < 32u; ++k) {
        const float r = half / (float)k;
        term *= r * r;
        sum += term;
        if (term < 1e-9f * sum) {
            break;
        }
    }
    return sum;
}

/* Tap j of phase mu sits at distance d = j - (K/2 - 1) - mu source samples from
   the output instant. Row PHASES (mu = 1) exists only as the upper neighbour for
   coefficient interpolation. Rows are normalised to unity DC gain. */
void k1_poly24_design_reference(k1_poly24_t *st)
{
    const float pi = 3.14159265358979323846f;
    const float fc = K1_POLY24_CUTOFF_HZ / (float)st->source_hz;
    const float half = (float)K1_POLY24_TAPS * 0.5f;
    const float i0_beta = ref_bessel_i0(K1_POLY24_KAISER_BETA);
    for (uint32_t p = 0u; p <= K1_POLY24_PHASES; ++p) {
        const float mu = (float)p / (float)K1_POLY24_PHASES;
        float sum = 0.0f;
        for (uint32_t j = 0u; j < K1_POLY24_TAPS; ++j) {
            const float d = (float)j - (half - 1.0f) - mu;
            const float x = 2.0f * fc * d;
            const float sinc = fabsf(x) < 1e-6f ? 1.0f : sinf(pi * x) / (pi * x);
            const float r = d / half;
            const float arg = 1.0f - r * r;
            const float w = arg > 0.0f
                ? ref_bessel_i0(K1_POLY24_KAISER_BETA * sqrtf(arg)) / i0_beta
                : 0.0f;
            st->table[p][j] = 2.0f * fc * sinc * w;
            sum += st->table[p][j];
        }
        for (uint32_t j = 0u; j < K1_POLY24_TAPS; ++j) {
            st->table[p][j] /= sum;
        }
    }
    st->step_q32 = ((uint64_t)st->source_hz << 32) / K1_POLY24_OUT_HZ;
}
