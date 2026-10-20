#include "k1/core/audio/k1_audio_hop.h"

#include <stddef.h>
#include <string.h>

int k1_audio_hop_init_interval(k1_audio_hop_t *hop, uint64_t stream_epoch, uint64_t hop_sequence) {
  if (hop == NULL || hop_sequence == 0) {
    return -1;
  }
  memset(hop, 0, sizeof(*hop));
  hop->contract_version = K1_AUDIO_HOP_CONTRACT_VERSION;
  hop->samples = K1_AUDIO_HOP_SAMPLES;
  hop->analysis_rate_hz = K1_AUDIO_HOP_RATE_HZ;
  hop->stream_epoch = stream_epoch;
  hop->hop_sequence = hop_sequence;
  hop->analysis_begin = (hop_sequence - 1U) * (uint64_t)K1_AUDIO_HOP_SAMPLES;
  hop->analysis_end_exclusive = hop->analysis_begin + (uint64_t)K1_AUDIO_HOP_SAMPLES;
  hop->timestamp_provenance = K1_AUDIO_TS_DMA_ISR_BACKPROJECTION;
  hop->physical_timestamp_valid = 0;
  hop->rate_state = K1_AUDIO_RATE_CALIBRATING;
  return 0;
}

uint64_t k1_audio_hop_media_end_48k(const k1_audio_hop_t *hop) {
  if (hop == NULL) {
    return 0;
  }
  return hop->analysis_end_exclusive * 2U;
}

int k1_audio_hop_accept(uint64_t *last_epoch, uint64_t *last_sequence, const k1_audio_hop_t *hop) {
  if (last_epoch == NULL || last_sequence == NULL || hop == NULL) {
    return -1;
  }
  if (hop->contract_version != K1_AUDIO_HOP_CONTRACT_VERSION ||
      hop->samples != K1_AUDIO_HOP_SAMPLES ||
      hop->analysis_rate_hz != K1_AUDIO_HOP_RATE_HZ ||
      hop->hop_sequence == 0) {
    return -1;
  }
  if (hop->analysis_end_exclusive != hop->analysis_begin + (uint64_t)K1_AUDIO_HOP_SAMPLES) {
    return -1;
  }
  if (*last_epoch == hop->stream_epoch && *last_sequence == hop->hop_sequence) {
    return -2; /* duplicate */
  }
  if (*last_epoch == hop->stream_epoch && hop->hop_sequence != *last_sequence + 1U && *last_sequence != 0) {
    return -3; /* stale or skipped */
  }
  if (*last_epoch != 0 && hop->stream_epoch != *last_epoch && hop->hop_sequence != 1U) {
    return -3;
  }
  *last_epoch = hop->stream_epoch;
  *last_sequence = hop->hop_sequence;
  return 0;
}
