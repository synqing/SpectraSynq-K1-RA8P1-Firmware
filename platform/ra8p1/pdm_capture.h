#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Bounded PDM completion and conversion/submission units.
   Host-safe: no FSP/RT-Thread. Hardware R_PDM_* is not called here.
   First DATA/interval is never treated as full-buffer completion.
   Submission bytes equal conversion bytes (full-duration). Stop tail is
   not invented. */

#define K1_PDM_CAPTURE_ADAPTER_ID "k1-ra8p1-pdm-capture-v1"
#define K1_PDM_MAX_REQUESTED_FRAMES 16000u

int k1_pdm_configure(uint32_t requested_frames,
                     uint32_t capture_channels,
                     uint32_t capture_element_bytes,
                     uint32_t output_channels,
                     uint32_t output_element_bytes,
                     uint32_t callback_interval);
void k1_pdm_start(void);
int k1_pdm_on_data(uint32_t interval_elements);
void k1_pdm_request_stop(void);
int k1_pdm_on_stopped(uint32_t driver_count);
int k1_pdm_convert(const int32_t *capture, size_t capture_count,
                   int16_t *output, size_t output_count);

uint32_t k1_pdm_requested_frames(void);
uint32_t k1_pdm_capture_channels(void);
uint32_t k1_pdm_capture_element_bytes(void);
uint32_t k1_pdm_output_channels(void);
uint32_t k1_pdm_output_element_bytes(void);
uint32_t k1_pdm_callback_interval(void);
uint32_t k1_pdm_capture_bytes(void);
uint32_t k1_pdm_conversion_bytes(void);
uint32_t k1_pdm_submission_bytes(void);
uint32_t k1_pdm_received_elements(void);
uint32_t k1_pdm_final_stopped_count(void);
int k1_pdm_configured(void);
int k1_pdm_running(void);
int k1_pdm_first_data_seen(void);
int k1_pdm_first_data_does_not_prove_complete(void);
int k1_pdm_full_buffer_complete(void);
int k1_pdm_stop_requested(void);
int k1_pdm_stopped(void);
int k1_pdm_stop_tail_known(void);
int k1_pdm_ownership_transferred(void);

#ifdef __cplusplus
}
#endif
