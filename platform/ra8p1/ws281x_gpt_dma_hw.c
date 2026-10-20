#include "ws281x_gpt_dma_hw.h"

#ifdef K1_WS281X_HW_TEST
#include "ws281x_hw_mock.h"
#else
#include "common_data.h"
#include "r_dmac.h"
#include "r_gpt.h"
#include "r_ioport.h"
#include "vector_data.h"
#endif
#include <stdint.h>
#include "titan_led_pins.h"
#include "ws281x_diag.h"

#include <string.h>

#ifndef K1_PALETTE_WS2816_GPT_PAIR

#ifndef K1_RA8P1_TARGET
#error "ws281x_gpt_dma_hw.c is the RA8P1 transmitter; do not compile it on host"
#endif

#ifdef K1_PALETTE_WS2816
#error "single-lane GPT DMA cannot share emit with the GPIO WS2816 emitter"
#endif

/* Default channel is DMAC2. led-first remaps LED to DMAC0; PDM fall still
   occupies DMAC2, so this vector remains required on live-audio images. */
#ifndef VECTOR_NUMBER_DMAC2_INT
#error "DMAC2 vector is required; stage_led_gpt_vectors must add it"
#endif

#define GPT_PRV_GTCCRA 0u
#define GPT_PRV_GTCCRC 2u

static gpt_instance_ctrl_t gpt6_ctrl;
static gpt_instance_ctrl_t gpt0_ctrl;
static dmac_instance_ctrl_t dmac_ctrl;
static transfer_info_t dmac_info;
static uint32_t duty[K1_WS281X_GPT_DUTY_CAP] __attribute__((aligned(32)));
static k1_ws281x_duty_meta_t meta;
static volatile uint32_t state;
static volatile uint32_t owned;
static volatile uint32_t dma_complete;
static volatile uint32_t waveform_complete;
static volatile uint32_t reset_ready;
static volatile uint32_t fault;
static volatile uint32_t completions;
static volatile uint32_t errors;
static volatile uint32_t ownership_violations;
static uint32_t clock_hz;
static uint32_t cpu_hz;
static uint32_t bits;
static uint32_t reset_cycles;
static uint32_t timeout_cycles;
static uint32_t emit_started;
static uint32_t stop_cycles;
static uint32_t reset_started;
static uint32_t pfs_after;
static uint32_t initialised;
static uint32_t opened;
static uint32_t attempts;
static volatile uint32_t dma_irqs, stop_irqs, first_fault;
static k1_ws281x_hw_snapshot_t snapshots[5];
static k1_ws281x_first_fault_witness_t fault_witness;
static uint32_t witness_prepared;
static uint32_t last_prepare_cycles, maximum_prepare_cycles;
static void stop_hardware(void);
static void snapshot(unsigned index, uint32_t tag)
{
    k1_ws281x_hw_snapshot_t *d = &snapshots[index];
    if (d->tag) return; /* First frame/first fault only: bounded ISR work. */
    d->attempt=attempts; d->cycles=DWT->CYCCNT; d->state=state; d->fault=fault;
    if (gpt6_ctrl.p_reg) {
        d->gpt6_gtcr=gpt6_ctrl.p_reg->GTCR; d->gpt6_gtcnt=gpt6_ctrl.p_reg->GTCNT;
        d->gpt6_gtpr=gpt6_ctrl.p_reg->GTPR; d->gpt6_gtccra=gpt6_ctrl.p_reg->GTCCR[0];
        d->gpt6_gtccrc=gpt6_ctrl.p_reg->GTCCR[2]; d->gpt6_gtber=gpt6_ctrl.p_reg->GTBER;
        d->gpt6_gtior=gpt6_ctrl.p_reg->GTIOR; d->gpt6_gtpsr=gpt6_ctrl.p_reg->GTPSR;
    }
    if (gpt0_ctrl.p_reg) {
        d->gpt0_gtcr=gpt0_ctrl.p_reg->GTCR; d->gpt0_gtcnt=gpt0_ctrl.p_reg->GTCNT;
        d->gpt0_gtpr=gpt0_ctrl.p_reg->GTPR; d->gpt0_gtupsr=gpt0_ctrl.p_reg->GTUPSR;
    }
    if (dmac_ctrl.p_reg) {
        d->dmac_count=dmac_ctrl.p_reg->DMCRA; d->dmac_src=dmac_ctrl.p_reg->DMSAR;
        d->dmac_dest=dmac_ctrl.p_reg->DMDAR; d->dmac_enable=dmac_ctrl.p_reg->DMCNT;
        d->dmac_mode=dmac_ctrl.p_reg->DMTMD; d->dmac_irq=dmac_ctrl.p_reg->DMINT;
        d->dmac_request=dmac_ctrl.p_reg->DMREQ;
    }
    d->delink=R_ICU->DELSR[K1_WS281X_GPT_DMA_CHANNEL];
    d->elc_a=R_ELC->ELSR[ELC_PERIPHERAL_GPT_A].HA_b.ELS;
    d->elc_b=R_ELC->ELSR[ELC_PERIPHERAL_GPT_B].HA_b.ELS;
    d->elc_enable=R_ELC->ELCR_b.ELCON; d->mstpcrc=R_MSTP->MSTPCRC;
    d->gpt_irq_owner=R_FSP_IsrContextGet(GPT0_COUNTER_OVERFLOW_IRQn)==&gpt0_ctrl;
    d->dma_irq_owner=R_FSP_IsrContextGet(K1_WS281X_GPT_DMA_IRQ)==&dmac_ctrl;
    d->gpt_irq_pending=NVIC_GetPendingIRQ(GPT0_COUNTER_OVERFLOW_IRQn);
    d->dma_irq_pending=NVIC_GetPendingIRQ(K1_WS281X_GPT_DMA_IRQ);
    d->pfs=R_PFS->PORT[6].PIN[1].PmnPFS;
    __DMB(); d->tag=tag;
}

