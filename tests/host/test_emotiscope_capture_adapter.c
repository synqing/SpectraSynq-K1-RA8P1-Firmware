#include "k1_asrc_24k.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

_Static_assert(K1_ASRC24_OUT_HZ == 12800u, "Emotiscope rate");
_Static_assert(K1_ASRC24_OUT_FRAMES == 64u, "Emotiscope block");
_Static_assert(K1_ASRC24_RING == 1024u, "Do not conceal pressure with a larger queue");

static void conservation(uint32_t rate) {
    k1_asrc24_t st, before;
    int16_t input[296] = {0}, output[66];
    uint32_t submitted = 0, blocks = 0, count;
    const uint32_t total = 10u * rate;
    const uint64_t increment = k1_asrc24_phase_increment_q16(rate);
    k1_asrc24_reset(&st, 19u, rate);
    while (submitted < total) {
        count = total - submitted;
        if (count > 296u) count = 296u;
        assert(k1_asrc24_push(&st, input, count) == 0);
        submitted += count;
        for (;;) {
            memcpy(&before, &st, sizeof st);
            memset(output, 0x5a, sizeof output);
            int result = k1_asrc24_pull180(&st, output + 1);
            if (result == k1_asrc24_err_not_ready) {
                assert(memcmp(&before, &st, sizeof st) == 0);
                for (unsigned i = 0; i < 66; ++i)
                    assert(output[i] == (int16_t)0x5a5a);
                break;
            }
            assert(result == 0);
            assert(output[0] == (int16_t)0x5a5a);
            assert(output[65] == (int16_t)0x5a5a);
            blocks++;
            assert(st.produced == blocks * 64u);
            assert(st.consumed_samples == ((uint64_t)st.produced * increment >> 16));
            assert(st.frac_q16 == ((uint64_t)st.produced * increment & 65535u));
        }
        assert(st.consumed_samples + st.filled == submitted);
        assert(st.discarded_samples == 0 && st.push_rejected == 0);
    }
    /* Ten seconds, with at most one incomplete 5ms block held for lookahead. */
    assert(st.produced >= 128000u - 64u && st.produced <= 128000u);
    assert(st.epoch == 19u);
    printf("conservation rate=%u input=%u output=%u pending=%u phase=%u\n",
           rate, submitted, st.produced, st.filled, st.frac_q16);
    /* A rejected push cannot advance either the ring or FIR history. */
    before = st;
    assert(k1_asrc24_push(&st, input, 1025u) == k1_asrc24_err_arg);
    assert(st.filled == before.filled);
    assert(memcmp(st.filter_history, before.filter_history,
                  sizeof st.filter_history) == 0);
    assert(st.filter_write_pos == before.filter_write_pos);
    assert(st.push_rejected == 1u && st.discarded_samples == 1025u);
}

static double tone_gain(uint32_t rate, double frequency) {
    k1_asrc24_t st;
    int16_t input[296], output[64];
    uint32_t position = 0, rendered = 0, used = 0;
    double energy = 0.0;
    k1_asrc24_reset(&st, 1u, rate);
    while (position < rate) {
        uint32_t count = rate - position;
        if (count > 296u) count = 296u;
        for (uint32_t i = 0; i < count; ++i) {
            input[i] = (int16_t)lrint(12000.0 *
                sin(6.2831853071795864769 * frequency * (position + i) / rate));
        }
        assert(k1_asrc24_push(&st, input, count) == 0);
        position += count;
        while (k1_asrc24_pull180(&st, output) == 0) {
            for (unsigned i = 0; i < 64; ++i, ++rendered) {
                /* Exclude FIR startup and use enough cycles at55Hz. */
                if (rendered >= 1280u) {
                    energy += (double)output[i] * output[i];
                    used++;
                }
            }
        }
    }
    assert(used > 11000u);
    return 20.0 * log10(sqrt(energy / used) / (12000.0 / sqrt(2.0)) + 1e-12);
}

static void filter_quality(void) {
    const uint32_t rates[] = {36000u, 40000u, 44000u};
    const double pass[] = {55.0, 220.0, 1000.0, 2000.0, 4000.0};
    const double stop[] = {6400.0, 6600.0, 7200.0, 9000.0, 11000.0, 14000.0, 17000.0};
    for (unsigned r = 0; r < 3; ++r) {
        for (unsigned i = 0; i < sizeof pass / sizeof pass[0]; ++i) {
            double gain = tone_gain(rates[r], pass[i]);
            printf("passband rate=%u tone=%.0f gain_db=%.5f\n", rates[r], pass[i], gain);
            assert(gain > -0.6 && gain < 0.1);
        }
        for (unsigned i = 0; i < sizeof stop / sizeof stop[0]; ++i) {
            double gain = tone_gain(rates[r], stop[i]);
            printf("stopband rate=%u tone=%.0f gain_db=%.5f\n", rates[r], stop[i], gain);
            assert(gain < -45.0);
        }
    }
}

int main(void) {
    const uint32_t rates[] = {36000u, 40000u, 41440u, 44000u};
    for (unsigned i = 0; i < 4; ++i) conservation(rates[i]);
    filter_quality();
    puts("EMOTISCOPE_CAPTURE_ADAPTER=PASS");
    return 0;
}
