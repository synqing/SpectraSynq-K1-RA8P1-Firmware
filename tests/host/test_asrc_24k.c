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

int main(void) {
  k1_asrc24_t st;
  int16_t src[400];
  int16_t out[K1_ASRC24_OUT_FRAMES];
  int16_t out2[K1_ASRC24_OUT_FRAMES];
  uint32_t rates[3] = {36000u, 40000u, 44000u};
  unsigned r;

  assert(k1_asrc24_phase_increment_q16(40000u) ==
         (uint32_t)(((uint64_t)40000u << 16) / 24000u));

  k1_asrc24_reset(&st, 1u, 40000u);
  memset(src, 0, sizeof src);
  assert(k1_asrc24_push(&st, src, 10u) == k1_asrc24_ok);
  assert(k1_asrc24_pull180(&st, out) == k1_asrc24_err_starved);

  k1_asrc24_reset(&st, 1u, 20000u);
  assert(st.source_hz == 40000u);
  assert(k1_asrc24_set_rate(&st, 20000u) == k1_asrc24_err_rate);

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
