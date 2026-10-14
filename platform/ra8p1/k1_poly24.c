#include "k1_poly24.h"

#include <math.h>
#include <string.h>
#ifdef K1_POLY24_CMSIS
#include "arm_math.h"
#endif

_Static_assert(K1_POLY24_PHASES == 128u, "phase index uses the top 7 bits of mu");
_Static_assert(K1_POLY24_TAPS % 2u == 0u && K1_POLY24_TAPS >= 4u, "even tap count");

/* Zeroth-order modified Bessel function (series), float only. */
static float k1_poly24_bessel_i0(float x)
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
static void k1_poly24_design(k1_poly24_t *st)
{
    const float pi = 3.14159265358979323846f;
    const float fc = K1_POLY24_CUTOFF_HZ / (float)st->source_hz;
    const float half = (float)K1_POLY24_TAPS * 0.5f;
    const float i0_beta = k1_poly24_bessel_i0(K1_POLY24_KAISER_BETA);
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
                ? k1_poly24_bessel_i0(K1_POLY24_KAISER_BETA * sqrtf(arg)) / i0_beta
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

void k1_poly24_reset(k1_poly24_t *st, uint32_t source_hz)
{
    if (st == 0) {
        return;
    }
    memset(st, 0, sizeof(*st));
    st->source_hz = (source_hz < K1_POLY24_MIN_IN_HZ || source_hz > K1_POLY24_MAX_IN_HZ)
        ? 40000u : source_hz;
    st->pos_q32 = (uint64_t)(K1_POLY24_TAPS / 2u - 1u) << 32;
    /* The ring is already zero: count the alignment pre-roll as filled. */
    st->filled = K1_POLY24_ALIGN_ZEROS;
    k1_poly24_design(st);
}

void k1_poly24_flush(k1_poly24_t *st)
{
    if (st == 0) {
        return;
    }
    memset(st->ring, 0, sizeof(st->ring));
    st->read_pos = 0u;
    st->filled = K1_POLY24_ALIGN_ZEROS;
    st->pos_q32 = (uint64_t)(K1_POLY24_TAPS / 2u - 1u) << 32;
    st->consumed = 0u;
}

uint64_t k1_poly24_last_output_source_q32(const k1_poly24_t *st)
{
    /* Ring index of the last output's centre, minus the pre-roll. */
    const uint64_t centre = (st->consumed << 32) + st->pos_q32 - st->step_q32;
    const uint64_t zeros = (uint64_t)K1_POLY24_ALIGN_ZEROS << 32;
    return centre >= zeros ? centre - zeros : 0u;
}

int k1_poly24_set_rate(k1_poly24_t *st, uint32_t source_hz)
{
    if (st == 0) {
        return k1_poly24_err_arg;
    }
    if (source_hz < K1_POLY24_MIN_IN_HZ || source_hz > K1_POLY24_MAX_IN_HZ) {
        return k1_poly24_err_rate;
    }
    st->source_hz = source_hz;
    k1_poly24_design(st);
    return k1_poly24_ok;
}

int k1_poly24_push(k1_poly24_t *st, const int16_t *src, uint32_t count)
{
    if (st == 0 || (count > 0u && src == 0)) {
        return k1_poly24_err_arg;
    }
    if (count > K1_POLY24_RING - st->filled) {
        st->rejected += 1u;
        return k1_poly24_err_full;
    }
    for (uint32_t i = 0u; i < count; ++i) {
        st->ring[(st->read_pos + st->filled) % K1_POLY24_RING] = src[i];
        st->filled += 1u;
    }
    return k1_poly24_ok;
}

static int16_t k1_poly24_sat(float v)
{
    const float r = v + (v >= 0.0f ? 0.5f : -0.5f);
    if (r >= 32767.0f) {
        return 32767;
    }
    if (r <= -32768.0f) {
        return (int16_t)-32768;
    }
    return (int16_t)r;
}

int k1_poly24_pull180(k1_poly24_t *st, int16_t out[K1_POLY24_OUT_FRAMES])
{
    if (st == 0 || out == 0) {
        return k1_poly24_err_arg;
    }
    const uint64_t last = st->pos_q32 + (uint64_t)(K1_POLY24_OUT_FRAMES - 1u) * st->step_q32;
    if ((uint64_t)st->filled < (last >> 32) + K1_POLY24_TAPS / 2u + 1u) {
        return k1_poly24_err_not_ready;
    }
    float window[K1_POLY24_TAPS];
    uint64_t pos = st->pos_q32;
    for (uint32_t o = 0u; o < K1_POLY24_OUT_FRAMES; ++o) {
        const uint32_t n = (uint32_t)(pos >> 32);
        const uint32_t frac = (uint32_t)pos;  /* mu in Q32 */
        const uint32_t first = st->read_pos + n - (K1_POLY24_TAPS / 2u - 1u);
        for (uint32_t j = 0u; j < K1_POLY24_TAPS; ++j) {
            window[j] = (float)st->ring[(first + j) % K1_POLY24_RING];
        }
        /* Phase index and interpolation weight from the top bits of mu. */
        const uint32_t phase = frac >> (32u - 7u);  /* 128 phases */
        const float t = (float)(frac & ((1u << (32u - 7u)) - 1u)) * (1.0f / (float)(1u << (32u - 7u)));
        const float *a = st->table[phase];
        const float *b = st->table[phase + 1u];
        float s0 = 0.0f;
        float s1 = 0.0f;
#ifdef K1_POLY24_CMSIS
        /* Lane P4 step 8: CMSIS-DSP's MVE kernel (the plain loop below is not
           vectorised by GCC 13.3 here: 0 q-register vfma in the -O3 image). */
        arm_dot_prod_f32(window, a, K1_POLY24_TAPS, &s0);
        arm_dot_prod_f32(window, b, K1_POLY24_TAPS, &s1);
#else
        for (uint32_t j = 0u; j < K1_POLY24_TAPS; ++j) {
            s0 += window[j] * a[j];
            s1 += window[j] * b[j];
        }
#endif
        out[o] = k1_poly24_sat(s0 + t * (s1 - s0));
        pos += st->step_q32;
    }
    /* Keep K/2-1 samples of history before the next output's centre. */
    const uint32_t drop = (uint32_t)(pos >> 32) - (K1_POLY24_TAPS / 2u - 1u);
    st->read_pos = (st->read_pos + drop) % K1_POLY24_RING;
    st->filled -= drop;
    st->consumed += drop;
    st->pos_q32 = pos - ((uint64_t)drop << 32);
    st->produced += K1_POLY24_OUT_FRAMES;
    return k1_poly24_ok;
}
