#include "ws281x_gpt_dma_hw_pair.h"

#if !defined(K1_PALETTE_WS2816_GPT_PAIR) && !defined(K1_WS281X_HW_TEST)

#else

#ifdef K1_WS281X_HW_TEST
#include "ws281x_hw_mock.h"
#else
#include "common_data.h"
#include "r_dmac.h"
#include "r_gpt.h"
#include "r_ioport.h"
#include "vector_data.h"
#endif
#include "titan_led_pins.h"
#include "ws281x_diag.h"
#include "ws281x_p601_loopback.h"

#include <string.h>

#ifndef K1_RA8P1_TARGET
#error "ws281x_gpt_dma_hw_pair.c is the RA8P1 pair transmitter"
#endif

#ifndef VECTOR_NUMBER_DMAC2_INT
#error "DMAC2 vector is required for LED lane A"
#endif
#ifndef VECTOR_NUMBER_DMAC3_INT
#error "DMAC3 vector is required; stage_led_pair_vectors must add it"
#endif
#include "k1_dmac_lane_guard.h"
#ifndef VECTOR_NUMBER_GPT1_COUNTER_OVERFLOW
#error "GPT1 overflow vector is required for the A1 stop counter"
#endif

#ifdef K1_PALETTE_GPT_DMA
#error "WS2816 pair backend cannot share GPT ownership with the WS2812 lane"
#endif
#ifdef K1_PALETTE_WS2816
#error "WS2816 pair backend cannot share emit with the GPIO WS2816 emitter"
#endif

#define GPT_PRV_GTCCRA 0u
#define GPT_PRV_GTCCRB 1u
#define GPT_PRV_GTCCRC 2u
#define GPT_PRV_GTCCRE 3u
#define GPT_PRV_GTCCRD 4u
#define GPT_PRV_GTCCRF 5u
/* FSP PeriodSet writes 0x550000 (CCRA/CCRB/PR single + CCRSWT). Double
   buffer is CCRA=10, CCRB=10, PR=01, CCRSWT=0: overflow does A←C←D / B←D←F.
   Mask covers CCRA, CCRB, PR and CCRSWT (CMSIS GTBER bits 16–22). */
#define GPT_GTBER_DOUBLE_BUFFER 0x1A0000u
#define GPT_GTBER_BUFFER_FIELD_MASK 0x7F0000u

static gpt_instance_ctrl_t gpt6_ctrl;
static gpt_instance_ctrl_t gpt7_ctrl;
static gpt_instance_ctrl_t gpt0_ctrl;
static gpt_instance_ctrl_t gpt1_ctrl;
static dmac_instance_ctrl_t dmac0_ctrl;
static dmac_instance_ctrl_t dmac3_ctrl;
static transfer_info_t dmac0_info;
static transfer_info_t dmac3_info;
static uint32_t duty_a0[K1_WS281X_GPT_DUTY_CAP] __attribute__((aligned(32)));
static uint32_t duty_a1[K1_WS281X_GPT_DUTY_CAP] __attribute__((aligned(32)));
static uint8_t owned_a0[K1_WS281X_PAIR_LANE_BYTES];
static uint8_t owned_a1[K1_WS281X_PAIR_LANE_BYTES];
static uint8_t pending_a0[K1_WS281X_PAIR_LANE_BYTES];
static uint8_t pending_a1[K1_WS281X_PAIR_LANE_BYTES];
static k1_ws281x_duty_meta_t meta_a0;
static k1_ws281x_duty_meta_t meta_a1;

static volatile uint32_t pair_state;
static volatile uint32_t owned;
static volatile uint32_t dma_complete[2];
static volatile uint32_t waveform_complete[2];
static volatile uint32_t hw_stopped[2];
static volatile uint32_t reset_ready[2];
static volatile uint32_t lane_fault[2];
static volatile uint32_t dma_irqs[2];
static volatile uint32_t stop_irqs[2];
static volatile uint32_t completions;
static volatile uint32_t errors;
static volatile uint32_t replacements;
static uint32_t clock_hz_a0;
static uint32_t clock_hz_a1;
static uint32_t cpu_hz;
static uint32_t bits_a0;
static uint32_t bits_a1;
static uint32_t reset_cycles;
static uint32_t timeout_cycles;
static uint32_t emit_started;
static volatile uint32_t reset_started[2];
static uint32_t last_gtstr;
static uint32_t pfs_a0;
static uint32_t pfs_a1;
static uint32_t initialised;
static uint32_t opened;
static uint32_t submitted_generation;
static uint32_t completed_generation;
static uint32_t pending_valid;
static uint32_t pending_generation;
static uint32_t pending_profile;
static uint32_t first_fault;
static k1_ws281x_pair_first_fault_t pair_first_fault;
static k1_ws281x_pair_last_stop_t pair_last_stop[2];
static k1_ws281x_pair_last_frame_t pair_last_frame;
static k1_ws281x_pair_reset_witness_t reset_witness;
static uint32_t previous_stop_valid, previous_stop[2], previous_generation;

static void gpt0_overflow(timer_callback_args_t *args);
static void gpt1_overflow(timer_callback_args_t *args);
static void dmac0_done(dmac_callback_args_t *args);
static void dmac3_done(dmac_callback_args_t *args);
static void stop_hardware(void);
static void set_fault(unsigned lane, uint32_t code);
static int start_owned_generation(uint32_t profile, uint32_t generation);

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

