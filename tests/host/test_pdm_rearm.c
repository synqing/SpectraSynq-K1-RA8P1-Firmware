#include "pdm_capture.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    uint32_t slot, epoch, seq, filling;
    uint64_t t0, t1;
    k1_pdm_stream_t stream;
    assert(k1_pdm_stream_context_configure(&stream, 4u, 4u) == K1_PDM_STREAM_OK);
    assert(k1_pdm_stream_context_start(&stream, 0) == K1_PDM_STREAM_OK);
    assert(k1_pdm_stream_context_filling_slot(&stream, &filling) == K1_PDM_STREAM_OK);
    assert(filling == 0u);
    assert(k1_pdm_stream_context_on_data(&stream, 4u, 10) == K1_PDM_STREAM_SLOT_READY);
    assert(k1_pdm_stream_context_filling_slot(&stream, &filling) == K1_PDM_STREAM_OK);
    assert(filling == 1u);
    assert(k1_pdm_stream_context_acquire(&stream, &slot, &epoch, &seq, &t0, &t1) ==
           K1_PDM_STREAM_OK);
    assert(slot == 0u);
    assert(k1_pdm_stream_context_on_data(&stream, 4u, 20) == K1_PDM_STREAM_OVERFLOW);
    assert(k1_pdm_stream_context_filling_slot(&stream, &filling) == K1_PDM_STREAM_INVALID);
    assert(stream.overflow_events >= 1u);
    puts("K1_PDM_REARM=PASS stalled_consumer_denies_rearm");
    return 0;
}
