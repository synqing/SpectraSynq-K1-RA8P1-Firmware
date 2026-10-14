/* Lane P4 host probe for k1_poly24: a tone at the true source rate, resampled
   with the rate the resampler was told, written as int16 output. */
#include "k1_poly24.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 8) {
        fprintf(stderr, "usage: probe true_hz told_hz tone_hz amp seconds chunk out.s16\n");
        return 2;
    }
    const double true_hz = atof(argv[1]);
    const uint32_t told = (uint32_t)atoi(argv[2]);
    const double tone = atof(argv[3]);
    const double amp = atof(argv[4]);
    const double seconds = atof(argv[5]);
    const uint32_t chunk = (uint32_t)atoi(argv[6]);
    FILE *out = fopen(argv[7], "wb");
    if (!out) return 3;
    static k1_poly24_t st;
    k1_poly24_reset(&st, told);
    const uint64_t total = (uint64_t)(true_hz * seconds);
    int16_t *in = malloc(sizeof(int16_t) * chunk);
    int16_t frame[K1_POLY24_OUT_FRAMES];
    uint64_t n = 0;
    while (n < total) {
        uint32_t c = 0;
        for (; c < chunk && n < total; ++c, ++n)
            in[c] = (int16_t)lrint(amp * sin(2.0 * M_PI * tone * (double)n / true_hz));
        if (k1_poly24_push(&st, in, c) != k1_poly24_ok) return 4;
        while (k1_poly24_pull180(&st, frame) == k1_poly24_ok)
            fwrite(frame, sizeof frame, 1, out);
    }
    fclose(out);
    free(in);
    printf("{\"produced\":%u,\"rejected\":%u}\n", st.produced, st.rejected);
    return 0;
}