static gpt_extended_cfg_t gpt7_extend = {
    .gtioca = {.output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW},
    .gtiocb = {.output_enabled = true, .stop_level = GPT_PIN_LEVEL_LOW},
    .start_source = GPT_SOURCE_NONE,
    .stop_source = GPT_SOURCE_GPT_D,
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

static timer_cfg_t gpt7_cfg = {
    .mode = TIMER_MODE_PWM,
    .period_counts = 250u,
    .duty_cycle_counts = 50u,
    .source_div = TIMER_SOURCE_DIV_1,
    .channel = 8u, /* GPT8, GTIOC8B on P604 / U18 pin 12 */
    .p_callback = NULL,
    .p_context = NULL,
    .p_extend = &gpt7_extend,
    .cycle_end_ipl = BSP_IRQ_DISABLED,
    .cycle_end_irq = FSP_INVALID_VECTOR,
};

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

static gpt_extended_cfg_t gpt1_extend = {
    .gtioca = {.output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW},
    .gtiocb = {.output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW},
    .start_source = GPT_SOURCE_NONE,
    .stop_source = GPT_SOURCE_NONE,
    .clear_source = GPT_SOURCE_NONE,
    .capture_a_source = GPT_SOURCE_NONE,
    .capture_b_source = GPT_SOURCE_NONE,
    .count_up_source = GPT_SOURCE_GPT_C,
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

static timer_cfg_t gpt1_cfg = {
    .mode = TIMER_MODE_PERIODIC,
    .period_counts = 8u,
    .duty_cycle_counts = 0u,
    .source_div = TIMER_SOURCE_DIV_1,
    .channel = K1_WS281X_GPT_EVENT_TIMER_A1,
    .p_callback = gpt1_overflow,
    .p_context = NULL,
    .p_extend = &gpt1_extend,
    .cycle_end_ipl = 12u,
    .cycle_end_irq = GPT1_COUNTER_OVERFLOW_IRQn,
};

static const dmac_extended_cfg_t dmac0_extend = {
    .channel = K1_WS281X_GPT_DMA_CHANNEL,
    .irq = K1_WS281X_GPT_DMA_IRQ,
    .ipl = 12u,
    .offset = 0,
    .src_buffer_size = 0u,
    .activation_source = ELC_EVENT_GPT6_COUNTER_OVERFLOW,
    .p_callback = dmac0_done,
    .p_callback_memory = NULL,
    .p_context = NULL,
};

static const dmac_extended_cfg_t dmac3_extend = {
    .channel = K1_WS281X_GPT_DMA_CHANNEL_A1,
    .irq = K1_WS281X_GPT_DMA_IRQ_A1,
    .ipl = 12u,
    .offset = 0,
    .src_buffer_size = 0u,
    .activation_source = ELC_EVENT_GPT8_COUNTER_OVERFLOW,
    .p_callback = dmac3_done,
    .p_callback_memory = NULL,
    .p_context = NULL,
};

static const transfer_cfg_t dmac0_cfg = {
    .p_info = &dmac0_info,
    .p_extend = &dmac0_extend,
};

static const transfer_cfg_t dmac3_cfg = {
    .p_info = &dmac3_info,
    .p_extend = &dmac3_extend,
};

static uint32_t gpt_to_cpu(uint32_t gpt_counts, uint32_t clock_hz)
{
    if (clock_hz == 0u || cpu_hz == 0u) {
        return gpt_counts;
    }
    return (uint32_t)(((uint64_t)gpt_counts * (uint64_t)cpu_hz +
                       (uint64_t)clock_hz - 1ull) /
                      (uint64_t)clock_hz);
}

static void force_pin_low(bsp_io_port_pin_t pin, unsigned port_bit)
{
    const uint32_t gpio = IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                          IOPORT_CFG_PORT_OUTPUT_LOW | IOPORT_CFG_DRIVE_HIGH;
    R_BSP_PinAccessEnable();
    (void)R_IOPORT_PinCfg(&g_ioport_ctrl, pin, gpio);
    R_PORT6->PCNTR3 = (1u << port_bit) << 16;
    R_BSP_PinAccessDisable();
}

static int configure_pin(bsp_io_port_pin_t pin, unsigned port, unsigned pad,
                         uint32_t *pfs_out)
{
    volatile uint32_t *const pfs = &R_PFS->PORT[port].PIN[pad].PmnPFS;
    const uint32_t config = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN |
                            (uint32_t)IOPORT_PERIPHERAL_GPT1 |
                            (uint32_t)IOPORT_CFG_DRIVE_HIGH;
    fsp_err_t error;
    R_BSP_PinAccessEnable();
    error = R_IOPORT_PinCfg(&g_ioport_ctrl, pin, config);
    R_BSP_PinAccessDisable();
    *pfs_out = *pfs;
    if (error != FSP_SUCCESS) {
        return 0;
    }
    const uint32_t mask = R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (0x1fu << 24);
    if ((*pfs_out & mask) != (config & mask)) {
        return 0;
    }
    return 1;
}

static void force_lane_idle_low(unsigned lane)
{
    if (lane == 0u) {
        force_pin_low((bsp_io_port_pin_t)K1_TITAN_DIN_A, 1u);
        (void)configure_pin((bsp_io_port_pin_t)K1_TITAN_DIN_A, 6u, 1u, &pfs_a0);
        return;
    }
    force_pin_low((bsp_io_port_pin_t)K1_TITAN_DIN_B_GPT, 4u);
    (void)configure_pin((bsp_io_port_pin_t)K1_TITAN_DIN_B_GPT, 6u, 4u, &pfs_a1);
}

static void force_idle_low(void)
{
    force_lane_idle_low(0u);
    force_lane_idle_low(1u);
}

static void link_elc(void)
{
    R_BSP_MODULE_START(FSP_IP_ELC, 0);
    __DSB();
    /* Restored PIN12-P604 working transmitter: stop timers count compare
       matches, not PWM overflow. Extra overflow-count / bits+1 periods
       injected extra wire edges (operator: random white flashes). */
    R_ELC->ELSR[ELC_PERIPHERAL_GPT_A].HA_b.ELS =
        (uint16_t)ELC_EVENT_GPT6_CAPTURE_COMPARE_A;
    R_ELC->ELSR[ELC_PERIPHERAL_GPT_B].HA_b.ELS =
        (uint16_t)ELC_EVENT_GPT0_COUNTER_OVERFLOW;
    R_ELC->ELSR[ELC_PERIPHERAL_GPT_C].HA_b.ELS =
        (uint16_t)ELC_EVENT_GPT8_CAPTURE_COMPARE_B;
    R_ELC->ELSR[ELC_PERIPHERAL_GPT_D].HA_b.ELS =
        (uint16_t)ELC_EVENT_GPT1_COUNTER_OVERFLOW;
    R_ELC->ELCR_b.ELCON = 1u;
}

static void clean_duty(uint32_t *duty, uint32_t count)
{
    if ((SCB->CCR & SCB_CCR_DC_Msk) != 0u && count != 0u) {
        SCB_CleanDCache_by_Addr((void *)duty,
                                (int32_t)(count * sizeof(uint32_t)));
        __DSB();
        __ISB();
    }
}

static void unwind_opened(unsigned stage)
{
    if (stage > 5u) {
        (void)R_DMAC_Close(&dmac3_ctrl);
    }
    if (stage > 4u) {
        (void)R_DMAC_Close(&dmac0_ctrl);
    }
    if (stage > 3u) {
        (void)R_GPT_Close(&gpt1_ctrl);
    }
    if (stage > 2u) {
        (void)R_GPT_Close(&gpt0_ctrl);
    }
    if (stage > 1u) {
        (void)R_GPT_Close(&gpt7_ctrl);
    }
    if (stage > 0u) {
        (void)R_GPT_Close(&gpt6_ctrl);
    }
}

static void setup_transfer_info(transfer_info_t *info)
{
    memset(info, 0, sizeof(*info));
    info->transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED;
    info->transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
    info->transfer_settings_word_b.irq = TRANSFER_IRQ_END;
    info->transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED;
    info->transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
    info->transfer_settings_word_b.size = TRANSFER_SIZE_4_BYTE;
    info->transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;
}

static void capture_dma_count_fault(unsigned lane, uint32_t site)
{
    dmac_instance_ctrl_t *const dmac[2] = {&dmac0_ctrl, &dmac3_ctrl};
    unsigned i;
    if (pair_first_fault.valid) {
        return;
    }
    pair_first_fault.site = site;
    pair_first_fault.lane = lane;
    pair_first_fault.generation = submitted_generation;
    pair_first_fault.pair_state = pair_state;
    pair_first_fault.cyccnt = DWT->CYCCNT;
    pair_first_fault.dma_irqs[0] = dma_irqs[0];
    pair_first_fault.dma_irqs[1] = dma_irqs[1];
    pair_first_fault.stop_irqs[0] = stop_irqs[0];
    pair_first_fault.stop_irqs[1] = stop_irqs[1];
    for (i = 0u; i < 2u; i++) {
        if (dmac[i]->p_reg == NULL) {
            continue;
        }
        pair_first_fault.dmcra[i] = dmac[i]->p_reg->DMCRA;
        pair_first_fault.dmsar[i] = dmac[i]->p_reg->DMSAR;
        pair_first_fault.dmdar[i] = dmac[i]->p_reg->DMDAR;
        pair_first_fault.dmcnt[i] = dmac[i]->p_reg->DMCNT;
    }
    pair_first_fault.dmctl = R_DMA->DMCTL;
    if (gpt7_ctrl.p_reg != NULL) {
        pair_first_fault.gpt8_gtcnt = gpt7_ctrl.p_reg->GTCNT;
        pair_first_fault.gpt8_gtcr = gpt7_ctrl.p_reg->GTCR;
    }
    if (gpt1_ctrl.p_reg != NULL) {
        pair_first_fault.gpt1_gtcnt = gpt1_ctrl.p_reg->GTCNT;
        pair_first_fault.gpt1_gtpr = gpt1_ctrl.p_reg->GTPR;
    }
    pair_first_fault.valid = 1u;
}

static void dmac_done_lane(unsigned lane, dmac_instance_ctrl_t *ctrl)
{
    if (!owned || pair_state == K1_WS281X_PAIR_FAULT) {
        return;
    }
    if (ctrl->p_reg->DMCRA != 0u) {
        capture_dma_count_fault(lane, K1_WS281X_PAIR_FAULT_SITE_DMA_CALLBACK_COUNT_NONZERO);
        set_fault(lane, K1_WS281X_FAULT_DMA);
        return;
    }
    (void)R_DMAC_Disable(ctrl);
    if (dma_complete[lane]) {
        return;
    }
    dma_irqs[lane] += 1u;
    dma_complete[lane] = 1u;
    /* DMA completion does not prove this generation has stopped on the wire.
       stop_irqs is lifetime telemetry, not a per-generation stop receipt.
       Only stop_overflow may record hardware stop and start the reset wait. */
}

static void dmac0_done(dmac_callback_args_t *args)
{
    (void)args;
    dmac_done_lane(0u, &dmac0_ctrl);
}

static void dmac3_done(dmac_callback_args_t *args)
{
    (void)args;
    dmac_done_lane(1u, &dmac3_ctrl);
}

static void stop_overflow(unsigned lane, timer_callback_args_t *args,
                          gpt_instance_ctrl_t *pwm, gpt_instance_ctrl_t *stop,
                          dmac_instance_ctrl_t *dmac)
{
    if (!owned || !args || args->event != TIMER_EVENT_CYCLE_END) {
        return;
    }
    if (waveform_complete[lane]) {
        return;
    }
    stop_irqs[lane] += 1u;
    (void)R_GPT_Stop(stop);
    pair_last_stop[lane].generation = submitted_generation;
    pair_last_stop[lane].lane = lane;
    pair_last_stop[lane].cyccnt = DWT->CYCCNT;
    pair_last_stop[lane].pwm_gtcr = pwm->p_reg->GTCR;
    pair_last_stop[lane].pwm_gtcnt = pwm->p_reg->GTCNT;
    pair_last_stop[lane].pwm_gtccr = lane == 0u ? pwm->p_reg->GTCCR[GPT_PRV_GTCCRA]
                                                : pwm->p_reg->GTCCR[GPT_PRV_GTCCRB];
    pair_last_stop[lane].stop_gtcnt = stop->p_reg->GTCNT;
    pair_last_stop[lane].stop_gtpr = stop->p_reg->GTPR;
    pair_last_stop[lane].dmcra = dmac->p_reg->DMCRA;
    pair_last_stop[lane].pwm_running =
        (pwm->p_reg->GTCR & R_GPT0_GTCR_CST_Msk) ? 1u : 0u;
    pair_last_stop[lane].pwm_gtber = pwm->p_reg->GTBER;
    /* Legacy field name: the deep physical buffer is D for A, F for B. */
    pair_last_stop[lane].pwm_gtccr_e = pwm->p_reg->GTCCR[
        lane == 0u ? GPT_PRV_GTCCRD : GPT_PRV_GTCCRF];
    pair_last_stop[lane].elc_gpt_a = R_ELC->ELSR[ELC_PERIPHERAL_GPT_A].HA_b.ELS;
    pair_last_stop[lane].elc_gpt_b = R_ELC->ELSR[ELC_PERIPHERAL_GPT_B].HA_b.ELS;
    pair_last_stop[lane].valid = 1u;
    if (pwm->p_reg->GTCR & R_GPT0_GTCR_CST_Msk) {
        set_fault(lane, K1_WS281X_FAULT_MISSING_STOP);
        return;
    }
    if (dmac->p_reg->DMCRA != 0u) {
        capture_dma_count_fault(lane, K1_WS281X_PAIR_FAULT_SITE_STOP_CALLBACK_COUNT_NONZERO);
        set_fault(lane, K1_WS281X_FAULT_DMA);
        return;
    }
    /* Publish the timestamp before either completion flag. */
    reset_started[lane] = DWT->CYCCNT;
    hw_stopped[lane] = 1u;
    waveform_complete[lane] = 1u;
}

static void gpt0_overflow(timer_callback_args_t *args)
{
    stop_overflow(0u, args, &gpt6_ctrl, &gpt0_ctrl, &dmac0_ctrl);
}

static void gpt1_overflow(timer_callback_args_t *args)
{
    stop_overflow(1u, args, &gpt7_ctrl, &gpt1_ctrl, &dmac3_ctrl);
}

static void stop_hardware(void)
{
    (void)R_DMAC_Disable(&dmac0_ctrl);
    (void)R_DMAC_Disable(&dmac3_ctrl);
    (void)R_GPT_Stop(&gpt6_ctrl);
    (void)R_GPT_Stop(&gpt7_ctrl);
    (void)R_GPT_Stop(&gpt0_ctrl);
    (void)R_GPT_Stop(&gpt1_ctrl);
    k1_p601_loopback_stop_and_score();
    force_idle_low();
}

static void set_fault(unsigned lane, uint32_t code)
{
    lane_fault[lane] = code;
    if (!first_fault) {
        first_fault = code;
    }
    if (opened) {
        stop_hardware();
    }
    pair_state = K1_WS281X_PAIR_FAULT;
    owned = 0u;
    pending_valid = 0u;
    errors += 1u;
}

static int arm_lane(gpt_instance_ctrl_t *pwm, gpt_instance_ctrl_t *stop,
                    dmac_instance_ctrl_t *dmac, uint32_t *duty,
                    uint32_t bits, uint32_t period_counts,
                    uint32_t compare, uint32_t buffer, uint32_t deep)
{
    /* Bit pipeline (saw-wave PWM, DMA on overflow, stop on last compare):
       preload compare=duty[0], buffer=duty[1], deep=duty[2];
       DMA duty[3]..duty[bits-1] into deep (bits-3 words).
       Overflow k: compare←buffer←deep, then DMA writes duty[k+3] to deep.
       Last DMA is at overflow bits-4. Last payload reaches compare at
       overflow bits-2. Last compare (bit bits-1) stops PWM with stop_level
       LOW. remaining==1 at that stop is a missed deep-buffer write, not
       padding: the last payload bit would already have used stale data. */
    const uint32_t dma_count = bits > 3u ? bits - 3u : 0u;
    const uint32_t mid = bits > 1u ? duty[1] : duty[0];
    const uint32_t deep_duty = bits > 2u ? duty[2] : mid;
    if (R_GPT_PeriodSet(pwm, period_counts) != FSP_SUCCESS ||
        R_GPT_PeriodSet(stop, bits) != FSP_SUCCESS ||
        R_GPT_Reset(pwm) != FSP_SUCCESS || R_GPT_Reset(stop) != FSP_SUCCESS) {
        return 0;
    }
    pwm->p_reg->GTBER = GPT_GTBER_DOUBLE_BUFFER;
    pwm->p_reg->GTCCR[compare] = duty[0];
    pwm->p_reg->GTCCR[buffer] = mid;
    pwm->p_reg->GTCCR[deep] = deep_duty;
    if (pwm->p_reg->GTCCR[compare] != duty[0] ||
        pwm->p_reg->GTCCR[buffer] != mid ||
        pwm->p_reg->GTCCR[deep] != deep_duty ||
        (pwm->p_reg->GTBER & GPT_GTBER_BUFFER_FIELD_MASK) !=
            GPT_GTBER_DOUBLE_BUFFER) {
        return 0;
    }
    if (dma_count > 0u) {
        if (R_DMAC_Reset(dmac, &duty[3],
                         (void *)&pwm->p_reg->GTCCR[deep],
                         (uint16_t)dma_count) != FSP_SUCCESS ||
            R_DMAC_Enable(dmac) != FSP_SUCCESS) {
            return 0;
        }
    }
    return 1;
}

static int start_owned_generation(uint32_t profile, uint32_t generation)
{
    k1_ws281x_diag_timing_t timing;
    uint32_t dma_count;
    if (!k1_ws281x_diag_profile(profile, &timing) ||
        !k1_ws281x_build_gpt_duty(owned_a0, K1_WS281X_PAIR_LANE_BYTES, profile,
                                  clock_hz_a0, duty_a0, K1_WS281X_GPT_DUTY_CAP,
                                  &meta_a0) ||
        !k1_ws281x_build_gpt_duty(owned_a1, K1_WS281X_PAIR_LANE_BYTES, profile,
                                  clock_hz_a1, duty_a1, K1_WS281X_GPT_DUTY_CAP,
                                  &meta_a1)) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    if (meta_a0.period_counts != meta_a1.period_counts ||
        meta_a0.t0h_counts != meta_a1.t0h_counts ||
        meta_a0.t1h_counts != meta_a1.t1h_counts) {
        set_fault(0u, K1_WS281X_FAULT_CLOCK);
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    bits_a0 = meta_a0.bits;
    bits_a1 = meta_a1.bits;
    if (bits_a0 != 3840u || bits_a1 != 3840u) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    dma_count = bits_a0 - 3u;
    if (dma_count != 3837u) {
        return K1_WS281X_SUBMIT_INVALID;
    }
    reset_cycles = gpt_to_cpu((uint32_t)(((uint64_t)timing.reset_us * 1000ull *
                                          (uint64_t)clock_hz_a0 +
                                          999999999ull) /
                                         1000000000ull),
                              clock_hz_a0);
    timeout_cycles = gpt_to_cpu((bits_a0 + 4u) * meta_a0.period_counts,
                                clock_hz_a0) +
                     reset_cycles + cpu_hz / 100u;
    clean_duty(duty_a0, bits_a0);
    clean_duty(duty_a1, bits_a1);
    (void)R_GPT_Stop(&gpt6_ctrl);
    (void)R_GPT_Stop(&gpt7_ctrl);
    (void)R_GPT_Stop(&gpt0_ctrl);
    (void)R_GPT_Stop(&gpt1_ctrl);
    (void)R_DMAC_Disable(&dmac0_ctrl);
    (void)R_DMAC_Disable(&dmac3_ctrl);
    if (!arm_lane(&gpt6_ctrl, &gpt0_ctrl, &dmac0_ctrl, duty_a0, bits_a0,
                  meta_a0.period_counts, GPT_PRV_GTCCRA, GPT_PRV_GTCCRC,
                  GPT_PRV_GTCCRD) ||
        !arm_lane(&gpt7_ctrl, &gpt1_ctrl, &dmac3_ctrl, duty_a1, bits_a1,
                  meta_a1.period_counts, GPT_PRV_GTCCRB, GPT_PRV_GTCCRE,
                  GPT_PRV_GTCCRF)) {
        stop_hardware();
        set_fault(0u, K1_WS281X_FAULT_RESOURCE);
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    dma_complete[0] = dma_complete[1] = 0u;
    waveform_complete[0] = waveform_complete[1] = 0u;
    hw_stopped[0] = hw_stopped[1] = 0u;
    reset_ready[0] = reset_ready[1] = 0u;
    lane_fault[0] = lane_fault[1] = K1_WS281X_FAULT_NONE;
    R_BSP_IrqClearPending(GPT0_COUNTER_OVERFLOW_IRQn);
    R_BSP_IrqClearPending(GPT1_COUNTER_OVERFLOW_IRQn);
    R_BSP_IrqClearPending(K1_WS281X_GPT_DMA_IRQ);
    R_BSP_IrqClearPending(K1_WS281X_GPT_DMA_IRQ_A1);
    if (R_GPT_Enable(&gpt0_ctrl) != FSP_SUCCESS ||
        R_GPT_Enable(&gpt1_ctrl) != FSP_SUCCESS ||
        R_GPT_Enable(&gpt6_ctrl) != FSP_SUCCESS ||
        R_GPT_Enable(&gpt7_ctrl) != FSP_SUCCESS ||
        R_GPT_Start(&gpt0_ctrl) != FSP_SUCCESS ||
        R_GPT_Start(&gpt1_ctrl) != FSP_SUCCESS) {
        stop_hardware();
        set_fault(0u, K1_WS281X_FAULT_RESOURCE);
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    /* Bit 4 is GTIOA[4], the level at count start, not the output enable.
       FSP sets OAE (bit 8) because this output is enabled. Hold stays clear. */
    gpt6_ctrl.p_reg->GTIOR |= 1u << 4;
    /* Bit 20 is GTIOB[4], the same initial-level role for output B.
       OBE is bit 24. FSP sets it by shifting OAE when gtiocb is enabled. */
    gpt7_ctrl.p_reg->GTIOR |= 1u << 20;
    k1_p601_loopback_arm(bits_a0, meta_a0.period_counts, generation, owned_a0);
    owned = 1u;
    submitted_generation = generation;
    pair_state = K1_WS281X_PAIR_EMITTING;
    emit_started = DWT->CYCCNT;
    last_gtstr = (1u << K1_WS281X_GPT_TIMER) | (1u << 8u);
    const uint32_t start_before = DWT->CYCCNT;
    gpt6_ctrl.p_reg->GTSTR = last_gtstr;
    const uint32_t start_after = DWT->CYCCNT;
    if (previous_stop_valid) {
        const uint32_t a = start_before - previous_stop[0];
        const uint32_t b = start_before - previous_stop[1];
        const uint32_t elapsed = a < b ? a : b;
        /* Only continuous, short-gap samples count; poll expires idle history. */
        if (elapsed < cpu_hz / 10u) {
            if (!reset_witness.start_samples || elapsed < reset_witness.start_min_cycles)
                reset_witness.start_min_cycles = elapsed;
            reset_witness.start_samples += 1u;
            reset_witness.previous_generation = previous_generation;
            reset_witness.next_generation = generation;
            reset_witness.stop_a = previous_stop[0];
            reset_witness.stop_b = previous_stop[1];
            reset_witness.start_before = start_before;
            reset_witness.start_after = start_after;
        }
        previous_stop_valid = 0u;
    }
    return K1_WS281X_SUBMIT_ACCEPTED;
}

int k1_ws281x_gpt_dma_hw_pair_init(void)
{
    timer_info_t info6;
    timer_info_t info7;
    if (initialised) {
        return first_fault == K1_WS281X_FAULT_NONE ? 1 : 0;
    }
    if (R_FSP_IsrContextGet(GPT0_COUNTER_OVERFLOW_IRQn) != NULL ||
        R_FSP_IsrContextGet(GPT1_COUNTER_OVERFLOW_IRQn) != NULL ||
        R_FSP_IsrContextGet(K1_WS281X_GPT_DMA_IRQ) != NULL ||
        R_FSP_IsrContextGet(K1_WS281X_GPT_DMA_IRQ_A1) != NULL) {
        first_fault = K1_WS281X_FAULT_RESOURCE;
        return 0;
    }
    setup_transfer_info(&dmac0_info);
    setup_transfer_info(&dmac3_info);
    if (R_GPT_Open(&gpt6_ctrl, &gpt6_cfg) != FSP_SUCCESS) {
        first_fault = K1_WS281X_FAULT_RESOURCE;
        return 0;
    }
    if (R_GPT_Open(&gpt7_ctrl, &gpt7_cfg) != FSP_SUCCESS) {
        unwind_opened(1u);
        first_fault = K1_WS281X_FAULT_RESOURCE;
        return 0;
    }
    if (R_GPT_Open(&gpt0_ctrl, &gpt0_cfg) != FSP_SUCCESS) {
        unwind_opened(2u);
        first_fault = K1_WS281X_FAULT_RESOURCE;
        return 0;
    }
    if (R_GPT_Open(&gpt1_ctrl, &gpt1_cfg) != FSP_SUCCESS) {
        unwind_opened(3u);
        first_fault = K1_WS281X_FAULT_RESOURCE;
        return 0;
    }
    if (R_DMAC_Open(&dmac0_ctrl, &dmac0_cfg) != FSP_SUCCESS) {
        unwind_opened(4u);
        first_fault = K1_WS281X_FAULT_DMA;
        return 0;
    }
    if (R_DMAC_Open(&dmac3_ctrl, &dmac3_cfg) != FSP_SUCCESS) {
        unwind_opened(5u);
        first_fault = K1_WS281X_FAULT_DMA;
        return 0;
    }
    if (R_GPT_InfoGet(&gpt6_ctrl, &info6) != FSP_SUCCESS ||
        R_GPT_InfoGet(&gpt7_ctrl, &info7) != FSP_SUCCESS ||
        info6.clock_frequency == 0u || info7.clock_frequency == 0u ||
        info6.clock_frequency != info7.clock_frequency) {
        unwind_opened(6u);
        first_fault = K1_WS281X_FAULT_CLOCK;
        return 0;
    }
    clock_hz_a0 = info6.clock_frequency;
    clock_hz_a1 = info7.clock_frequency;
    cpu_hz = SystemCoreClock;
    if (cpu_hz == 0u) {
        unwind_opened(6u);
        first_fault = K1_WS281X_FAULT_CLOCK;
        return 0;
    }
    if (!configure_pin((bsp_io_port_pin_t)K1_TITAN_DIN_A, 6u, 1u, &pfs_a0) ||
        !configure_pin((bsp_io_port_pin_t)K1_TITAN_DIN_B_GPT, 6u, 4u, &pfs_a1)) {
        unwind_opened(6u);
        first_fault = K1_WS281X_FAULT_PIN;
        return 0;
    }
    link_elc();
    /* Bit 4 is GTIOA[4], the level at count start, not the output enable.
       FSP sets OAE (bit 8) because this output is enabled. Hold stays clear. */
    gpt6_ctrl.p_reg->GTIOR |= 1u << 4;
    /* Bit 20 is GTIOB[4], the same initial-level role for output B.
       OBE is bit 24. FSP sets it by shifting OAE when gtiocb is enabled. */
    gpt7_ctrl.p_reg->GTIOR |= 1u << 20;
    force_idle_low();
    opened = 1u;
    initialised = 1u;
    pair_state = K1_WS281X_PAIR_IDLE;
    k1_p601_loopback_init(clock_hz_a0);
    first_fault = K1_WS281X_FAULT_NONE;
    memset(&pair_first_fault, 0, sizeof(pair_first_fault));
    memset(pair_last_stop, 0, sizeof(pair_last_stop));
    memset(&pair_last_frame, 0, sizeof(pair_last_frame));
    memset(&reset_witness, 0, sizeof(reset_witness));
    previous_stop_valid = 0u;
    return 1;
}

int k1_ws281x_gpt_dma_hw_pair_submit(const uint8_t *lane_a0, size_t a0_bytes,
                                     const uint8_t *lane_a1, size_t a1_bytes,
                                     uint32_t profile, uint32_t generation)
{
    int status;
    if (!initialised || !opened || pair_state == K1_WS281X_PAIR_FAULT) {
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    if (pair_state == K1_WS281X_PAIR_FAULT) {
        return K1_WS281X_SUBMIT_UNAVAILABLE;
    }
    status = k1_ws281x_gpt_dma_pair_validate(lane_a0, a0_bytes, lane_a1,
                                             a1_bytes, profile, clock_hz_a0);
    if (status != K1_WS281X_SUBMIT_ACCEPTED) {
        return status;
    }
    if (owned) {
        memcpy(pending_a0, lane_a0, K1_WS281X_PAIR_LANE_BYTES);
        memcpy(pending_a1, lane_a1, K1_WS281X_PAIR_LANE_BYTES);
        pending_valid = 1u;
        pending_generation = generation;
        pending_profile = profile;
        replacements += 1u;
        return K1_WS281X_SUBMIT_BUSY;
    }
    memcpy(owned_a0, lane_a0, K1_WS281X_PAIR_LANE_BYTES);
    memcpy(owned_a1, lane_a1, K1_WS281X_PAIR_LANE_BYTES);
    pending_valid = 0u;
    return start_owned_generation(profile, generation);
}

int k1_ws281x_gpt_dma_hw_pair_poll(void)
{
    uint32_t now;
    uint32_t ready;
    if (!initialised) {
        return K1_WS281X_PAIR_FAULT;
    }
    now = DWT->CYCCNT;
    if (previous_stop_valid && (uint32_t)(now - previous_stop[0]) > cpu_hz / 10u)
        previous_stop_valid = 0u;
#ifdef K1_WS281X_HW_TEST
    k1_test_poll_after_clock();
#endif
    if (owned && pair_state == K1_WS281X_PAIR_EMITTING &&
        (uint32_t)(now - emit_started) > timeout_cycles) {
        set_fault(0u, K1_WS281X_FAULT_TIMEOUT);
        return (int)pair_state;
    }
    if (owned && waveform_complete[0] && waveform_complete[1]) {
        if (dma_complete[0] && dma_complete[1] && hw_stopped[0] &&
            hw_stopped[1]) {
            const uint32_t started0 = reset_started[0];
            const uint32_t started1 = reset_started[1];
            /* Stop ISRs can run after the initial timeout clock read. Read
               again AFTER their published timestamps; otherwise unsigned
               subtraction can turn a future timestamp into a huge wait. */
            const uint32_t reset_now = DWT->CYCCNT;
            const uint32_t age0 = reset_now - started0;
            const uint32_t age1 = reset_now - started1;
            const uint32_t elapsed = age0 < age1 ? age0 : age1;
            const uint32_t old0 = now - started0;
            const uint32_t old1 = now - started1;
            if ((old0 < old1 ? old0 : old1) >= reset_cycles &&
                elapsed < reset_cycles) {
                reset_witness.stale_clock_rejections += 1u;
            }
            if (elapsed >= reset_cycles) {
                if (!reset_witness.admissions || elapsed < reset_witness.min_cycles) {
                    reset_witness.min_cycles = elapsed;
                }
                reset_witness.admissions += 1u;
                reset_witness.last_cycles = elapsed;
                reset_witness.required_cycles = reset_cycles;
                reset_witness.generation = submitted_generation;
                reset_witness.cpu_hz = cpu_hz;
                reset_witness.admission = reset_now;
                previous_stop[0] = started0;
                previous_stop[1] = started1;
                previous_generation = submitted_generation;
                previous_stop_valid = 1u;
                reset_ready[0] = 1u;
                reset_ready[1] = 1u;
                pair_state = K1_WS281X_PAIR_READY;
                if (k1_ws281x_gpt_dma_pair_lanes_complete(
                        dma_complete[0], dma_complete[1], hw_stopped[0],
                        hw_stopped[1], waveform_complete[0],
                        waveform_complete[1], reset_ready[0], reset_ready[1],
                        lane_fault[0], lane_fault[1], pair_state)) {
                    k1_p601_loopback_stop_and_score();
                    {
                        uint32_t i;
                        uint32_t crc = 2166136261u;
                        uint32_t max_ch = 0u;
                        uint32_t white = 0u;
                        for (i = 0u; i < K1_WS281X_PAIR_LANE_BYTES; i++) {
                            crc ^= owned_a0[i];
                            crc *= 16777619u;
                        }
                        for (i = 0u; i < 80u; i++) {
                            const uint32_t g =
                                ((uint32_t)owned_a0[i * 6u] << 8) |
                                owned_a0[i * 6u + 1u];
                            const uint32_t r =
                                ((uint32_t)owned_a0[i * 6u + 2u] << 8) |
                                owned_a0[i * 6u + 3u];
                            const uint32_t b =
                                ((uint32_t)owned_a0[i * 6u + 4u] << 8) |
                                owned_a0[i * 6u + 5u];
                            if (g > max_ch) {
                                max_ch = g;
                            }
                            if (r > max_ch) {
                                max_ch = r;
                            }
                            if (b > max_ch) {
                                max_ch = b;
                            }
                            if (g >= 0xC000u && r >= 0xC000u && b >= 0xC000u) {
                                white += 1u;
                            }
                        }
                        pair_last_frame.generation = submitted_generation;
                        pair_last_frame.crc_a = crc;
                        pair_last_frame.max_ch = max_ch;
                        pair_last_frame.white_pixels = white;
                        pair_last_frame.valid = 1u;
                    }
                    owned = 0u;
                    completions += 1u;
                    completed_generation = submitted_generation;
                    if (pending_valid) {
                        memcpy(owned_a0, pending_a0, K1_WS281X_PAIR_LANE_BYTES);
                        memcpy(owned_a1, pending_a1, K1_WS281X_PAIR_LANE_BYTES);
                        pending_valid = 0u;
                        (void)start_owned_generation(pending_profile,
                                                     pending_generation);
                    }
                }
            }
        }
    }
    ready = (pair_state == K1_WS281X_PAIR_IDLE ||
             pair_state == K1_WS281X_PAIR_READY)
                ? 1u
                : 0u;
    if (ready) {
        owned = 0u;
    }
    return (int)pair_state;
}

int k1_ws281x_gpt_dma_hw_pair_transmitting(void)
{
    /* Observation only: busy()/ready() retire work and can start a queued
       generation. Main-loop USB admission must not start what it observes. */
    if (!initialised) return 0;
    return ((gpt6_ctrl.p_reg->GTCR | gpt7_ctrl.p_reg->GTCR) &
            R_GPT0_GTCR_CST_Msk) != 0u;
}

int k1_ws281x_gpt_dma_hw_pair_busy(void)
{
    (void)k1_ws281x_gpt_dma_hw_pair_poll();
    return owned != 0u && pair_state != K1_WS281X_PAIR_READY &&
           pair_state != K1_WS281X_PAIR_IDLE;
}

int k1_ws281x_gpt_dma_hw_pair_ready(void)
{
    const int current = k1_ws281x_gpt_dma_hw_pair_poll();
    return current == K1_WS281X_PAIR_IDLE || current == K1_WS281X_PAIR_READY;
}

void k1_ws281x_gpt_dma_hw_pair_abort_low(void)
{
    if (!initialised) {
        return;
    }
    stop_hardware();
    if (pair_state != K1_WS281X_PAIR_FAULT) {
        set_fault(0u, K1_WS281X_FAULT_EXTRA_EVENT);
    }
}

uint32_t k1_ws281x_gpt_dma_hw_pair_clock_hz(void) { return clock_hz_a0; }
uint32_t k1_ws281x_gpt_dma_hw_pair_clock_hz_a1(void) { return clock_hz_a1; }
uint32_t k1_ws281x_gpt_dma_hw_pair_completions(void) { return completions; }
uint32_t k1_ws281x_gpt_dma_hw_pair_errors(void) { return errors; }
uint32_t k1_ws281x_gpt_dma_hw_pair_replacements(void) { return replacements; }
uint32_t k1_ws281x_gpt_dma_hw_pair_submitted_generation(void)
{
    return submitted_generation;
}
uint32_t k1_ws281x_gpt_dma_hw_pair_completed_generation(void)
{
    return completed_generation;
}
uint32_t k1_ws281x_gpt_dma_hw_pair_last_fault(void) { return first_fault; }
uint32_t k1_ws281x_gpt_dma_hw_pair_lane_fault(unsigned lane)
{
    return lane < 2u ? lane_fault[lane] : K1_WS281X_FAULT_RESOURCE;
}
uint32_t k1_ws281x_gpt_dma_hw_pair_lane_dma_complete(unsigned lane)
{
    return lane < 2u ? dma_complete[lane] : 0u;
}
uint32_t k1_ws281x_gpt_dma_hw_pair_lane_hw_stopped(unsigned lane)
{
    return lane < 2u ? hw_stopped[lane] : 0u;
}
uint32_t k1_ws281x_gpt_dma_hw_pair_gtstr(void) { return last_gtstr; }
uint32_t k1_ws281x_gpt_dma_hw_pair_pfs_a0(void) { return pfs_a0; }
uint32_t k1_ws281x_gpt_dma_hw_pair_pfs_a1(void) { return pfs_a1; }
const uint32_t *k1_ws281x_gpt_dma_hw_pair_duty(unsigned lane)
{
    return lane == 1u ? duty_a1 : duty_a0;
}
const char *k1_ws281x_gpt_dma_hw_pair_backend(void)
{
    if (!initialised) {
        return "ws2816_gpt_pair_uninitialised";
    }
    if (first_fault == K1_WS281X_FAULT_NONE) {
        return "ws2816_gpt_pair_p601_p603";
    }
    return "ws2816_gpt_pair_fault";
}

const k1_ws281x_pair_first_fault_t *k1_ws281x_gpt_dma_hw_pair_first_fault_record(void)
{
    return &pair_first_fault;
}

const k1_ws281x_pair_last_stop_t *k1_ws281x_gpt_dma_hw_pair_last_stop(unsigned lane)
{
    static const k1_ws281x_pair_last_stop_t empty;
    return lane < 2u ? &pair_last_stop[lane] : &empty;
}

const k1_ws281x_pair_last_frame_t *k1_ws281x_gpt_dma_hw_pair_last_frame(void)
{
    return &pair_last_frame;
}

const k1_ws281x_pair_reset_witness_t *k1_ws281x_gpt_dma_hw_pair_reset_witness(void)
{
    return &reset_witness;
}

uint32_t k1_ws281x_gpt_dma_hw_pair_registers(uint32_t *out, uint32_t capacity)
{
    uint32_t i = 0u;
    if (!out || capacity < K1_WS281X_PAIR_REG_WORDS || !initialised || owned)
        return 0u;
#ifndef K1_WS281X_HW_TEST
#define WORD(value) out[i++] = (uint32_t)(value)
    WORD(2u); WORD(0u); WORD(DWT->CYCCNT);
    WORD(SystemCoreClock); WORD(clock_hz_a0);
    /* RA8P1 manual Rev1.30 sections60.5.43/7.2.11. MCUVER requires byte read. */
    WORD(*(volatile const uint8_t *)0x02C1EC48u);
    WORD(*(volatile const uint32_t *)0x02E07600u);
    WORD(R_SRAM->SRAMWTSC); WORD(R_SYSTEM->VSCR);
    WORD(R_SYSTEM->SCKDIVCR); WORD(R_SYSTEM->SCKDIVCR2);
    WORD(R_SYSTEM->SCKSCR); WORD(R_SYSTEM->PLLCCR); WORD(R_SYSTEM->PLL2CCR);
    WORD(R_SYSTEM->GPTCKCR); WORD(SCB->CCR); WORD(MPU->CTRL);
    WORD(R_DMA->DMCTL); WORD(R_BUS->BUSSABT0.S0BI);
    WORD(R_MSTP->MSTPCRC);
    WORD(R_PFS->PORT[6].PIN[1].PmnPFS); WORD(R_PFS->PORT[6].PIN[4].PmnPFS);
    for (uint32_t lane = 0u; lane < 2u; ++lane) {
        R_GPT0_Type *g = lane ? gpt7_ctrl.p_reg : gpt6_ctrl.p_reg;
        WORD(g->GTCR); WORD(g->GTCNT); WORD(g->GTPR);
        WORD(g->GTBER); WORD(g->GTIOR); WORD(g->GTPSR);
        for (uint32_t c = 0u; c < 6u; ++c) WORD(g->GTCCR[c]);
    }
    for (uint32_t channel = 0u; channel < 4u; ++channel) {
        R_DMAC0_Type *d = (R_DMAC0_Type *)(R_DMAC0_BASE + channel * 0x40u);
        WORD(d->DMSAR); WORD(d->DMDAR); WORD(d->DMCRA); WORD(d->DMTMD);
        WORD(d->DMCNT); WORD(d->DMBWR); WORD(R_ICU->DELSR[channel]);
    }
    for (uint32_t route = 0u; route < 8u; ++route) WORD(R_ELC->ELSR[route].HA_b.ELS);
    WORD(R_SYSTEM->PLLCCR2); WORD(R_SYSTEM->PLL2CCR2); WORD(R_SYSTEM->GPTCKDIVCR);
    WORD(DWT->CYCCNT);
    out[1] = i * sizeof(uint32_t);
#undef WORD
#else
    (void)i;
#endif
    return i;
}

#endif
