#pragma once
// Optional secondary160 backend; defaults disabled and requires separate admission.
#include "ws281x_gpt_dma_hw_pair.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t initialised, submitted_generation, completed_generation;
    uint32_t completions, errors, fault, pfs_p603, pfs_p714;
    struct {
        uint32_t fault, dma_complete, hw_stopped, stop_generation, stop_valid;
    } lane[2];
} k1_ws281x_top_status_t;
/* One bounded coherent snapshot; formatting happens after interrupts resume. */
void k1_ws281x_gpt_dma_hw_top_status(k1_ws281x_top_status_t *out);
uint32_t k1_ws281x_gpt_dma_hw_top_registers(uint32_t *out, uint32_t capacity);
const k1_ws281x_pair_reset_witness_t *k1_ws281x_gpt_dma_hw_top_reset_witness(void);
int k1_ws281x_gpt_dma_hw_top_init(void);
int k1_ws281x_gpt_dma_hw_top_submit(const uint8_t *lane_a0, size_t a0_bytes,
                                     const uint8_t *lane_a1, size_t a1_bytes,
                                     uint32_t profile, uint32_t generation);
int k1_ws281x_gpt_dma_hw_top_poll(void);
int k1_ws281x_gpt_dma_hw_top_transmitting(void);
int k1_ws281x_gpt_dma_hw_top_busy(void);
int k1_ws281x_gpt_dma_hw_top_ready(void);
void k1_ws281x_gpt_dma_hw_top_abort_low(void);
uint32_t k1_ws281x_gpt_dma_hw_top_clock_hz(void);
uint32_t k1_ws281x_gpt_dma_hw_top_clock_hz_a1(void);
uint32_t k1_ws281x_gpt_dma_hw_top_completions(void);
uint32_t k1_ws281x_gpt_dma_hw_top_errors(void);
uint32_t k1_ws281x_gpt_dma_hw_top_replacements(void);
uint32_t k1_ws281x_gpt_dma_hw_top_submitted_generation(void);
uint32_t k1_ws281x_gpt_dma_hw_top_completed_generation(void);
uint32_t k1_ws281x_gpt_dma_hw_top_last_fault(void);
uint32_t k1_ws281x_gpt_dma_hw_top_lane_fault(unsigned lane);
uint32_t k1_ws281x_gpt_dma_hw_top_lane_dma_complete(unsigned lane);
uint32_t k1_ws281x_gpt_dma_hw_top_lane_hw_stopped(unsigned lane);
uint32_t k1_ws281x_gpt_dma_hw_top_gtstr(void);
uint32_t k1_ws281x_gpt_dma_hw_top_pfs_a0(void);
uint32_t k1_ws281x_gpt_dma_hw_top_pfs_a1(void);
const uint32_t *k1_ws281x_gpt_dma_hw_top_duty(unsigned lane);
const char *k1_ws281x_gpt_dma_hw_top_backend(void);
const k1_ws281x_pair_first_fault_t *k1_ws281x_gpt_dma_hw_top_first_fault_record(void);
const k1_ws281x_pair_last_stop_t *k1_ws281x_gpt_dma_hw_top_last_stop(unsigned lane);
const k1_ws281x_pair_last_frame_t *k1_ws281x_gpt_dma_hw_top_last_frame(void);
#ifdef __cplusplus
}
#endif
