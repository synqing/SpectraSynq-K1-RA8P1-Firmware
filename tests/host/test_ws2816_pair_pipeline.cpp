#include "core/visual/ws2816_pack.h"
#include "ws281x_gpt_dma_pair.h"

#include <cassert>
#include <cstdio>
#include <cstring>

int main()
{
    k1::core::visual::Pixel16 pixels[160]{};
    pixels[0] = {0x12AB, 0x34CD, 0x56EF};
    pixels[79].green = 0x0100;
    pixels[80].blue = 0x0001;
    pixels[159] = {0x00FF, 0x0000, 0x0000};
    uint8_t packed_a0[480];
    uint8_t packed_a1[480];
    uint8_t again_a0[480];
    uint8_t again_a1[480];
    assert(k1::core::visual::splitChannel160(pixels, 160, packed_a0, packed_a1));
    assert(k1::core::visual::splitChannel160(pixels, 160, again_a0, again_a1));
    assert(std::memcmp(packed_a0, again_a0, 480) == 0);
    assert(std::memcmp(packed_a1, again_a1, 480) == 0);
    assert(packed_a0[0] == 0x34 && packed_a0[5] == 0xEF);
    assert(packed_a0[79 * 6] == 0x01);
    assert(packed_a1[5] == 0x01);
    assert(packed_a1[159u % 80u * 6u + 2u] == 0x00);
    assert(packed_a1[79 * 6 + 2] == 0x00 && packed_a1[79 * 6 + 3] == 0xFF);

    k1_ws281x_gpt_dma_pair_t pair{};
    k1_ws281x_gpt_dma_pair_reset(&pair);
    assert(k1_ws281x_gpt_dma_pair_submit(&pair, packed_a0, 480, packed_a1, 480,
                                         3u, 7u, 100000000u) ==
           K1_WS281X_SUBMIT_ACCEPTED);
    assert(pair.generation == 7u);
    assert(pair.lane_a0.bits == 3840u);
    const uint32_t first = pair.lane_a0.duty[0];
    pixels[0].red = 0xFFFF;
    uint8_t mutated[480];
    uint8_t unused[480];
    assert(k1::core::visual::splitChannel160(pixels, 160, mutated, unused));
    assert(k1_ws281x_gpt_dma_pair_submit(&pair, mutated, 480, packed_a1, 480, 3u,
                                         8u, 100000000u) ==
           K1_WS281X_SUBMIT_BUSY);
    assert(pair.lane_a0.duty[0] == first);
    assert(std::memcmp(packed_a0, again_a0, 480) == 0);

    std::puts("K1_WS2816_PAIR_PIPELINE=PASS pack_once=PASS centre=79/80 "
              "low_byte=PASS gen_match=PASS no_second_scale=PASS");
    return 0;
}
