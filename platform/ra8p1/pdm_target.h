#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 40 kHz capture + measured-rate ASRC into unchanged 24 kHz/180 AP.
   16 kHz diagnostic remains a historical image; do not reflash it to
   repeat that pass. BSP sincdec 124 was 16 kHz at 2 MHz; 49 is 40 kHz. */
#define K1_PDM_TARGET_SAMPLE_RATE_HZ 40000u
/* Must be a multiple of 8: FSP R_PDM_Start rejects other lengths
   (interrupt threshold 8). 300 failed on silicon as FSP_ERR_INVALID_SIZE. */
#define K1_PDM_TARGET_SLOT_ELEMENTS 296u
#if (K1_PDM_TARGET_SLOT_ELEMENTS % 8u) != 0u
#error "PDM slot length must be a multiple of 8 (FSP interrupt threshold)"
#endif
#define K1_PDM_TARGET_SLOT_DURATION_US 7400u
#define K1_PDM_TARGET_SINCDEC_40K 49u
/* Table 50.7 has no M=50 row. 4th-order neighbours: M=62 SINCRNG 9, M=42
   SINCRNG 11. 50^4 ≈ 2^22.6 → {S,[22:4]} = 10. BSP 5 is the M=125 window. */
#define K1_PDM_TARGET_SINCRNG_40K 10u
#if (K1_PDM_TARGET_SINCDEC_40K == 49u) && (K1_PDM_TARGET_SINCRNG_40K == 5u)
#error "SINCRNG 5 is Table 50.7 M=125; M=50 cannot keep that window"
#endif
#define K1_PDM_TARGET_PROFILE "ap_40k_asrc24"
#define K1_PDM_TARGET_LANE_COUNT 2u

/* Titan Mini HW V1.0 onboard LMD2718T261-OA1 (U13/U14), shared P502/P812.
   Expected: U14 LOW sampled RISE/ch2 (programme); U13 HIGH sampled FALL/ch0
   (measurement). Acoustic identity is still required. */
#define K1_PDM_TARGET_PROGRAMME_LANE 0u
#define K1_PDM_TARGET_MEASUREMENT_LANE 1u
#define K1_PDM_TARGET_RISE_CHANNEL 2u
#define K1_PDM_TARGET_FALL_CHANNEL 0u
#ifndef K1_PDM_TARGET_RISE_DMA_CHANNEL
#define K1_PDM_TARGET_RISE_DMA_CHANNEL 0u
#endif
#ifndef K1_PDM_TARGET_FALL_DMA_CHANNEL
#define K1_PDM_TARGET_FALL_DMA_CHANNEL 1u
#endif
#ifndef K1_PDM_TARGET_RISE_DMA_IRQ
#define K1_PDM_TARGET_RISE_DMA_IRQ DMAC0_INT_IRQn
#endif
#ifndef K1_PDM_TARGET_FALL_DMA_IRQ
#define K1_PDM_TARGET_FALL_DMA_IRQ DMAC1_INT_IRQn
#endif

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
uint32_t k1_pdm_target_rearm_denied(void);
uint32_t k1_pdm_target_sat_neg(uint32_t lane);
uint32_t k1_pdm_target_sat_pos(uint32_t lane);
uint32_t k1_pdm_target_packing_mismatch(uint32_t lane);
uint32_t k1_pdm_target_first_sat_raw(uint32_t lane);
uint32_t k1_pdm_target_asrc_starved(void);
uint32_t k1_pdm_target_ap_hops(void);
uint32_t k1_pdm_target_last_hop_dt_us(void);
uint32_t k1_pdm_target_last_hop_peak(void);
uint32_t k1_pdm_target_last_hop_gain_q8(void);
uint32_t k1_pdm_target_gain_clip_pos(void);
uint32_t k1_pdm_target_gain_clip_neg(void);
uint32_t k1_pdm_target_measured_hz(void);
uint32_t k1_pdm_target_rate_locked(void);
int k1_pdm_target_pull_ap_hop(int16_t out180[180]);
uint32_t k1_pdm_target_spare_slots(void);

#ifdef __cplusplus
}
#endif
