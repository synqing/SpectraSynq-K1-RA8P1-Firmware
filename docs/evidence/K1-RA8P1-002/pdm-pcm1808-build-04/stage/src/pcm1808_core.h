#pragma once

#include <stddef.h>
#include <stdint.h>

#define K1_PCM1808_INPUT_RATE_HZ 48000u
#define K1_PCM1808_CANONICAL_RATE_HZ 12800u
#define K1_PCM1808_INPUT_FRAMES_PER_HOP 360u
#define K1_PCM1808_OUTPUT_SAMPLES_PER_HOP 96u
#define K1_PCM1808_WORDS_PER_HOP (K1_PCM1808_INPUT_FRAMES_PER_HOP * 2u)
#define K1_PCM1808_CANONICAL_TRIM_Q15 2048

#ifdef __cplusplus
extern "C" {
#endif

typedef struct k1_pcm1808_core k1_pcm1808_core_t;

size_t k1_pcm1808_core_state_bytes(void);
void k1_pcm1808_core_reset(k1_pcm1808_core_t *state);
int32_t k1_pcm1808_ssi_word_to_s24(uint32_t word);
int k1_pcm1808_make_canonical_hop(k1_pcm1808_core_t *state,
                                  const uint32_t *stereo_words,
                                  size_t word_count,
                                  int16_t *output,
                                  size_t output_capacity);

#ifdef __cplusplus
}
#endif
