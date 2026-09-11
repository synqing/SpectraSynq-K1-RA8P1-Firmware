#include "k1_exact_stream.h"
#include "k1_crc32.h"

#include <string.h>

void k1_exact_stats(const int16_t *pcm, size_t samples, k1_exact_stats_t *out)
{
    size_t i;
    int32_t peak = 0;
    if (out == 0) {
        return;
    }
    memset(out, 0, sizeof(*out));
    if (pcm == 0) {
        return;
    }
    for (i = 0; i < samples; ++i) {
        int32_t v = pcm[i];
        int32_t a = v < 0 ? -v : v;
        if (a > peak) {
            peak = a;
        }
        if (v == 32767 || v == -32768) {
            out->clip_count += 1u;
        }
        if (v == 0) {
            out->zero_count += 1u;
        }
        out->square_sum += (uint64_t)((int64_t)v * (int64_t)v);
    }
    out->peak = peak;
}

int k1_exact_hop_pack(k1_exact_hop_t *hop, uint32_t sequence, uint32_t epoch,
                      uint64_t start_us, uint64_t end_us, uint32_t flags,
                      const int16_t *pcm)
{
    if (hop == 0 || pcm == 0) {
        return K1_EXACT_INVALID;
    }
    memset(hop, 0, sizeof(*hop));
    hop->magic = K1_EXACT_MAGIC;
    hop->version = K1_EXACT_VERSION;
    hop->sample_rate_hz = K1_EXACT_RATE_HZ;
    hop->hop_samples = K1_EXACT_HOP_SAMPLES;
    hop->channels = K1_EXACT_CHANNELS;
    hop->sequence = sequence;
    hop->epoch = epoch;
    hop->flags = flags;
    hop->capture_start_us = start_us;
    hop->capture_end_us = end_us;
    hop->pcm_bytes = K1_EXACT_PCM_BYTES;
    memcpy(hop->pcm, pcm, K1_EXACT_PCM_BYTES);
    hop->crc32 = k1_crc32_ieee((const uint8_t *)hop->pcm, K1_EXACT_PCM_BYTES);
    return K1_EXACT_OK;
}

int k1_exact_hop_check(const k1_exact_hop_t *hop)
{
    uint32_t crc;
    if (hop == 0) {
        return K1_EXACT_INVALID;
    }
    if (hop->magic != K1_EXACT_MAGIC || hop->version != K1_EXACT_VERSION ||
        hop->sample_rate_hz != K1_EXACT_RATE_HZ ||
        hop->hop_samples != K1_EXACT_HOP_SAMPLES ||
        hop->channels != K1_EXACT_CHANNELS ||
        hop->pcm_bytes != K1_EXACT_PCM_BYTES) {
        return K1_EXACT_INVALID;
    }
    crc = k1_crc32_ieee((const uint8_t *)hop->pcm, K1_EXACT_PCM_BYTES);
    if (crc != hop->crc32) {
        return K1_EXACT_MUTATED;
    }
    return K1_EXACT_OK;
}

void k1_exact_stream_reset(k1_exact_stream_t *stream)
{
    if (stream == 0) {
        return;
    }
    memset(stream, 0, sizeof(*stream));
}

int k1_exact_stream_ingest(k1_exact_stream_t *stream, k1_ram_diag_t *diag,
                           k1_exact_hop_t *hop)
{
    int check;
    uint32_t disc = 0u;
    if (stream == 0 || hop == 0) {
        return K1_EXACT_INVALID;
    }
    if (stream->halted) {
        k1_ram_diag_note_overflow(diag, hop->capture_end_us);
        return K1_EXACT_OVERFLOW;
    }
    check = k1_exact_hop_check(hop);
    if (check != K1_EXACT_OK) {
        if (check == K1_EXACT_MUTATED) {
            k1_ram_diag_note_crc_fail(diag, hop->capture_end_us);
        }
        return check;
    }
    if (stream->started) {
        if (hop->epoch != stream->epoch) {
            disc = 1u;
        } else if (hop->sequence != stream->next_sequence) {
            disc = 1u;
        } else if (hop->capture_start_us != stream->last_end_us) {
            disc = 1u;
        }
        if (disc) {
            hop->flags |= K1_EXACT_FLAG_DISCONTINUITY;
            k1_ram_diag_note_discontinuity(diag, hop->sequence,
                                           hop->capture_end_us);
        }
    }
    k1_ram_diag_note_queue_own(diag);
    stream->started = 1;
    stream->epoch = hop->epoch;
    stream->next_sequence = hop->sequence + 1u;
    stream->last_end_us = hop->capture_end_us;
    stream->hops_accepted += 1u;
    k1_ram_diag_note_queue_release(diag);
    return K1_EXACT_OK;
}
