#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* IEEE 802.3 CRC-32. Mechanism from SenseGlow MIT 2e5abea; polynomial is
   the public CRC-32/ISO-HDLC constant, not a product licence. */
static inline uint32_t k1_crc32_ieee(const uint8_t *bytes, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i;
    uint32_t b;
    if (bytes == 0 && length != 0u) {
        return 0u;
    }
    for (i = 0; i < length; ++i) {
        crc ^= bytes[i];
        for (b = 0; b < 8u; ++b) {
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(crc & 1u));
        }
    }
    return ~crc;
}

#ifdef __cplusplus
}
#endif
