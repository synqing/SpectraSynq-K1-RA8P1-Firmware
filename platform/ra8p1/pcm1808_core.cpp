#include "pcm1808_core.h"

#include <math.h>
#include <string.h>

#include "pcm1808/k1_pcm1808_map.h"
#include "pcm1808/k1_pcm1808_unpack.h"
#include "pcm1808/k1_resample_48k_to_12k8.h"

struct k1_pcm1808_core {
    K1PcmSrc resampler;
    float input[K1_PCM1808_INPUT_FRAMES_PER_HOP];
};

size_t k1_pcm1808_core_state_bytes(void) { return sizeof(k1_pcm1808_core_t); }

void k1_pcm1808_core_reset(k1_pcm1808_core_t *state) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
    k1_pcm_src_reset(&state->resampler);
}

int32_t k1_pcm1808_ssi_word_to_s24(uint32_t word) {
    // FSP r_ssi sets SSICR.PDTA for 18/20/22/24-bit PCM, so SSIFRDR data is
    // right-justified. This is the RA8P1 FIFO layout, not an ESP word-layout
    // assumption copied from the donor.
    const uint32_t sample = word & 0x00ffffffu;
    return (int32_t)((sample & 0x00800000u) ? (sample | 0xff000000u) : sample);
}

int k1_pcm1808_make_canonical_hop(k1_pcm1808_core_t *state,
                                  const uint32_t *stereo_words,
                                  size_t word_count,
                                  int16_t *output,
                                  size_t output_capacity) {
    if (!state || !stereo_words || !output ||
        word_count != K1_PCM1808_WORDS_PER_HOP ||
        output_capacity < K1_PCM1808_OUTPUT_SAMPLES_PER_HOP) return -1;

    for (size_t frame = 0; frame < K1_PCM1808_INPUT_FRAMES_PER_HOP; ++frame) {
        const int32_t slot0 = k1_pcm1808_ssi_word_to_s24(stereo_words[frame * 2u]);
        const int32_t slot1 = k1_pcm1808_ssi_word_to_s24(stereo_words[frame * 2u + 1u]);
        state->input[frame] = (float)k1_pcm_physical_mid(slot0, slot1);
    }

    float resampled[K1_PCM1808_OUTPUT_SAMPLES_PER_HOP];
    k1_pcm_src_hop360(&state->resampler, state->input, resampled);
    for (size_t i = 0; i < K1_PCM1808_OUTPUT_SAMPLES_PER_HOP; ++i) {
        const int64_t sample = (int64_t)lrintf(resampled[i]);
        const int32_t trimmed = (int32_t)((sample * K1_PCM1808_CANONICAL_TRIM_Q15) / 32768LL);
        output[i] = k1_pcm1808_s24_to_s16(trimmed);
    }
    return (int)K1_PCM1808_OUTPUT_SAMPLES_PER_HOP;
}
