#include "k1_asrc_24k.h"

#include <string.h>

void k1_asrc24_reset(k1_asrc24_t *st, uint32_t epoch, uint32_t source_hz)
{
    if (st == 0) {
        return;
    }
    memset(st, 0, sizeof(*st));
    st->epoch = epoch;
    if (source_hz < K1_ASRC24_MIN_IN_HZ || source_hz > K1_ASRC24_MAX_IN_HZ) {
        st->source_hz = K1_ASRC24_NOMINAL_IN_HZ;
    } else {
        st->source_hz = source_hz;
    }
}

uint32_t k1_asrc24_phase_increment_q16(uint32_t source_hz)
{
    /* increment = source_hz / 24000 in Q16. Nominal 40000/24000 = 5/3. */
    return (uint32_t)(((uint64_t)source_hz << 16) / K1_ASRC24_OUT_HZ);
}

int k1_asrc24_set_rate(k1_asrc24_t *st, uint32_t source_hz)
{
    if (st == 0) {
        return k1_asrc24_err_arg;
    }
    if (source_hz < K1_ASRC24_MIN_IN_HZ || source_hz > K1_ASRC24_MAX_IN_HZ) {
        return k1_asrc24_err_rate;
    }
    st->source_hz = source_hz;
    return k1_asrc24_ok;
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

int k1_asrc24_push(k1_asrc24_t *st, const int16_t *src, uint32_t count)
{
    uint32_t i;
    if (st == 0 || (count > 0u && src == 0)) {
        return k1_asrc24_err_arg;
    }
    for (i = 0; i < count; ++i) {
        if (st->filled >= K1_ASRC24_RING) {
            return k1_asrc24_err_arg;
        }
        st->ring[st->write_pos] = src[i];
        st->write_pos = (st->write_pos + 1u) % K1_ASRC24_RING;
        st->filled += 1u;
    }
    return k1_asrc24_ok;
}

int k1_asrc24_pull180(k1_asrc24_t *st, int16_t out180[K1_ASRC24_OUT_FRAMES])
{
    uint32_t o;
    const uint32_t inc = k1_asrc24_phase_increment_q16(st ? st->source_hz : 0);
    if (st == 0 || out180 == 0) {
        return k1_asrc24_err_arg;
    }
    if (st->source_hz < K1_ASRC24_MIN_IN_HZ || st->source_hz > K1_ASRC24_MAX_IN_HZ) {
        return k1_asrc24_err_rate;
    }
    for (o = 0; o < K1_ASRC24_OUT_FRAMES; ++o) {
        const uint32_t idx =
            (st->read_pos + (uint32_t)st->src_index) % K1_ASRC24_RING;
        const uint32_t next = (idx + 1u) % K1_ASRC24_RING;
        int32_t a;
        int32_t b;
        int32_t y;
        if (st->filled < st->src_index + 2u) {
            st->starved += 1u;
            return k1_asrc24_err_starved;
        }
        a = st->ring[idx];
        b = st->ring[next];
        y = a * (int32_t)(65536u - st->frac_q16) + b * (int32_t)st->frac_q16;
        out180[o] = sat16((y + 32768) >> 16);
        st->frac_q16 += inc;
        st->src_index += st->frac_q16 >> 16;
        st->frac_q16 &= 0xffffu;
        st->produced += 1u;
    }
    {
        const uint32_t consume = (uint32_t)st->src_index;
        if (consume > st->filled) {
            return k1_asrc24_err_arg;
        }
        st->read_pos = (st->read_pos + consume) % K1_ASRC24_RING;
        st->filled -= consume;
        st->src_index = 0u;
    }
    return k1_asrc24_ok;
}
