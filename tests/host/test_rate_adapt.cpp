#include "k1_rate_adapt.h"

#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>

int main()
{
    k1_rate_adapt_t st{};
    k1_rate_adapt_reset(&st);
    assert(st.delay_samples == K1_RATE_DELAY_INPUT_SAMPLES);

    std::vector<int16_t> in(K1_RATE_IN_FRAMES * 2u);
    std::vector<int16_t> out(K1_RATE_OUT_FRAMES * 2u);
    std::vector<int16_t> out2(K1_RATE_OUT_FRAMES * 2u);
    for (uint32_t i = 0; i < K1_RATE_IN_FRAMES; ++i) {
        in[i * 2u] = (int16_t)(i * 40);
        in[i * 2u + 1u] = (int16_t)(-(int32_t)i * 40);
    }
    assert(k1_rate_adapt_block(&st, in.data(), out.data()));
    assert(st.blocks == 1u);
    int16_t first_im1 = out[94u * 2u];
    int16_t first_im2 = out[94u * 2u + 1u];
    assert(first_im1 > 0);
    assert(first_im2 < 0);

    for (uint32_t i = 0; i < K1_RATE_IN_FRAMES; ++i) {
        in[i * 2u] = 1000;
        in[i * 2u + 1u] = -1000;
    }
    assert(k1_rate_adapt_block(&st, in.data(), out2.data()));
    assert(st.blocks == 2u);
    assert(out2[0] != out[0] || out2[2] != out[2]);

    k1_rate_adapt_t cold{};
    k1_rate_adapt_reset(&cold);
    std::vector<int16_t> cold_out(K1_RATE_OUT_FRAMES * 2u);
    assert(k1_rate_adapt_block(&cold, in.data(), cold_out.data()));
    assert(std::memcmp(cold_out.data(), out2.data(),
                       K1_RATE_OUT_FRAMES * 4u) != 0);

    std::printf("K1_RATE_ADAPT=PASS in=%u out=%u delay_samples=%u delay_us=%u "
                "im1_nonneg=%d im2_nonpos=%d state_across_blocks=PASS\n",
                K1_RATE_IN_FRAMES, K1_RATE_OUT_FRAMES, K1_RATE_DELAY_INPUT_SAMPLES,
                K1_RATE_DELAY_US, first_im1 >= 0, first_im2 <= 0);
    return 0;
}
