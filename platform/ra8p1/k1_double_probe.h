#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void k1_double_probe_reset(void);
void k1_double_probe_snapshot(uint32_t *calls, uint32_t *cycles);
void k1_double_probe_inject_for_test(uint32_t calls);

#ifdef __cplusplus
}
#endif