static gpt_extended_cfg_t gpt6_extend = {
    .gtioca = {.output_enabled = true, .stop_level = GPT_PIN_LEVEL_LOW},
    .gtiocb = {.output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW},
    .start_source = GPT_SOURCE_NONE,
    .stop_source = GPT_SOURCE_GPT_B,
    .clear_source = GPT_SOURCE_NONE,
    .capture_a_source = GPT_SOURCE_NONE,
    .capture_b_source = GPT_SOURCE_NONE,
    .count_up_source = GPT_SOURCE_NONE,
    .count_down_source = GPT_SOURCE_NONE,
    .capture_filter_gtioca = GPT_CAPTURE_FILTER_NONE,
    .capture_filter_gtiocb = GPT_CAPTURE_FILTER_NONE,
    .capture_a_ipl = BSP_IRQ_DISABLED,
    .capture_b_ipl = BSP_IRQ_DISABLED,
    .compare_match_c_ipl = BSP_IRQ_DISABLED,
    .compare_match_d_ipl = BSP_IRQ_DISABLED,
    .compare_match_e_ipl = BSP_IRQ_DISABLED,
    .compare_match_f_ipl = BSP_IRQ_DISABLED,
    .capture_a_irq = FSP_INVALID_VECTOR,
    .capture_b_irq = FSP_INVALID_VECTOR,
    .compare_match_c_irq = FSP_INVALID_VECTOR,
    .compare_match_d_irq = FSP_INVALID_VECTOR,
    .compare_match_e_irq = FSP_INVALID_VECTOR,
    .compare_match_f_irq = FSP_INVALID_VECTOR,
    .compare_match_value = {0, 0, 0, 0, 0, 0},
    .compare_match_status = 0u,
    .p_pwm_cfg = NULL,
    .gtior_setting.gtior = 0u,
    .gtioca_polarity = GPT_GTIOC_POLARITY_NORMAL,
    .gtiocb_polarity = GPT_GTIOC_POLARITY_NORMAL,
};

static timer_cfg_t gpt6_cfg = {
    .mode = TIMER_MODE_PWM,
    .period_counts = 250u,
    .duty_cycle_counts = 50u,
    .source_div = TIMER_SOURCE_DIV_1,
    .channel = K1_WS281X_GPT_TIMER,
    .p_callback = NULL,
    .p_context = NULL,
    .p_extend = &gpt6_extend,
    .cycle_end_ipl = BSP_IRQ_DISABLED,
    .cycle_end_irq = FSP_INVALID_VECTOR,
};

