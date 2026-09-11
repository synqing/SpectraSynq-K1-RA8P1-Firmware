#pragma once

#include "titan_ram_diag.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact 12.8 kHz / 96-sample stereo hop. SSIE dataset-tool ideas (WAV,
   peak/RMS/clip/zero) without DC removal, scaling, or 16 kHz relabel.
   PCM bytes are the authority; statistics are derived. */

#define K1_EXACT_MAGIC 0x3053314Bu /* K1S0 little-endian */
#define K1_EXACT_VERSION 1u
#define K1_EXACT_RATE_HZ 12800u
#define K1_EXACT_HOP_SAMPLES 96u
#define K1_EXACT_CHANNELS 2u
#define K1_EXACT_PCM_BYTES (K1_EXACT_HOP_SAMPLES * K1_EXACT_CHANNELS * 2u)
#define K1_EXACT_HOP_US 7500u
#define K1_EXACT_FLAG_DISCONTINUITY 1u

enum {
    K1_EXACT_OK = 0,
    K1_EXACT_INVALID = -1,
    K1_EXACT_OVERFLOW = -2,
    K1_EXACT_MUTATED = -3
};

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t sample_rate_hz;
    uint32_t hop_samples;
    uint32_t channels;
    uint32_t sequence;
    uint32_t epoch;
    uint32_t flags;
    uint64_t capture_start_us;
    uint64_t capture_end_us;
    uint32_t crc32;
    uint32_t pcm_bytes;
    int16_t pcm[K1_EXACT_HOP_SAMPLES * K1_EXACT_CHANNELS];
} k1_exact_hop_t;

typedef struct {
    int32_t peak;
    uint32_t clip_count;
    uint32_t zero_count;
    uint64_t square_sum;
} k1_exact_stats_t;

typedef struct {
    uint32_t next_sequence;
    uint32_t epoch;
    uint32_t hops_accepted;
    uint32_t halted;
    uint64_t last_end_us;
    int started;
} k1_exact_stream_t;

void k1_exact_stats(const int16_t *pcm, size_t samples, k1_exact_stats_t *out);
int k1_exact_hop_pack(k1_exact_hop_t *hop, uint32_t sequence, uint32_t epoch,
                      uint64_t start_us, uint64_t end_us, uint32_t flags,
                      const int16_t *pcm);
int k1_exact_hop_check(const k1_exact_hop_t *hop);
void k1_exact_stream_reset(k1_exact_stream_t *stream);
int k1_exact_stream_ingest(k1_exact_stream_t *stream, k1_ram_diag_t *diag,
                           k1_exact_hop_t *hop);

#ifdef __cplusplus
}
#endif
