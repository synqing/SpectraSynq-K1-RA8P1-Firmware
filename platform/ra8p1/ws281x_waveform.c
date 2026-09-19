#include "ws281x_waveform.h"

#include <string.h>

int k1_ws281x_pack_grb24(uint8_t r, uint8_t g, uint8_t b, uint8_t *out)
{
    if (out == 0) {
        return 0;
    }
    out[0] = g;
    out[1] = r;
    out[2] = b;
    return 1;
}

uint32_t k1_ws281x_profile_max_pixels(uint32_t profile)
{
    k1_ws281x_diag_timing_t timing;
    if (!k1_ws281x_diag_profile(profile, &timing)) {
        return 0u;
    }
    return timing.bytes_per_pixel == 3u ? K1_WS281X_WAVE_MAX_PIXELS_WS2812
                                        : K1_WS281X_WAVE_MAX_PIXELS_WS2816;
}

uint32_t k1_ws281x_ns_to_counts_ceil(uint32_t clock_hz, uint32_t ns)
{
    uint64_t num;
    if (clock_hz == 0u || ns == 0u) {
        return 0u;
    }
    num = (uint64_t)clock_hz * (uint64_t)ns + 999999999ull;
    return (uint32_t)(num / 1000000000ull);
}

int k1_ws281x_gpt_quantise(uint32_t profile, uint32_t clock_hz,
                           k1_ws281x_duty_meta_t *meta)
{
    k1_ws281x_diag_timing_t timing;
    uint32_t period;
    uint32_t t0h;
    uint32_t t1h;
    if (meta == 0 || !k1_ws281x_diag_profile(profile, &timing) ||
        clock_hz == 0u) {
        return 0;
    }
    period = k1_ws281x_ns_to_counts_ceil(clock_hz, timing.period_ns);
    t0h = k1_ws281x_ns_to_counts_ceil(clock_hz, timing.t0h_ns);
    t1h = k1_ws281x_ns_to_counts_ceil(clock_hz, timing.t1h_ns);
    if (period < 2u || t0h == 0u || t1h == 0u || t0h >= period ||
        t1h >= period || t1h <= t0h) {
        return 0;
    }
    memset(meta, 0, sizeof(*meta));
    meta->clock_hz = clock_hz;
    meta->period_counts = period;
    meta->t0h_counts = t0h;
    meta->t1h_counts = t1h;
    meta->period_reg = period - 1u;
    return 1;
}

int k1_ws281x_build_gpt_duty(const uint8_t *bytes, size_t nbytes,
                             uint32_t profile, uint32_t clock_hz,
                             uint32_t *duty, size_t capacity,
                             k1_ws281x_duty_meta_t *meta)
{
    k1_ws281x_diag_timing_t timing;
    size_t i;
    unsigned bit;
    size_t out = 0;
    if (bytes == 0 || duty == 0 || meta == 0 || nbytes == 0u) {
        return 0;
    }
    if (!k1_ws281x_diag_profile(profile, &timing)) {
        return 0;
    }
    if ((nbytes % timing.bytes_per_pixel) != 0u) {
        return 0;
    }
    if ((nbytes / timing.bytes_per_pixel) >
        k1_ws281x_profile_max_pixels(profile)) {
        return 0;
    }
    if (!k1_ws281x_gpt_quantise(profile, clock_hz, meta)) {
        return 0;
    }
    if (nbytes * 8u > capacity) {
        return 0;
    }
    for (i = 0; i < nbytes; ++i) {
        uint8_t value = bytes[i];
        for (bit = 0; bit < 8u; ++bit) {
            uint32_t high =
                ((value & (uint8_t)(0x80u >> bit)) != 0u) ? meta->t1h_counts
                                                          : meta->t0h_counts;
            duty[out++] = high - 1u;
        }
    }
    meta->bits = (uint32_t)out;
    return 1;
}

int k1_ws281x_gpio_podr_cost_lanes(uint32_t lanes, uint32_t pixels,
                                   uint32_t profile, uint32_t slot_ns,
                                   k1_ws281x_cost_t *cost)
{
    k1_ws281x_diag_timing_t timing;
    uint32_t bits;
    uint32_t slots_per_bit;
    uint32_t reset_slots;
    uint32_t data_slots;
    if (cost == 0 || lanes == 0u || pixels == 0u || slot_ns == 0u ||
        !k1_ws281x_diag_profile(profile, &timing)) {
        return 0;
    }
    if ((timing.period_ns % slot_ns) != 0u) {
        return 0;
    }
    bits = pixels * timing.bytes_per_pixel * 8u;
    slots_per_bit = timing.period_ns / slot_ns;
    reset_slots = (timing.reset_us * 1000u) / slot_ns;
    data_slots = bits * slots_per_bit;
    memset(cost, 0, sizeof(*cost));
    cost->lanes = lanes;
    cost->pixels = pixels;
    cost->bits = bits;
    cost->slots = data_slots + reset_slots;
    cost->data_bytes = data_slots * 2u;
    cost->reset_bytes = reset_slots * 2u;
    cost->bytes = cost->data_bytes + cost->reset_bytes;
    cost->payload_bytes_per_s = (uint32_t)(2000000000ull / slot_ns);
    cost->data_ns = data_slots * slot_ns;
    cost->reset_ns = reset_slots * slot_ns;
    cost->emit_ns = cost->data_ns + cost->reset_ns;
    return 1;
}

int k1_ws281x_gpio_podr_cost(uint32_t pixels, uint32_t profile,
                             uint32_t slot_ns, k1_ws281x_cost_t *cost)
{
    return k1_ws281x_gpio_podr_cost_lanes(1u, pixels, profile, slot_ns, cost);
}

int k1_ws281x_gpt_cost_lanes(uint32_t lanes, uint32_t pixels, uint32_t profile,
                             k1_ws281x_cost_t *cost)
{
    k1_ws281x_diag_timing_t timing;
    uint32_t bits;
    if (cost == 0 || lanes == 0u || pixels == 0u ||
        !k1_ws281x_diag_profile(profile, &timing)) {
        return 0;
    }
    bits = pixels * timing.bytes_per_pixel * 8u;
    memset(cost, 0, sizeof(*cost));
    cost->lanes = lanes;
    cost->pixels = pixels;
    cost->bits = bits;
    cost->slots = bits;
    cost->data_bytes = bits * 4u;
    cost->reset_bytes = 0u;
    cost->bytes = cost->data_bytes;
    cost->payload_bytes_per_s =
        (uint32_t)((4000000000ull) / (uint64_t)timing.period_ns);
    cost->data_ns = bits * timing.period_ns;
    cost->reset_ns = timing.reset_us * 1000u;
    cost->emit_ns = cost->data_ns + cost->reset_ns;
    return 1;
}

int k1_ws281x_gpt_cost(uint32_t pixels, uint32_t profile,
                       k1_ws281x_cost_t *cost)
{
    return k1_ws281x_gpt_cost_lanes(1u, pixels, profile, cost);
}

uint32_t k1_ws281x_gpt_buffer_bytes(uint32_t lanes, uint32_t pixels,
                                    uint32_t profile, uint32_t double_buffered)
{
    k1_ws281x_cost_t cost;
    uint32_t copies;
    if (!k1_ws281x_gpt_cost_lanes(lanes, pixels, profile, &cost)) {
        return 0u;
    }
    copies = double_buffered > 1u ? double_buffered : 1u;
    return cost.data_bytes * lanes * copies;
}