static void gpt0_overflow(timer_callback_args_t *args);

static gpt_extended_cfg_t gpt0_extend = {
    .gtioca = {.output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW},
    .gtiocb = {.output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW},
    .start_source = GPT_SOURCE_NONE,
    .stop_source = GPT_SOURCE_NONE,
    .clear_source = GPT_SOURCE_NONE,
    .capture_a_source = GPT_SOURCE_NONE,
    .capture_b_source = GPT_SOURCE_NONE,
    .count_up_source = GPT_SOURCE_GPT_A,
    .count_down_source = GPT_SOURCE_NONE,
    .capture_filter_gtioca = GPT_CAPTURE_FILTER_NONE,
    .capture_filter_gtiocb = GPT_CAPTURE_FILTER_NONE,
    .capture_a_ipl = BSP_IRQ_DISABLED,
    .capture_b_ipl = BSP_IRQ_DISABLED,
    .compare_match_c_ipl = BSP_IRQ_DISABLED,
    .compare_match_d_ipl = BSP_IRQ_DISABLED,
    .compare_match_e_ipl = BSP_IRQ_DISABLED,
    .compare_match_f_ipl = BSP_IRQ_DISABLED,
    .capture_a_irq = FSP_INVALID_VECTOR,
    .capture_b_irq = FSP_INVALID_VECTOR,
    .compare_match_c_irq = FSP_INVALID_VECTOR,
    .compare_match_d_irq = FSP_INVALID_VECTOR,
    .compare_match_e_irq = FSP_INVALID_VECTOR,
    .compare_match_f_irq = FSP_INVALID_VECTOR,
    .compare_match_value = {0, 0, 0, 0, 0, 0},
    .compare_match_status = 0u,
    .p_pwm_cfg = NULL,
    .gtior_setting.gtior = 0u,
    .gtioca_polarity = GPT_GTIOC_POLARITY_NORMAL,
    .gtiocb_polarity = GPT_GTIOC_POLARITY_NORMAL,
};

static timer_cfg_t gpt0_cfg = {
    .mode = TIMER_MODE_PERIODIC,
    .period_counts = 8u,
    .duty_cycle_counts = 0u,
    .source_div = TIMER_SOURCE_DIV_1,
    .channel = K1_WS281X_GPT_EVENT_TIMER,
    .p_callback = gpt0_overflow,
    .p_context = NULL,
    .p_extend = &gpt0_extend,
    .cycle_end_ipl = 12u,
    .cycle_end_irq = GPT0_COUNTER_OVERFLOW_IRQn,
};

static void dmac_done(dmac_callback_args_t *args);

static const dmac_extended_cfg_t dmac_extend = {
    .channel = K1_WS281X_GPT_DMA_CHANNEL,
    .irq = K1_WS281X_GPT_DMA_IRQ,
    .ipl = 12u,
    .offset = 0,
    .src_buffer_size = 0u,
    .activation_source = ELC_EVENT_GPT6_COUNTER_OVERFLOW,
    .p_callback = dmac_done,
    .p_callback_memory = NULL,
    .p_context = NULL,
};

static const transfer_cfg_t dmac_cfg = {
    .p_info = &dmac_info,
    .p_extend = &dmac_extend,
};

static uint32_t gpt_to_cpu(uint32_t gpt_counts)
{
    if (clock_hz == 0u || cpu_hz == 0u) {
        return gpt_counts;
    }
    return (uint32_t)(((uint64_t)gpt_counts * (uint64_t)cpu_hz +
                       (uint64_t)clock_hz - 1ull) /
                      (uint64_t)clock_hz);
}

static void set_fault(uint32_t code)
{
    fault = code;
    if (!first_fault) first_fault=code;
    snapshot(4, 5); /* Before cleanup changes the registers. */
    if (!fault_witness.valid) {
        if (!witness_prepared) memset(&fault_witness, 0, sizeof(fault_witness));
        fault_witness.terminal = snapshots[4];
        fault_witness.elapsed_cycles = fault_witness.start_requested
            ? (uint32_t)(fault_witness.terminal.cycles - emit_started) : 0u;
        fault_witness.cpu_hz = cpu_hz;
        fault_witness.dma_complete = dma_complete;
        fault_witness.waveform_complete = waveform_complete;
        fault_witness.reset_ready = reset_ready;
        fault_witness.dmsts = dmac_ctrl.p_reg ? dmac_ctrl.p_reg->DMSTS : 0u;
        fault_witness.dmctl = R_DMA->DMCTL;
        __DMB();
        fault_witness.valid = 1u;
    }
    if (opened) stop_hardware();
    state = K1_WS281X_TX_FAULT;
    owned = 0u;
    errors += 1u;
}

