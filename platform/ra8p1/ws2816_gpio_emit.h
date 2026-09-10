#pragma once

#include <stddef.h>
#include <stdint.h>

#define K1_PACKED_BYTES_PER_LANE 480u

#ifdef __cplusplus
extern "C" {
#endif

/* Packed-lane LED port. DualMCU PixelPort::submit(Pixel8) is not this
 * interface. submit != transfer-complete != latch. Never dithers. */

typedef enum {
  kPackedAccepted = 0,
  kPackedBusy = 1,
  kPackedWrongCount = 2
} packed_submit_result_t;

typedef struct {
  uint32_t submit_cycles;
  uint32_t transfer_done_cycles;
  uint32_t latch_ready_cycles;
  uint32_t emit_cycles;
  uint32_t latch_cycles;
  uint32_t bit_period_min_cycles;
  uint32_t bit_period_max_cycles;
} packed_lane_completion_t;

typedef struct {
  void (*set_a)(int high);
  void (*set_b)(int high);
  uint32_t (*now_cycles)(void);
} ws2816_bit_sink_t;

static inline packed_submit_result_t k1_ws2816_require_packed_lanes(
    size_t a_bytes, size_t b_bytes) {
  return (a_bytes == K1_PACKED_BYTES_PER_LANE &&
          b_bytes == K1_PACKED_BYTES_PER_LANE)
             ? kPackedAccepted
             : kPackedWrongCount;
}

static inline void k1_ws2816_emit_lockstep_bits(
    const uint8_t* lane_a, const uint8_t* lane_b, size_t bytes,
    const ws2816_bit_sink_t* sink, uint32_t t0h, uint32_t t1h, uint32_t period,
    packed_lane_completion_t* completion) {
  uint32_t bit_min = 0xffffffffU;
  uint32_t bit_max = 0;
  uint32_t previous = sink->now_cycles();
  int have_previous = 0;
  const uint32_t t1h_use = t1h > t0h ? t1h : t0h;
  const uint32_t period_use = period > t1h_use ? period : t1h_use;
  for (size_t i = 0; i < bytes; ++i) {
    uint8_t ba = lane_a[i];
    uint8_t bb = lane_b[i];
    for (int bit = 7; bit >= 0; --bit) {
      const int one_a = ((ba >> bit) & 1U) != 0U;
      const int one_b = ((bb >> bit) & 1U) != 0U;
      const uint32_t start = sink->now_cycles();
      if (have_previous) {
        const uint32_t dt = start - previous;
        if (dt < bit_min) bit_min = dt;
        if (dt > bit_max) bit_max = dt;
      }
      previous = start;
      have_previous = 1;
      sink->set_a(1);
      sink->set_b(1);
      while ((sink->now_cycles() - start) < t0h) {
      }
      if (!one_a) sink->set_a(0);
      if (!one_b) sink->set_b(0);
      while ((sink->now_cycles() - start) < t1h_use) {
      }
      sink->set_a(0);
      sink->set_b(0);
      while ((sink->now_cycles() - start) < period_use) {
      }
    }
  }
  if (completion != NULL) {
    completion->bit_period_min_cycles =
        (have_previous && bit_min != 0xffffffffU) ? bit_min : 0;
    completion->bit_period_max_cycles = bit_max;
  }
}

void k1_ws2816_set_clock(uint32_t hz);
packed_submit_result_t k1_ws2816_submit_packed_lanes(
    const uint8_t* lane_a, size_t a_bytes, const uint8_t* lane_b,
    size_t b_bytes, packed_lane_completion_t* completion);

#ifdef __cplusplus
}
#endif
