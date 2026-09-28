/* Register/FSP seam test for the paired GPT/DMA transmitter. Not silicon. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define K1_RA8P1_TARGET 1
#define K1_WS281X_HW_TEST 1
#define K1_PALETTE_WS2816_GPT_PAIR 1
static void k1_test_poll_after_clock(void);
#include "../platform/ra8p1/ws281x_gpt_dma_hw_pair.c"
#include "../platform/ra8p1/ws281x_gpt_dma_pair.c"
#include "../platform/ra8p1/ws281x_gpt_dma.c"

static uint8_t lane0[480];
static uint8_t lane1[480];
static int occupied;
static void (*poll_clock_hook)(void);
static void k1_test_poll_after_clock(void) { if (poll_clock_hook) poll_clock_hook(); }

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
    dma_irqs[0] = dma_irqs[1] = stop_irqs[0] = stop_irqs[1] = 0;
    memset(&pair_first_fault, 0, sizeof(pair_first_fault));
    memset(pair_last_stop, 0, sizeof(pair_last_stop));
    memset(&pair_last_frame, 0, sizeof(pair_last_frame));
    memset(&reset_witness, 0, sizeof(reset_witness));
    previous_stop_valid = 0u;
    dma_complete[0] = dma_complete[1] = 0;
    waveform_complete[0] = waveform_complete[1] = 0;
    hw_stopped[0] = hw_stopped[1] = 0;
    reset_ready[0] = reset_ready[1] = 0;
    lane_fault[0] = lane_fault[1] = 0;
    elc_started = fail_start = fail_reset = irq_mask = 0;
    mstp.MSTPCRC = 0xffffffff;
    memset(pin_cfg_hits, 0, sizeof(pin_cfg_hits));
    poll_clock_hook = NULL;
    DWT->CYCCNT = 100;
}

static void submit(uint32_t generation)
{
    assert(k1_ws281x_gpt_dma_hw_pair_init());
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 3u,
                                            generation) == 0);
    assert(gpt6_ctrl.p_reg->GTCCR[0] == duty_a0[0]);
    assert(gpt6_ctrl.p_reg->GTCCR[2] == duty_a0[1]);
    assert(gpt6_ctrl.p_reg->GTCCR[4] == duty_a0[2]);
    assert(gpt7_ctrl.p_reg == &gpt_regs[8]);
    assert(gpt7_ctrl.p_reg->GTCCR[1] == duty_a1[0]);
    assert(gpt7_ctrl.p_reg->GTCCR[3] == duty_a1[1]);
    assert(gpt7_ctrl.p_reg->GTCCR[5] == duty_a1[2]);
    assert(gpt6_ctrl.p_reg->GTBER == 0x1A0000u);
    assert(gpt7_ctrl.p_reg->GTBER == 0x1A0000u);
    assert(dmac0_ctrl.p_reg->DMCRA == 3837);
    assert(dmac3_ctrl.p_reg->DMCRA == 3837);
    assert(dmac0_ctrl.p_reg->DMSAR == (uint32_t)(uintptr_t)&duty_a0[3]);
    assert(dmac3_ctrl.p_reg->DMSAR == (uint32_t)(uintptr_t)&duty_a1[3]);
    assert(dmac0_ctrl.p_reg->DMDAR ==
           (uint32_t)(uintptr_t)&gpt6_ctrl.p_reg->GTCCR[4]);
    assert(dmac3_ctrl.p_reg->DMDAR ==
           (uint32_t)(uintptr_t)&gpt7_ctrl.p_reg->GTCCR[5]);
    assert(k1_ws281x_gpt_dma_hw_pair_gtstr() == ((1u << 6) | (1u << 8)));
    assert(gpt6_ctrl.p_reg->GTPSR == GPT_SOURCE_GPT_B);
    assert(gpt7_ctrl.p_reg->GTPSR == GPT_SOURCE_GPT_D);
}

static void done_dma(void)
{
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL].DMCRA = 0;
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL_A1].DMCRA = 0;
    dmac0_done(NULL);
    dmac3_done(NULL);
}

/*
 * Software wire model. Not a pin capture.
 *
 * Stop source is independently the configured ELC chain, not an instantaneous
 * compare-edge kill:
 *   GPT6 compare-A → ELC GPT_A → GPT0 event count
 *   GPT0 overflow → ELC GPT_B → GPT6 GTPSR stop
 * FASTLED-PORT-DESIGN.md: compare is not the falling edge; FSP buffered PWM
 * has a one-count output-transition delay; stop must finish in the last bit's
 * low phase before the next rising edge. Overflow still transfers A←C←E while
 * CST is set (GTBER double-buffer).
 *
 * elc_stop_lag_periods:
 *   0 = old verifier (PWM dies at the compare, no further cascade)
 *   1 = last compare arms GPT0 overflow stop; this period still overflows
 *       and cascades once before CST clears — one extra pulse if that
 *       rising edge is emitted
 */
