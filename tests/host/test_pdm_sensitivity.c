#include "k1_pdm_sensitivity.h"

#include <stdio.h>
#include <stdlib.h>

static int16_t apply(int32_t sample, uint32_t peak) {
    return k1_pdm_scale_sample((int16_t)sample, k1_pdm_loud_guard_q8(peak));
}

int main(void) {
    const uint32_t nom = K1_PDM_SENSITIVITY_Q8;
    const uint32_t keep =
        (uint32_t)(((uint64_t)K1_PDM_LOUD_GUARD_CEILING << 8) / nom);
    if (nom != 4096u) abort();
    if (k1_pdm_loud_guard_q8(0) != nom) abort();
    if (k1_pdm_loud_guard_q8(100) != nom) abort();
    if (k1_pdm_loud_guard_q8(keep) != nom) abort();
    if (k1_pdm_loud_guard_q8(keep + 1u) >= nom) abort();
    if (k1_pdm_scale_sample(100, nom) != (int16_t)((100 * (int32_t)nom) >> 8))
        abort();
    if (k1_pdm_scale_sample(-100, nom) !=
        (int16_t)((-100 * (int32_t)nom) >> 8))
        abort();
    if (apply(12500, 12500) > 30000) abort();
    if (apply(12508, 12508) > 30000) abort();
    if (apply(14000, 14000) > 30000) abort();
    if (apply(-14000, 14000) < -30000) abort();
    if (apply(20000, 20000) > 30000) abort();
    if (apply(-20000, 20000) < -30000) abort();
    if (apply(32767, 32767) == 32767) abort();
    if (apply(-32767, 32767) == -32767) abort();
    puts("PDM_SENSITIVITY_PASS q8=4096 loud_guard_clips_int16=true");
    return 0;
}
