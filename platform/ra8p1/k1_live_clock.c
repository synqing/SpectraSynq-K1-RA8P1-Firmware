#include "k1_live_clock.h"

#include <stddef.h>

void k1_live_clock_init(k1_live_clock_t *clock, uint32_t clock_hz, uint32_t origin) {
  if (clock == NULL) {
    return;
  }
  clock->microseconds = 0;
  clock->remainder = 0;
  clock->cycles_extended = 0;
  clock->last_cycles = 0;
  clock->clock_hz = clock_hz;
  clock->origin = origin;
  clock->seen = 0;
  clock->valid = clock_hz != 0;
}

int k1_live_clock_bind_hz(k1_live_clock_t *clock, uint32_t clock_hz) {
  if (clock == NULL || clock_hz == 0) {
    return -1;
  }
  if (clock->seen && clock->clock_hz != clock_hz) {
    clock->valid = 0;
    return -1;
  }
  clock->clock_hz = clock_hz;
  clock->valid = 1;
  return 0;
}

int k1_live_clock_sample(k1_live_clock_t *clock, uint32_t cycles, k1_live_clock_sample_t *out) {
  if (clock == NULL || out == NULL) {
    return -1;
  }
  if (!clock->valid || clock->clock_hz == 0) {
    out->cycles = clock->cycles_extended;
    out->microseconds = clock->microseconds;
    out->clock_hz = clock->clock_hz;
    out->origin = clock->origin;
    out->health = K1_LIVE_CLOCK_INVALID;
    return -1;
  }
  if (clock->seen) {
    const uint32_t delta = cycles - clock->last_cycles;
    clock->cycles_extended += (uint64_t)delta;
    const uint64_t numerator = (uint64_t)delta * 1000000U + clock->remainder;
    clock->microseconds += numerator / clock->clock_hz;
    clock->remainder = numerator % clock->clock_hz;
  } else {
    clock->cycles_extended = 0;
  }
  clock->last_cycles = cycles;
  clock->seen = 1;
  out->cycles = clock->cycles_extended;
  out->microseconds = clock->microseconds;
  out->clock_hz = clock->clock_hz;
  out->origin = clock->origin;
  out->health = K1_LIVE_CLOCK_OK;
  return 0;
}
