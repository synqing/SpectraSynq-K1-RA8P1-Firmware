#pragma once

#include "ws281x_gpt_dma.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Target GPT6 PWM + DMAC + GPT0 event-count transmitter on P601/GTIOC6A.
   Host tests execute this implementation with register/FSP substitutes. Clock comes from
   R_GPT_InfoGet, never from CPU frequency or a 100 MHz guess.
   P004 has no GPT function; this driver never claims DIN-B. */

#ifndef K1_WS281X_GPT_DMA_CHANNEL
#define K1_WS281X_GPT_DMA_CHANNEL 2u
#endif
#ifndef K1_WS281X_GPT_DMA_IRQ
#define K1_WS281X_GPT_DMA_IRQ DMAC2_INT_IRQn
#endif

/* Versioned binary diagnostic opcode 22; first transaction snapshots survive faults.
   Snapshot tags: 1 armed, 2 started, 3 DMA IRQ, 4 GPT stop IRQ, 5 first fault.
   All fields are uint32_t, little endian on target. No register reads by host. */
#define K1_WS281X_GPT_DIAG_OPCODE 22u
#define K1_WS281X_GPT_DIAG_VERSION 1u
typedef struct {
    uint32_t tag;
    uint32_t attempt;
    uint32_t cycles;
    uint32_t state;
    uint32_t fault;
    uint32_t gpt6_gtcr;
    uint32_t gpt6_gtcnt;
    uint32_t gpt6_gtpr;
    uint32_t gpt6_gtccra;
    uint32_t gpt6_gtccrc;
    uint32_t gpt6_gtber;
    uint32_t gpt6_gtior;
    uint32_t gpt6_gtpsr;
    uint32_t gpt0_gtcr;
    uint32_t gpt0_gtcnt;
    uint32_t gpt0_gtpr;
    uint32_t gpt0_gtupsr;
    uint32_t dmac_count;
    uint32_t dmac_src;
    uint32_t dmac_dest;
    uint32_t dmac_enable;
    uint32_t dmac_mode;
    uint32_t dmac_irq;
    uint32_t dmac_request;
    uint32_t delink;
    uint32_t elc_a;
    uint32_t elc_b;
    uint32_t elc_enable;
    uint32_t mstpcrc;
    uint32_t gpt_irq_owner;
    uint32_t dma_irq_owner;
    uint32_t gpt_irq_pending;
    uint32_t dma_irq_pending;
    uint32_t pfs;
} k1_ws281x_hw_snapshot_t;
typedef struct {
    uint32_t version, bytes, attempts, dma_irqs, stop_irqs, frames;
    uint32_t errors, first_fault, state, owned, clock_hz, bits;
    k1_ws281x_hw_snapshot_t snapshots[5];
} k1_ws281x_hw_diag_t;
void k1_ws281x_gpt_dma_hw_snapshot(k1_ws281x_hw_diag_t *out);
uint32_t k1_ws281x_gpt_dma_hw_dma_irqs(void);

/* Opcode 22 with a little-endian uint32 request value of 2. Empty requests
   retain the exact 728-byte v1 layout. The v2 header uses version=2 and its
   full byte count. FNV-1a hashes cover packed bytes and little-endian duty
   words, not electrical output. valid is published last at the first fault.
   Payload preparation happens before transmission, never in the fault ISR. */
#define K1_WS281X_GPT_DIAG_V2_VERSION 2u
#define K1_WS281X_GPT_WITNESS_PAYLOAD_CAP (K1_WS281X_GPT_DUTY_CAP / 8u)
typedef struct {
    uint32_t valid, frame_id, profile, payload_bytes;
    uint32_t payload_fnv1a32, duty_fnv1a32, expected_dma_words, dma_source_start;
    uint32_t start_requested, elapsed_cycles, cpu_hz, dma_complete;
    uint32_t waveform_complete, reset_ready, dmsts, dmctl;
    k1_ws281x_hw_snapshot_t terminal;
    uint8_t packed_grb[K1_WS281X_GPT_WITNESS_PAYLOAD_CAP];
} k1_ws281x_first_fault_witness_t;
typedef struct {
    k1_ws281x_hw_diag_t header;
    uint32_t dmctl, last_prepare_cycles, maximum_prepare_cycles;
    k1_ws281x_first_fault_witness_t first_fault;
} k1_ws281x_hw_diag_v2_t;
void k1_ws281x_gpt_dma_hw_snapshot_v2(k1_ws281x_hw_diag_v2_t *out);

int k1_ws281x_gpt_dma_hw_init(void);
int k1_ws281x_gpt_dma_hw_submit(const uint8_t *bytes, size_t nbytes,
                                uint32_t profile);
int k1_ws281x_gpt_dma_hw_poll(void);
int k1_ws281x_gpt_dma_hw_busy(void);
int k1_ws281x_gpt_dma_hw_ready(void);
void k1_ws281x_gpt_dma_hw_abort_low(void);
uint32_t k1_ws281x_gpt_dma_hw_clock_hz(void);
uint32_t k1_ws281x_gpt_dma_hw_completions(void);
uint32_t k1_ws281x_gpt_dma_hw_errors(void);
uint32_t k1_ws281x_gpt_dma_hw_ownership_violations(void);
uint32_t k1_ws281x_gpt_dma_hw_last_fault(void);
uint32_t k1_ws281x_gpt_dma_hw_last_bits(void);
uint32_t k1_ws281x_gpt_dma_hw_pfs(void);
const char *k1_ws281x_gpt_dma_hw_backend(void);

#ifdef __cplusplus
}
#endif