enum { MODEL_BITS = 8, MODEL_CAP = MODEL_BITS + 4 };
#define MODEL_CANARY 0xA5A5A5A5u

struct wire_trace {
    uint32_t emitted[MODEL_CAP];
    uint32_t n;
    uint32_t remaining;
    uint32_t src;
    uint32_t A, C, E, pending, wait;
    int pwm;
};

static void wire_clear(struct wire_trace *w)
{
    uint32_t i;
    for (i = 0; i < MODEL_CAP; ++i) {
        w->emitted[i] = MODEL_CANARY;
    }
    w->n = 0;
    w->pwm = 1;
}

static void wire_emit(struct wire_trace *w, uint32_t value)
{
    assert(w->n < MODEL_CAP);
    w->emitted[w->n] = value;
    w->n += 1u;
}

static void wire_canaries(const struct wire_trace *w)
{
    uint32_t i;
    for (i = w->n; i < MODEL_CAP; ++i) {
        assert(w->emitted[i] == MODEL_CANARY);
    }
}

static void double_buf_cascade(struct wire_trace *w, const uint32_t *duty,
                               uint32_t dma_wait)
{
    if (w->wait) {
        w->wait -= 1u;
        if (w->wait == 0u) {
            w->E = w->pending;
        }
    }
    w->A = w->C;
    w->C = w->E;
    if (w->remaining && w->src < MODEL_BITS) {
        w->pending = duty[w->src++];
        w->remaining -= 1u;
        if (dma_wait != 0u && w->remaining == 0u) {
            w->wait = dma_wait;
        } else if (w->wait == 0u) {
            w->E = w->pending;
        }
    }
}

static void run_double_buf(struct wire_trace *w, const uint32_t *duty,
                           uint32_t stop_at, uint32_t dma_wait,
                           uint32_t elc_stop_lag_periods)
{
    uint32_t compares = 0;
    uint32_t stop_in = 0;
    uint32_t steps;
    w->A = duty[0];
    w->C = duty[1];
    w->E = duty[2];
    w->remaining = MODEL_BITS - 3u;
    w->src = 3u;
    w->pending = 0;
    w->wait = 0;
    wire_clear(w);
    for (steps = 0; steps < MODEL_CAP && w->pwm; ++steps) {
        wire_emit(w, w->A);
        compares += 1u;
        if (compares == stop_at && stop_in == 0u) {
            stop_in = elc_stop_lag_periods + 1u;
        }
        if (stop_in == 1u) {
            w->pwm = 0;
            break;
        }
        double_buf_cascade(w, duty, dma_wait);
        if (stop_in > 1u) {
            stop_in -= 1u;
        }
    }
    wire_canaries(w);
}

static void stop_both(void)
{
    timer_callback_args_t a = {.event = TIMER_EVENT_CYCLE_END};
    gpt_regs[6].GTCR = 0;
    gpt_regs[8].GTCR = 0;
    gpt0_overflow(&a);
    gpt1_overflow(&a);
}

static void interrupted_poll(void)
{
    timer_callback_args_t a = {.event = TIMER_EVENT_CYCLE_END};
    poll_clock_hook = NULL;
    DWT->CYCCNT += 100u;
    gpt_regs[6].GTCR = 0;
    gpt0_overflow(&a);
    DWT->CYCCNT += 200u;
    gpt_regs[8].GTCR = 0;
    gpt1_overflow(&a);
    DWT->CYCCNT += 100u;
}

static void test_reset_interrupt_race(void)
{
    fresh();
    submit(800u);
    done_dma();
    poll_clock_hook = interrupted_poll;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 0u); /* Only 100 cycles since the later stop. */
    assert(owned == 1u);
    assert(reset_witness.stale_clock_rejections == 1u);
    assert(reset_witness.admissions == 0u);
    DWT->CYCCNT = reset_started[1] + reset_cycles;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 1u);
    assert(reset_witness.min_cycles == reset_cycles);
}

