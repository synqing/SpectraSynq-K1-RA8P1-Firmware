#include "ws281x_gpt_dma_pair.h"

#include <string.h>

static void bind_lane_identity(k1_ws281x_gpt_dma_pair_t *pair)
{
    pair->lane_a0.gpt_timer = K1_WS281X_GPT_TIMER;
    pair->lane_a0.event_timer = K1_WS281X_GPT_EVENT_TIMER;
    pair->lane_a0.pin = K1_WS281X_GPT_PIN_P601;
    pair->lane_a1.gpt_timer = K1_WS281X_GPT_TIMER_A1;
    pair->lane_a1.event_timer = K1_WS281X_GPT_EVENT_TIMER_A1;
    pair->lane_a1.pin = K1_WS281X_GPT_PIN_P603;
}

static int either_owned(const k1_ws281x_gpt_dma_pair_t *pair)
{
    return pair->lane_a0.owned || pair->lane_a1.owned;
}

static void refresh_pair_state(k1_ws281x_gpt_dma_pair_t *pair)
{
    const uint32_t s0 = pair->lane_a0.state;
    const uint32_t s1 = pair->lane_a1.state;
    if (s0 == K1_WS281X_TX_FAULT || s1 == K1_WS281X_TX_FAULT) {
        pair->pair_state = K1_WS281X_PAIR_FAULT;
        return;
    }
    if (s0 == K1_WS281X_TX_READY && s1 == K1_WS281X_TX_READY) {
        pair->pair_state = K1_WS281X_PAIR_READY;
        pair->completed_generation = pair->submitted_generation;
        return;
    }
    if (s0 == K1_WS281X_TX_PRELOAD && s1 == K1_WS281X_TX_PRELOAD) {
        pair->pair_state = K1_WS281X_PAIR_PRELOAD;
        return;
    }
    if (s0 == K1_WS281X_TX_IDLE && s1 == K1_WS281X_TX_IDLE) {
        pair->pair_state = K1_WS281X_PAIR_IDLE;
        return;
    }
    pair->pair_state = K1_WS281X_PAIR_EMITTING;
}

static int stash_pending(k1_ws281x_gpt_dma_pair_t *pair, const uint8_t *a0,
                         const uint8_t *a1, uint32_t profile,
                         uint32_t generation, uint32_t clock_hz)
{
    memcpy(pair->pending_a0, a0, K1_WS281X_PAIR_LANE_BYTES);
    memcpy(pair->pending_a1, a1, K1_WS281X_PAIR_LANE_BYTES);
    pair->pending_valid = 1u;
    pair->pending_profile = profile;
    pair->pending_generation = generation;
    pair->pending_clock_hz = clock_hz;
    pair->pending_replacements += 1u;
    return K1_WS281X_SUBMIT_BUSY;
}

void k1_ws281x_gpt_dma_pair_reset(k1_ws281x_gpt_dma_pair_t *pair)
{
    if (pair == 0) {
        return;
    }
    memset(pair, 0, sizeof(*pair));
    k1_ws281x_gpt_dma_reset(&pair->lane_a0);
    k1_ws281x_gpt_dma_reset(&pair->lane_a1);
    bind_lane_identity(pair);
    pair->pair_state = K1_WS281X_PAIR_IDLE;
}

int k1_ws281x_gpt_dma_pair_validate(const uint8_t *lane_a0, size_t a0_bytes,
                                    const uint8_t *lane_a1, size_t a1_bytes,
                                    uint32_t profile, uint32_t clock_hz)
{
    k1_ws281x_diag_timing_t timing;
    if (lane_a0 == 0 || lane_a1 == 0) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    if (a0_bytes != K1_WS281X_PAIR_LANE_BYTES ||
        a1_bytes != K1_WS281X_PAIR_LANE_BYTES || clock_hz == 0u ||
        profile != K1_WS281X_PAIR_PROFILE_WS2816C ||
        !k1_ws281x_diag_profile(profile, &timing) ||
        timing.bytes_per_pixel != 6u) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    return K1_WS281X_SUBMIT_ACCEPTED;
}

