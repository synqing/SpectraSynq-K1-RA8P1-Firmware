#include "ws281x_p601_loopback.h"

#if defined(K1_P601_LOOPBACK) && defined(K1_RA8P1_TARGET) && !defined(K1_WS281X_HW_TEST)

#include <string.h>

static k1_p601_loopback_result_t live;
static k1_p601_loopback_result_t latched;

#include "common_data.h"
#include "r_dmac.h"
#include "r_gpt.h"
#include "vector_data.h"

#ifndef ELC_EVENT_GPT2_COUNTER_OVERFLOW
#define ELC_EVENT_GPT2_COUNTER_OVERFLOW ((elc_event_t)0x199)
#endif

#define GPT_PRV_UNUSED 0u

static gpt_instance_ctrl_t gpt2_ctrl;
static dmac_instance_ctrl_t dmac4_ctrl;
static transfer_info_t dmac4_info;
static uint16_t samples[K1_P601_LOOPBACK_SAMPLES] __attribute__((aligned(32)));
static uint32_t gpt_clock_hz;
static uint32_t opened;
static uint32_t requested;
static uint8_t captured_payload[480];

int k1_p601_loopback_request(void)
{
    /* One immutable capture per boot. A subsequent arm or shutdown cannot erase it. */
    if (!opened || requested || live.armed || latched.valid) return 0;
    requested = 1u;
    return 1;
}

uint32_t k1_p601_loopback_read(uint32_t offset, uint8_t *out, uint32_t capacity)
{
    uint32_t i;
    uint32_t total;
    if (!latched.valid || live.armed || out == NULL) return 0u;
    total = sizeof(latched) + sizeof(captured_payload) + latched.samples * 2u;
    if (offset >= total) return 0u;
    if (capacity > total - offset) capacity = total - offset;
    for (i = 0u; i < capacity; ++i) {
        uint32_t pos = offset + i;
        if (pos < sizeof(latched)) out[i] = ((const uint8_t *)&latched)[pos];
        else if ((pos -= sizeof(latched)) < sizeof(captured_payload)) out[i] = captured_payload[pos];
        else out[i] = ((const uint8_t *)samples)[pos - sizeof(captured_payload)];
    }
    return capacity;
}