static void test_reset_wrap_and_pending(void)
{
    fresh();
    DWT->CYCCNT = UINT32_MAX - 200u;
    submit(801u);
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 3u, 802u)
           == K1_WS281X_SUBMIT_BUSY);
    done_dma();
    poll_clock_hook = interrupted_poll;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 0u && submitted_generation == 801u && pending_valid);
    DWT->CYCCNT = reset_started[1] + reset_cycles - 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 0u && submitted_generation == 801u);
    DWT->CYCCNT += 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 1u && submitted_generation == 802u && !pending_valid);
    assert(reset_witness.admissions == 1u && reset_witness.generation == 801u);
    assert(reset_witness.start_samples == 1u);
    assert(reset_witness.start_min_cycles == reset_cycles);
    assert(reset_witness.previous_generation == 801u);
    assert(reset_witness.next_generation == 802u);
    assert(reset_witness.start_before == DWT->CYCCNT);
    assert(reset_witness.start_after == DWT->CYCCNT);
    assert(reset_witness.min_cycles == reset_cycles);
}

static void test_start_witness_expires_after_idle(void)
{
    fresh();
    submit(803u);
    done_dma();
    stop_both();
    DWT->CYCCNT += reset_cycles;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(previous_stop_valid && completions == 1u);
    DWT->CYCCNT += cpu_hz / 10u + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(!previous_stop_valid);
    /* A later counter wrap must not turn old history into a recent gap. */
    DWT->CYCCNT = reset_started[0] + reset_cycles;
    submit(804u);
    assert(reset_witness.start_samples == 0u);
}

static void test_one_lane_cannot_admit_reset(void)
{
    timer_callback_args_t a = {.event = TIMER_EVENT_CYCLE_END};
    fresh();
    submit(805u);
    done_dma();
    gpt_regs[6].GTCR = 0;
    gpt0_overflow(&a);
    DWT->CYCCNT += reset_cycles;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 0u && reset_witness.admissions == 0u && owned);
    gpt_regs[8].GTCR = 0;
    gpt1_overflow(&a);
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 0u && reset_witness.admissions == 0u);
    DWT->CYCCNT += reset_cycles;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 1u && reset_witness.admissions == 1u);
}

