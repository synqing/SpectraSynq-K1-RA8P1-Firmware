#include "k1_live_clock.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  k1_live_clock_link_t link = {10000000ULL, 5000ULL, 1};
  int ok = 0;
  assert(k1_live_clock_capture_to_live(&link, 10000000ULL, &ok) == 5000ULL && ok);
  assert(k1_live_clock_capture_to_live(&link, 10007500ULL, &ok) == 12500ULL && ok);
  assert(k1_live_clock_capture_to_live(&link, 9999999ULL, &ok) == 4999 && ok);
  assert(k1_live_clock_capture_to_live(&link, 9995000ULL, &ok) == 0 && ok);
  assert(k1_live_clock_capture_to_live(&link, 9994999ULL, &ok) == 0 && !ok);
  // A1000us-old capture remains1000us older after clock translation.
  assert(k1_live_clock_capture_to_live(&link, 9999000ULL, &ok) == 4000 && ok);
  link.bound = 0;
  assert(k1_live_clock_capture_to_live(&link, 10007500ULL, &ok) == 0 && !ok);
  k1_live_clock_t clock;
  k1_live_clock_init(&clock, 1000000000U, 1U);
  k1_live_clock_sample_t sample;
  assert(k1_live_clock_sample(&clock, 1000U, &sample) == 0);
  assert(sample.microseconds == 0);
  assert(k1_live_clock_sample(&clock, 1000U + 7500U, &sample) == 0);
  assert(sample.microseconds == 7U);
  puts("live clock link ok");
  return 0;
}
