#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define K1_P601_LOOPBACK_SAMPLES 40960u
#define K1_P601_LOOPBACK_PIN_BIT 1u /* P601 is PORT6 pin 1 */

typedef struct {
    uint32_t generation;
    uint32_t payload_fnv1a;
    uint32_t truncated;
    uint32_t period_counts;
    uint32_t clock_hz;
    uint32_t valid;
    uint32_t armed;
    uint32_t samples;
    uint32_t expected_cells;
    uint32_t rising;
    uint32_t falling;
    uint32_t first_rise;
    uint32_t last_fall;
    uint32_t extra_rising;
    uint32_t high_after_last;
    uint32_t sample_period_counts;
    uint32_t open_ok;
    uint32_t start_level;
    uint32_t min_high;
    uint32_t max_high;
    uint32_t peek_head;
    uint32_t peek_tail;
} k1_p601_loopback_result_t;

#if defined(K1_P601_LOOPBACK) && defined(K1_RA8P1_TARGET) && !defined(K1_WS281X_HW_TEST)
void k1_p601_loopback_init(uint32_t gpt_clock_hz);
int k1_p601_loopback_request(void);
uint32_t k1_p601_loopback_read(uint32_t offset, uint8_t *out, uint32_t capacity);
void k1_p601_loopback_arm(uint32_t expected_cells, uint32_t pwm_period_counts,
                          uint32_t generation, const uint8_t *payload);
void k1_p601_loopback_stop_and_score(void);
const k1_p601_loopback_result_t *k1_p601_loopback_result(void);
const uint16_t *k1_p601_loopback_samples(void);
#else
static k1_p601_loopback_result_t k1_p601_loopback_stub;
static inline void k1_p601_loopback_init(uint32_t gpt_clock_hz)
{
    (void)gpt_clock_hz;
    k1_p601_loopback_stub.valid = 0u;
}
static inline void k1_p601_loopback_arm(uint32_t expected_cells,
                                        uint32_t pwm_period_counts, uint32_t generation, const uint8_t *payload)
{
    (void)generation;
    (void)payload;
    (void)expected_cells;
    (void)pwm_period_counts;
}
static inline void k1_p601_loopback_stop_and_score(void) {}
static inline const k1_p601_loopback_result_t *k1_p601_loopback_result(void)
{
    return &k1_p601_loopback_stub;
}
static inline const uint16_t *k1_p601_loopback_samples(void) { return 0; }
#endif

#ifdef __cplusplus
}
#endif
