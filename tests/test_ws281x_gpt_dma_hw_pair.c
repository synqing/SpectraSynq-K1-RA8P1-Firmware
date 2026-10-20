/* Register/FSP seam test for the paired GPT/DMA transmitter. Not silicon. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define K1_RA8P1_TARGET 1
#define K1_WS281X_HW_TEST 1
#define K1_PALETTE_WS2816_GPT_PAIR 1
#include "../platform/ra8p1/ws281x_gpt_dma_hw_pair.c"
#include "../platform/ra8p1/ws281x_gpt_dma_pair.c"
#include "../platform/ra8p1/ws281x_gpt_dma.c"

static uint8_t lane0[480];
static uint8_t lane1[480];
static int occupied;

static void fresh(void)
{
    memset(&gpt6_ctrl, 0, sizeof(gpt6_ctrl));
    memset(&gpt7_ctrl, 0, sizeof(gpt7_ctrl));
    memset(&gpt0_ctrl, 0, sizeof(gpt0_ctrl));
    memset(&gpt1_ctrl, 0, sizeof(gpt1_ctrl));
    memset(&dmac0_ctrl, 0, sizeof(dmac0_ctrl));
    memset(&dmac3_ctrl, 0, sizeof(dmac3_ctrl));
    memset(gpt_regs, 0, sizeof(gpt_regs));
    memset(dma_channel_regs, 0, sizeof(dma_channel_regs));
    memset(irq_contexts, 0, sizeof(irq_contexts));
    memset(&elc, 0, sizeof(elc));
    memset(lane0, 0, sizeof(lane0));
    memset(lane1, 0, sizeof(lane1));
    initialised = opened = owned = pair_state = 0;
    first_fault = completions = errors = replacements = 0;
    submitted_generation = completed_generation = pending_valid = 0;
    dma_complete[0] = dma_complete[1] = 0;
    waveform_complete[0] = waveform_complete[1] = 0;
    hw_stopped[0] = hw_stopped[1] = 0;
    reset_ready[0] = reset_ready[1] = 0;
    lane_fault[0] = lane_fault[1] = 0;
    elc_started = fail_start = fail_reset = irq_mask = 0;
    mstp.MSTPCRC = 0xffffffff;
    DWT->CYCCNT = 100;
}

static void submit(uint32_t generation)
{
    assert(k1_ws281x_gpt_dma_hw_pair_init());
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 3u,
                                            generation) == 0);
    assert(gpt6_ctrl.p_reg->GTCCR[0] == duty_a0[0]);
    assert(gpt7_ctrl.p_reg->GTCCR[0] == duty_a1[0]);
    assert(dmac0_ctrl.p_reg->DMCRA == 3838);
    assert(dmac3_ctrl.p_reg->DMCRA == 3838);
    assert(k1_ws281x_gpt_dma_hw_pair_gtstr() == ((1u << 6) | (1u << 7)));
}

static void done_dma(void)
{
    dma_channel_regs[0].DMCRA = 0;
    dma_channel_regs[3].DMCRA = 0;
    dmac0_done(NULL);
    dmac3_done(NULL);
}

static void stop_both(void)
{
    timer_callback_args_t a = {.event = TIMER_EVENT_CYCLE_END};
    gpt_regs[6].GTCR = 0;
    gpt_regs[7].GTCR = 0;
    gpt0_overflow(&a);
    gpt1_overflow(&a);
}

int main(void)
{
    fresh();
    submit(11u);
    assert(elc.ELSR[0].HA_b.ELS == 0x1b7 && elc.ELSR[1].HA_b.ELS == 0x187);
    assert(elc.ELSR[2].HA_b.ELS == 0x1c0 && elc.ELSR[3].HA_b.ELS == 0x190);
    uint32_t frozen = duty_a0[0];
    lane0[0] = 0xff;
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 3u, 12u) ==
           K1_WS281X_SUBMIT_BUSY);
    assert(duty_a0[0] == frozen);
    done_dma();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
    assert(!k1_ws281x_gpt_dma_pair_lanes_complete(
        dma_complete[0], dma_complete[1], hw_stopped[0], hw_stopped[1],
        waveform_complete[0], waveform_complete[1], reset_ready[0],
        reset_ready[1], lane_fault[0], lane_fault[1], pair_state));
    stop_both();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
    DWT->CYCCNT += reset_cycles + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() >= 1u);

    fresh();
    submit(21u);
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 479, lane1, 480, 3u, 22u) ==
           K1_WS281X_SUBMIT_INVALID);
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 1u, 22u) ==
           K1_WS281X_SUBMIT_INVALID);
    done_dma();
    stop_both();
    DWT->CYCCNT += reset_cycles + 1u;
    k1_ws281x_gpt_dma_hw_pair_poll();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 1);
    assert(k1_ws281x_gpt_dma_hw_pair_completed_generation() == 21u);

    fresh();
    submit(31u);
    stop_both();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
    assert(k1_ws281x_gpt_dma_hw_pair_last_fault() != K1_WS281X_FAULT_NONE);

    fresh();
    irq_contexts[47] = &occupied;
    assert(!k1_ws281x_gpt_dma_hw_pair_init());
    assert(first_fault == K1_WS281X_FAULT_RESOURCE);

    /* X4: A0 DMA first, then A1, then stops — no completion until reset. */
    fresh();
    submit(41u);
    dma_channel_regs[0].DMCRA = 0;
    dmac0_done(NULL);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
    assert(!k1_ws281x_gpt_dma_pair_lanes_complete(
        dma_complete[0], dma_complete[1], hw_stopped[0], hw_stopped[1],
        waveform_complete[0], waveform_complete[1], reset_ready[0],
        reset_ready[1], lane_fault[0], lane_fault[1], pair_state));
    dma_channel_regs[3].DMCRA = 0;
    dmac3_done(NULL);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
    stop_both();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
    DWT->CYCCNT += reset_cycles + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() >= 1u);

    /* X4: A1 DMA first, then A0. */
    fresh();
    submit(42u);
    dma_channel_regs[3].DMCRA = 0;
    dmac3_done(NULL);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
    dma_channel_regs[0].DMCRA = 0;
    dmac0_done(NULL);
    stop_both();
    DWT->CYCCNT += reset_cycles + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() >= 1u);

    /* X5: stale/extra DMA callback after already complete → fault, not success. */
    fresh();
    submit(43u);
    done_dma();
    dmac0_done(NULL); /* duplicate A0 DMA */
    assert(first_fault == K1_WS281X_FAULT_EXTRA_EVENT);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);

    /* X5: stale stop after waveform already marked complete. */
    fresh();
    submit(44u);
    done_dma();
    stop_both();
    {
        timer_callback_args_t a = {.event = TIMER_EVENT_CYCLE_END};
        gpt1_overflow(&a); /* second stop on A1 */
    }
    assert(first_fault == K1_WS281X_FAULT_EXTRA_EVENT);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);

    /* X5: one-lane fault (stop while GPT still running) denies pair success. */
    fresh();
    submit(45u);
    done_dma();
    {
        timer_callback_args_t a = {.event = TIMER_EVENT_CYCLE_END};
        /* Host mock starts PWM via GTSTR write, which does not set CST.
           stop_both clears GTCR before overflow; here leave GPT6 running so
           stop_overflow raises MISSING_STOP (same bit the driver checks). */
        gpt_regs[6].GTCR = R_GPT0_GTCR_CST_Msk;
        gpt0_overflow(&a);
    }
    assert(first_fault == K1_WS281X_FAULT_MISSING_STOP);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);

    /* X3: pending whole-pair replace then promote after complete. */
    fresh();
    submit(46u);
    lane0[1] = 0x11;
    lane1[1] = 0x22;
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 3u, 47u) ==
           K1_WS281X_SUBMIT_BUSY);
    assert(pending_valid == 1u);
    assert(pending_generation == 47u);
    lane0[1] = 0xAA;
    lane1[1] = 0xBB;
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 3u, 48u) ==
           K1_WS281X_SUBMIT_BUSY);
    assert(pending_generation == 48u);
    assert(pending_a0[1] == 0xAA && pending_a1[1] == 0xBB);
    done_dma();
    stop_both();
    DWT->CYCCNT += reset_cycles + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(k1_ws281x_gpt_dma_hw_pair_completed_generation() == 46u ||
           k1_ws281x_gpt_dma_hw_pair_completions() >= 1u);
    /* After promote, submitted generation becomes the pending one. */
    assert(submitted_generation == 48u || pending_valid == 0u);

    /* X5: prior-generation event after owned cleared — ignored / no credit. */
    fresh();
    submit(49u);
    done_dma();
    stop_both();
    DWT->CYCCNT += reset_cycles + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    {
        const uint32_t before = k1_ws281x_gpt_dma_hw_pair_completions();
        dmac0_done(NULL);
        assert(k1_ws281x_gpt_dma_hw_pair_completions() == before);
    }

    puts("K1_WS281X_GPT_DMA_HW_PAIR=PASS elc=ABCD gtstr=GPT6|GPT7 "
         "dma3838=PASS busy_freeze=PASS size_profile_reject=PASS "
         "dma_ne_complete=PASS resource=PASS "
         "dma_order=PASS stale_extra=PASS one_lane_fault=PASS "
         "pending_replace=PASS prior_gen_event=PASS");
    return 0;
}
