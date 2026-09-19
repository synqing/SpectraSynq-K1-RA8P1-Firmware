#pragma once

#include <stdint.h>

/* K1 CONFIG.SENSITIVITY analogue on PCM. SINCRNG 10 is the filter window;
   do not also keep the old ×16 that existed to un-bury SINCRNG 5.
   Q8: 2.4 → 614. Loud-guard keeps a snap from railing int16. */
#define K1_PDM_SENSITIVITY_Q8 614u
#define K1_PDM_LOUD_GUARD_CEILING 30000u

static inline uint32_t k1_pdm_loud_guard_q8(uint32_t peak) {
    const uint32_t nominal = K1_PDM_SENSITIVITY_Q8;
    const uint64_t ceiling_q8 =
        ((uint64_t)K1_PDM_LOUD_GUARD_CEILING << 8);
    if (peak == 0u) {
        return nominal;
    }
    if ((uint64_t)peak * (uint64_t)nominal <= ceiling_q8) {
        return nominal;
    }
    return (uint32_t)(ceiling_q8 / peak);
}

static inline int16_t k1_pdm_scale_sample(int16_t sample, uint32_t gain_q8) {
    const int32_t scaled = ((int32_t)sample * (int32_t)gain_q8) >> 8;
    if (scaled > 32767) {
        return 32767;
    }
    if (scaled < -32768) {
        return (int16_t)-32768;
    }
    return (int16_t)scaled;
}
