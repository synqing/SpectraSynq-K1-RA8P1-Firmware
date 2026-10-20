#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  K1_AUDIO_HOP_CONTRACT_VERSION = 1,
  K1_AUDIO_HOP_SAMPLES = 180,
  K1_AUDIO_HOP_RATE_HZ = 24000
};

typedef enum {
  K1_AUDIO_TS_DMA_ISR_BACKPROJECTION = 1,
  K1_AUDIO_TS_MEASURED_ACOUSTIC = 2
} k1_audio_timestamp_provenance_t;

typedef enum {
  K1_AUDIO_RATE_CALIBRATING = 1,
  K1_AUDIO_RATE_LOCKED = 2
} k1_audio_rate_state_t;

typedef enum {
  K1_AUDIO_READ_OK = 0,
  K1_AUDIO_READ_NOT_READY = 1,
  K1_AUDIO_READ_DISCONTINUITY = 2,
  K1_AUDIO_READ_FAULT = 3
} k1_audio_read_result_t;

typedef struct k1_audio_hop {
  uint32_t contract_version;
  uint32_t samples;
  uint32_t analysis_rate_hz;
  uint64_t stream_epoch;
  uint64_t raw_capture_epoch;
  uint64_t hop_sequence;
  uint64_t analysis_begin;
  uint64_t analysis_end_exclusive;
  uint32_t source_identity;
  uint32_t programme_lane;
  uint64_t source_first_index;
  uint16_t source_first_frac_q16;
  uint64_t source_last_index;
  uint16_t source_last_frac_q16;
  uint64_t source_next_index;
  uint16_t source_next_frac_q16;
  uint64_t source_support_last_index;
  uint64_t supporting_block_source_end_exclusive;
  uint64_t source_block_first_sequence;
  uint64_t source_block_last_sequence;
  uint32_t source_rate_hz;
  uint32_t rate_segment;
  uint32_t phase_increment_q16;
  uint64_t supporting_dma_receipt_cycles;
  uint64_t supporting_dma_receipt_us;
  uint64_t newest_sample_capture_estimate_us;
  uint64_t descriptor_ready_us;
  uint32_t timestamp_provenance;
  uint8_t capture_estimate_valid;
  uint8_t uncertainty_known;
  uint8_t physical_timestamp_valid;
  uint32_t discontinuity_reason;
  uint32_t rate_state;
  int16_t pcm[K1_AUDIO_HOP_SAMPLES];
} k1_audio_hop_t;

int k1_audio_hop_init_interval(k1_audio_hop_t *hop, uint64_t stream_epoch, uint64_t hop_sequence);
uint64_t k1_audio_hop_media_end_48k(const k1_audio_hop_t *hop);
int k1_audio_hop_accept(uint64_t *last_epoch, uint64_t *last_sequence, const k1_audio_hop_t *hop);

#ifdef __cplusplus
}
#endif
