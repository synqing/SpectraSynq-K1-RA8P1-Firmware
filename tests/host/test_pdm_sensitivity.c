#include "k1_pdm_sensitivity.h"

#include <stdio.h>
#include <stdlib.h>

static int16_t apply(int32_t sample, uint32_t peak) {
    return k1_pdm_scale_sample((int16_t)sample, k1_pdm_loud_guard_q8(peak));
}

int main(void) {
    const uint32_t nom = K1_PDM_SENSITIVITY_Q8;
    if (k1_pdm_loud_guard_q8(0) != nom) abort();
    if (k1_pdm_loud_guard_q8(100) != nom) abort();
    if (k1_pdm_loud_guard_q8(12508) != nom) abort();
    if (k1_pdm_loud_guard_q8(13000) >= nom) abort();
    if (k1_pdm_scale_sample(100, nom) != 239) abort();
    if (k1_pdm_scale_sample(-100, nom) != -240) abort();
    if (apply(12500, 12500) > 30000) abort();
    if (apply(12508, 12508) > 30000) abort();
    if (apply(14000, 14000) > 30000) abort();
    if (apply(-14000, 14000) < -30000) abort();
    if (apply(20000, 20000) > 30000) abort();
    if (apply(-20000, 20000) < -30000) abort();
    if (apply(32767, 32767) == 32767) abort();
    if (apply(-32767, 32767) == -32767) abort();
    puts("PDM_SENSITIVITY_PASS q8_guard_12508=nominal 13000=reduced no_int16_clip_at_14000 visual_16x_not_stacked=true");
    return 0;
}
