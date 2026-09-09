#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { K1_P4_DSP_ALONE=1, K1_P4_NPU_ALONE=2, K1_P4_CONCURRENT=3, K1_P4_SATURATION=4 };
void k1_p4_initialise(uint32_t clock_hz);
bool k1_p4_start(uint32_t releases, uint32_t mode, uint32_t flags);
bool k1_p4_active(void);
void k1_p4_step(void);
size_t k1_p4_status(char *output, size_t capacity);
