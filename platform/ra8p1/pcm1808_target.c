#include "pcm1808_target.h"

#error "PCM1808 target blocked: no practical framed input route exists on U18; use a proper U11 mating breakout or bridge board"

#include <board.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hal_data.h"
#include "pcm1808_core.h"

#ifndef PCM1808_SSI1_RXI_IRQn
#error "PCM1808_SSI1_RXI_IRQn must be allocated in the disposable build stage"
#endif
#ifndef PCM1808_SSI1_INT_IRQn
#error "PCM1808_SSI1_INT_IRQn must be allocated in the disposable build stage"
#endif

#define K1_PCM1808_CORE_STORAGE_BYTES 2048u

static unsigned char core_storage[K1_PCM1808_CORE_STORAGE_BYTES] __attribute__((aligned(8)));
static uint32_t receive[2][K1_PCM1808_WORDS_PER_HOP] __attribute__((aligned(32)));
static ssi_instance_ctrl_t ssi1_ctrl;
static dtc_instance_ctrl_t ssi1_rx_dtc_ctrl;
static transfer_info_t ssi1_rx_dtc_info DTC_TRANSFER_INFO_ALIGNMENT = {0};
static const dtc_extended_cfg_t ssi1_rx_dtc_extend = {
    .activation_source = PCM1808_SSI1_RXI_IRQn,
};
static const transfer_cfg_t ssi1_rx_dtc_cfg = {
    .p_info = &ssi1_rx_dtc_info,
    .p_extend = &ssi1_rx_dtc_extend,
};
static const transfer_instance_t ssi1_rx_dtc = {
    .p_ctrl = &ssi1_rx_dtc_ctrl,
    .p_cfg = &ssi1_rx_dtc_cfg,
    .p_api = &g_transfer_on_dtc,
};
static volatile uint32_t filling;
static volatile uint32_t ready_mask;
static volatile uint32_t callbacks;
static volatile uint32_t overflow_events;
static volatile uint32_t idle_events;
static volatile int32_t last_fsp_error;
static bool initialised;
static bool running;
static uint32_t processed_hops;
static uint32_t processed_samples;
static uint32_t sample_hash = 2166136261u;
static int32_t sample_min = INT32_MAX;
static int32_t sample_max = INT32_MIN;
static uint32_t sample_peak;
static uint64_t sample_square_sum;
static uint32_t first_callback_cycle;
static uint32_t last_callback_cycle;

static k1_pcm1808_core_t *core(void) {
    return (k1_pcm1808_core_t *)core_storage;
}

static void note_sample(int16_t sample) {
    const int32_t wide = sample;
    const uint32_t magnitude = wide < 0 ? (uint32_t)(-wide) : (uint32_t)wide;
    if (wide < sample_min) sample_min = wide;
    if (wide > sample_max) sample_max = wide;
    if (magnitude > sample_peak) sample_peak = magnitude;
    sample_square_sum += (uint64_t)((int64_t)wide * wide);
    sample_hash = (sample_hash ^ (uint16_t)sample) * 16777619u;
}

static void pcm1808_callback(i2s_callback_args_t *args) {
    uint32_t cycle;
    uint32_t completed;
    uint32_t expected;
    fsp_err_t error;
    if (!args) return;
    if (args->event != I2S_EVENT_RX_FULL) {
        ++idle_events;
        running = false;
        return;
    }
    cycle = DWT->CYCCNT;
    if (!callbacks) first_callback_cycle = cycle;
    last_callback_cycle = cycle;
    ++callbacks;
    completed = filling;
    expected = 0;
    if (!__atomic_compare_exchange_n(&ready_mask, &expected, 1u << completed,
                                     false, __ATOMIC_RELEASE, __ATOMIC_RELAXED)) {
        ++overflow_events;
        running = false;
        (void)R_SSI_Stop(&ssi1_ctrl);
        return;
    }
    filling ^= 1u;
    error = R_SSI_Read(&ssi1_ctrl, receive[filling], sizeof(receive[filling]));
    if (FSP_SUCCESS != error) {
        last_fsp_error = error;
        running = false;
        (void)R_SSI_Stop(&ssi1_ctrl);
    }
}

static const i2s_cfg_t ssi1_cfg = {
    .channel = 1,
    .pcm_width = I2S_PCM_WIDTH_24_BITS,
    .word_length = I2S_WORD_LENGTH_32_BITS,
    .ws_continue = I2S_WS_CONTINUE_OFF,
    .operating_mode = I2S_MODE_SLAVE,
    .p_transfer_tx = NULL,
    .p_transfer_rx = &ssi1_rx_dtc,
    .p_callback = pcm1808_callback,
    .p_context = NULL,
    .p_extend = NULL,
    .rxi_ipl = 6,
    .txi_ipl = BSP_IRQ_DISABLED,
    .idle_err_ipl = 6,
    .txi_irq = FSP_INVALID_VECTOR,
    .rxi_irq = PCM1808_SSI1_RXI_IRQn,
    .int_irq = PCM1808_SSI1_INT_IRQn,
};

