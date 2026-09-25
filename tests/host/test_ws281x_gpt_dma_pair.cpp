#include "core/visual/ws2816_pack.h"
#include "ws281x_gpt_dma_pair.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

static void run_pair_to_ready(k1_ws281x_gpt_dma_pair_t *pair)
{
    assert(k1_ws281x_gpt_dma_pair_poll(pair) == K1_WS281X_PAIR_PRELOAD);
    assert(k1_ws281x_gpt_dma_pair_start(pair));
    while (!k1_ws281x_gpt_dma_pair_complete(pair)) {
        assert(k1_ws281x_gpt_dma_pair_poll(pair) != K1_WS281X_PAIR_FAULT);
        if (pair->lane_a0.dma_complete && pair->lane_a1.dma_complete &&
            (!pair->lane_a0.hw_stopped || !pair->lane_a1.hw_stopped)) {
            assert(!k1_ws281x_gpt_dma_pair_complete(pair));
        }
        assert(k1_ws281x_gpt_dma_pair_step_overflow(pair));
    }
    assert(pair->lane_a0.reset_ready && pair->lane_a1.reset_ready);
    assert(pair->completed_generation == pair->submitted_generation);
}

int main()
{
    assert(k1_ws281x_gpt_pin_can_pwm(K1_WS281X_GPT_PIN_P601));
    assert(k1_ws281x_gpt_pin_can_pwm(K1_WS281X_GPT_PIN_P603));
    assert(!k1_ws281x_gpt_pin_can_pwm(K1_WS281X_GPT_PIN_P004));

    k1::core::visual::Pixel16 pixels[160]{};
    pixels[0].red = 0x12AB;
    pixels[0].green = 0x34CD;
    pixels[0].blue = 0x56EF;
    pixels[79].red = 0x0100;
    pixels[80].red = 0x0001;
    pixels[159].blue = 0x00FF;
    uint8_t lane_a0[480];
    uint8_t lane_a1[480];
    assert(k1::core::visual::splitChannel160(pixels, 160, lane_a0, lane_a1));
    assert(lane_a0[0] == 0x34 && lane_a0[1] == 0xCD && lane_a0[2] == 0x12 &&
           lane_a0[3] == 0xAB && lane_a0[4] == 0x56 && lane_a0[5] == 0xEF);
    pixels[0].red = 0x12AC;
    uint8_t lane_a0_lo[480];
    uint8_t lane_a1_lo[480];
    assert(k1::core::visual::splitChannel160(pixels, 160, lane_a0_lo, lane_a1_lo));
    assert(lane_a0_lo[3] == 0xAC && lane_a0_lo[2] == 0x12);
    assert(lane_a0[79 * 6 + 2] == 0x01 && lane_a0[79 * 6 + 3] == 0x00);
    assert(lane_a1[0 * 6 + 2] == 0x00 && lane_a1[0 * 6 + 3] == 0x01);

    k1_ws281x_gpt_dma_pair_t pair{};
    k1_ws281x_gpt_dma_pair_reset(&pair);
    assert(k1_ws281x_gpt_dma_pair_submit(&pair, lane_a0, 479u, lane_a1, 480u, 3u,
                                         7u, 100000000u) ==
           K1_WS281X_SUBMIT_INVALID);
    assert(!pair.lane_a0.owned && !pair.lane_a1.owned);

    assert(k1_ws281x_gpt_dma_pair_submit(&pair, lane_a0, 480u, lane_a1, 480u, 1u,
                                         7u, 100000000u) ==
           K1_WS281X_SUBMIT_INVALID);

    assert(k1_ws281x_gpt_dma_pair_submit(&pair, lane_a0, 480u, lane_a1, 480u, 3u,
                                         11u, 100000000u) ==
           K1_WS281X_SUBMIT_ACCEPTED);
    assert(pair.lane_a0.sequence == 11u && pair.lane_a1.sequence == 11u);
    assert(pair.lane_a0.pin == K1_WS281X_GPT_PIN_P601);
    assert(pair.lane_a1.pin == K1_WS281X_GPT_PIN_P603);
    assert(pair.lane_a0.bits == 3840u && pair.lane_a1.bits == 3840u);
    assert(pair.lane_a0.dma_remaining == 3838u);
    assert(pair.lane_a1.dma_remaining == 3838u);
    assert(k1_ws281x_gpt_dma_pair_submit(&pair, lane_a0_lo, 480u, lane_a1, 480u,
                                         3u, 12u, 100000000u) ==
           K1_WS281X_SUBMIT_BUSY);
    assert(pair.pending_replacements == 1u);
    assert(pair.pending_valid == 1u);
    assert(pair.lane_a0.owned && pair.lane_a1.owned);
    k1_ws281x_gpt_dma_pair_abort(&pair);
    assert(!pair.lane_a0.owned && !pair.lane_a1.owned);

    k1_ws281x_gpt_dma_pair_reset(&pair);
    for (uint32_t last0 = 0; last0 < 2; ++last0) {
        for (uint32_t last1 = 0; last1 < 2; ++last1) {
            uint32_t a0[8] = {10, 20, 30, 40, 50, 60, 70, last0 ? 80u : 0u};
            uint32_t a1[8] = {11, 21, 31, 41, 51, 61, 71, last1 ? 1u : 0u};
            k1_ws281x_gpt_dma_pair_reset(&pair);
            assert(k1_ws281x_gpt_dma_pair_submit_test_bits(&pair, a0, a1, 8u,
                                                           40u + last0 * 2u + last1,
                                                           100000000u, 1250u,
                                                           300u) ==
                   K1_WS281X_SUBMIT_ACCEPTED);
            assert(pair.lane_a0.duty[7] == a0[7]);
            assert(pair.lane_a1.duty[7] == a1[7]);
            run_pair_to_ready(&pair);
            assert(pair.lane_a0.hw_stopped && pair.lane_a1.hw_stopped);
            assert(pair.lane_a0.reset_ready && pair.lane_a1.reset_ready);
        }
    }

    k1_ws281x_gpt_dma_pair_reset(&pair);
    assert(k1_ws281x_gpt_dma_pair_submit(&pair, lane_a0, 480u, lane_a1, 480u, 3u,
                                         21u, 100000000u) ==
           K1_WS281X_SUBMIT_ACCEPTED);
    uint32_t frozen[8];
    std::memcpy(frozen, pair.lane_a0.duty, sizeof(frozen));
    uint8_t mutated[480];
    std::memcpy(mutated, lane_a0, 480u);
    mutated[0] = static_cast<uint8_t>(mutated[0] ^ 0xffu);
    assert(k1_ws281x_gpt_dma_pair_submit(&pair, mutated, 480u, lane_a1, 480u, 3u,
                                         22u, 100000000u) ==
           K1_WS281X_SUBMIT_BUSY);
    assert(std::memcmp(frozen, pair.lane_a0.duty, sizeof(frozen)) == 0);
    run_pair_to_ready(&pair);

    uint32_t duty_a0[8] = {10, 20, 30, 40, 50, 60, 70, 80};
    uint32_t duty_a1[8] = {11, 21, 31, 41, 51, 61, 71, 1};
    k1_ws281x_gpt_dma_pair_reset(&pair);
    assert(k1_ws281x_gpt_dma_pair_submit_test_bits(&pair, duty_a0, duty_a1, 8u,
                                                   33u, 100000000u, 1250u,
                                                   300u) ==
           K1_WS281X_SUBMIT_ACCEPTED);
    assert(k1_ws281x_gpt_dma_pair_start(&pair));
    while (!pair.lane_a0.hw_stopped || !pair.lane_a1.hw_stopped) {
        if (pair.lane_a0.dma_complete && pair.lane_a1.dma_complete &&
            (!pair.lane_a0.hw_stopped || !pair.lane_a1.hw_stopped)) {
            assert(!k1_ws281x_gpt_dma_pair_complete(&pair));
        }
        assert(k1_ws281x_gpt_dma_pair_step_overflow(&pair));
    }
    assert(pair.lane_a0.waveform_complete && pair.lane_a1.waveform_complete);
    assert(!k1_ws281x_gpt_dma_pair_complete(&pair));
    while (!k1_ws281x_gpt_dma_pair_complete(&pair)) {
        assert(k1_ws281x_gpt_dma_pair_step_overflow(&pair));
    }

    k1_ws281x_gpt_dma_pair_reset(&pair);
    assert(k1_ws281x_gpt_dma_pair_submit_test_bits(&pair, duty_a0, duty_a1, 8u,
                                                   34u, 100000000u, 1250u,
                                                   300u) ==
           K1_WS281X_SUBMIT_ACCEPTED);
    k1_ws281x_gpt_dma_disarm_stop(&pair.lane_a1);
    assert(k1_ws281x_gpt_dma_pair_start(&pair));
    while (pair.pair_state != K1_WS281X_PAIR_FAULT) {
        k1_ws281x_gpt_dma_pair_step_overflow(&pair);
    }
    assert(!k1_ws281x_gpt_dma_pair_complete(&pair));
    assert(pair.lane_a1.fault == K1_WS281X_FAULT_MISSING_STOP);

    /* X3: newer pending generation replaces the whole older pending pair. */
    k1_ws281x_gpt_dma_pair_reset(&pair);
    assert(k1_ws281x_gpt_dma_pair_submit(&pair, lane_a0, 480u, lane_a1, 480u, 3u,
                                         50u, 100000000u) ==
           K1_WS281X_SUBMIT_ACCEPTED);
    uint8_t pend_old0[480];
    uint8_t pend_old1[480];
    uint8_t pend_new0[480];
    uint8_t pend_new1[480];
    std::memcpy(pend_old0, lane_a0, 480u);
    std::memcpy(pend_old1, lane_a1, 480u);
    std::memcpy(pend_new0, lane_a0_lo, 480u);
    std::memcpy(pend_new1, lane_a1, 480u);
    pend_old0[10] = 0x11;
    pend_old1[10] = 0x22;
    pend_new0[10] = 0xAA;
    pend_new1[10] = 0xBB;
    assert(k1_ws281x_gpt_dma_pair_submit(&pair, pend_old0, 480u, pend_old1, 480u,
                                         3u, 51u, 100000000u) ==
           K1_WS281X_SUBMIT_BUSY);
    assert(pair.pending_generation == 51u);
    assert(pair.pending_a0[10] == 0x11 && pair.pending_a1[10] == 0x22);
    assert(k1_ws281x_gpt_dma_pair_submit(&pair, pend_new0, 480u, pend_new1, 480u,
                                         3u, 52u, 100000000u) ==
           K1_WS281X_SUBMIT_BUSY);
    assert(pair.pending_valid == 1u);
    assert(pair.pending_generation == 52u);
    assert(pair.pending_replacements == 2u);
    assert(pair.pending_a0[10] == 0xAA && pair.pending_a1[10] == 0xBB);
    assert(pair.pending_a0[10] != 0x11);
    /* Active owned duty unchanged by pending replace. */
    assert(pair.lane_a0.owned && pair.lane_a1.owned);
    k1_ws281x_gpt_dma_pair_abort(&pair);

    /* X4: A0 DMA-first then A1, and A1 DMA-first then A0 — no pair success early. */
    for (int a0_first = 0; a0_first < 2; ++a0_first) {
        k1_ws281x_gpt_dma_pair_reset(&pair);
        assert(k1_ws281x_gpt_dma_pair_submit_test_bits(
                   &pair, duty_a0, duty_a1, 8u, 60u + (uint32_t)a0_first,
                   100000000u, 1250u, 300u) == K1_WS281X_SUBMIT_ACCEPTED);
        assert(k1_ws281x_gpt_dma_pair_start(&pair));
        k1_ws281x_gpt_dma_t *first =
            a0_first ? &pair.lane_a0 : &pair.lane_a1;
        k1_ws281x_gpt_dma_t *second =
            a0_first ? &pair.lane_a1 : &pair.lane_a0;
        while (!first->dma_complete) {
            assert(k1_ws281x_gpt_dma_step_overflow(first));
        }
        assert(!k1_ws281x_gpt_dma_pair_complete(&pair));
        while (!second->dma_complete) {
            assert(k1_ws281x_gpt_dma_step_overflow(second));
        }
        assert(!k1_ws281x_gpt_dma_pair_complete(&pair));
        while (!first->hw_stopped) {
            assert(k1_ws281x_gpt_dma_step_overflow(first));
        }
        assert(!k1_ws281x_gpt_dma_pair_complete(&pair));
        while (!second->hw_stopped) {
            assert(k1_ws281x_gpt_dma_step_overflow(second));
        }
        assert(!k1_ws281x_gpt_dma_pair_complete(&pair));
        while (!k1_ws281x_gpt_dma_pair_complete(&pair)) {
            assert(k1_ws281x_gpt_dma_pair_step_overflow(&pair));
        }
        assert(pair.lane_a0.reset_ready && pair.lane_a1.reset_ready);
    }

    /* X5: lanes_complete rejects partial / fault / non-READY pair_state. */
    assert(!k1_ws281x_gpt_dma_pair_lanes_complete(1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
                                                  K1_WS281X_PAIR_EMITTING));
    assert(!k1_ws281x_gpt_dma_pair_lanes_complete(1, 0, 1, 1, 1, 1, 1, 1, 0, 0,
                                                  K1_WS281X_PAIR_READY));
    assert(!k1_ws281x_gpt_dma_pair_lanes_complete(1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
                                                  K1_WS281X_PAIR_READY));
    assert(k1_ws281x_gpt_dma_pair_lanes_complete(1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
                                                 K1_WS281X_PAIR_READY));

    /* X2: profile-4 (GRB48 on FastLED one-high) is not pair profile 3. */
    k1_ws281x_gpt_dma_pair_reset(&pair);
    assert(k1_ws281x_gpt_dma_pair_submit(&pair, lane_a0, 480u, lane_a1, 480u, 4u,
                                         70u, 100000000u) ==
           K1_WS281X_SUBMIT_INVALID);

    /* X2: one-lane busy rejects the whole pair (A0 owned alone). */
    k1_ws281x_gpt_dma_pair_reset(&pair);
    pair.lane_a0.owned = 1u;
    assert(k1_ws281x_gpt_dma_pair_submit(&pair, lane_a0, 480u, lane_a1, 480u, 3u,
                                         71u, 100000000u) ==
           K1_WS281X_SUBMIT_BUSY);
    assert(pair.pending_valid == 1u);
    assert(!pair.lane_a1.owned);

    std::puts("K1_WS281X_GPT_DMA_PAIR=PASS true16=PASS split=PASS "
              "size_reject=PASS profile1_reject=PASS busy_pending=PASS "
              "gen_match=PASS dma_ne_stop=PASS last_bits=PASS "
              "lane_fault=PASS p603=PWM p004=NOT_PWM "
              "pending_replace=PASS dma_order=PASS lanes_complete_neg=PASS "
              "profile4_reject=PASS one_lane_busy=PASS");
    return 0;
}
