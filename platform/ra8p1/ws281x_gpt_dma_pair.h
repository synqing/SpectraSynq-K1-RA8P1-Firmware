#pragma once

#include "ws281x_gpt_dma.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Host pair model for one logical channel_A on two GPT lanes.
   Target hardware is not this file. P603 remains an unconfirmed wire route.
   Pair success requires both DMA complete, both hardware stops, and both
   reset intervals. DMA-complete alone never credits the pair. */

#define K1_WS281X_PAIR_LANE_BYTES 480u
#define K1_WS281X_PAIR_PROFILE_WS2816C 3u

enum {
    K1_WS281X_PAIR_IDLE = 0,
    K1_WS281X_PAIR_PRELOAD = 1,
    K1_WS281X_PAIR_EMITTING = 2,
    K1_WS281X_PAIR_READY = 3,
    K1_WS281X_PAIR_FAULT = 4
};

typedef struct {
    k1_ws281x_gpt_dma_t lane_a0;
    k1_ws281x_gpt_dma_t lane_a1;
    uint32_t generation;
    uint32_t submitted_generation;
    uint32_t completed_generation;
    uint32_t pending_replacements;
    uint32_t pair_state;
    uint32_t pending_valid;
    uint32_t pending_generation;
    uint32_t pending_profile;
    uint32_t pending_clock_hz;
    uint8_t pending_a0[K1_WS281X_PAIR_LANE_BYTES];
    uint8_t pending_a1[K1_WS281X_PAIR_LANE_BYTES];
} k1_ws281x_gpt_dma_pair_t;

void k1_ws281x_gpt_dma_pair_reset(k1_ws281x_gpt_dma_pair_t *pair);
int k1_ws281x_gpt_dma_pair_validate(const uint8_t *lane_a0, size_t a0_bytes,
                                    const uint8_t *lane_a1, size_t a1_bytes,
                                    uint32_t profile, uint32_t clock_hz);
int k1_ws281x_gpt_dma_pair_lanes_complete(uint32_t dma_a0, uint32_t dma_a1,
                                          uint32_t stop_a0, uint32_t stop_a1,
                                          uint32_t wave_a0, uint32_t wave_a1,
                                          uint32_t reset_a0, uint32_t reset_a1,
                                          uint32_t fault_a0, uint32_t fault_a1,
                                          uint32_t pair_state);
int k1_ws281x_gpt_dma_pair_submit(k1_ws281x_gpt_dma_pair_t *pair,
                                  const uint8_t *lane_a0, size_t a0_bytes,
                                  const uint8_t *lane_a1, size_t a1_bytes,
                                  uint32_t profile, uint32_t generation,
                                  uint32_t clock_hz);
int k1_ws281x_gpt_dma_pair_submit_test_bits(k1_ws281x_gpt_dma_pair_t *pair,
                                            const uint32_t *duty_a0,
                                            const uint32_t *duty_a1,
                                            uint32_t bits, uint32_t generation,
                                            uint32_t clock_hz,
                                            uint32_t period_ns,
                                            uint32_t reset_us);
int k1_ws281x_gpt_dma_pair_start(k1_ws281x_gpt_dma_pair_t *pair);
int k1_ws281x_gpt_dma_pair_poll(const k1_ws281x_gpt_dma_pair_t *pair);
int k1_ws281x_gpt_dma_pair_step_overflow(k1_ws281x_gpt_dma_pair_t *pair);
void k1_ws281x_gpt_dma_pair_abort(k1_ws281x_gpt_dma_pair_t *pair);
int k1_ws281x_gpt_dma_pair_complete(const k1_ws281x_gpt_dma_pair_t *pair);

#ifdef __cplusplus
}
#endif
