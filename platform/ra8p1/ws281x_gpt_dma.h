#pragma once

#include "ws281x_waveform.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PRE-SILICON GPT PWM+DMA state model. FastLED-PORT-DESIGN 2026-09-10.
   Not a flashed driver. GPIO smoke emitter remains. P601/GPT6 is the first
   lane. P004 cannot PWM. DMA complete ≠ waveform complete ≠ reset-ready. */

#define K1_WS281X_GPT_TIMER 6u
#define K1_WS281X_GPT_EVENT_TIMER 0u
#define K1_WS281X_GPT_PIN_P601 0u
#define K1_WS281X_GPT_PIN_P004 1u
#define K1_WS281X_GPT_COMPARE_TO_PIN_DELAY 1u

enum {
    K1_WS281X_TX_IDLE = 0,
    K1_WS281X_TX_PRELOAD = 1,
    K1_WS281X_TX_EMITTING = 2,
    K1_WS281X_TX_DMA_COMPLETE = 3,
    K1_WS281X_TX_WAVEFORM_COMPLETE = 4,
    K1_WS281X_TX_LATCH_PENDING = 5,
    K1_WS281X_TX_READY = 6,
    K1_WS281X_TX_FAULT = 7,
    K1_WS281X_TX_BUSY = 8
};

enum {
    K1_WS281X_SUBMIT_ACCEPTED = 0,
    K1_WS281X_SUBMIT_BUSY = 1,
    K1_WS281X_SUBMIT_INVALID = 2,
    K1_WS281X_SUBMIT_UNAVAILABLE = 3
};

enum {
    K1_WS281X_FAULT_NONE = 0,
    K1_WS281X_FAULT_SHIFTED_PRELOAD = 1,
    K1_WS281X_FAULT_MISSING_STOP = 2,
    K1_WS281X_FAULT_EXTRA_EVENT = 3,
    K1_WS281X_FAULT_PIN = 4
};

typedef struct {
    uint32_t profile;
    const uint8_t *bytes;
    size_t byte_count;
    size_t pixel_count;
    uint32_t pin;
    uint32_t clock_hz;
    uint64_t sequence;
} k1_ws281x_wire_frame_t;

typedef struct {
    uint32_t state;
    uint32_t pin;
    uint32_t gpt_timer;
    uint32_t event_timer;
    uint32_t clock_hz;
    uint32_t bits;
    uint32_t dma_remaining;
    uint32_t dma_complete;
    uint32_t waveform_complete;
    uint32_t reset_ready;
    uint32_t events_seen;
    uint32_t gtccra;
    uint32_t gtccrc;
    uint32_t pin_high;
    uint32_t fault;
    uint32_t reset_counts;
    uint32_t reset_elapsed;
    uint32_t owned;
    uint32_t sequence;
    uint32_t duty[K1_WS281X_GPT_DUTY_CAP];
    k1_ws281x_duty_meta_t meta;
} k1_ws281x_gpt_dma_t;

int k1_ws281x_gpt_pin_can_pwm(uint32_t pin);
void k1_ws281x_gpt_dma_reset(k1_ws281x_gpt_dma_t *tx);
int k1_ws281x_gpt_dma_submit(k1_ws281x_gpt_dma_t *tx,
                             const k1_ws281x_wire_frame_t *frame);
int k1_ws281x_gpt_dma_submit_test_bits(k1_ws281x_gpt_dma_t *tx,
                                       const uint32_t *duty, uint32_t bits,
                                       uint32_t clock_hz);
int k1_ws281x_gpt_dma_poll(const k1_ws281x_gpt_dma_t *tx);
void k1_ws281x_gpt_dma_abort_low(k1_ws281x_gpt_dma_t *tx);
int k1_ws281x_gpt_dma_step_overflow(k1_ws281x_gpt_dma_t *tx);
int k1_ws281x_gpt_dma_inject_shifted_preload(k1_ws281x_gpt_dma_t *tx);
int k1_ws281x_gpt_dma_inject_extra_event(k1_ws281x_gpt_dma_t *tx);
void k1_ws281x_gpt_dma_disable_stop(k1_ws281x_gpt_dma_t *tx);

#ifdef __cplusplus
}
#endif
