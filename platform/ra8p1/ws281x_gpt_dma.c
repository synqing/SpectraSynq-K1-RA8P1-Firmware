#include "ws281x_gpt_dma.h"

#include <string.h>

int k1_ws281x_gpt_pin_can_pwm(uint32_t pin)
{
    return pin == K1_WS281X_GPT_PIN_P601;
}

void k1_ws281x_gpt_dma_reset(k1_ws281x_gpt_dma_t *tx)
{
    if (tx == 0) {
        return;
    }
    memset(tx, 0, sizeof(*tx));
    tx->state = K1_WS281X_TX_IDLE;
    tx->gpt_timer = K1_WS281X_GPT_TIMER;
    tx->event_timer = K1_WS281X_GPT_EVENT_TIMER;
    tx->pin = K1_WS281X_GPT_PIN_P601;
    tx->stop_armed = 1u;
}

static void fault(k1_ws281x_gpt_dma_t *tx, uint32_t code)
{
    tx->fault = code;
    tx->state = K1_WS281X_TX_FAULT;
    tx->pin_high = 0u;
    tx->owned = 0u;
}

static void preload(k1_ws281x_gpt_dma_t *tx, uint32_t reset_us, uint32_t period_ns)
{
    tx->gtccra = tx->duty[0];
    tx->gtccrc = tx->bits > 1u ? tx->duty[1] : tx->duty[0];
    tx->dma_index = 2u;
    tx->dma_remaining = tx->bits > 2u ? tx->bits - 2u : 0u;
    tx->dma_complete = tx->dma_remaining == 0u ? 1u : 0u;
    tx->gpt0_count = 0u;
    tx->hw_stopped = 0u;
    tx->waveform_complete = 0u;
    tx->reset_ready = 0u;
    tx->reset_elapsed = 0u;
    tx->reset_periods =
        period_ns == 0u ? 1u
                        : (reset_us * 1000u + period_ns - 1u) / period_ns;
    tx->first_pulse_duty = tx->gtccra;
    tx->first_pulse_ok = 0u;
    tx->fault = K1_WS281X_FAULT_NONE;
    tx->stop_armed = 1u;
    tx->pin_high = 0u;
    tx->owned = 1u;
    tx->state = K1_WS281X_TX_PRELOAD;
}

