#pragma once

#include <stdint.h>

/* Extend a free-running 32-bit cycle counter to 64 bits.

   The caller must make the counter read and this update atomic against
   every other caller. If a later sample is committed first and this
   earlier sample is committed afterwards, the comparison sees a false
   wrap and adds 2^32 cycles. On the Titan that false wrap, about once a
   second, made the microsecond clock run about 5.3 times too fast. The
   PDM rate estimate then fell to about 8 kHz, outside the 36–44 kHz
   ASRC window, and the rate lock never set even though the microphone
   was really at about 41 kHz. */
static inline uint64_t k1_cycle_extend(uint32_t now, uint32_t *last, uint64_t *high)
{
    if (now < *last) {
        *high += (1ull << 32);
    }
    *last = now;
    return *high | (uint64_t) now;
}
