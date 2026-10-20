#pragma once

#include "ws281x_gpt_dma_pair.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Target two-lane WS2816 transmitter: GPT6/P601 + GPT8/P604.
   pdm-first defaults: LED A DMAC2, LED B DMAC3, GPT0/GPT1 stops, ELC A/B and
   C/D. led-first overrides A to DMAC0. Microphone keeps DMAC0/DMAC1. */

#ifndef K1_WS281X_GPT_DMA_CHANNEL
#define K1_WS281X_GPT_DMA_CHANNEL 2u
#endif
#ifndef K1_WS281X_GPT_DMA_IRQ
#define K1_WS281X_GPT_DMA_IRQ DMAC2_INT_IRQn
#endif
#ifndef K1_WS281X_GPT_DMA_CHANNEL_A1
#define K1_WS281X_GPT_DMA_CHANNEL_A1 3u
#endif
#ifndef K1_WS281X_GPT_DMA_IRQ_A1
#define K1_WS281X_GPT_DMA_IRQ_A1 DMAC3_INT_IRQn
#endif

#define K1_WS281X_GPT_PAIR_START_MASK                                      \
    ((1u << K1_WS281X_GPT_TIMER) | (1u << K1_WS281X_GPT_TIMER_A1))

#define K1_WS281X_PAIR_FAULT_SITE_NONE 0u
#define K1_WS281X_PAIR_FAULT_SITE_DMA_CALLBACK_COUNT_NONZERO 1u
#define K1_WS281X_PAIR_FAULT_SITE_STOP_CALLBACK_COUNT_NONZERO 2u
#define K1_WS281X_PAIR_FAULT_SITE_TIMEOUT 3u

typedef struct {
    uint32_t valid;
    uint32_t site;
    uint32_t lane;
    uint32_t generation;
    uint32_t pair_state;
    uint32_t cyccnt;
    uint32_t dmcra[2];
    uint32_t dmsar[2];
    uint32_t dmdar[2];
    uint32_t dmcnt[2];
    uint32_t dmctl;
    uint32_t gpt8_gtcnt;
    uint32_t gpt8_gtcr;
    uint32_t gpt1_gtcnt;
    uint32_t gpt1_gtpr;
    uint32_t dma_irqs[2];
    uint32_t stop_irqs[2];
} k1_ws281x_pair_first_fault_t;

/* Written on every hardware-stop callback, including successful frames.
   First-fault stays latched; this record is overwritten. Not a pin capture. */
typedef struct {
    uint32_t valid;
    uint32_t generation;
    uint32_t lane;
    uint32_t cyccnt;
    uint32_t pwm_gtcr;
    uint32_t pwm_gtcnt;
    uint32_t pwm_gtccr;
    uint32_t stop_gtcnt;
    uint32_t stop_gtpr;
    uint32_t dmcra;
    uint32_t pwm_running;
    uint32_t pwm_gtber;
    uint32_t pwm_gtccr_e;
    uint32_t elc_gpt_a;
    uint32_t elc_gpt_b;
} k1_ws281x_pair_last_stop_t;

typedef struct {
    uint32_t valid;
    uint32_t generation;
    uint32_t crc_a;
    uint32_t max_ch;
    uint32_t white_pixels;
} k1_ws281x_pair_last_frame_t;

/* Main-loop admission timing measured after both stop callbacks. This is
   a conservative reset-low duration, not a pin capture or per-cell witness.
   Zero admissions means no measurement; cycles use the reported CPU clock. */
typedef struct {
    uint32_t admissions;
    uint32_t min_cycles;
    uint32_t last_cycles;
    uint32_t required_cycles;
    uint32_t stale_clock_rejections;
    uint32_t generation;
    uint32_t cpu_hz;
    uint32_t stop_a, stop_b, admission;
    uint32_t start_samples, start_min_cycles;
    uint32_t previous_generation, next_generation;
    uint32_t start_before, start_after;
} k1_ws281x_pair_reset_witness_t;
/* Read-only fixed v1 word record, available only while pair ownership is idle.
   Opcode17/HWST host decoder owns the exact ordered field names. */
#define K1_WS281X_PAIR_REG_WORDS 128u
uint32_t k1_ws281x_gpt_dma_hw_pair_registers(uint32_t *out, uint32_t capacity);

const k1_ws281x_pair_reset_witness_t *k1_ws281x_gpt_dma_hw_pair_reset_witness(void);

int k1_ws281x_gpt_dma_hw_pair_init(void);
int k1_ws281x_gpt_dma_hw_pair_submit(const uint8_t *lane_a0, size_t a0_bytes,
                                     const uint8_t *lane_a1, size_t a1_bytes,
                                     uint32_t profile, uint32_t generation);
int k1_ws281x_gpt_dma_hw_pair_poll(void);
/* Pure hardware observation; does not poll or start pending work. */
int k1_ws281x_gpt_dma_hw_pair_transmitting(void);
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
const k1_ws281x_pair_first_fault_t *k1_ws281x_gpt_dma_hw_pair_first_fault_record(void);
const k1_ws281x_pair_last_stop_t *k1_ws281x_gpt_dma_hw_pair_last_stop(unsigned lane);
const k1_ws281x_pair_last_frame_t *k1_ws281x_gpt_dma_hw_pair_last_frame(void);

#ifdef __cplusplus
}
#endif
