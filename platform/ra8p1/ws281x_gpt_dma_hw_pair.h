#pragma once

#include "ws281x_gpt_dma_pair.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Target two-lane WS2816 transmitter: GPT6/P601 + GPT7/P603, DMAC0 + DMAC3,
   GPT0 + GPT1 independent hardware stops, ELC A/B and C/D. One GTSTR write
   starts both PWM channels. Pair success uses the host-tested complete
   predicate. Physical pulse claims stay open until a scored DIN capture. */

#ifndef K1_WS281X_GPT_DMA_CHANNEL
#define K1_WS281X_GPT_DMA_CHANNEL 0u
#endif
#ifndef K1_WS281X_GPT_DMA_IRQ
#define K1_WS281X_GPT_DMA_IRQ DMAC0_INT_IRQn
#endif
#ifndef K1_WS281X_GPT_DMA_CHANNEL_A1
#define K1_WS281X_GPT_DMA_CHANNEL_A1 3u
#endif
#ifndef K1_WS281X_GPT_DMA_IRQ_A1
#define K1_WS281X_GPT_DMA_IRQ_A1 DMAC3_INT_IRQn
#endif

#define K1_WS281X_GPT_PAIR_START_MASK                                      \
    ((1u << K1_WS281X_GPT_TIMER) | (1u << K1_WS281X_GPT_TIMER_A1))

int k1_ws281x_gpt_dma_hw_pair_init(void);
int k1_ws281x_gpt_dma_hw_pair_submit(const uint8_t *lane_a0, size_t a0_bytes,
                                     const uint8_t *lane_a1, size_t a1_bytes,
                                     uint32_t profile, uint32_t generation);
int k1_ws281x_gpt_dma_hw_pair_poll(void);
int k1_ws281x_gpt_dma_hw_pair_busy(void);
int k1_ws281x_gpt_dma_hw_pair_ready(void);
void k1_ws281x_gpt_dma_hw_pair_abort_low(void);
uint32_t k1_ws281x_gpt_dma_hw_pair_clock_hz(void);
uint32_t k1_ws281x_gpt_dma_hw_pair_clock_hz_a1(void);
uint32_t k1_ws281x_gpt_dma_hw_pair_completions(void);
uint32_t k1_ws281x_gpt_dma_hw_pair_errors(void);
uint32_t k1_ws281x_gpt_dma_hw_pair_replacements(void);
uint32_t k1_ws281x_gpt_dma_hw_pair_submitted_generation(void);
uint32_t k1_ws281x_gpt_dma_hw_pair_completed_generation(void);
uint32_t k1_ws281x_gpt_dma_hw_pair_last_fault(void);
uint32_t k1_ws281x_gpt_dma_hw_pair_lane_fault(unsigned lane);
uint32_t k1_ws281x_gpt_dma_hw_pair_lane_dma_complete(unsigned lane);
uint32_t k1_ws281x_gpt_dma_hw_pair_lane_hw_stopped(unsigned lane);
uint32_t k1_ws281x_gpt_dma_hw_pair_gtstr(void);
uint32_t k1_ws281x_gpt_dma_hw_pair_pfs_a0(void);
uint32_t k1_ws281x_gpt_dma_hw_pair_pfs_a1(void);
const uint32_t *k1_ws281x_gpt_dma_hw_pair_duty(unsigned lane);
const char *k1_ws281x_gpt_dma_hw_pair_backend(void);

#ifdef __cplusplus
}
#endif
