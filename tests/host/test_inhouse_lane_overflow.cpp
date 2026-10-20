// Compile the actual private implementation into this test translation unit;
// no test-only production API or independent rewrite of the step is required.
#include "../../platform/ra8p1/inhouse_audio.cpp"
#include <cassert>
#include <cstdio>

static std::uint32_t oracle_overflows = 0;
#define K1_GDFT_LANE4_Q0_OBSERVE(value) do { \
  if ((value) > INT32_MAX || (value) < INT32_MIN) ++oracle_overflows; \
} while (0)
#include "k1_gdft_lane4_exact.h"

int main() {
  std::uint32_t positive = 0, negative = 0;
  for (const auto coefficient : {INT32_C(-32768), INT32_C(32767)}) {
    for (const auto q1 : {INT32_MIN, INT32_C(-1), INT32_C(0), INT32_C(1), INT32_MAX}) {
      for (const auto q2 : {INT32_MIN, INT32_C(0), INT32_MAX}) {
        for (const auto sample : {INT16_MIN, 0, INT16_MAX}) {
          k1::titan::InhouseLane candidate{coefficient, 1U, q1, q2};
          K1GdftLane4ExactState oracle{coefficient, 1U, q1, q2};
          std::uint32_t observed = 0;
          oracle_overflows = 0;
          k1::titan::stepInhouseLane(candidate, sample, observed);
          k1_gdft_lane4_exact_step(oracle, sample);
          assert(candidate.q1 == oracle.q1 && candidate.q2 == oracle.q2);
          assert(observed == oracle_overflows);
          observed ? ++positive : ++negative;
        }
      }
    }
  }
  assert(positive != 0 && negative != 0);
  std::printf("source overflow witness: %u overflow and %u non-overflow boundary steps\n", positive, negative);
}
