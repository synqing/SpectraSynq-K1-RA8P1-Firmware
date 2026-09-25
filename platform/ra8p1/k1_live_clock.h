#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  K1_LIVE_CLOCK_OK = 0,
  K1_LIVE_CLOCK_INVALID = 1
};

typedef struct k1_live_clock_sample {
  uint64_t cycles;
  uint64_t microseconds;
  uint32_t clock_hz;
  uint32_t origin;
  uint8_t health;
} k1_live_clock_sample_t;

typedef struct k1_live_clock {
  uint64_t microseconds;
  uint64_t remainder;
  uint64_t cycles_extended;
  uint32_t last_cycles;
  uint32_t clock_hz;
  uint32_t origin;
  int seen;
  int valid;
} k1_live_clock_t;

void k1_live_clock_init(k1_live_clock_t *clock, uint32_t clock_hz, uint32_t origin);
int k1_live_clock_bind_hz(k1_live_clock_t *clock, uint32_t clock_hz);
int k1_live_clock_sample(k1_live_clock_t *clock, uint32_t cycles, k1_live_clock_sample_t *out);

#ifdef __cplusplus
}
#endif
