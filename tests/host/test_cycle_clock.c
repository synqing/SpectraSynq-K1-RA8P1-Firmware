#include "k1_asrc_24k.h"
#include "k1_cycle_clock.h"

#include <assert.h>
#include <stdio.h>

static uint64_t extend(uint32_t now, uint32_t *last, uint64_t *high)
{
  return k1_cycle_extend(now, last, high);
}

int main(void)
{
  uint32_t last = 0u;
  uint64_t high = 0u;
  k1_asrc24_t st;

  assert(extend(1000u, &last, &high) == 1000u);
  assert(extend(2000u, &last, &high) == 2000u);
  assert(high == 0u);

  assert(extend(10u, &last, &high) == ((1ull << 32) | 10u));
  assert(high == (1ull << 32));

  /* The unlocked interleaving: a later sample lands first, then the
     earlier sample is committed and invents a wrap. */
  last = 100u;
  high = 0u;
  assert(extend(500u, &last, &high) == 500u);
  assert(extend(200u, &last, &high) == ((1ull << 32) | 200u));

  /* Live pair, 22 Sep 2026: 208088 samples in 5.032 s of wall time is
     41354 Hz and must lock. The same samples over the stretched chip
     clock (5.266x) are about 7853 Hz and must be refused. */
  k1_asrc24_reset(&st, 1u, 40000u);
  assert(k1_asrc24_set_rate(&st, (uint32_t)((208088ull * 1000000ull) / 5032000ull)) ==
         k1_asrc24_ok);
  k1_asrc24_reset(&st, 1u, 40000u);
  assert(k1_asrc24_set_rate(&st, (uint32_t)((208088ull * 1000000ull) / 26498000ull)) ==
         k1_asrc24_err_rate);

  puts("K1_CYCLE_CLOCK=PASS");
  return 0;
}
