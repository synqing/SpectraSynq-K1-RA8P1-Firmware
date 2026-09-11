#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define K1_RAM_DIAG_MAGIC 0x4B314447u /* K1DG */
#define K1_RAM_DIAG_VERSION 1u
#define K1_RAM_DIAG_EVENT_CAP 16u

enum {
    K1_DIAG_EVT_NONE = 0,
    K1_DIAG_EVT_DISCONTINUITY = 1,
    K1_DIAG_EVT_OVERFLOW = 2,
    K1_DIAG_EVT_STALE_OWNER = 3,
    K1_DIAG_EVT_DMA_DONE = 4,
    K1_DIAG_EVT_DMA_FAULT = 5,
    K1_DIAG_EVT_CRC_FAIL = 6,
    K1_DIAG_EVT_FRAME_SUBMIT = 7,
    K1_DIAG_EVT_FRAME_COMPLETE = 8
};

typedef struct {
    uint32_t type;
    uint32_t aux;
    uint64_t timestamp_us;
} k1_diag_event_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t sample_discontinuities;
    uint32_t queue_owned;
    uint32_t queue_released;
    uint32_t queue_overflow;
    uint32_t stale_owner;
    uint32_t dma_complete;
    uint32_t dma_fault;
    uint32_t crc_fail;
    uint32_t overrun_drop;
    uint32_t frame_submit_count;
    uint32_t frame_complete_count;
    uint32_t last_submit_us;
    uint32_t last_complete_us;
    uint32_t frame_age_us;
    uint32_t commit_seq;
    uint32_t event_count;
    k1_diag_event_t events[K1_RAM_DIAG_EVENT_CAP];
} k1_ram_diag_t;

void k1_ram_diag_reset(k1_ram_diag_t *diag);
k1_ram_diag_t *k1_ram_diag_block(void);
void k1_ram_diag_event(k1_ram_diag_t *diag, uint32_t type, uint32_t aux,
                       uint64_t timestamp_us);
void k1_ram_diag_note_discontinuity(k1_ram_diag_t *diag, uint32_t aux,
                                    uint64_t timestamp_us);
void k1_ram_diag_note_queue_own(k1_ram_diag_t *diag);
void k1_ram_diag_note_queue_release(k1_ram_diag_t *diag);
void k1_ram_diag_note_overflow(k1_ram_diag_t *diag, uint64_t timestamp_us);
void k1_ram_diag_note_stale_owner(k1_ram_diag_t *diag, uint64_t timestamp_us);
void k1_ram_diag_note_dma_done(k1_ram_diag_t *diag, uint32_t cycles,
                               uint64_t timestamp_us);
void k1_ram_diag_note_dma_fault(k1_ram_diag_t *diag, uint32_t err,
                                uint64_t timestamp_us);
void k1_ram_diag_note_crc_fail(k1_ram_diag_t *diag, uint64_t timestamp_us);
void k1_ram_diag_note_frame_submit(k1_ram_diag_t *diag, uint32_t now_us);
void k1_ram_diag_note_frame_complete(k1_ram_diag_t *diag, uint32_t now_us);

#ifdef __cplusplus
}
#endif
