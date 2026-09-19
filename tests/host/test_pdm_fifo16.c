#include "k1_pdm_fifo16.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    k1_pdm_fifo16_view_t view;
    assert(k1_pdm_fifo16_extract(0x00000000u) == 0);
    assert(k1_pdm_fifo16_extract(0x00000001u) == 1);
    assert(k1_pdm_fifo16_extract(0x00007fffu) == 32767);
    assert(k1_pdm_fifo16_extract(0x000fffffu) == -1);
    assert(k1_pdm_fifo16_extract(0x000f8000u) == -32768);
    assert(k1_pdm_fifo16_extract(0x00008000u) == -32768);
    assert(k1_pdm_fifo16_extract(0x12340000u) == 0);

    k1_pdm_fifo16_inspect(0x00007fffu, &view);
    assert(view.sample == 32767 && view.sat_pos == 1u && view.sat_neg == 0u);
    assert(view.packing_mismatch == 0u);

    k1_pdm_fifo16_inspect(0x000f8000u, &view);
    assert(view.sample == -32768 && view.sat_neg == 1u && view.sign_nibble == 0xfu);
    assert(view.packing_mismatch == 0u);

    k1_pdm_fifo16_inspect(0x00008000u, &view);
    assert(view.sat_neg == 1u && view.packing_mismatch == 1u);

    k1_pdm_fifo16_inspect(0x12340000u, &view);
    assert(view.sample == 0 && view.high20 != 0u && view.packing_mismatch == 1u);

    puts("K1_PDM_FIFO16=PASS");
    return 0;
}