static void link_elc(void)
{
    /* RA8P1 manual 20.4.3: ELC starts in module-stop, register access requires
       release first. ELCON alone cannot start a clock-gated ELC. */
    R_BSP_MODULE_START(FSP_IP_ELC, 0);
    __DSB();
    /* GPT6 compare-A -> ELC GPT A -> GPT0 count-up.
       GPT0 overflow -> ELC GPT B -> GPT6 hardware stop.
       Write only those two slots. */
    R_ELC->ELSR[ELC_PERIPHERAL_GPT_A].HA_b.ELS =
        (uint16_t)ELC_EVENT_GPT6_CAPTURE_COMPARE_A;
    R_ELC->ELSR[ELC_PERIPHERAL_GPT_B].HA_b.ELS =
        (uint16_t)ELC_EVENT_GPT0_COUNTER_OVERFLOW;
    R_ELC->ELCR_b.ELCON = 1u;
}

static int configure_pin(void)
{
    volatile uint32_t *const pfs = &R_PFS->PORT[6].PIN[1].PmnPFS;
    const uint32_t config = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN |
                            (uint32_t)IOPORT_PERIPHERAL_GPT1 |
                            (uint32_t)IOPORT_CFG_DRIVE_HIGH;
    fsp_err_t error;
    R_BSP_PinAccessEnable();
    error = R_IOPORT_PinCfg(&g_ioport_ctrl, (bsp_io_port_pin_t)K1_TITAN_DIN_A,
                            config);
    R_BSP_PinAccessDisable();
    pfs_after = *pfs;
    if (error != FSP_SUCCESS) {
        return 0;
    }
    const uint32_t mask=R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (0x1fu << 24);
    if ((pfs_after & mask) != (config & mask)) {
        return 0;
    }
    return 1;
}

static void force_idle_low(void)
{
    const uint32_t gpio = IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                          IOPORT_CFG_PORT_OUTPUT_LOW | IOPORT_CFG_DRIVE_HIGH;
    R_BSP_PinAccessEnable();
    (void)R_IOPORT_PinCfg(&g_ioport_ctrl, (bsp_io_port_pin_t)K1_TITAN_DIN_A,
                          gpio);
    R_PORT6->PCNTR3 = (1u << 1) << 16;
    R_BSP_PinAccessDisable();
    (void)configure_pin();
}

static void clean_duty(uint32_t count)
{
    if ((SCB->CCR & SCB_CCR_DC_Msk) != 0u && count != 0u) {
        SCB_CleanDCache_by_Addr((void *)duty,
                                (int32_t)(count * sizeof(uint32_t)));
        __DSB();
        __ISB();
    }
}

static void dmac_done(dmac_callback_args_t *args)
{
    (void)args;
    if (!owned || (state != K1_WS281X_TX_EMITTING &&
                   state != K1_WS281X_TX_DMA_COMPLETE &&
                   state != K1_WS281X_TX_WAVEFORM_COMPLETE &&
                   state != K1_WS281X_TX_LATCH_PENDING)) return;
    if (dmac_ctrl.p_reg->DMCRA != 0u) { set_fault(K1_WS281X_FAULT_DMA); return; }
    if (dma_complete) { set_fault(K1_WS281X_FAULT_EXTRA_EVENT); return; }
    ++dma_irqs;
    snapshot(2, 3);
    dma_complete = 1u;
    if (state == K1_WS281X_TX_EMITTING) {
        state = K1_WS281X_TX_DMA_COMPLETE;
    }
}

