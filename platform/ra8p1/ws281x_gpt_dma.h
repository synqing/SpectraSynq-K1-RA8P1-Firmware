#pragma once

#include "ws281x_waveform.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GPT PWM+DMA transmitter. Host file is the state model; target hardware
   is ws281x_gpt_dma_hw.c. FastLED-PORT-DESIGN. GPT6 saw-wave + GPT0 event
   count + DMAC2 to GTCCRC. P601 only. DMA complete ≠ GPT0 overflow / hw
   stop ≠ reset-ready. Compare-to-pin delay is one GPT count. GPIO smoke
   emitter stays as a diagnostic path and cannot satisfy GPT acceptance. */

#define K1_WS281X_GPT_TIMER 6u
#define K1_WS281X_GPT_EVENT_TIMER 0u
#define K1_WS281X_GPT_TIMER_A1 7u
#define K1_WS281X_GPT_EVENT_TIMER_A1 1u
#define K1_WS281X_GPT_PIN_P601 0u
#define K1_WS281X_GPT_PIN_P004 1u
#define K1_WS281X_GPT_PIN_P603 2u
#define K1_WS281X_GPT_COMPARE_TO_PIN_DELAY 1u

enum {
    K1_WS281X_TX_IDLE = 0,
    K1_WS281X_TX_PRELOAD = 1,
    K1_WS281X_TX_EMITTING = 2,
    K1_WS281X_TX_DMA_COMPLETE = 3,
    K1_WS281X_TX_WAVEFORM_COMPLETE = 4,
    K1_WS281X_TX_LATCH_PENDING = 5,
    K1_WS281X_TX_READY = 6,
    K1_WS281X_TX_FAULT = 7
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
    K1_WS281X_FAULT_PIN = 4,
    K1_WS281X_FAULT_TIMEOUT = 5,
    K1_WS281X_FAULT_DMA = 6,
    K1_WS281X_FAULT_CLOCK = 7,
    K1_WS281X_FAULT_RESOURCE = 8
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
    uint32_t dma_index;
    uint32_t dma_remaining;
    uint32_t dma_complete;
    uint32_t gpt0_count;
    uint32_t stop_armed;
    uint32_t hw_stopped;
    uint32_t waveform_complete;
    uint32_t reset_ready;
    uint32_t reset_periods;
    uint32_t reset_elapsed;
    uint32_t gtccra;
    uint32_t gtccrc;
    uint32_t first_pulse_duty;
    uint32_t first_pulse_ok;
    uint32_t pin_high;
    uint32_t fault;
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
                                       uint32_t clock_hz, uint32_t period_ns,
                                       uint32_t reset_us);
int k1_ws281x_gpt_dma_start(k1_ws281x_gpt_dma_t *tx);
int k1_ws281x_gpt_dma_poll(const k1_ws281x_gpt_dma_t *tx);
void k1_ws281x_gpt_dma_abort_low(k1_ws281x_gpt_dma_t *tx);
int k1_ws281x_gpt_dma_step_overflow(k1_ws281x_gpt_dma_t *tx);
void k1_ws281x_gpt_dma_shift_preload(k1_ws281x_gpt_dma_t *tx);
void k1_ws281x_gpt_dma_disarm_stop(k1_ws281x_gpt_dma_t *tx);

#ifdef __cplusplus
}
#endif
