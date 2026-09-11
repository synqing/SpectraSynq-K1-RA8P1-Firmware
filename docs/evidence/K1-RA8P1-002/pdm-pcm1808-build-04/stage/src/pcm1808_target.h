#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int k1_pcm1808_target_initialise(void);
void k1_pcm1808_target_poll(void);
int k1_pcm1808_target_initialised(void);
int k1_pcm1808_target_running(void);
size_t k1_pcm1808_target_metrics(char *output, size_t capacity);

#ifdef __cplusplus
}
#endif
