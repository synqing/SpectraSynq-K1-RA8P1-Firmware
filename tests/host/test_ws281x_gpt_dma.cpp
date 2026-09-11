#include "ws281x_gpt_dma.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

static void run_frame(k1_ws281x_gpt_dma_t *tx, const uint8_t *bytes,
                      size_t nbytes, size_t pixels, uint32_t n_bits)
{
    k1_ws281x_wire_frame_t frame{};
    frame.profile = 3u;
    frame.bytes = bytes;
    frame.byte_count = nbytes;
    frame.pixel_count = pixels;
    frame.pin = K1_WS281X_GPT_PIN_P601;
    frame.clock_hz = 100000000u;
    assert(k1_ws281x_gpt_dma_submit(tx, &frame) == K1_WS281X_SUBMIT_ACCEPTED);
    assert(tx->gtccra == tx->duty[0]);
    if (n_bits > 1u) {
        assert(tx->gtccrc == tx->duty[1]);
    }
    uint32_t saw_dma = 0u;
    uint32_t saw_wave = 0u;
    while (k1_ws281x_gpt_dma_poll(tx) != K1_WS281X_TX_READY) {
        int state = k1_ws281x_gpt_dma_poll(tx);
        if (state == K1_WS281X_TX_DMA_COMPLETE) {
            saw_dma = 1u;
            assert(tx->waveform_complete == 0u);
            assert(tx->reset_ready == 0u);
        }
        if (tx->waveform_complete && !tx->reset_ready) {
            saw_wave = 1u;
        }
        assert(k1_ws281x_gpt_dma_step_overflow(tx));
    }
    assert(tx->dma_complete == 1u);
    assert(tx->waveform_complete == 1u);
    assert(tx->reset_ready == 1u);
    assert(tx->pin_high == 0u);
    assert(saw_dma == 1u || n_bits <= 2u);
    assert(saw_wave == 1u || n_bits == 0u);
    (void)saw_dma;
    (void)saw_wave;
}

int main()
{
    assert(k1_ws281x_gpt_pin_can_pwm(K1_WS281X_GPT_PIN_P601));
    assert(!k1_ws281x_gpt_pin_can_pwm(K1_WS281X_GPT_PIN_P004));

    uint8_t eight[6]{};
    eight[0] = 0x80;
    k1_ws281x_gpt_dma_t tx{};
    k1_ws281x_gpt_dma_reset(&tx);
    k1_ws281x_wire_frame_t frame{};
    frame.profile = 3u;
    frame.bytes = eight;
    frame.byte_count = 6u;
    frame.pixel_count = 1u;
    frame.pin = K1_WS281X_GPT_PIN_P601;
    frame.clock_hz = 100000000u;
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_ACCEPTED);
    assert(tx.bits == 48u);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_BUSY);
    k1_ws281x_gpt_dma_abort_low(&tx);
    assert(tx.pin_high == 0u);

    frame.pin = K1_WS281X_GPT_PIN_P004;
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) ==
           K1_WS281X_SUBMIT_UNAVAILABLE);
    assert(tx.fault == K1_WS281X_FAULT_PIN);

    k1_ws281x_gpt_dma_reset(&tx);
    uint32_t bits8[8] = {10, 20, 10, 20, 10, 20, 10, 20};
    assert(k1_ws281x_gpt_dma_submit_test_bits(&tx, bits8, 8u, 100000000u) ==
           K1_WS281X_SUBMIT_ACCEPTED);
    assert(tx.gtccra == 10u && tx.gtccrc == 20u);
    assert(tx.dma_remaining == 6u);
    while (k1_ws281x_gpt_dma_poll(&tx) != K1_WS281X_TX_READY) {
        assert(k1_ws281x_gpt_dma_step_overflow(&tx));
    }
    assert(tx.dma_complete && tx.waveform_complete && tx.reset_ready);

    k1_ws281x_gpt_dma_reset(&tx);
    uint32_t bits16[16];
    for (uint32_t i = 0; i < 16u; ++i) {
        bits16[i] = (i & 1u) ? 20u : 10u;
    }
    assert(k1_ws281x_gpt_dma_submit_test_bits(&tx, bits16, 16u, 100000000u) ==
           K1_WS281X_SUBMIT_ACCEPTED);
    while (k1_ws281x_gpt_dma_poll(&tx) != K1_WS281X_TX_READY) {
        int state = k1_ws281x_gpt_dma_poll(&tx);
        if (state == K1_WS281X_TX_DMA_COMPLETE) {
            assert(!tx.waveform_complete);
        }
        assert(k1_ws281x_gpt_dma_step_overflow(&tx));
    }

    std::vector<uint8_t> pix(6, 0);
    pix[0] = 0xA5;
    k1_ws281x_gpt_dma_reset(&tx);
    run_frame(&tx, pix.data(), 6u, 1u, 48u);

    k1_ws281x_gpt_dma_reset(&tx);
    std::vector<uint8_t> two(12, 0);
    two[0] = 0xFF;
    run_frame(&tx, two.data(), 12u, 2u, 96u);

    k1_ws281x_gpt_dma_reset(&tx);
    std::vector<uint8_t> max(80u * 6u, 0);
    max[0] = 0x80;
    run_frame(&tx, max.data(), max.size(), 80u, 80u * 48u);

    k1_ws281x_gpt_dma_reset(&tx);
    frame.pin = K1_WS281X_GPT_PIN_P601;
    frame.bytes = pix.data();
    frame.byte_count = 6u;
    frame.pixel_count = 1u;
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_ACCEPTED);
    assert(k1_ws281x_gpt_dma_inject_shifted_preload(&tx));
    assert(tx.fault == K1_WS281X_FAULT_SHIFTED_PRELOAD);

    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_ACCEPTED);
    while (tx.events_seen < 8u) {
        assert(k1_ws281x_gpt_dma_step_overflow(&tx));
    }
    k1_ws281x_gpt_dma_disable_stop(&tx);
    assert(k1_ws281x_gpt_dma_step_overflow(&tx) == 0);
    assert(tx.fault == K1_WS281X_FAULT_MISSING_STOP);

    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_ACCEPTED);
    while (k1_ws281x_gpt_dma_poll(&tx) != K1_WS281X_TX_READY) {
        assert(k1_ws281x_gpt_dma_step_overflow(&tx));
    }
    assert(k1_ws281x_gpt_dma_inject_extra_event(&tx) == 0);
    assert(tx.fault == K1_WS281X_FAULT_EXTRA_EVENT);

    std::puts("K1_WS281X_GPT_DMA=PASS n8=PASS n16=PASS max80=PASS "
              "shifted=PASS missing_stop=PASS extra_event=PASS p004=UNAVAILABLE "
              "busy=PASS dma_ne_wave_ne_reset=PASS");
    return 0;
}
