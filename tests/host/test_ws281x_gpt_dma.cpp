#include "ws281x_gpt_dma.h"

#include <cassert>
#include <cstdio>
#include <vector>

static void start_ok(k1_ws281x_gpt_dma_t *tx)
{
    assert(k1_ws281x_gpt_dma_poll(tx) == K1_WS281X_TX_PRELOAD);
    assert(k1_ws281x_gpt_dma_start(tx));
    assert(tx->first_pulse_ok);
    assert(tx->gtccra == tx->duty[0]);
}

static void run_to_ready(k1_ws281x_gpt_dma_t *tx)
{
    start_ok(tx);
    uint32_t saw_dma = 0u;
    uint32_t saw_wave = 0u;
    while (k1_ws281x_gpt_dma_poll(tx) != K1_WS281X_TX_READY) {
        int state = k1_ws281x_gpt_dma_poll(tx);
        if (state == K1_WS281X_TX_DMA_COMPLETE) {
            saw_dma = 1u;
            assert(tx->waveform_complete == 0u);
            assert(tx->hw_stopped == 0u);
            assert(tx->reset_ready == 0u);
        }
        if (tx->waveform_complete && !tx->reset_ready) {
            saw_wave = 1u;
            assert(tx->dma_complete == 1u);
        }
        assert(k1_ws281x_gpt_dma_step_overflow(tx));
    }
    assert(tx->dma_complete);
    assert(tx->waveform_complete);
    assert(tx->reset_ready);
    assert(tx->pin_high == 0u);
    assert(saw_dma == 1u);
    assert(saw_wave == 1u);
}

