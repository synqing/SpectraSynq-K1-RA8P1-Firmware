#include "titan_ram_diag.h"

#include <string.h>

static k1_ram_diag_t g_diag;

static void commit(k1_ram_diag_t *diag)
{
    diag->commit_seq += 1u;
}

void k1_ram_diag_reset(k1_ram_diag_t *diag)
{
    if (diag == 0) {
        return;
    }
    memset(diag, 0, sizeof(*diag));
    diag->magic = K1_RAM_DIAG_MAGIC;
    diag->version = K1_RAM_DIAG_VERSION;
}

k1_ram_diag_t *k1_ram_diag_block(void)
{
    if (g_diag.magic != K1_RAM_DIAG_MAGIC) {
        k1_ram_diag_reset(&g_diag);
    }
    return &g_diag;
}

void k1_ram_diag_event(k1_ram_diag_t *diag, uint32_t type, uint32_t aux,
                       uint64_t timestamp_us)
{
    uint32_t index;
    if (diag == 0) {
        return;
    }
    index = diag->event_count % K1_RAM_DIAG_EVENT_CAP;
    diag->events[index].type = type;
    diag->events[index].aux = aux;
    diag->events[index].timestamp_us = timestamp_us;
    diag->event_count += 1u;
    commit(diag);
}

void k1_ram_diag_note_discontinuity(k1_ram_diag_t *diag, uint32_t aux,
                                    uint64_t timestamp_us)
{
    if (diag == 0) {
        return;
    }
    diag->sample_discontinuities += 1u;
    k1_ram_diag_event(diag, K1_DIAG_EVT_DISCONTINUITY, aux, timestamp_us);
}

void k1_ram_diag_note_queue_own(k1_ram_diag_t *diag)
{
    if (diag == 0) {
        return;
    }
    diag->queue_owned += 1u;
    commit(diag);
}

void k1_ram_diag_note_queue_release(k1_ram_diag_t *diag)
{
    if (diag == 0) {
        return;
    }
    diag->queue_released += 1u;
    commit(diag);
}

void k1_ram_diag_note_overflow(k1_ram_diag_t *diag, uint64_t timestamp_us)
{
    if (diag == 0) {
        return;
    }
    diag->queue_overflow += 1u;
    k1_ram_diag_event(diag, K1_DIAG_EVT_OVERFLOW, 0, timestamp_us);
}

void k1_ram_diag_note_stale_owner(k1_ram_diag_t *diag, uint64_t timestamp_us)
{
    if (diag == 0) {
        return;
    }
    diag->stale_owner += 1u;
    k1_ram_diag_event(diag, K1_DIAG_EVT_STALE_OWNER, 0, timestamp_us);
}

void k1_ram_diag_note_dma_done(k1_ram_diag_t *diag, uint32_t cycles,
                               uint64_t timestamp_us)
{
    if (diag == 0) {
        return;
    }
    diag->dma_complete += 1u;
    k1_ram_diag_event(diag, K1_DIAG_EVT_DMA_DONE, cycles, timestamp_us);
}

void k1_ram_diag_note_dma_fault(k1_ram_diag_t *diag, uint32_t err,
                                uint64_t timestamp_us)
{
    if (diag == 0) {
        return;
    }
    diag->dma_fault += 1u;
    k1_ram_diag_event(diag, K1_DIAG_EVT_DMA_FAULT, err, timestamp_us);
}

void k1_ram_diag_note_crc_fail(k1_ram_diag_t *diag, uint64_t timestamp_us)
{
    if (diag == 0) {
        return;
    }
    diag->crc_fail += 1u;
    k1_ram_diag_event(diag, K1_DIAG_EVT_CRC_FAIL, 0, timestamp_us);
}

void k1_ram_diag_note_frame_submit(k1_ram_diag_t *diag, uint32_t now_us)
{
    if (diag == 0) {
        return;
    }
    diag->frame_submit_count += 1u;
    diag->last_submit_us = now_us;
    diag->frame_age_us = 0u;
    k1_ram_diag_event(diag, K1_DIAG_EVT_FRAME_SUBMIT, now_us, now_us);
}

void k1_ram_diag_note_frame_complete(k1_ram_diag_t *diag, uint32_t now_us)
{
    if (diag == 0) {
        return;
    }
    diag->frame_complete_count += 1u;
    diag->last_complete_us = now_us;
    diag->frame_age_us = now_us - diag->last_submit_us;
    k1_ram_diag_event(diag, K1_DIAG_EVT_FRAME_COMPLETE, diag->frame_age_us,
                      now_us);
}
