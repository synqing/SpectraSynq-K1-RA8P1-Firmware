#include "pcm1808_core.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>

static uint32_t pack_s24(int32_t value) { return (uint32_t)value & 0x00ffffffu; }

int main() {
    assert(k1_pcm1808_ssi_word_to_s24(0x00000000u) == 0);
    assert(k1_pcm1808_ssi_word_to_s24(0x007fffffu) == 8388607);
    assert(k1_pcm1808_ssi_word_to_s24(0x00800000u) == -8388608);
    assert(k1_pcm1808_ssi_word_to_s24(0x00ffffffu) == -1);

    std::vector<uint64_t> storage((k1_pcm1808_core_state_bytes() + sizeof(uint64_t) - 1u) /
                                  sizeof(uint64_t));
    auto *state = reinterpret_cast<k1_pcm1808_core_t *>(storage.data());
    k1_pcm1808_core_reset(state);
    uint32_t input[K1_PCM1808_WORDS_PER_HOP] = {};
    int16_t output[K1_PCM1808_OUTPUT_SAMPLES_PER_HOP] = {};
    assert(k1_pcm1808_make_canonical_hop(state, input, 1, output, 96) == -1);

    double energy = 0.0;
    for (int hop = 0; hop < 10; ++hop) {
        for (size_t frame = 0; frame < K1_PCM1808_INPUT_FRAMES_PER_HOP; ++frame) {
            const int n = hop * (int)K1_PCM1808_INPUT_FRAMES_PER_HOP + (int)frame;
            const int32_t tone = (int32_t)lrint(1000000.0 * sin(2.0 * 3.141592653589793 * 997.0 * n / 48000.0));
            input[frame * 2u] = pack_s24(tone);
            input[frame * 2u + 1u] = pack_s24(tone);
        }
        assert(k1_pcm1808_make_canonical_hop(state, input, K1_PCM1808_WORDS_PER_HOP,
                                              output, K1_PCM1808_OUTPUT_SAMPLES_PER_HOP) == 96);
        if (hop >= 2) for (int16_t value : output) energy += (double)value * value;
    }
    assert(energy > 1000000.0);
    return 0;
}