static void gpt0_overflow(timer_callback_args_t *args)
{
    if (!owned || !args || args->event != TIMER_EVENT_CYCLE_END) return;
    if (waveform_complete) { set_fault(K1_WS281X_FAULT_EXTRA_EVENT); return; }
    ++stop_irqs;
    snapshot(3, 4); /* Observe hardware stop BEFORE any CPU stop. */
    if (gpt6_ctrl.p_reg->GTCR & R_GPT0_GTCR_CST_Msk) {
        set_fault(K1_WS281X_FAULT_MISSING_STOP); return;
    }
    /* Preloaded words are already excluded from the DMA length. Any remaining
       word means the submitted stream was not consumed. Snapshot before cleanup;
       never fabricate DMA completion or credit an incomplete frame. */
    if (dmac_ctrl.p_reg->DMCRA != 0u) {
        set_fault(K1_WS281X_FAULT_DMA);
        return;
    }
    stop_cycles = DWT->CYCCNT;
    waveform_complete = 1u;
    (void)R_GPT_Stop(&gpt0_ctrl);
    reset_started = DWT->CYCCNT;
    state = K1_WS281X_TX_WAVEFORM_COMPLETE;
}

static void stop_hardware(void)
{
    (void)R_DMAC_Disable(&dmac_ctrl);
    (void)R_GPT_Stop(&gpt6_ctrl);
    (void)R_GPT_Stop(&gpt0_ctrl);
    force_idle_low();
}

int k1_ws281x_gpt_dma_hw_init(void)
{
    timer_info_t info;
    fsp_err_t error;
    if (initialised) {
        return fault == K1_WS281X_FAULT_NONE ? 1 : 0;
    }
    /* The BSP also defines g_timer0: a separate control object is not a lock. */
    if (R_FSP_IsrContextGet(GPT0_COUNTER_OVERFLOW_IRQn) != NULL ||
        R_FSP_IsrContextGet(K1_WS281X_GPT_DMA_IRQ) != NULL) {
        set_fault(K1_WS281X_FAULT_RESOURCE); return 0;
    }
    memset(&dmac_info, 0, sizeof(dmac_info));
    dmac_info.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED;
    dmac_info.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
    dmac_info.transfer_settings_word_b.irq = TRANSFER_IRQ_END;
    dmac_info.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED;
    dmac_info.transfer_settings_word_b.src_addr_mode =
        TRANSFER_ADDR_MODE_INCREMENTED;
    dmac_info.transfer_settings_word_b.size = TRANSFER_SIZE_4_BYTE;
    dmac_info.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;
    dmac_info.p_dest = NULL;
    dmac_info.p_src = NULL;
    dmac_info.length = 0u;
    dmac_info.num_blocks = 0u;

    error = R_GPT_Open(&gpt6_ctrl, &gpt6_cfg);
    if (error != FSP_SUCCESS) {
        fault = K1_WS281X_FAULT_RESOURCE;
        return 0;
    }
    error = R_GPT_Open(&gpt0_ctrl, &gpt0_cfg);
    if (error != FSP_SUCCESS) {
        (void)R_GPT_Close(&gpt6_ctrl);
        fault = K1_WS281X_FAULT_RESOURCE;
        return 0;
    }
    error = R_DMAC_Open(&dmac_ctrl, &dmac_cfg);
    if (error != FSP_SUCCESS) {
        (void)R_GPT_Close(&gpt0_ctrl);
        (void)R_GPT_Close(&gpt6_ctrl);
        fault = K1_WS281X_FAULT_DMA;
        return 0;
    }
    if (R_GPT_InfoGet(&gpt6_ctrl, &info) != FSP_SUCCESS ||
        info.clock_frequency == 0u) {
        (void)R_DMAC_Close(&dmac_ctrl);
        (void)R_GPT_Close(&gpt0_ctrl);
        (void)R_GPT_Close(&gpt6_ctrl);
        fault = K1_WS281X_FAULT_CLOCK;
        return 0;
    }
    clock_hz = info.clock_frequency;
    cpu_hz = SystemCoreClock;
    if (cpu_hz == 0u) {
        (void)R_DMAC_Close(&dmac_ctrl);
        (void)R_GPT_Close(&gpt0_ctrl);
        (void)R_GPT_Close(&gpt6_ctrl);
        fault = K1_WS281X_FAULT_CLOCK;
        return 0;
    }
    if (!configure_pin()) {
        (void)R_DMAC_Close(&dmac_ctrl);
        (void)R_GPT_Close(&gpt0_ctrl);
        (void)R_GPT_Close(&gpt6_ctrl);
        fault = K1_WS281X_FAULT_PIN;
        return 0;
    }
    link_elc();
    /* Distinguish HIGH-at-start (GTIOA[4]) from LOW-at-stop (OADFLT).
       FSP's stop_level default uses LOW for both, losing the first pulse. */
    gpt6_ctrl.p_reg->GTIOR |= 1u << 4;
    force_idle_low();
    opened = 1u;
    initialised = 1u;
    state = K1_WS281X_TX_IDLE;
    fault = K1_WS281X_FAULT_NONE;
    return 1;
}