int main(void)
{
    test_reset_interrupt_race();
    test_reset_wrap_and_pending();
    test_start_witness_expires_after_idle();
    test_one_lane_cannot_admit_reset();
    fresh();
    submit(11u);
    assert(elc.ELSR[0].HA_b.ELS == 0x1b7 && elc.ELSR[1].HA_b.ELS == 0x187);
    assert(elc.ELSR[2].HA_b.ELS == 0x1ca && elc.ELSR[3].HA_b.ELS == 0x190);
    uint32_t frozen_a[sizeof(duty_a0)/sizeof(duty_a0[0])];
    uint32_t frozen_b[sizeof(duty_a1)/sizeof(duty_a1[0])];
    memcpy(frozen_a, duty_a0, sizeof(duty_a0));
    memcpy(frozen_b, duty_a1, sizeof(duty_a1));
    lane0[0] = 0xff;
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 3u, 12u) ==
           K1_WS281X_SUBMIT_BUSY);
    assert(memcmp(duty_a0, frozen_a, sizeof(duty_a0)) == 0);
    assert(memcmp(duty_a1, frozen_b, sizeof(duty_a1)) == 0);
    assert(submitted_generation == 11u && pending_generation == 12u);
    done_dma();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
    assert(!k1_ws281x_gpt_dma_pair_lanes_complete(
        dma_complete[0], dma_complete[1], hw_stopped[0], hw_stopped[1],
        waveform_complete[0], waveform_complete[1], reset_ready[0],
        reset_ready[1], lane_fault[0], lane_fault[1], pair_state));
    stop_both();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
    assert(k1_ws281x_gpt_dma_hw_pair_last_stop(0)->valid == 1u);
    assert(k1_ws281x_gpt_dma_hw_pair_last_stop(1)->valid == 1u);
    assert(k1_ws281x_gpt_dma_hw_pair_last_stop(0)->pwm_running == 0u);
    assert(k1_ws281x_gpt_dma_hw_pair_last_stop(1)->pwm_running == 0u);
    assert(k1_ws281x_gpt_dma_hw_pair_last_stop(0)->dmcra == 0u);
    assert(k1_ws281x_gpt_dma_hw_pair_last_stop(1)->dmcra == 0u);
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
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL].DMCRA = 0;
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
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL].DMCRA = 0;
    dmac0_done(NULL);
    stop_both();
    DWT->CYCCNT += reset_cycles + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() >= 1u);

    /* Leftover DMA IRQ after remaining 0: pin already idle-low, not a new bit. */
    fresh();
    submit(43u);
    done_dma();
    dmac0_done(NULL);
    assert(first_fault == K1_WS281X_FAULT_NONE);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);

    /* Leftover stop IRQ after complete: frame already idle-low. */
    fresh();
    submit(44u);
    done_dma();
    stop_both();
    {
        timer_callback_args_t a = {.event = TIMER_EVENT_CYCLE_END};
        gpt1_overflow(&a);
    }
    assert(first_fault == K1_WS281X_FAULT_NONE);

    /* Overflow while PWM still running: ELC hardware stop missed. */
    fresh();
    submit(45u);
    done_dma();
    {
        timer_callback_args_t a = {.event = TIMER_EVENT_CYCLE_END};
        gpt_regs[6].GTCR = R_GPT0_GTCR_CST_Msk;
        gpt0_overflow(&a);
        assert(first_fault == K1_WS281X_FAULT_MISSING_STOP);
        assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
        assert(k1_ws281x_gpt_dma_hw_pair_last_stop(0)->valid == 1u);
        assert(k1_ws281x_gpt_dma_hw_pair_last_stop(0)->pwm_running == 1u);
    }

    /* Lane A DMA complete must not retouch P604 while lane B is still shifting. */
    fresh();
    submit(49u);
    {
        const uint32_t b_hits = pin_cfg_hits[6][4];
        dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL].DMCRA = 0;
        dmac0_done(NULL);
        assert(first_fault == K1_WS281X_FAULT_NONE);
        assert(pin_cfg_hits[6][4] == b_hits);
        assert(waveform_complete[0] == 0u);
    }

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

    /* Fault 6 site: DMA callback with remaining count 1 on B. */
    fresh();
    submit(61u);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL].DMCRA = 0;
    dmac0_done(NULL);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL_A1].DMCRA = 1u;
    dmac3_done(NULL);
    assert(pair_first_fault.valid == 1u);
    assert(pair_first_fault.site == K1_WS281X_PAIR_FAULT_SITE_DMA_CALLBACK_COUNT_NONZERO);
    assert(pair_first_fault.lane == 1u);
    assert(pair_first_fault.generation == 61u);
    assert(pair_first_fault.dmcra[1] == 1u);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL_A1].DMCRA = 99u;
    dmac3_done(NULL);
    assert(pair_first_fault.dmcra[1] == 1u);
    assert(pair_first_fault.site == K1_WS281X_PAIR_FAULT_SITE_DMA_CALLBACK_COUNT_NONZERO);

    /* Fault 6 site: DMA callback remaining near the start of the transfer. */
    fresh();
    submit(62u);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL].DMCRA = 0;
    dmac0_done(NULL);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL_A1].DMCRA = 3837u;
    dmac3_done(NULL);
    assert(pair_first_fault.site == K1_WS281X_PAIR_FAULT_SITE_DMA_CALLBACK_COUNT_NONZERO);
    assert(pair_first_fault.dmcra[1] == 3837u);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);

    /* Remaining 1 while PWM still running: ELC failed to stop the wire. */
    fresh();
    submit(67u);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL].DMCRA = 0;
    dmac0_done(NULL);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL_A1].DMCRA = 1u;
    gpt_regs[8].GTCR = R_GPT0_GTCR_CST_Msk;
    {
        timer_callback_args_t a = {.event = TIMER_EVENT_CYCLE_END};
        gpt1_overflow(&a);
    }
    assert(first_fault == K1_WS281X_FAULT_MISSING_STOP);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);

    /* Leftover DMA IRQ after complete is not a mid-frame extra event. */
    fresh();
    submit(68u);
    done_dma();
    dmac0_done(NULL);
    assert(first_fault == K1_WS281X_FAULT_NONE);

    /* Fault 6 site: stop callback with remaining count 1 on B. */
    fresh();
    submit(63u);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL].DMCRA = 0;
    dmac0_done(NULL);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL_A1].DMCRA = 1u;
    stop_both();
    assert(pair_first_fault.valid == 1u);
    assert(pair_first_fault.site == K1_WS281X_PAIR_FAULT_SITE_STOP_CALLBACK_COUNT_NONZERO);
    assert(pair_first_fault.lane == 1u);
    assert(pair_first_fault.dmcra[1] == 1u);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);

    /* Zero remaining at stop, delayed DMA callback: complete, no fault 6. */
    fresh();
    submit(64u);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL].DMCRA = 0;
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL_A1].DMCRA = 0;
    stop_both();
    assert(pair_first_fault.valid == 0u);
    dmac0_done(NULL);
    dmac3_done(NULL);
    DWT->CYCCNT += reset_cycles + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(k1_ws281x_gpt_dma_hw_pair_completions() >= 1u);
    assert(pair_first_fault.valid == 0u);

    /* Pending replacement then B remaining count 1: first fault preserved, no complete. */
    fresh();
    submit(65u);
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 3u, 66u) ==
           K1_WS281X_SUBMIT_BUSY);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL].DMCRA = 0;
    dmac0_done(NULL);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL_A1].DMCRA = 1u;
    dmac3_done(NULL);
    assert(pair_first_fault.site == K1_WS281X_PAIR_FAULT_SITE_DMA_CALLBACK_COUNT_NONZERO);
    assert(pair_first_fault.generation == 65u);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);
    dmac3_done(NULL);
    assert(pair_first_fault.generation == 65u);

    /* Double-buffer event order. Software model, not a scope capture.
       Instant-at-compare (lag 0) is the old verifier assumption.
       ELC overflow stop (lag 1) is the configured GPT0-overflow chain. */
    {
        uint32_t duty[MODEL_BITS] = {10, 20, 30, 40, 50, 60, 70, 80};
        struct wire_trace tr;
        uint32_t i, A, C, remaining, src, pending, wait, compares;

        run_double_buf(&tr, duty, MODEL_BITS, 0u, 0u);
        assert(tr.n == MODEL_BITS);
        for (i = 0; i < MODEL_BITS; ++i) {
            assert(tr.emitted[i] == duty[i]);
        }
        assert(!tr.pwm);

        /* ELC lag 1: last compare arms stop; overflow cascade still runs.
           That extra period is the extra-edge case the old buffer could not
           store without writing emitted[BITS]. */
        run_double_buf(&tr, duty, MODEL_BITS, 0u, 1u);
        assert(tr.n == MODEL_BITS + 1u);
        for (i = 0; i < MODEL_BITS; ++i) {
            assert(tr.emitted[i] == duty[i]);
        }
        assert(tr.emitted[MODEL_BITS] != MODEL_CANARY);
        assert(!tr.pwm);

        /* Old single-buffer: last DMA at overflow BITS-3. If that write is
           still pending at overflow BITS-2, the last bit is stale. */
        A = duty[0]; C = duty[1]; remaining = MODEL_BITS - 2u; src = 2u;
        pending = 0; wait = 0; compares = 0;
        wire_clear(&tr);
        for (i = 0; i < MODEL_BITS; ++i) {
            wire_emit(&tr, A);
            compares += 1u;
            if (compares == MODEL_BITS) {
                break;
            }
            if (wait) {
                wait -= 1u;
                if (wait == 0u) {
                    C = pending;
                }
            }
            A = C;
            if (remaining && src < MODEL_BITS) {
                pending = duty[src++];
                remaining -= 1u;
                wait = (remaining == 0u) ? 2u : 0u;
                if (wait == 0u) {
                    C = pending;
                }
            }
        }
        wire_canaries(&tr);
        assert(tr.n == MODEL_BITS);
        assert(tr.emitted[MODEL_BITS - 1u] != duty[MODEL_BITS - 1u]);
        assert(remaining == 1u || wait != 0u);

        run_double_buf(&tr, duty, MODEL_BITS - 1u, 0u, 0u);
        assert(tr.n == MODEL_BITS - 1u);
        assert(tr.emitted[MODEL_BITS - 2u] == duty[MODEL_BITS - 2u]);

        run_double_buf(&tr, duty, MODEL_BITS + 1u, 0u, 0u);
        assert(tr.n == MODEL_BITS + 1u);
        for (i = 0; i < MODEL_BITS; ++i) {
            assert(tr.emitted[i] == duty[i]);
        }
    }

    /* Remaining 1 at a stopped last compare is still a fault, not success. */
    fresh();
    submit(73u);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL].DMCRA = 0;
    dmac0_done(NULL);
    dma_channel_regs[K1_WS281X_GPT_DMA_CHANNEL_A1].DMCRA = 1u;
    stop_both();
    assert(first_fault == K1_WS281X_FAULT_DMA);
    assert(k1_ws281x_gpt_dma_hw_pair_completions() == 0);

    /* Pending coloured work is replaced by a later black pair. */
    fresh();
    submit(74u);
    memset(lane0, 0x11, sizeof(lane0));
    memset(lane1, 0x22, sizeof(lane1));
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 3u, 75u) ==
           K1_WS281X_SUBMIT_BUSY);
    memset(lane0, 0, sizeof(lane0));
    memset(lane1, 0, sizeof(lane1));
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 3u, 76u) ==
           K1_WS281X_SUBMIT_BUSY);
    assert(pending_generation == 76u);
    assert(pending_a0[0] == 0 && pending_a1[0] == 0);
    done_dma();
    stop_both();
    DWT->CYCCNT += reset_cycles + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(submitted_generation == 76u);
    assert(owned_a0[0] == 0 && owned_a1[0] == 0);

    /* A lifetime stop IRQ is not a stop receipt for the next generation. */
    fresh();
    submit(71u);
    done_dma();
    stop_both();
    DWT->CYCCNT += reset_cycles + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 1u && stop_irqs[0] > 0u && stop_irqs[1] > 0u);
    submit(72u);
    gpt_regs[6].GTCR = R_GPT0_GTCR_CST_Msk;
    gpt_regs[8].GTCR = R_GPT0_GTCR_CST_Msk;
    done_dma();
    assert(waveform_complete[0] == 0u && waveform_complete[1] == 0u);
    assert(hw_stopped[0] == 0u && hw_stopped[1] == 0u);
    DWT->CYCCNT += reset_cycles + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 1u && submitted_generation == 72u);
    assert(completed_generation == 71u && owned == 1u);
    stop_both();
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 1u);
    DWT->CYCCNT += reset_cycles + 1u;
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(completions == 2u && completed_generation == 72u);

    /* USB admission observes hardware without retiring or starting work. */
    fresh();
    assert(!k1_ws281x_gpt_dma_hw_pair_transmitting());
    submit(801u);
    gpt_regs[6].GTCR = R_GPT0_GTCR_CST_Msk;
    gpt_regs[8].GTCR = 0u;
    assert(k1_ws281x_gpt_dma_hw_pair_transmitting());
    gpt_regs[6].GTCR = 0u;
    gpt_regs[8].GTCR = R_GPT0_GTCR_CST_Msk;
    assert(k1_ws281x_gpt_dma_hw_pair_transmitting());
    assert(k1_ws281x_gpt_dma_hw_pair_submit(lane0, 480, lane1, 480, 3u, 802u) ==
           K1_WS281X_SUBMIT_BUSY);
    done_dma();
    stop_both();
    DWT->CYCCNT += reset_cycles + 1u;
    assert(!k1_ws281x_gpt_dma_hw_pair_transmitting());
    assert(submitted_generation == 801u && pending_valid && completions == 0u);
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    assert(submitted_generation == 802u && completions == 1u);

    puts("K1_WS281X_GPT_DMA_HW_PAIR=PASS elc=ABCD gtstr=GPT6|GPT8 "
         "dma3837=PASS busy_freeze=PASS size_profile_reject=PASS "
         "dma_ne_complete=PASS resource=PASS "
         "dma_order=PASS stale_extra=PASS one_lane_fault=PASS "
         "pending_replace=PASS prior_gen_event=PASS "
         "fault6_dma_site=PASS fault6_stop_site=PASS "
         "zero_count_stop_then_dma=PASS first_fault_preserve=PASS "
         "next_generation_requires_actual_stop=PASS "
         "double_buffer_last_bit=PASS remaining1_still_fault=PASS "
         "pending_black_replace=PASS elc_stop_lag=PASS "
         "extra_pulse_no_oob=PASS");
    return 0;
}