int main()
{
    assert(k1_ws281x_gpt_pin_can_pwm(K1_WS281X_GPT_PIN_P601));
    assert(!k1_ws281x_gpt_pin_can_pwm(K1_WS281X_GPT_PIN_P004));

    k1_ws281x_gpt_dma_t tx{};
    k1_ws281x_wire_frame_t frame{};
    uint8_t pix[6]{};
    pix[0] = 0x80;
    frame.profile = 3u;
    frame.bytes = pix;
    frame.byte_count = 6u;
    frame.pixel_count = 1u;
    frame.pin = K1_WS281X_GPT_PIN_P601;
    frame.clock_hz = 100000000u;

    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_ACCEPTED);
    assert(k1_ws281x_gpt_dma_poll(&tx) == K1_WS281X_TX_PRELOAD);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_BUSY);
    k1_ws281x_gpt_dma_abort_low(&tx);

    frame.pin = K1_WS281X_GPT_PIN_P004;
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) ==
           K1_WS281X_SUBMIT_UNAVAILABLE);
    assert(tx.fault == K1_WS281X_FAULT_PIN);
    frame.pin = K1_WS281X_GPT_PIN_P601;

    uint32_t bits8[8] = {10, 20, 30, 40, 50, 60, 70, 80};
    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit_test_bits(&tx, bits8, 8u, 100000000u, 1250u,
                                              300u) == K1_WS281X_SUBMIT_ACCEPTED);
    assert(tx.gtccra == 10u && tx.gtccrc == 20u && tx.dma_remaining == 6u);
    start_ok(&tx);
    assert(k1_ws281x_gpt_dma_step_overflow(&tx));
    assert(tx.gtccra == 20u && tx.gtccrc == 30u && tx.gpt0_count == 1u);
    while (tx.dma_remaining > 0u) {
        assert(k1_ws281x_gpt_dma_step_overflow(&tx));
    }
    assert(tx.dma_complete);
    assert(!tx.waveform_complete);
    assert(tx.gpt0_count == 6u);
    assert(k1_ws281x_gpt_dma_step_overflow(&tx));
    assert(tx.gpt0_count == 7u);
    assert(!tx.hw_stopped);
    assert(k1_ws281x_gpt_dma_step_overflow(&tx));
    assert(tx.gpt0_count == 8u);
    assert(tx.hw_stopped);
    assert(tx.waveform_complete);
    assert(!tx.reset_ready);
    while (k1_ws281x_gpt_dma_poll(&tx) != K1_WS281X_TX_READY) {
        assert(k1_ws281x_gpt_dma_step_overflow(&tx));
    }

    uint32_t bits16[16];
    for (uint32_t i = 0; i < 16u; ++i) {
        bits16[i] = 10u + i;
    }
    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit_test_bits(&tx, bits16, 16u, 100000000u,
                                              1250u, 300u) ==
           K1_WS281X_SUBMIT_ACCEPTED);
    run_to_ready(&tx);

    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_ACCEPTED);
    run_to_ready(&tx);

    std::vector<uint8_t> two(12, 0);
    two[0] = 0xFF;
    frame.bytes = two.data();
    frame.byte_count = 12u;
    frame.pixel_count = 2u;
    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_ACCEPTED);
    run_to_ready(&tx);

    std::vector<uint8_t> max(80u * 6u, 0);
    max[0] = 0x80;
    frame.bytes = max.data();
    frame.byte_count = max.size();
    frame.pixel_count = 80u;
    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_ACCEPTED);
    run_to_ready(&tx);

    std::vector<uint8_t> ws2812_128(128u * 3u, 0);
    ws2812_128[0] = 0x80;
    frame.profile = 1u;
    frame.bytes = ws2812_128.data();
    frame.byte_count = ws2812_128.size();
    frame.pixel_count = 128u;
    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_profile_max_pixels(1u) == 128u);
    assert(k1_ws281x_profile_max_pixels(3u) == 80u);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_ACCEPTED);
    assert(tx.bits == 3072u);
    run_to_ready(&tx);

    frame.pixel_count = 129u;
    frame.byte_count = 129u * 3u;
    std::vector<uint8_t> too_many(129u * 3u, 0);
    frame.bytes = too_many.data();
    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_INVALID);

    std::vector<uint8_t> ws2816_81(81u * 6u, 0);
    frame.profile = 3u;
    frame.bytes = ws2816_81.data();
    frame.byte_count = ws2816_81.size();
    frame.pixel_count = 81u;
    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_INVALID);

    frame.bytes = pix;
    frame.byte_count = 6u;
    frame.pixel_count = 1u;
    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit(&tx, &frame) == K1_WS281X_SUBMIT_ACCEPTED);
    k1_ws281x_gpt_dma_shift_preload(&tx);
    assert(tx.fault == K1_WS281X_FAULT_NONE);
    assert(!k1_ws281x_gpt_dma_start(&tx));
    assert(tx.fault == K1_WS281X_FAULT_SHIFTED_PRELOAD);
    assert(!tx.first_pulse_ok);

    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit_test_bits(&tx, bits8, 8u, 100000000u, 1250u,
                                              300u) == K1_WS281X_SUBMIT_ACCEPTED);
    k1_ws281x_gpt_dma_disarm_stop(&tx);
    start_ok(&tx);
    while (tx.gpt0_count < 7u) {
        assert(k1_ws281x_gpt_dma_step_overflow(&tx));
    }
    assert(k1_ws281x_gpt_dma_step_overflow(&tx) == 0);
    assert(tx.fault == K1_WS281X_FAULT_MISSING_STOP);

    k1_ws281x_gpt_dma_reset(&tx);
    assert(k1_ws281x_gpt_dma_submit_test_bits(&tx, bits8, 8u, 100000000u, 1250u,
                                              300u) == K1_WS281X_SUBMIT_ACCEPTED);
    run_to_ready(&tx);
    assert(k1_ws281x_gpt_dma_step_overflow(&tx) == 0);
    assert(tx.fault == K1_WS281X_FAULT_EXTRA_EVENT);

    std::puts("K1_WS281X_GPT_DMA=PASS n8=PASS n16=PASS max80=PASS "
              "ws2812_128=PASS ws2812_129=REJECT ws2816_81=REJECT "
              "first_pulse=PASS shifted=DETECT missing_stop=DETECT "
              "extra_event=DETECT p004=UNAVAILABLE busy=PASS "
              "dma_ne_wave_ne_reset=PASS");
    return 0;
}
