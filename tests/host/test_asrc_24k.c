#include "k1_asrc_24k.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void fill_sine(int16_t *dst, uint32_t count, uint32_t rate_hz, float freq)
{
  uint32_t i;
  for (i = 0; i < count; ++i) {
    const float t = (float)i / (float)rate_hz;
    dst[i] = (int16_t)(sinf(2.0f * 3.14159265f * freq * t) * 16000.0f);
  }
}

static void assert_pull_not_ready_is_atomic(k1_asrc24_t *st)
{
  k1_asrc24_t before;
  int16_t out[K1_ASRC24_OUT_FRAMES];
  uint32_t i;
  memcpy(&before, st, sizeof(*st));
  memset(out, 0x5a, sizeof(out));
  assert(k1_asrc24_pull180(st, out) == k1_asrc24_err_not_ready);
  assert(memcmp(st, &before, sizeof(*st)) == 0);
  for (i = 0; i < K1_ASRC24_OUT_FRAMES; ++i) {
    assert(out[i] == (int16_t)0x5a5a);
  }
}

static void assert_exact_required(uint32_t rate_hz)
{
  k1_asrc24_t st;
  int16_t src[K1_ASRC24_RING];
  int16_t out[K1_ASRC24_OUT_FRAMES];
  uint32_t required;
  k1_asrc24_reset(&st, 1u, rate_hz);
  required = k1_asrc24_required_for_pull180(&st);
  assert(required > 0u && required < K1_ASRC24_RING);
  memset(src, 1, sizeof(src));
  assert(k1_asrc24_push(&st, src, required - 1u) == k1_asrc24_ok);
  assert_pull_not_ready_is_atomic(&st);
  assert(k1_asrc24_push(&st, src, 1u) == k1_asrc24_ok);
  assert(st.filled == required);
  assert(k1_asrc24_pull180(&st, out) == k1_asrc24_ok);
  assert(st.produced == K1_ASRC24_OUT_FRAMES);
}

int main(void) {
  k1_asrc24_t st;
  k1_asrc24_t before;
  int16_t src[400];
  int16_t out[K1_ASRC24_OUT_FRAMES];
  int16_t out2[K1_ASRC24_OUT_FRAMES];
  int16_t junk[8];
  uint32_t rates[4] = {36000u, 40000u, 41530u, 44000u};
  unsigned r;

  assert(k1_asrc24_phase_increment_q16(40000u) ==
         (uint32_t)(((uint64_t)40000u << 16) / 24000u));

  k1_asrc24_reset(&st, 1u, 40000u);
  memset(src, 0, sizeof src);
  assert(k1_asrc24_push(&st, src, 10u) == k1_asrc24_ok);
  assert_pull_not_ready_is_atomic(&st);

  k1_asrc24_reset(&st, 1u, 20000u);
  assert(st.source_hz == 40000u);
  assert(k1_asrc24_set_rate(&st, 20000u) == k1_asrc24_err_rate);

  k1_asrc24_reset(&st, 1u, 40000u);
  memset(junk, 7, sizeof junk);
  memcpy(&before, &st, sizeof(st));
  assert(k1_asrc24_push(&st, junk, K1_ASRC24_RING + 1u) == k1_asrc24_err_arg);
  assert(st.filled == before.filled);
  assert(st.write_pos == before.write_pos);
  assert(st.read_pos == before.read_pos);
  assert(st.push_rejected == before.push_rejected + 1u);
  assert(st.discarded_samples == before.discarded_samples + K1_ASRC24_RING + 1u);
  assert(memcmp(st.ring, before.ring, sizeof(st.ring)) == 0);

  for (r = 0; r < 4u; ++r) {
    assert_exact_required(rates[r]);
  }

  for (r = 0; r < 3u; ++r) {
    k1_asrc24_reset(&st, 7u, rates[r]);
    fill_sine(src, 400u, rates[r], 1000.0f);
    assert(k1_asrc24_push(&st, src, 400u) == k1_asrc24_ok);
    assert(k1_asrc24_pull180(&st, out) == k1_asrc24_ok);
    assert(st.produced == 180u);
    assert(st.epoch == 7u);
  }

  k1_asrc24_reset(&st, 1u, 40000u);
  fill_sine(src, 400u, 40000u, 500.0f);
  assert(k1_asrc24_push(&st, src, 200u) == k1_asrc24_ok);
  assert(k1_asrc24_push(&st, src + 200u, 200u) == k1_asrc24_ok);
  assert(k1_asrc24_pull180(&st, out) == k1_asrc24_ok);

  k1_asrc24_reset(&st, 1u, 40000u);
  assert(k1_asrc24_push(&st, src, 400u) == k1_asrc24_ok);
  assert(k1_asrc24_pull180(&st, out2) == k1_asrc24_ok);
  assert(memcmp(out, out2, sizeof out) == 0);

  k1_asrc24_reset(&st, 1u, 40000u);
  fill_sine(src, 300u, 40000u, 800.0f);
  for (r = 0; r < 8u; ++r) {
    assert(k1_asrc24_push(&st, src, 300u) == k1_asrc24_ok);
    assert(k1_asrc24_pull180(&st, out) == k1_asrc24_ok);
  }
  assert(st.produced == 8u * 180u);
  assert(st.starved == 0u);

  k1_asrc24_reset(&st, 1u, 40000u);
  assert(k1_asrc24_set_rate(&st, 41530u) == k1_asrc24_ok);
  assert(st.source_hz == 41530u);

  puts("K1_ASRC24=PASS");
  return 0;
}
