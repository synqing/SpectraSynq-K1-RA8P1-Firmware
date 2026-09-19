#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* RA8P1 PDM FIFO (PDDRR) for PDM_PCM_WIDTH_16_BITS_0_14:
   FSP clips filter bits [14:0] plus one sign bit. This helper takes the
   16-bit field in bits 15:0. Bits 19:16 should be sign replicas; bits
   31:20 should be zero. No gain is applied. */

typedef struct {
    int16_t sample;
    uint32_t raw;
    uint32_t high20;
    uint32_t sign_nibble;
    uint32_t packing_mismatch;
    uint32_t sat_neg;
    uint32_t sat_pos;
} k1_pdm_fifo16_view_t;

static inline int16_t k1_pdm_fifo16_extract(uint32_t word) {
    const uint32_t u = word & 0xffffu;
    const int32_t s = (u & 0x8000u) ? (int32_t)u - 65536 : (int32_t)u;
    return (int16_t)s;
}

static inline void k1_pdm_fifo16_inspect(uint32_t word, k1_pdm_fifo16_view_t *view) {
    const uint32_t replica = (word & 0x8000u) ? 0xfu : 0u;
    if (view == 0) {
        return;
    }
    view->raw = word;
    view->sample = k1_pdm_fifo16_extract(word);
    view->high20 = word >> 20;
    view->sign_nibble = (word >> 16) & 0xfu;
    view->packing_mismatch =
        (view->sign_nibble != replica || view->high20 != 0u) ? 1u : 0u;
    view->sat_neg = (view->sample == (int16_t)-32768) ? 1u : 0u;
    view->sat_pos = (view->sample == (int16_t)32767) ? 1u : 0u;
}

#ifdef __cplusplus
}
#endif