int k1_ws281x_gpt_dma_submit(k1_ws281x_gpt_dma_t *tx,
                             const k1_ws281x_wire_frame_t *frame)
{
    k1_ws281x_diag_timing_t timing;
    if (tx == 0 || frame == 0) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    if (tx->owned) {
        return K1_WS281X_SUBMIT_BUSY;
    }
    if (!k1_ws281x_gpt_pin_can_pwm(frame->pin)) {
        fault(tx, K1_WS281X_FAULT_PIN);
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    if (frame->bytes == 0 || frame->byte_count == 0u ||
        frame->pixel_count == 0u ||
        !k1_ws281x_diag_profile(frame->profile, &timing) ||
        frame->byte_count != frame->pixel_count * timing.bytes_per_pixel ||
        !k1_ws281x_build_gpt_duty(frame->bytes, frame->byte_count,
                                  frame->profile, frame->clock_hz, tx->duty,
                                  K1_WS281X_GPT_DUTY_CAP, &tx->meta)) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    tx->pin = frame->pin;
    tx->clock_hz = frame->clock_hz;
    tx->bits = tx->meta.bits;
    tx->sequence = (uint32_t)frame->sequence;
    preload(tx, timing.reset_us, timing.period_ns);
    return K1_WS281X_SUBMIT_ACCEPTED;
}

int k1_ws281x_gpt_dma_submit_test_bits(k1_ws281x_gpt_dma_t *tx,
                                       const uint32_t *duty, uint32_t bits,
                                       uint32_t clock_hz, uint32_t period_ns,
                                       uint32_t reset_us)
{
    uint32_t i;
    if (tx == 0 || duty == 0 || bits < 2u || bits > K1_WS281X_GPT_DUTY_CAP ||
        clock_hz == 0u || period_ns == 0u) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    if (tx->owned) {
        return K1_WS281X_SUBMIT_BUSY;
    }
    for (i = 0; i < bits; ++i) {
        tx->duty[i] = duty[i];
    }
    tx->meta.bits = bits;
    tx->meta.clock_hz = clock_hz;
    tx->clock_hz = clock_hz;
    tx->pin = K1_WS281X_GPT_PIN_P601;
    tx->bits = bits;
    preload(tx, reset_us, period_ns);
    return K1_WS281X_SUBMIT_ACCEPTED;
}

int k1_ws281x_gpt_dma_start(k1_ws281x_gpt_dma_t *tx)
{
    if (tx == 0 || tx->state != K1_WS281X_TX_PRELOAD) {
        return 0;
    }
    tx->first_pulse_duty = tx->gtccra;
    if (tx->gtccra != tx->duty[0]) {
        tx->first_pulse_ok = 0u;
        fault(tx, K1_WS281X_FAULT_SHIFTED_PRELOAD);
        return 0;
    }
    tx->first_pulse_ok = 1u;
    tx->pin_high = 1u;
    tx->state = K1_WS281X_TX_EMITTING;
    return 1;
}

int k1_ws281x_gpt_dma_poll(const k1_ws281x_gpt_dma_t *tx)
{
    if (tx == 0) {
        return K1_WS281X_TX_FAULT;
    }
    return (int)tx->state;
}

void k1_ws281x_gpt_dma_abort_low(k1_ws281x_gpt_dma_t *tx)
{
    if (tx == 0) {
        return;
    }
    tx->pin_high = 0u;
    tx->owned = 0u;
    tx->dma_remaining = 0u;
    tx->hw_stopped = 1u;
    tx->state = K1_WS281X_TX_IDLE;
}

int k1_ws281x_gpt_dma_step_overflow(k1_ws281x_gpt_dma_t *tx)
{
    if (tx == 0) {
        return 0;
    }
    if (tx->state == K1_WS281X_TX_FAULT) {
        return 0;
    }
    if (tx->state == K1_WS281X_TX_PRELOAD || tx->state == K1_WS281X_TX_IDLE) {
        return 0;
    }
    if (tx->hw_stopped) {
        if (tx->state == K1_WS281X_TX_READY) {
            fault(tx, K1_WS281X_FAULT_EXTRA_EVENT);
            return 0;
        }
        tx->reset_elapsed += 1u;
        tx->state = K1_WS281X_TX_LATCH_PENDING;
        if (tx->reset_elapsed >= tx->reset_periods) {
            tx->reset_ready = 1u;
            tx->owned = 0u;
            tx->state = K1_WS281X_TX_READY;
        }
        return 1;
    }
    if (tx->state != K1_WS281X_TX_EMITTING &&
        tx->state != K1_WS281X_TX_DMA_COMPLETE) {
        return 0;
    }

    tx->gtccra = tx->gtccrc;
    if (tx->dma_remaining > 0u) {
        tx->gtccrc = tx->duty[tx->dma_index];
        tx->dma_index += 1u;
        tx->dma_remaining -= 1u;
        if (tx->dma_remaining == 0u) {
            tx->dma_complete = 1u;
            tx->state = K1_WS281X_TX_DMA_COMPLETE;
        }
    }
    tx->gpt0_count += 1u;
    if (tx->gpt0_count >= tx->bits) {
        if (!tx->stop_armed) {
            fault(tx, K1_WS281X_FAULT_MISSING_STOP);
            return 0;
        }
        tx->pin_high = 0u;
        tx->hw_stopped = 1u;
        tx->waveform_complete = 1u;
        tx->state = K1_WS281X_TX_WAVEFORM_COMPLETE;
        tx->reset_elapsed = 0u;
        return 1;
    }
    return 1;
}

void k1_ws281x_gpt_dma_shift_preload(k1_ws281x_gpt_dma_t *tx)
{
    if (tx == 0 || tx->bits < 2u || tx->state != K1_WS281X_TX_PRELOAD) {
        return;
    }
    tx->gtccra = tx->duty[1];
}

void k1_ws281x_gpt_dma_disarm_stop(k1_ws281x_gpt_dma_t *tx)
{
    if (tx == 0) {
        return;
    }
    tx->stop_armed = 0u;
}