int k1_ws281x_gpt_dma_pair_lanes_complete(uint32_t dma_a0, uint32_t dma_a1,
                                          uint32_t stop_a0, uint32_t stop_a1,
                                          uint32_t wave_a0, uint32_t wave_a1,
                                          uint32_t reset_a0, uint32_t reset_a1,
                                          uint32_t fault_a0, uint32_t fault_a1,
                                          uint32_t pair_state)
{
    if (pair_state == K1_WS281X_PAIR_FAULT) {
        return 0;
    }
    if (!dma_a0 || !dma_a1 || !stop_a0 || !stop_a1 || !wave_a0 || !wave_a1 ||
        !reset_a0 || !reset_a1) {
        return 0;
    }
    if (fault_a0 != K1_WS281X_FAULT_NONE || fault_a1 != K1_WS281X_FAULT_NONE) {
        return 0;
    }
    return pair_state == K1_WS281X_PAIR_READY;
}

int k1_ws281x_gpt_dma_pair_submit(k1_ws281x_gpt_dma_pair_t *pair,
                                  const uint8_t *lane_a0, size_t a0_bytes,
                                  const uint8_t *lane_a1, size_t a1_bytes,
                                  uint32_t profile, uint32_t generation,
                                  uint32_t clock_hz)
{
    k1_ws281x_wire_frame_t frame_a0;
    k1_ws281x_wire_frame_t frame_a1;
    int status;
    if (pair == 0) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    status = k1_ws281x_gpt_dma_pair_validate(lane_a0, a0_bytes, lane_a1,
                                             a1_bytes, profile, clock_hz);
    if (status != K1_WS281X_SUBMIT_ACCEPTED) {
        return status;
    }
    if (either_owned(pair)) {
        return stash_pending(pair, lane_a0, lane_a1, profile, generation,
                             clock_hz);
    }

    memset(&frame_a0, 0, sizeof(frame_a0));
    memset(&frame_a1, 0, sizeof(frame_a1));
    frame_a0.profile = profile;
    frame_a1.profile = profile;
    frame_a0.bytes = lane_a0;
    frame_a1.bytes = lane_a1;
    frame_a0.byte_count = a0_bytes;
    frame_a1.byte_count = a1_bytes;
    frame_a0.pixel_count = 80u;
    frame_a1.pixel_count = 80u;
    frame_a0.pin = K1_WS281X_GPT_PIN_P601;
    frame_a1.pin = K1_WS281X_GPT_PIN_P603;
    frame_a0.clock_hz = clock_hz;
    frame_a1.clock_hz = clock_hz;
    frame_a0.sequence = generation;
    frame_a1.sequence = generation;

    k1_ws281x_gpt_dma_reset(&pair->lane_a0);
    k1_ws281x_gpt_dma_reset(&pair->lane_a1);
    bind_lane_identity(pair);
    status = k1_ws281x_gpt_dma_submit(&pair->lane_a0, &frame_a0);
    if (status != K1_WS281X_SUBMIT_ACCEPTED) {
        k1_ws281x_gpt_dma_abort_low(&pair->lane_a0);
        k1_ws281x_gpt_dma_abort_low(&pair->lane_a1);
        refresh_pair_state(pair);
        return status;
    }
    status = k1_ws281x_gpt_dma_submit(&pair->lane_a1, &frame_a1);
    if (status != K1_WS281X_SUBMIT_ACCEPTED) {
        k1_ws281x_gpt_dma_abort_low(&pair->lane_a0);
        k1_ws281x_gpt_dma_abort_low(&pair->lane_a1);
        refresh_pair_state(pair);
        return status;
    }
    pair->generation = generation;
    pair->submitted_generation = generation;
    pair->pending_valid = 0u;
    refresh_pair_state(pair);
    return K1_WS281X_SUBMIT_ACCEPTED;
}

