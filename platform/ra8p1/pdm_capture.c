#include "pdm_capture.h"

static uint32_t requested_frames;
static uint32_t capture_channels;
static uint32_t capture_element_bytes;
static uint32_t output_channels;
static uint32_t output_element_bytes;
static uint32_t callback_interval;
static uint32_t received_elements;
static uint32_t final_stopped_count;
static int configured;
static int running;
static int first_data_seen;
static int full_buffer_complete;
static int stop_requested;
static int stopped;
static int ownership_transferred;

static uint32_t k1_pdm_mul(uint32_t a, uint32_t b) {
    if (b != 0u && a > (0xFFFFFFFFu / b)) return 0u;
    return a * b;
}

static uint32_t k1_pdm_product3(uint32_t a, uint32_t b, uint32_t c) {
    const uint32_t ab = k1_pdm_mul(a, b);
    if (a != 0u && b != 0u && ab == 0u) return 0u;
    const uint32_t abc = k1_pdm_mul(ab, c);
    if (ab != 0u && c != 0u && abc == 0u) return 0u;
    return abc;
}

static int16_t k1_pdm_to_int16(int32_t sample) {
    const int32_t shifted = sample >> 16;
    if (shifted > 32767) return (int16_t)32767;
    if (shifted < -32768) return (int16_t)-32768;
    return (int16_t)shifted;
}

int k1_pdm_configure(uint32_t frames,
                     uint32_t in_channels,
                     uint32_t in_elem,
                     uint32_t out_channels,
                     uint32_t out_elem,
                     uint32_t interval) {
    configured = 0;
    running = 0;
    first_data_seen = 0;
    full_buffer_complete = 0;
    stop_requested = 0;
    stopped = 0;
    ownership_transferred = 0;
    received_elements = 0;
    final_stopped_count = 0;
    requested_frames = 0;
    capture_channels = 0;
    capture_element_bytes = 0;
    output_channels = 0;
    output_element_bytes = 0;
    callback_interval = 0;
    if (frames == 0u || frames > K1_PDM_MAX_REQUESTED_FRAMES) return -1;
    if (in_channels != 1u) return -1;
    if (in_elem != 4u) return -1;
    if (out_channels != 1u && out_channels != 2u) return -1;
    if (out_elem != 2u) return -1;
    if (interval == 0u || interval > frames) return -1;
    if (k1_pdm_product3(frames, in_channels, in_elem) == 0u) return -1;
    if (k1_pdm_product3(frames, out_channels, out_elem) == 0u) return -1;
    requested_frames = frames;
    capture_channels = in_channels;
    capture_element_bytes = in_elem;
    output_channels = out_channels;
    output_element_bytes = out_elem;
    callback_interval = interval;
    configured = 1;
    return 0;
}

void k1_pdm_start(void) {
    if (!configured) return;
    running = 1;
    first_data_seen = 0;
    full_buffer_complete = 0;
    stop_requested = 0;
    stopped = 0;
    ownership_transferred = 0;
    received_elements = 0;
    final_stopped_count = 0;
}

int k1_pdm_on_data(uint32_t interval_elements) {
    if (!configured || !running || stopped || interval_elements == 0u) return -1;
    if (received_elements >= requested_frames)
        received_elements = requested_frames;
    else if (interval_elements >= requested_frames - received_elements)
        received_elements = requested_frames;
    else
        received_elements += interval_elements;
    first_data_seen = 1;
    /* First DATA is recorded; completion is only the cumulative count. */
    if (received_elements >= requested_frames) {
        received_elements = requested_frames;
        full_buffer_complete = 1;
    }
    return 0;
}

void k1_pdm_request_stop(void) {
    if (!configured || !running) return;
    stop_requested = 1;
}

int k1_pdm_on_stopped(uint32_t driver_count) {
    if (!configured || !running || !stop_requested || stopped) return -1;
    stopped = 1;
    running = 0;
    final_stopped_count = driver_count;
    if (full_buffer_complete && driver_count == requested_frames)
        ownership_transferred = 1;
    else
        ownership_transferred = 0;
    return 0;
}

int k1_pdm_convert(const int32_t *capture, size_t capture_count,
                   int16_t *output, size_t output_count) {
    size_t frame;
    if (!ownership_transferred || capture == NULL || output == NULL) return -1;
    if (capture_count != (size_t)requested_frames) return -1;
    if (output_count != (size_t)k1_pdm_mul(requested_frames, output_channels)) return -1;
    for (frame = 0; frame < (size_t)requested_frames; ++frame) {
        const int16_t sample = k1_pdm_to_int16(capture[frame]);
        if (output_channels == 1u) {
            output[frame] = sample;
        } else {
            output[frame * 2u] = sample;
            output[frame * 2u + 1u] = sample;
        }
    }
    return 0;
}

uint32_t k1_pdm_requested_frames(void) { return requested_frames; }
uint32_t k1_pdm_capture_channels(void) { return capture_channels; }
uint32_t k1_pdm_capture_element_bytes(void) { return capture_element_bytes; }
uint32_t k1_pdm_output_channels(void) { return output_channels; }
uint32_t k1_pdm_output_element_bytes(void) { return output_element_bytes; }
uint32_t k1_pdm_callback_interval(void) { return callback_interval; }
uint32_t k1_pdm_capture_bytes(void) {
    return k1_pdm_product3(requested_frames, capture_channels, capture_element_bytes);
}
uint32_t k1_pdm_conversion_bytes(void) {
    return k1_pdm_product3(requested_frames, output_channels, output_element_bytes);
}
uint32_t k1_pdm_submission_bytes(void) {
    /* Corrected: full converted duration, not the vendor half-length playback. */
    return k1_pdm_conversion_bytes();
}
uint32_t k1_pdm_received_elements(void) { return received_elements; }
uint32_t k1_pdm_final_stopped_count(void) { return final_stopped_count; }
int k1_pdm_configured(void) { return configured; }
int k1_pdm_running(void) { return running; }
int k1_pdm_first_data_seen(void) { return first_data_seen; }
int k1_pdm_first_data_does_not_prove_complete(void) { return 1; }
int k1_pdm_full_buffer_complete(void) { return full_buffer_complete; }
int k1_pdm_stop_requested(void) { return stop_requested; }
int k1_pdm_stopped(void) { return stopped; }
int k1_pdm_stop_tail_known(void) { return 0; }
int k1_pdm_ownership_transferred(void) { return ownership_transferred; }
