#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Lane P4: measured-rate polyphase resampler from the PDM stream (~41.4 kHz,
   MOCO-clocked) to exactly 24 kHz for the in-house analysis. Windowed-sinc
   (Kaiser) taps are tabulated once per locked source rate in float; each output
   sample is a K-tap dot product with coefficients interpolated linearly between
   adjacent phases. No doubles or trig per sample. Runs beside k1_asrc_24k, whose
   12.8 kHz donor output is unchanged. */

#define K1_POLY24_OUT_HZ 24000u
#define K1_POLY24_OUT_FRAMES 180u
#ifndef K1_POLY24_TAPS
#define K1_POLY24_TAPS 96u
#endif
#define K1_POLY24_PHASES 128u
#define K1_POLY24_CUTOFF_HZ 11250.0f
#define K1_POLY24_KAISER_BETA 7.0f
#define K1_POLY24_MIN_IN_HZ 36000u
#define K1_POLY24_MAX_IN_HZ 44000u
#define K1_POLY24_RING 1024u

enum {
  k1_poly24_ok = 0,
  k1_poly24_err_arg = 1,
  k1_poly24_err_rate = 2,
  k1_poly24_err_not_ready = 3,
  k1_poly24_err_full = 4
};

typedef struct {
  uint32_t source_hz;
  /* Source samples per output sample, Q32. */
  uint64_t step_q32;
  /* Position of the next output relative to ring[read_pos], Q32 source samples. */
  uint64_t pos_q32;
  int16_t ring[K1_POLY24_RING];
  uint32_t read_pos;
  uint32_t filled;
  uint32_t produced;
  uint32_t rejected;
  float table[K1_POLY24_PHASES + 1u][K1_POLY24_TAPS];
} k1_poly24_t;

void k1_poly24_reset(k1_poly24_t *st, uint32_t source_hz);
int k1_poly24_set_rate(k1_poly24_t *st, uint32_t source_hz);
int k1_poly24_push(k1_poly24_t *st, const int16_t *src, uint32_t count);
int k1_poly24_pull180(k1_poly24_t *st, int16_t out[K1_POLY24_OUT_FRAMES]);
/* Stream clock: output sample m names source instant m * step - DELAY, the same
   causal convention as k1_asrc24's 12.8 kHz Emotiscope profile (47 samples), so
   both outputs' sample indices name the same source moment. reset() pre-fills
   2 * DELAY zero samples to achieve it (P4 step-3 gate, run 1: without it the
   24 kHz index named a moment 2.35 ms later than the 12.8 kHz index). */
#define K1_POLY24_DELAY_SOURCE_SAMPLES (K1_POLY24_TAPS / 2u - 1u)
#define K1_POLY24_ALIGN_ZEROS (2u * K1_POLY24_DELAY_SOURCE_SAMPLES)

#ifdef __cplusplus
}
#endif