int k1_ws281x_gpt_dma_hw_submit(const uint8_t *bytes, size_t nbytes,
                                uint32_t profile)
{
    k1_ws281x_diag_timing_t timing;
    uint32_t dma_count;
    uint32_t prepare_started;
    fsp_err_t error;
    if (!initialised || !opened || state == K1_WS281X_TX_FAULT) {
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    (void)k1_ws281x_gpt_dma_hw_poll();
    if (state == K1_WS281X_TX_FAULT) return K1_WS281X_SUBMIT_UNAVAILABLE;
    if (owned) {
        ownership_violations += 1u;
        return K1_WS281X_SUBMIT_BUSY;
    }
    witness_prepared = 0u;
    prepare_started = DWT->CYCCNT;
    if (bytes == 0 || nbytes == 0u || nbytes > K1_WS281X_GPT_WITNESS_PAYLOAD_CAP ||
        !k1_ws281x_diag_profile(profile, &timing) ||
        !k1_ws281x_build_gpt_duty(bytes, nbytes, profile, clock_hz, duty,
                                  K1_WS281X_GPT_DUTY_CAP, &meta)) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    if (!configure_pin()) {
        set_fault(K1_WS281X_FAULT_PIN);
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    if (state == K1_WS281X_TX_FAULT) return K1_WS281X_SUBMIT_UNAVAILABLE;
    if (R_FSP_IsrContextGet(GPT0_COUNTER_OVERFLOW_IRQn) != &gpt0_ctrl ||
        R_FSP_IsrContextGet(K1_WS281X_GPT_DMA_IRQ) != &dmac_ctrl) {
        set_fault(K1_WS281X_FAULT_RESOURCE); return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    ++attempts;
    bits = meta.bits;
    if (bits < 2u) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    dma_count = bits > 2u ? bits - 2u : 0u;
    /* Retain the current submission, not the first successful frame. Faults
       latch this storage; subsequent submissions cannot overwrite it. */
    memset(&fault_witness, 0, sizeof(fault_witness));
    fault_witness.frame_id = attempts;
    fault_witness.profile = profile;
    fault_witness.payload_bytes = (uint32_t)nbytes;
    memcpy(fault_witness.packed_grb, bytes, nbytes);
    fault_witness.payload_fnv1a32 = 2166136261u;
    for (size_t i = 0u; i < nbytes; ++i)
        fault_witness.payload_fnv1a32 =
            (fault_witness.payload_fnv1a32 ^ bytes[i]) * 16777619u;
    fault_witness.duty_fnv1a32 = 2166136261u;
    for (uint32_t i = 0u; i < bits; ++i)
        for (unsigned shift = 0u; shift < 32u; shift += 8u)
            fault_witness.duty_fnv1a32 =
                (fault_witness.duty_fnv1a32 ^ ((duty[i] >> shift) & 255u)) * 16777619u;
    fault_witness.expected_dma_words = dma_count;
    fault_witness.dma_source_start = (uint32_t)(uintptr_t)&duty[2];
    witness_prepared = 1u;
    reset_cycles = gpt_to_cpu((uint32_t)(((uint64_t)timing.reset_us * 1000ull *
                                          (uint64_t)clock_hz +
                                          999999999ull) /
                                         1000000000ull));
    timeout_cycles =
        gpt_to_cpu((bits + 4u) * meta.period_counts) + reset_cycles +
        cpu_hz / 100u;
    clean_duty(bits);
    (void)R_GPT_Stop(&gpt6_ctrl);
    (void)R_GPT_Stop(&gpt0_ctrl);
    (void)R_DMAC_Disable(&dmac_ctrl);
    error = R_GPT_PeriodSet(&gpt6_ctrl, meta.period_counts);
    if (error != FSP_SUCCESS) {
        set_fault(K1_WS281X_FAULT_RESOURCE);
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    error = R_GPT_PeriodSet(&gpt0_ctrl, bits);
    if (error != FSP_SUCCESS) {
        set_fault(K1_WS281X_FAULT_RESOURCE);
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    if (R_GPT_Reset(&gpt6_ctrl) != FSP_SUCCESS ||
        R_GPT_Reset(&gpt0_ctrl) != FSP_SUCCESS) {
        set_fault(K1_WS281X_FAULT_RESOURCE); return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    gpt6_ctrl.p_reg->GTCCR[GPT_PRV_GTCCRA] = duty[0];
    gpt6_ctrl.p_reg->GTCCR[GPT_PRV_GTCCRC] =
        bits > 1u ? duty[1] : duty[0];
    if (gpt6_ctrl.p_reg->GTCCR[GPT_PRV_GTCCRA] != duty[0] ||
        gpt6_ctrl.p_reg->GTCCR[GPT_PRV_GTCCRC] != duty[1]) {
        set_fault(K1_WS281X_FAULT_SHIFTED_PRELOAD);
        force_idle_low();
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    dma_complete = dma_count == 0u ? 1u : 0u;
    waveform_complete = 0u;
    reset_ready = 0u;
    fault = K1_WS281X_FAULT_NONE;
    if (dma_count > 0u) {
        error = R_DMAC_Reset(&dmac_ctrl, &duty[2],
                             (void *)&gpt6_ctrl.p_reg->GTCCR[GPT_PRV_GTCCRC],
                             (uint16_t)dma_count);
        if (error != FSP_SUCCESS) {
            set_fault(K1_WS281X_FAULT_DMA);
            force_idle_low();
            return K1_WS281X_SUBMIT_UNAVAILABLE;
        }
        error = R_DMAC_Enable(&dmac_ctrl);
        if (error != FSP_SUCCESS) {
            set_fault(K1_WS281X_FAULT_DMA);
            force_idle_low();
            return K1_WS281X_SUBMIT_UNAVAILABLE;
        }
    }
    R_BSP_IrqClearPending(GPT0_COUNTER_OVERFLOW_IRQn);
    R_BSP_IrqClearPending(K1_WS281X_GPT_DMA_IRQ);
    owned = 1u;
    state = K1_WS281X_TX_PRELOAD;
    if (R_GPT_Enable(&gpt0_ctrl) != FSP_SUCCESS ||
        R_GPT_Enable(&gpt6_ctrl) != FSP_SUCCESS ||
        R_GPT_Start(&gpt0_ctrl) != FSP_SUCCESS) {
        set_fault(K1_WS281X_FAULT_RESOURCE); return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    snapshot(0, 1);
    last_prepare_cycles = (uint32_t)(DWT->CYCCNT - prepare_started);
    if (last_prepare_cycles > maximum_prepare_cycles)
        maximum_prepare_cycles = last_prepare_cycles;
    emit_started = DWT->CYCCNT;
    fault_witness.start_requested = 1u;
    state = K1_WS281X_TX_EMITTING;
    error = R_GPT_Start(&gpt6_ctrl);
    if (error != FSP_SUCCESS) {
        stop_hardware();
        set_fault(K1_WS281X_FAULT_RESOURCE);
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    snapshot(1, 2);
    return K1_WS281X_SUBMIT_ACCEPTED;
}

int k1_ws281x_gpt_dma_hw_poll(void)
{
    uint32_t now;
    if (!initialised) {
        return K1_WS281X_TX_FAULT;
    }
    now = DWT->CYCCNT;
    if (owned &&
        (state == K1_WS281X_TX_EMITTING || state == K1_WS281X_TX_DMA_COMPLETE ||
         state == K1_WS281X_TX_WAVEFORM_COMPLETE || state == K1_WS281X_TX_LATCH_PENDING) &&
        (uint32_t)(now - emit_started) > timeout_cycles) {
        set_fault(K1_WS281X_FAULT_TIMEOUT);
        return (int)state;
    }
    if (state == K1_WS281X_TX_WAVEFORM_COMPLETE ||
        state == K1_WS281X_TX_LATCH_PENDING) {
        state = K1_WS281X_TX_LATCH_PENDING;
        if (dma_complete && waveform_complete &&
            (uint32_t)(now - reset_started) >= reset_cycles) {
            reset_ready = 1u;
            owned = 0u;
            completions += 1u;
            state = K1_WS281X_TX_READY;
        }
    }
    if (state == K1_WS281X_TX_READY) {
        owned = 0u;
    }
    return (int)state;
}

int k1_ws281x_gpt_dma_hw_busy(void)
{
    (void)k1_ws281x_gpt_dma_hw_poll();
    return owned != 0u && state != K1_WS281X_TX_READY &&
           state != K1_WS281X_TX_IDLE;
}

int k1_ws281x_gpt_dma_hw_ready(void)
{
    const int current = k1_ws281x_gpt_dma_hw_poll();
    return current == K1_WS281X_TX_IDLE || current == K1_WS281X_TX_READY;
}

void k1_ws281x_gpt_dma_hw_abort_low(void)
{
    if (!initialised) {
        return;
    }
    stop_hardware();
    /* Aborted frames never earn a completion. Fault remains latched until reset. */
    if (state != K1_WS281X_TX_FAULT) set_fault(K1_WS281X_FAULT_EXTRA_EVENT);
}

void k1_ws281x_gpt_dma_hw_snapshot(k1_ws281x_hw_diag_t *out)
{
    if (!out) return;
    const uint32_t mask=__get_PRIMASK(); __disable_irq();
    out->version=K1_WS281X_GPT_DIAG_VERSION; out->bytes=sizeof(*out);
    out->attempts=attempts; out->dma_irqs=dma_irqs; out->stop_irqs=stop_irqs;
    out->frames=completions; out->errors=errors; out->first_fault=first_fault;
    out->state=state; out->owned=owned; out->clock_hz=clock_hz; out->bits=bits;
    memcpy(out->snapshots, snapshots, sizeof(snapshots));
    __set_PRIMASK(mask);
}
uint32_t k1_ws281x_gpt_dma_hw_dma_irqs(void) { return dma_irqs; }
void k1_ws281x_gpt_dma_hw_snapshot_v2(k1_ws281x_hw_diag_v2_t *out)
{
    if (!out) return;
    const uint32_t mask = __get_PRIMASK();
    __disable_irq();
    k1_ws281x_gpt_dma_hw_snapshot(&out->header);
    out->header.version = K1_WS281X_GPT_DIAG_V2_VERSION;
    out->header.bytes = sizeof(*out);
    out->dmctl = R_DMA->DMCTL;
    out->last_prepare_cycles = last_prepare_cycles;
    out->maximum_prepare_cycles = maximum_prepare_cycles;
    if (fault_witness.valid) memcpy(&out->first_fault, &fault_witness, sizeof(fault_witness));
    else memset(&out->first_fault, 0, sizeof(out->first_fault));
    __set_PRIMASK(mask);
}
uint32_t k1_ws281x_gpt_dma_hw_clock_hz(void) { return clock_hz; }
uint32_t k1_ws281x_gpt_dma_hw_completions(void) { return completions; }
uint32_t k1_ws281x_gpt_dma_hw_errors(void) { return errors; }
uint32_t k1_ws281x_gpt_dma_hw_ownership_violations(void)
{
    return ownership_violations;
}
uint32_t k1_ws281x_gpt_dma_hw_last_fault(void) { return fault; }
uint32_t k1_ws281x_gpt_dma_hw_last_bits(void) { return bits; }
uint32_t k1_ws281x_gpt_dma_hw_pfs(void) { return pfs_after; }
const char *k1_ws281x_gpt_dma_hw_backend(void)
{
    if (!initialised) {
        return "gpt6_dma_uninitialised";
    }
    if (fault == K1_WS281X_FAULT_NONE) {
        return "gpt6_dma_p601";
    }
    return "gpt6_dma_fault";
}

#endif /* K1_PALETTE_WS2816_GPT_PAIR */
