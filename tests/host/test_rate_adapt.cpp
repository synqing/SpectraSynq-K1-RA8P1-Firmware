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
    assert(st.delay_samples == 1u);

    std::vector<int16_t> in(K1_RATE_IN_FRAMES * 2u, 0);
    std::vector<int16_t> out(K1_RATE_OUT_FRAMES * 2u, 0);
    in[0] = 32000;
    in[1] = -32000;
    assert(k1_rate_adapt_block(&st, in.data(), out.data()));
    assert(st.blocks == 1u);
    assert(out[0] > 0);
    assert(out[1] < 0);
    for (uint32_t i = 1; i < K1_RATE_OUT_FRAMES; ++i) {
        assert(out[i * 2u + 1u] <= 0);
    }

    std::vector<int16_t> dc(K1_RATE_IN_FRAMES * 2u, 0);
    for (uint32_t i = 0; i < K1_RATE_IN_FRAMES; ++i) {
        dc[i * 2u] = 8000;
        dc[i * 2u + 1u] = -8000;
    }
    std::vector<int16_t> warm(K1_RATE_OUT_FRAMES * 2u, 0);
    assert(k1_rate_adapt_block(&st, dc.data(), warm.data()));

    k1_rate_adapt_t cold{};
    k1_rate_adapt_reset(&cold);
    std::vector<int16_t> cold_out(K1_RATE_OUT_FRAMES * 2u, 0);
    assert(k1_rate_adapt_block(&cold, dc.data(), cold_out.data()));
    assert(std::memcmp(cold_out.data(), warm.data(),
                       K1_RATE_OUT_FRAMES * 4u) != 0);

    k1_rate_adapt_t ident{};
    k1_rate_adapt_reset(&ident);
    std::vector<int16_t> only_im1(K1_RATE_IN_FRAMES * 2u, 0);
    only_im1[10] = 20000;
    std::vector<int16_t> ident_out(K1_RATE_OUT_FRAMES * 2u, 0);
    assert(k1_rate_adapt_block(&ident, only_im1.data(), ident_out.data()));
    int32_t energy_im2 = 0;
    for (uint32_t i = 0; i < K1_RATE_OUT_FRAMES; ++i) {
        energy_im2 += ident_out[i * 2u + 1u] * ident_out[i * 2u + 1u];
    }
    assert(energy_im2 == 0);

    std::printf("K1_RATE_ADAPT=PASS in=%u out=%u delay_samples=%u delay_us=%u "
                "im1_pos=%d im2_neg=%d state_across_blocks=PASS "
                "channel_identity=PASS\n",
                K1_RATE_IN_FRAMES, K1_RATE_OUT_FRAMES,
                K1_RATE_DELAY_INPUT_SAMPLES, K1_RATE_DELAY_US, out[0] > 0,
                out[1] < 0);
    return 0;
}