int k1_ws281x_gpt_dma_pair_submit_test_bits(k1_ws281x_gpt_dma_pair_t *pair,
                                            const uint32_t *duty_a0,
                                            const uint32_t *duty_a1,
                                            uint32_t bits, uint32_t generation,
                                            uint32_t clock_hz,
                                            uint32_t period_ns,
                                            uint32_t reset_us)
{
    int status;
    if (pair == 0 || duty_a0 == 0 || duty_a1 == 0) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    if (either_owned(pair)) {
        return K1_WS281X_SUBMIT_BUSY;
    }
    k1_ws281x_gpt_dma_reset(&pair->lane_a0);
    k1_ws281x_gpt_dma_reset(&pair->lane_a1);
    status = k1_ws281x_gpt_dma_submit_test_bits(
        &pair->lane_a0, duty_a0, bits, clock_hz, period_ns, reset_us);
    if (status != K1_WS281X_SUBMIT_ACCEPTED) {
        k1_ws281x_gpt_dma_abort_low(&pair->lane_a0);
        return status;
    }
    status = k1_ws281x_gpt_dma_submit_test_bits(
        &pair->lane_a1, duty_a1, bits, clock_hz, period_ns, reset_us);
    if (status != K1_WS281X_SUBMIT_ACCEPTED) {
        k1_ws281x_gpt_dma_abort_low(&pair->lane_a0);
        k1_ws281x_gpt_dma_abort_low(&pair->lane_a1);
        refresh_pair_state(pair);
        return status;
    }
    bind_lane_identity(pair);
    pair->lane_a0.sequence = generation;
    pair->lane_a1.sequence = generation;
    pair->generation = generation;
    pair->submitted_generation = generation;
    refresh_pair_state(pair);
    return K1_WS281X_SUBMIT_ACCEPTED;
}

int k1_ws281x_gpt_dma_pair_start(k1_ws281x_gpt_dma_pair_t *pair)
{
    if (pair == 0 || pair->pair_state != K1_WS281X_PAIR_PRELOAD) {
        return 0;
    }
    if (!k1_ws281x_gpt_dma_start(&pair->lane_a0)) {
        k1_ws281x_gpt_dma_abort_low(&pair->lane_a1);
        refresh_pair_state(pair);
        return 0;
    }
    if (!k1_ws281x_gpt_dma_start(&pair->lane_a1)) {
        k1_ws281x_gpt_dma_abort_low(&pair->lane_a0);
        refresh_pair_state(pair);
        return 0;
    }
    refresh_pair_state(pair);
    return 1;
}

int k1_ws281x_gpt_dma_pair_poll(const k1_ws281x_gpt_dma_pair_t *pair)
{
    if (pair == 0) {
        return K1_WS281X_PAIR_FAULT;
    }
    return (int)pair->pair_state;
}

int k1_ws281x_gpt_dma_pair_step_overflow(k1_ws281x_gpt_dma_pair_t *pair)
{
    int moved = 0;
    if (pair == 0) {
        return 0;
    }
    moved |= k1_ws281x_gpt_dma_step_overflow(&pair->lane_a0);
    moved |= k1_ws281x_gpt_dma_step_overflow(&pair->lane_a1);
    if (pair->lane_a0.state == K1_WS281X_TX_FAULT ||
        pair->lane_a1.state == K1_WS281X_TX_FAULT) {
        if (pair->lane_a0.state != K1_WS281X_TX_FAULT) {
            k1_ws281x_gpt_dma_abort_low(&pair->lane_a0);
            pair->lane_a0.state = K1_WS281X_TX_FAULT;
            pair->lane_a0.fault = pair->lane_a1.fault;
        }
        if (pair->lane_a1.state != K1_WS281X_TX_FAULT) {
            k1_ws281x_gpt_dma_abort_low(&pair->lane_a1);
            pair->lane_a1.state = K1_WS281X_TX_FAULT;
            pair->lane_a1.fault = pair->lane_a0.fault;
        }
    }
    refresh_pair_state(pair);
    return moved;
}

void k1_ws281x_gpt_dma_pair_abort(k1_ws281x_gpt_dma_pair_t *pair)
{
    if (pair == 0) {
        return;
    }
    k1_ws281x_gpt_dma_abort_low(&pair->lane_a0);
    k1_ws281x_gpt_dma_abort_low(&pair->lane_a1);
    pair->pending_valid = 0u;
    refresh_pair_state(pair);
}

int k1_ws281x_gpt_dma_pair_complete(const k1_ws281x_gpt_dma_pair_t *pair)
{
    if (pair == 0) {
        return 0;
    }
    return k1_ws281x_gpt_dma_pair_lanes_complete(
        pair->lane_a0.dma_complete, pair->lane_a1.dma_complete,
        pair->lane_a0.hw_stopped, pair->lane_a1.hw_stopped,
        pair->lane_a0.waveform_complete, pair->lane_a1.waveform_complete,
        pair->lane_a0.reset_ready, pair->lane_a1.reset_ready,
        pair->lane_a0.fault, pair->lane_a1.fault, pair->pair_state);
}
