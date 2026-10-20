/* Lane P4: k1_poly24 stream clock. After every pull, the source position the
   resampler reports for its newest output must equal (n_out - 1) * step - DELAY
   (Q32, exact arithmetic), across flushes and odd push sizes. Prints worst error
   in source samples and the count of checks; rc 1 on any mismatch > 1e-6. */
#include "k1_poly24.h"
#include <stdio.h>
#include <stdint.h>

int main(void)
{
    static k1_poly24_t st;
    int16_t in[333] = {0};
    int16_t out[K1_POLY24_OUT_FRAMES];
    double worst = 0.0;
    unsigned checks = 0u;
    k1_poly24_reset(&st, 41417u);
    for (unsigned round = 0u; round < 2u; ++round) {
        uint64_t outputs = 0u;
        for (unsigned i = 0u; i < 400u; ++i) {
            const uint32_t n = 101u + (i * 37u) % 233u;
            if (k1_poly24_push(&st, in, n) != k1_poly24_ok) return 2;
            while (k1_poly24_pull180(&st, out) == k1_poly24_ok) {
                outputs += K1_POLY24_OUT_FRAMES;
                const double expect = (double)(outputs - 1u) * ((double)st.step_q32 / 4294967296.0)
                                      - (double)K1_POLY24_DELAY_SOURCE_SAMPLES;
                const double got = (double)k1_poly24_last_output_source_q32(&st) / 4294967296.0;
                const double err = got > expect ? got - expect : expect - got;
                if (expect >= 0.0 && err > worst) worst = err;
                checks += expect >= 0.0;
            }
        }
        k1_poly24_flush(&st);
    }
    printf("{\"worst_error_samples\":%.9f,\"checks\":%u}\n", worst, checks);
    return worst > 1e-6 || checks == 0u ? 1 : 0;
}
