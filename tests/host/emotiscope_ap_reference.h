// Independent compile of the pinned donor AP headers, not the Titan wrapper.
// The runner supplies pristine headers, with the separately recorded bounds fix.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace emotiscope_reference {
#define NUM_LEDS 128
#define NUM_FREQS 64
#define MAX_COMMAND_LENGTH 256
#define SAMPLE_HISTORY_LENGTH 4096
#define CHUNK_SIZE 64
#define SAMPLE_RATE 12800
#define NOVELTY_HISTORY_LENGTH 1024
#define NOVELTY_LOG_HZ 50
#define NUM_TEMPI 96
#define TEMPO_LOW 48
#define TEMPO_HIGH (TEMPO_LOW + NUM_TEMPI)
#define BEAT_SHIFT_PERCENT 0.16
#define REFERENCE_FPS 100
#define PI 3.1415926535897932384626433832795
#define IRAM_ATTR
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
template<class F> void profile_function(F f, const char*) { f(); }
inline void ___() {}
inline void broadcast(const char*) {}
inline void save_config() {}
inline void save_noise_spectrum() {}
struct QuietSerial { template<class T> void println(T) {} };
QuietSerial Serial;
uint32_t t_now_us = 0, t_now_ms = 0;
float sample_history[SAMPLE_HISTORY_LENGTH] = {};
float noise_spectrum[NUM_FREQS] = {};
inline float clip_float(float x) { return min(1.0f, max(0.0f, x)); }
inline void dsps_mulc_f32(const float* input, float* output, int n,
                          float gain, int in_step, int out_step) {
    for (int i = 0; i < n; ++i) output[i*out_step] = input[i*in_step]*gain;
}
inline void shift_array_left(float* a, uint16_t n, uint16_t shift) {
    if (shift >= n) { std::memset(a, 0, n*sizeof(float)); return; }
    std::memmove(a, a+shift, (n-shift)*sizeof(float));
    std::memset(a+n-shift, 0, shift*sizeof(float));
}
#include "reference_types.h"
#include "reference_goertzel.h"
#include "reference_vu.h"
#include "reference_tempo.h"
uint32_t last_render_us = 0;
bool rendered_once = false;
inline void init() {
    init_window_lookup();
    init_goertzel_constants_musical();
    init_tempo_goertzel_constants();
    init_vu();
}
inline void push_pcm64(const float* pcm, uint32_t now_us) {
    (void)now_us; // Donor AP sees the last GPU-published t_now values.
    std::memmove(sample_history, sample_history+64, (4096-64)*sizeof(float));
    std::memcpy(sample_history+4096-64, pcm, 64*sizeof(float));
    calculate_magnitudes();
    get_chromagram();
    run_vu();
    update_tempo();
}
inline void render_step(uint32_t now_us) {
    t_now_us = now_us; t_now_ms = now_us/1000u;
    const float delta = rendered_once ? float(now_us-last_render_us)/10000.0f : 0.0f;
    rendered_once = true;
    last_render_us = now_us;
    update_novelty();
    update_tempi_phase(delta);
}
#undef max
#undef min
#undef NUM_LEDS
#undef NUM_FREQS
#undef MAX_COMMAND_LENGTH
#undef SAMPLE_HISTORY_LENGTH
#undef CHUNK_SIZE
#undef SAMPLE_RATE
#undef NOVELTY_HISTORY_LENGTH
#undef NOVELTY_LOG_HZ
#undef NUM_TEMPI
#undef TEMPO_LOW
#undef TEMPO_HIGH
#undef BEAT_SHIFT_PERCENT
#undef REFERENCE_FPS
#undef PI
#undef IRAM_ATTR
}
