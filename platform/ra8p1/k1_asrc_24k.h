#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Measured-rate mono ASRC into the qualified AP: 180 samples at 24 kHz.
   Nominal source is 40 kHz (increment 5/3). Host tests cover 36–44 kHz.
   Legacy 16 kHz → 12.8 kHz stereo remains k1_rate_adapt. */

#define K1_ASRC24_OUT_HZ 24000u
#define K1_ASRC24_OUT_FRAMES 180u
#define K1_ASRC24_NOMINAL_IN_HZ 40000u
#define K1_ASRC24_MIN_IN_HZ 36000u
#define K1_ASRC24_MAX_IN_HZ 44000u
#define K1_ASRC24_RING 1024u

enum {
  k1_asrc24_ok = 0,
  k1_asrc24_err_arg = 1,
  k1_asrc24_err_rate = 2,
  k1_asrc24_err_starved = 3,
  k1_asrc24_err_epoch = 4
};

typedef struct {
  uint32_t epoch;
  uint32_t source_hz;
  uint64_t src_index;
  uint32_t frac_q16; /* source phase, 1.0 == 65536 */
  int16_t ring[K1_ASRC24_RING];
  uint32_t write_pos;
  uint32_t read_pos;
  uint32_t filled;
  uint32_t produced;
  uint32_t starved;
} k1_asrc24_t;

void k1_asrc24_reset(k1_asrc24_t *st, uint32_t epoch, uint32_t source_hz);
int k1_asrc24_set_rate(k1_asrc24_t *st, uint32_t source_hz);
uint32_t k1_asrc24_phase_increment_q16(uint32_t source_hz);
int k1_asrc24_push(k1_asrc24_t *st, const int16_t *src, uint32_t count);
int k1_asrc24_pull180(k1_asrc24_t *st, int16_t out180[K1_ASRC24_OUT_FRAMES]);

#ifdef __cplusplus
}
#endif