int k1_pcm1808_target_initialise(void) {
    uint32_t pin_cfg;
    fsp_err_t error;
    if (initialised) return running ? 0 : -1;
    if (k1_pcm1808_core_state_bytes() > K1_PCM1808_CORE_STORAGE_BYTES) {
        last_fsp_error = -1001;
        return -1;
    }
    k1_pcm1808_core_reset(core());
    pin_cfg = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SSI;
    error = R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_07_PIN_00, pin_cfg);
    if (FSP_SUCCESS == error) error = R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_07_PIN_01, pin_cfg);
    if (FSP_SUCCESS == error) error = R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_07_PIN_02, pin_cfg);
    if (FSP_SUCCESS == error) error = R_SSI_Open(&ssi1_ctrl, &ssi1_cfg);
    if (FSP_SUCCESS == error) error = R_SSI_Read(&ssi1_ctrl, receive[0], sizeof(receive[0]));
    if (FSP_SUCCESS != error) {
        last_fsp_error = error;
        return -1;
    }
    initialised = true;
    running = true;
    return 0;
}

void k1_pcm1808_target_poll(void) {
    const uint32_t mask = __atomic_load_n(&ready_mask, __ATOMIC_ACQUIRE);
    uint32_t slot;
    int16_t canonical[K1_PCM1808_OUTPUT_SAMPLES_PER_HOP];
    int made;
    size_t i;
    if (!mask) return;
    slot = (mask & 1u) ? 0u : 1u;
    made = k1_pcm1808_make_canonical_hop(core(), receive[slot],
                                         K1_PCM1808_WORDS_PER_HOP,
                                         canonical,
                                         K1_PCM1808_OUTPUT_SAMPLES_PER_HOP);
    if (made != (int)K1_PCM1808_OUTPUT_SAMPLES_PER_HOP) {
        last_fsp_error = -1002;
        running = false;
        (void)R_SSI_Stop(&ssi1_ctrl);
        return;
    }
    for (i = 0; i < K1_PCM1808_OUTPUT_SAMPLES_PER_HOP; ++i) note_sample(canonical[i]);
    ++processed_hops;
    processed_samples += K1_PCM1808_OUTPUT_SAMPLES_PER_HOP;
    __atomic_fetch_and(&ready_mask, ~(1u << slot), __ATOMIC_RELEASE);
}

int k1_pcm1808_target_initialised(void) { return initialised; }
int k1_pcm1808_target_running(void) { return running; }

size_t k1_pcm1808_target_metrics(char *output, size_t capacity) {
    const uint32_t intervals = callbacks > 1u ? callbacks - 1u : 0u;
    const uint32_t cycles = last_callback_cycle - first_callback_cycle;
    const uint32_t measured_hz = intervals && cycles && SystemCoreClock
        ? (uint32_t)(((uint64_t)intervals * K1_PCM1808_INPUT_FRAMES_PER_HOP * SystemCoreClock) / cycles)
        : 0u;
    int count;
    if (!output || !capacity) return 0;
    count = snprintf(output, capacity,
        "{\"pcm1808_target\":{\"contract\":\"k1-ra8p1-pcm1808-aux-v1\","
        "\"peripheral\":\"SSIE1\",\"role\":\"slave_receiver\",\"transfer\":\"DTC\","
        "\"bclk\":\"P702/U11-24\",\"lrck\":\"P701/U11-33\","
        "\"data\":\"P700/U11-26\",\"nominal_input_hz\":48000,"
        "\"canonical_hz\":12800,\"hop_in\":360,\"hop_out\":96,"
        "\"mono\":\"MID\",\"trim_q15\":2048,\"initialised\":%s,"
        "\"running\":%s,\"last_fsp_error\":%ld,\"callbacks\":%lu,"
        "\"overflow_events\":%lu,\"idle_events\":%lu,\"measured_input_hz\":%lu,"
        "\"processed_hops\":%lu,\"processed_samples\":%lu,\"sample_hash\":%lu,"
        "\"sample_min\":%ld,\"sample_max\":%ld,\"sample_peak\":%lu,"
        "\"sample_square_sum\":%llu,\"channel_map\":\"UNVALIDATED_ON_TITAN\","
        "\"clock_gate\":\"DMA_RATE_ONLY\",\"physical_capture\":\"NOT_CLAIMED_BY_BUILD\"}}",
        initialised ? "true" : "false", running ? "true" : "false",
        (long)last_fsp_error, (unsigned long)callbacks,
        (unsigned long)overflow_events, (unsigned long)idle_events,
        (unsigned long)measured_hz, (unsigned long)processed_hops,
        (unsigned long)processed_samples, (unsigned long)sample_hash,
        (long)(processed_samples ? sample_min : 0), (long)(processed_samples ? sample_max : 0),
        (unsigned long)sample_peak, (unsigned long long)sample_square_sum);
    return count > 0 && (size_t)count < capacity ? (size_t)count : 0;
}
