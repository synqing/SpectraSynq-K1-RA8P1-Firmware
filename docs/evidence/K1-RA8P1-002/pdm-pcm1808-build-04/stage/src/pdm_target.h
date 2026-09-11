#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The pinned Titan BSP filter is currently generated for 16 kHz. Using 120
   samples preserves the production firmware's 7.5 ms capture cadence while
   the separate 12.8 kHz filter-generation gate remains open. */
#define K1_PDM_TARGET_SAMPLE_RATE_HZ 16000u
#define K1_PDM_TARGET_SLOT_ELEMENTS 120u
#define K1_PDM_TARGET_SLOT_DURATION_US 7500u
#define K1_PDM_TARGET_LANE_COUNT 2u

/* Working S3 hardware contract carried onto Titan:
   - IM1: SELECT HIGH, rising-edge PDM, programme/AP source.
   - IM2: SELECT LOW, falling-edge PDM, measurement only.
   Both microphones share the same CLK and DATA wires. */
#define K1_PDM_TARGET_PROGRAMME_LANE 0u
#define K1_PDM_TARGET_MEASUREMENT_LANE 1u
#define K1_PDM_TARGET_RISE_CHANNEL 2u
#define K1_PDM_TARGET_FALL_CHANNEL 0u
#define K1_PDM_TARGET_RISE_DMA_CHANNEL 0u
#define K1_PDM_TARGET_FALL_DMA_CHANNEL 1u

int k1_pdm_target_initialise(void);
void k1_pdm_target_poll(void);

int k1_pdm_target_initialised(void);
int k1_pdm_target_running(void);
int32_t k1_pdm_target_last_fsp_error(void);
uint32_t k1_pdm_target_data_callbacks(uint32_t lane);
uint32_t k1_pdm_target_error_callbacks(uint32_t lane);
uint32_t k1_pdm_target_error_flags(uint32_t lane);
uint32_t k1_pdm_target_processed_slots(uint32_t lane);
uint32_t k1_pdm_target_processed_samples(uint32_t lane);
uint32_t k1_pdm_target_sample_hash(uint32_t lane);
int32_t k1_pdm_target_sample_min(uint32_t lane);
int32_t k1_pdm_target_sample_max(uint32_t lane);
uint32_t k1_pdm_target_sample_peak(uint32_t lane);
uint64_t k1_pdm_target_sample_square_sum(uint32_t lane);
uint32_t k1_pdm_target_overflow_events(uint32_t lane);
uint32_t k1_pdm_target_drop_events(uint32_t lane);
uint32_t k1_pdm_target_recovery_count(uint32_t lane);
uint64_t k1_pdm_target_first_capture_start_us(void);
uint64_t k1_pdm_target_last_capture_end_us(void);
uint32_t k1_pdm_target_paired_slots(void);
uint32_t k1_pdm_target_pair_skew_drops(void);
uint32_t k1_pdm_target_startup_discard_pairs(void);
uint64_t k1_pdm_target_max_pair_skew_us(void);

#ifdef __cplusplus
}
#endif
