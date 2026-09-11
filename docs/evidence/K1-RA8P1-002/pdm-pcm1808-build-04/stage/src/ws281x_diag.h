#pragma once
#include <stddef.h>
#include <stdint.h>

#define K1_WS281X_DIAG_OPCODE 13u
#define K1_WS281X_FRAME_OPCODE 14u
#define K1_WS281X_DIAG_MAX_PIXELS 128u
#define K1_WS281X_DIAG_MAX_BYTES (K1_WS281X_DIAG_MAX_PIXELS * 6u)

#ifdef __cplusplus
extern "C" {
#endif

/* Diagnostic only: no render scheduling, scaling, dithering or DMA claim.
 * Profile 1/2 nominal timings: FastLED 3.10.4 adedfc40, led_timing.h.
 * Profile 3: WS2816C-1313-4P V1.1 p4; not universal WS281x timing. */
typedef struct {
    uint32_t version, profile, pin, pixels, lit_pixels, red, green, blue;
} k1_ws281x_diag_request_t;

typedef struct {
    uint32_t t0h_ns, t1h_ns, period_ns, reset_us, bytes_per_pixel;
} k1_ws281x_diag_timing_t;

typedef struct {
    uint32_t pfs_before, pfs_after, emit_cycles, latch_cycles;
    uint32_t bit_period_min_cycles, bit_period_max_cycles;
    int32_t pin_config_error;
} k1_ws281x_diag_result_t;

static inline int k1_ws281x_diag_profile(uint32_t id, k1_ws281x_diag_timing_t *out) {
    if (!out) return 0;
    if (id == 1u) {
        const k1_ws281x_diag_timing_t value = {250u, 875u, 1250u, 300u, 3u};
        *out = value;
    } else if (id == 2u) {
        const k1_ws281x_diag_timing_t value = {225u, 580u, 1225u, 300u, 3u};
        *out = value;
    } else if (id == 3u) {
        const k1_ws281x_diag_timing_t value = {250u, 650u, 1250u, 300u, 6u};
        *out = value;
    } else return 0;
    return 1;
}

/* Returns zero before touching output for any invalid request. */
static inline size_t k1_ws281x_diag_pack(const k1_ws281x_diag_request_t *request,
                                       uint8_t *output, size_t capacity,
                                       k1_ws281x_diag_timing_t *timing) {
    if (!request || !output || !timing || request->version != 1u ||
        request->pin > 1u || request->pixels == 0u ||
        request->pixels > K1_WS281X_DIAG_MAX_PIXELS ||
        request->lit_pixels > request->pixels ||
        !k1_ws281x_diag_profile(request->profile, timing)) return 0;
    const uint32_t maximum = timing->bytes_per_pixel == 3u ? 255u : 65535u;
    if (request->red > maximum || request->green > maximum || request->blue > maximum)
        return 0;
    const size_t size = request->pixels * timing->bytes_per_pixel;
    if (capacity < size) return 0;
    size_t offset = 0;
    for (uint32_t pixel = 0; pixel < request->pixels; ++pixel) {
        const uint32_t channels[3] = {request->green, request->red, request->blue};
        for (unsigned channel = 0; channel < 3u; ++channel) {
            const uint32_t value = pixel < request->lit_pixels ? channels[channel] : 0u;
            if (timing->bytes_per_pixel == 6u) output[offset++] = (uint8_t)(value >> 8);
            output[offset++] = (uint8_t)value;
        }
    }
    return size;
}

/* Returns 0 only after a checked pin configuration and low reset interval.
 * The returned cycle markers are software observations, not GPIO capture. */
int k1_ws281x_diag_emit(const uint8_t *bytes, size_t size, uint32_t profile,
                       uint32_t pin, uint32_t clock_hz,
                       k1_ws281x_diag_result_t *result);

#ifdef __cplusplus
}
#endif