static gpt_extended_cfg_t gpt2_extend = {
    .gtioca = {.output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW},
    .gtiocb = {.output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW},
    .start_source = GPT_SOURCE_NONE,
    .stop_source = GPT_SOURCE_NONE,
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

static timer_cfg_t gpt2_cfg = {
    .mode = TIMER_MODE_PERIODIC,
    .period_counts = 31u,
    .duty_cycle_counts = 0u,
    .source_div = TIMER_SOURCE_DIV_1,
    .channel = 2u,
    .p_callback = NULL,
    .p_context = NULL,
    .p_extend = &gpt2_extend,
    .cycle_end_ipl = BSP_IRQ_DISABLED,
    .cycle_end_irq = FSP_INVALID_VECTOR,
};

static const dmac_extended_cfg_t dmac4_extend = {
    .channel = 4u,
    .irq = FSP_INVALID_VECTOR,
    .ipl = BSP_IRQ_DISABLED,
    .offset = 0,
    .src_buffer_size = 0u,
    .activation_source = ELC_EVENT_GPT2_COUNTER_OVERFLOW,
    .p_callback = NULL,
    .p_callback_memory = NULL,
    .p_context = NULL,
};

static const transfer_cfg_t dmac4_cfg = {
    .p_info = &dmac4_info,
    .p_extend = &dmac4_extend,
};

static void setup_info(void)
{
    memset(&dmac4_info, 0, sizeof(dmac4_info));
    dmac4_info.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
    dmac4_info.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
    dmac4_info.transfer_settings_word_b.irq = TRANSFER_IRQ_END;
    dmac4_info.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED;
    dmac4_info.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED;
    dmac4_info.transfer_settings_word_b.size = TRANSFER_SIZE_2_BYTE;
    dmac4_info.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;
    dmac4_info.p_src = (void *)&R_PORT6->PCNTR2;
    dmac4_info.p_dest = samples;
    dmac4_info.length = (uint16_t)K1_P601_LOOPBACK_SAMPLES;
    dmac4_info.num_blocks = 0;
}

void k1_p601_loopback_init(uint32_t clock_hz)
{
    gpt_clock_hz = clock_hz;
    memset(&live, 0, sizeof(live));
    memset(&latched, 0, sizeof(latched));
    if (opened) {
        live.open_ok = 1u;
        latched.open_ok = 1u;
        return;
    }
    setup_info();
    if (R_GPT_Open(&gpt2_ctrl, &gpt2_cfg) != FSP_SUCCESS) {
        return;
    }
    if (R_DMAC_Open(&dmac4_ctrl, &dmac4_cfg) != FSP_SUCCESS) {
        (void)R_GPT_Close(&gpt2_ctrl);
        return;
    }
    opened = 1u;
    live.open_ok = 1u;
    latched.open_ok = 1u;
}

void k1_p601_loopback_arm(uint32_t expected_cells, uint32_t pwm_period_counts,
                          uint32_t generation, const uint8_t *payload)
{
    uint32_t period = pwm_period_counts / 8u;
    if (!opened || !requested || latched.valid || live.armed || expected_cells != 3840u) {
        return;
    }
    requested = 0u;
    memcpy(captured_payload, payload, sizeof(captured_payload));
    live.payload_fnv1a = 2166136261u;
    for (uint32_t j = 0u; j < sizeof(captured_payload); ++j) {
        live.payload_fnv1a ^= captured_payload[j];
        live.payload_fnv1a *= 16777619u;
    }
    live.generation = generation;
    live.period_counts = pwm_period_counts;
    live.clock_hz = gpt_clock_hz;
    live.truncated = 0u;
    if (period < 2u) {
        period = 2u;
    }
    memset(samples, 0, sizeof(samples));
    live.armed = 1u;
    live.valid = 0u;
    live.samples = 0u;
    live.expected_cells = expected_cells;
    live.rising = 0u;
    live.falling = 0u;
    live.first_rise = 0u;
    live.last_fall = 0u;
    live.extra_rising = 0u;
    live.high_after_last = 0u;
    live.sample_period_counts = period;
    live.open_ok = 1u;
    live.start_level = 0u;
    live.min_high = 0xffffffffu;
    live.max_high = 0u;
    live.peek_head = 0u;
    live.peek_tail = 0u;
    setup_info();
    (void)R_GPT_Stop(&gpt2_ctrl);
    (void)R_DMAC_Disable(&dmac4_ctrl);
    (void)R_GPT_PeriodSet(&gpt2_ctrl, period);
    if (R_DMAC_Reset(&dmac4_ctrl, (void *)&R_PORT6->PCNTR2, samples,
                     (uint16_t)K1_P601_LOOPBACK_SAMPLES) != FSP_SUCCESS ||
        R_DMAC_Enable(&dmac4_ctrl) != FSP_SUCCESS) {
        live.armed = 0u;
        return;
    }
    if (R_GPT_Start(&gpt2_ctrl) != FSP_SUCCESS) {
        (void)R_DMAC_Disable(&dmac4_ctrl);
        live.armed = 0u;
        return;
    }
    /* Bounded pretrigger: retain low samples before the shared PWM start. */
    const uint32_t start = DWT->CYCCNT;
    while (dmac4_ctrl.p_reg->DMCRA > K1_P601_LOOPBACK_SAMPLES - 8u &&
           (uint32_t)(DWT->CYCCNT - start) < 10000u) {}

}

void k1_p601_loopback_stop_and_score(void)
{
    uint32_t i;
    uint32_t prev = 0u;
    uint32_t remaining;
    uint32_t high_run = 0u;
    uint32_t n;
    if (!opened || !live.armed) {
        return;
    }
    (void)R_GPT_Stop(&gpt2_ctrl);
    remaining = dmac4_ctrl.p_reg != NULL ? dmac4_ctrl.p_reg->DMCRA : 0u;
    (void)R_DMAC_Disable(&dmac4_ctrl);
    if ((SCB->CCR & SCB_CCR_DC_Msk) != 0u) {
        SCB_InvalidateDCache_by_Addr((void *)samples, (int32_t)sizeof(samples));
        __DSB();
        __ISB();
    }
    n = K1_P601_LOOPBACK_SAMPLES - remaining;
    if (n > K1_P601_LOOPBACK_SAMPLES) {
        n = K1_P601_LOOPBACK_SAMPLES;
    }
    live.samples = n;
    live.truncated = remaining == 0u ? 1u : 0u;
    if (n != 0u) {
        live.start_level = ((uint32_t)samples[0] >> K1_P601_LOOPBACK_PIN_BIT) & 1u;
    }
    for (i = 0u; i < n; i++) {
        const uint32_t bit = ((uint32_t)samples[i] >> K1_P601_LOOPBACK_PIN_BIT) & 1u;
        if (i < 32u) {
            live.peek_head |= bit << i;
        }
        if (n > 32u && i >= n - 32u) {
            live.peek_tail |= bit << (i - (n - 32u));
        }
        if (bit) {
            high_run += 1u;
        } else if (high_run != 0u) {
            if (high_run < live.min_high) {
                live.min_high = high_run;
            }
            if (high_run > live.max_high) {
                live.max_high = high_run;
            }
            high_run = 0u;
        }
        if (bit && prev == 0u) {
            live.rising += 1u;
            if (live.rising == 1u) {
                live.first_rise = i;
            }
            if (live.rising > live.expected_cells) {
                live.extra_rising += 1u;
            }
        }
        if (bit == 0u && prev != 0u) {
            live.falling += 1u;
            live.last_fall = i;
        }
        prev = bit;
    }
    if (high_run != 0u) {
        if (high_run < live.min_high) {
            live.min_high = high_run;
        }
        if (high_run > live.max_high) {
            live.max_high = high_run;
        }
    }
    if (live.min_high == 0xffffffffu) {
        live.min_high = 0u;
    }
    if (live.falling != 0u) {
        for (i = live.last_fall + 1u; i < n; i++) {
            if (((uint32_t)samples[i] >> K1_P601_LOOPBACK_PIN_BIT) & 1u) {
                live.high_after_last += 1u;
            }
        }
    }
    live.armed = 0u;
    live.valid = 1u;
    latched = live;
}

const k1_p601_loopback_result_t *k1_p601_loopback_result(void)
{
    return &latched;
}

const uint16_t *k1_p601_loopback_samples(void)
{
    return samples;
}

#endif
