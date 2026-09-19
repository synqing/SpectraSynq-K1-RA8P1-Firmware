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

static k1_pdm_stream_t default_stream;

static void k1_pdm_stream_release_fence(void) {
    __atomic_thread_fence(__ATOMIC_RELEASE);
}

static void k1_pdm_stream_acquire_fence(void) {
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

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

static void k1_pdm_stream_clear_slots(k1_pdm_stream_t *stream) {
    uint32_t index;
    for (index = 0; index < K1_PDM_STREAM_SLOT_COUNT; ++index) {
        stream->slots[index].state = K1_PDM_SLOT_FREE;
        stream->slots[index].received = 0u;
        stream->slots[index].epoch = 0u;
        stream->slots[index].sequence = 0u;
        stream->slots[index].capture_start_us = 0u;
        stream->slots[index].capture_end_us = 0u;
    }
    stream->active_slot = K1_PDM_STREAM_NO_SLOT;
}

static uint32_t k1_pdm_stream_find_free(k1_pdm_stream_t *stream) {
    uint32_t index;
    for (index = 0; index < K1_PDM_STREAM_SLOT_COUNT; ++index) {
        if (stream->slots[index].state == K1_PDM_SLOT_FREE) return index;
    }
    return K1_PDM_STREAM_NO_SLOT;
}

static void k1_pdm_stream_begin_slot(k1_pdm_stream_t *stream,
                                     uint32_t slot,
                                     uint64_t capture_start_us) {
    stream->slots[slot].state = K1_PDM_SLOT_FILLING;
    stream->slots[slot].received = 0u;
    stream->slots[slot].epoch = stream->epoch;
    stream->slots[slot].sequence = 0u;
    stream->slots[slot].capture_start_us = capture_start_us;
    stream->slots[slot].capture_end_us = 0u;
    stream->active_slot = slot;
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

int k1_pdm_stream_context_configure(k1_pdm_stream_t *stream,
                                    uint32_t elements_per_slot,
                                    uint32_t interval_elements) {
    if (stream == NULL) return K1_PDM_STREAM_INVALID;
    stream->configured = 0;
    stream->running = 0;
    stream->halted = 0;
    stream->elements_per_slot = 0u;
    stream->interval_elements = 0u;
    stream->epoch = 0u;
    stream->next_sequence = 0u;
    stream->completed_slots = 0u;
    stream->overflow_events = 0u;
    stream->drop_events = 0u;
    stream->recovery_count = 0u;
    k1_pdm_stream_clear_slots(stream);
    if (elements_per_slot == 0u ||
        elements_per_slot > K1_PDM_MAX_REQUESTED_FRAMES ||
        interval_elements == 0u ||
        interval_elements > elements_per_slot ||
        (elements_per_slot % interval_elements) != 0u) {
        return K1_PDM_STREAM_INVALID;
    }
    stream->elements_per_slot = elements_per_slot;
    stream->interval_elements = interval_elements;
    stream->configured = 1;
    return K1_PDM_STREAM_OK;
}

int k1_pdm_stream_context_start(k1_pdm_stream_t *stream,
                                uint64_t capture_start_us) {
    if (stream == NULL || !stream->configured || stream->running) {
        return K1_PDM_STREAM_INVALID;
    }
    k1_pdm_stream_clear_slots(stream);
    stream->epoch++;
    if (stream->epoch == 0u) stream->epoch = 1u;
    stream->halted = 0;
    stream->running = 1;
    k1_pdm_stream_begin_slot(stream, 0u, capture_start_us);
    return K1_PDM_STREAM_OK;
}

int k1_pdm_stream_context_on_data(k1_pdm_stream_t *stream,
                                  uint32_t interval_elements,
                                  uint64_t capture_end_us) {
    uint32_t next_slot;
    k1_pdm_stream_slot_t *slot;
    if (stream == NULL || !stream->configured || !stream->running || stream->halted ||
        stream->active_slot >= K1_PDM_STREAM_SLOT_COUNT ||
        interval_elements != stream->interval_elements) {
        return K1_PDM_STREAM_INVALID;
    }
    slot = &stream->slots[stream->active_slot];
    if (slot->state != K1_PDM_SLOT_FILLING ||
        capture_end_us < slot->capture_start_us ||
        interval_elements > stream->elements_per_slot - slot->received) {
        return K1_PDM_STREAM_INVALID;
    }
    slot->received += interval_elements;
    if (slot->received != stream->elements_per_slot) return K1_PDM_STREAM_OK;

    stream->next_sequence++;
    if (stream->next_sequence == 0u) stream->next_sequence = 1u;
    slot->epoch = stream->epoch;
    slot->sequence = stream->next_sequence;
    slot->capture_end_us = capture_end_us;
    k1_pdm_stream_release_fence();
    slot->state = K1_PDM_SLOT_READY;
    stream->completed_slots++;

    next_slot = k1_pdm_stream_find_free(stream);
    if (next_slot == K1_PDM_STREAM_NO_SLOT) {
        stream->active_slot = K1_PDM_STREAM_NO_SLOT;
        stream->running = 0;
        stream->halted = 1;
        stream->overflow_events++;
        stream->drop_events++;
        return K1_PDM_STREAM_OVERFLOW;
    }
    k1_pdm_stream_begin_slot(stream, next_slot, capture_end_us);
    return K1_PDM_STREAM_SLOT_READY;
}

int k1_pdm_stream_context_acquire(k1_pdm_stream_t *stream,
                                  uint32_t *slot,
                                  uint32_t *epoch,
                                  uint32_t *sequence,
                                  uint64_t *capture_start_us,
                                  uint64_t *capture_end_us) {
    uint32_t index;
    uint32_t selected = K1_PDM_STREAM_NO_SLOT;
    if (stream == NULL || slot == NULL || epoch == NULL || sequence == NULL ||
        capture_start_us == NULL || capture_end_us == NULL) {
        return K1_PDM_STREAM_INVALID;
    }
    for (index = 0; index < K1_PDM_STREAM_SLOT_COUNT; ++index) {
        if (stream->slots[index].state == K1_PDM_SLOT_READY &&
            (selected == K1_PDM_STREAM_NO_SLOT ||
             stream->slots[index].sequence < stream->slots[selected].sequence)) {
            selected = index;
        }
    }
    if (selected == K1_PDM_STREAM_NO_SLOT) return K1_PDM_STREAM_INVALID;
    k1_pdm_stream_acquire_fence();
    stream->slots[selected].state = K1_PDM_SLOT_CONSUMER;
    *slot = selected;
    *epoch = stream->slots[selected].epoch;
    *sequence = stream->slots[selected].sequence;
    *capture_start_us = stream->slots[selected].capture_start_us;
    *capture_end_us = stream->slots[selected].capture_end_us;
    return K1_PDM_STREAM_OK;
}

int k1_pdm_stream_context_release(k1_pdm_stream_t *stream,
                                  uint32_t slot,
                                  uint32_t epoch,
                                  uint32_t sequence) {
    if (stream == NULL || slot >= K1_PDM_STREAM_SLOT_COUNT ||
        stream->slots[slot].state != K1_PDM_SLOT_CONSUMER ||
        stream->slots[slot].epoch != epoch ||
        stream->slots[slot].sequence != sequence) {
        return K1_PDM_STREAM_STALE_OWNER;
    }
    stream->slots[slot].received = 0u;
    stream->slots[slot].capture_start_us = 0u;
    stream->slots[slot].capture_end_us = 0u;
    k1_pdm_stream_release_fence();
    stream->slots[slot].state = K1_PDM_SLOT_FREE;
    return K1_PDM_STREAM_OK;
}

void k1_pdm_stream_context_stop(k1_pdm_stream_t *stream) {
    if (stream == NULL || !stream->configured) return;
    if (stream->active_slot < K1_PDM_STREAM_SLOT_COUNT &&
        stream->slots[stream->active_slot].state == K1_PDM_SLOT_FILLING) {
        stream->slots[stream->active_slot].received = 0u;
        k1_pdm_stream_release_fence();
        stream->slots[stream->active_slot].state = K1_PDM_SLOT_FREE;
    }
    stream->active_slot = K1_PDM_STREAM_NO_SLOT;
    stream->running = 0;
}

int k1_pdm_stream_context_recover(k1_pdm_stream_t *stream,
                                  uint64_t capture_start_us) {
    uint32_t index;
    if (stream == NULL || !stream->configured || stream->running) {
        return K1_PDM_STREAM_INVALID;
    }
    for (index = 0; index < K1_PDM_STREAM_SLOT_COUNT; ++index) {
        if (stream->slots[index].state == K1_PDM_SLOT_CONSUMER) {
            return K1_PDM_STREAM_STALE_OWNER;
        }
    }
    k1_pdm_stream_clear_slots(stream);
    stream->epoch++;
    if (stream->epoch == 0u) stream->epoch = 1u;
    stream->halted = 0;
    stream->running = 1;
    stream->recovery_count++;
    k1_pdm_stream_begin_slot(stream, 0u, capture_start_us);
    return K1_PDM_STREAM_OK;
}

int k1_pdm_stream_context_filling_slot(const k1_pdm_stream_t *stream,
                                       uint32_t *slot) {
    if (stream == NULL || slot == NULL || !stream->configured || !stream->running ||
        stream->halted || stream->active_slot >= K1_PDM_STREAM_SLOT_COUNT) {
        return K1_PDM_STREAM_INVALID;
    }
    if (stream->slots[stream->active_slot].state != K1_PDM_SLOT_FILLING) {
        return K1_PDM_STREAM_OVERFLOW;
    }
    *slot = stream->active_slot;
    return K1_PDM_STREAM_OK;
}

int k1_pdm_stream_configure(uint32_t elements_per_slot,
                            uint32_t interval_elements) {
    return k1_pdm_stream_context_configure(&default_stream, elements_per_slot,
                                           interval_elements);
}
int k1_pdm_stream_start(uint64_t capture_start_us) {
    return k1_pdm_stream_context_start(&default_stream, capture_start_us);
}
int k1_pdm_stream_on_data(uint32_t interval_elements,
                          uint64_t capture_end_us) {
    return k1_pdm_stream_context_on_data(&default_stream, interval_elements,
                                         capture_end_us);
}
int k1_pdm_stream_acquire(uint32_t *slot, uint32_t *epoch, uint32_t *sequence,
                          uint64_t *capture_start_us, uint64_t *capture_end_us) {
    return k1_pdm_stream_context_acquire(&default_stream, slot, epoch, sequence,
                                         capture_start_us, capture_end_us);
}
int k1_pdm_stream_release(uint32_t slot, uint32_t epoch, uint32_t sequence) {
    return k1_pdm_stream_context_release(&default_stream, slot, epoch, sequence);
}
void k1_pdm_stream_stop(void) {
    k1_pdm_stream_context_stop(&default_stream);
}
int k1_pdm_stream_recover(uint64_t capture_start_us) {
    return k1_pdm_stream_context_recover(&default_stream, capture_start_us);
}

uint32_t k1_pdm_stream_active_slot(void) { return default_stream.active_slot; }
uint32_t k1_pdm_stream_slot_state(uint32_t slot) {
    return slot < K1_PDM_STREAM_SLOT_COUNT ? default_stream.slots[slot].state : K1_PDM_STREAM_NO_SLOT;
}
uint32_t k1_pdm_stream_slot_received(uint32_t slot) {
    return slot < K1_PDM_STREAM_SLOT_COUNT ? default_stream.slots[slot].received : 0u;
}
uint32_t k1_pdm_stream_epoch(void) { return default_stream.epoch; }
uint32_t k1_pdm_stream_completed_slots(void) { return default_stream.completed_slots; }
uint32_t k1_pdm_stream_overflow_events(void) { return default_stream.overflow_events; }
uint32_t k1_pdm_stream_drop_events(void) { return default_stream.drop_events; }
uint32_t k1_pdm_stream_recovery_count(void) { return default_stream.recovery_count; }
int k1_pdm_stream_configured(void) { return default_stream.configured; }
int k1_pdm_stream_running(void) { return default_stream.running; }
int k1_pdm_stream_halted(void) { return default_stream.halted; }

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
