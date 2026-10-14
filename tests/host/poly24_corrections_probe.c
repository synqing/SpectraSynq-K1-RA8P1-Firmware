/* Lane P4 corrections host probe.
     design <rate_hz>                       max |new - previous| coefficient, and design time
     lag <newest> <base> <content_q32>      k1_poly24_capture_lag_q32 result
     chain <consumed> <base> <newest>       last_output_source_q32 at a long stream, then lag
   Numbers are decimal uint64. Output is one JSON line. */
#include "k1_poly24.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void k1_poly24_design_reference(k1_poly24_t *st);

static k1_poly24_t now_st;
static k1_poly24_t ref_st;

static double seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + 1e-9 * (double)ts.tv_nsec;
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "design") == 0) {
        const uint32_t hz = (uint32_t)strtoul(argv[2], 0, 10);
        const double t0 = seconds();
        k1_poly24_reset(&now_st, hz);
        const double t1 = seconds();
        ref_st.source_hz = now_st.source_hz;
        k1_poly24_design_reference(&ref_st);
        const double t2 = seconds();
        double worst = 0.0;
        for (uint32_t p = 0u; p <= K1_POLY24_PHASES; ++p)
            for (uint32_t j = 0u; j < K1_POLY24_TAPS; ++j) {
                const double d = fabs((double)now_st.table[p][j] - (double)ref_st.table[p][j]);
                if (d > worst) worst = d;
            }
        printf("{\"rate_hz\":%u,\"max_abs_diff\":%.9g,\"new_s\":%.9g,\"reference_s\":%.9g,"
               "\"step_equal\":%d}\n", now_st.source_hz, worst, t1 - t0, t2 - t1,
               now_st.step_q32 == ref_st.step_q32);
        return 0;
    }
    if (argc == 5 && strcmp(argv[1], "lag") == 0) {
        uint64_t lag = 0u;
        const int ok = k1_poly24_capture_lag_q32(strtoull(argv[2], 0, 10), strtoull(argv[3], 0, 10),
                                                 strtoull(argv[4], 0, 10), &lag);
        printf("{\"ok\":%d,\"lag_q32\":%" PRIu64 "}\n", ok, lag);
        return 0;
    }
    if (argc == 5 && strcmp(argv[1], "chain") == 0) {
        /* A stream that has retired `consumed` ring samples: the last output's centre
           is consumed + (pos - step) - ALIGN_ZEROS stream samples. */
        k1_poly24_reset(&now_st, 41417u);
        now_st.consumed = strtoull(argv[2], 0, 10);
        now_st.pos_q32 = ((uint64_t)(K1_POLY24_TAPS / 2u - 1u) << 32) + 0x40000000u;
        const uint64_t content = k1_poly24_last_output_source_q32(&now_st);
        uint64_t lag = 0u;
        const int ok = k1_poly24_capture_lag_q32(strtoull(argv[4], 0, 10), strtoull(argv[3], 0, 10),
                                                 content, &lag);
        printf("{\"ok\":%d,\"lag_q32\":%" PRIu64 ",\"pos_q32\":%" PRIu64 ",\"step_q32\":%" PRIu64 "}\n",
               ok, lag, now_st.pos_q32, now_st.step_q32);
        return 0;
    }
    fprintf(stderr, "usage: see header\n");
    return 2;
}
