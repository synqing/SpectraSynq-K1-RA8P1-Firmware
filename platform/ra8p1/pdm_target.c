/* Dual onboard LMD2718T261-OA1 capture for Titan Mini (U13/U14).
   Shared P502/P812 (PDMDAT2/PDMCLK2). Expected MCU sampling:
   U14 SELECT-low → RISE/ch2 programme; U13 SELECT-high → FALL/ch0 measurement.
   40 kHz capture admits into 24 kHz/180 via measured-rate ASRC. */
#include "pdm_target.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "hal_data.h"
#include "k1_asrc_24k.h"
#include "k1_cycle_clock.h"
#include "k1/core/audio/k1_audio_hop.h"
#include "k1_pdm_sensitivity.h"
#include "k1_pdm_fifo16.h"
#include "pdm_capture.h"
#include "r_dmac.h"
#include "r_pdm.h"
#include "titan_onboard_mic.h"

static void k1_pdm_target_dmac_isr(dmac_callback_args_t *args);

typedef struct {
    uint32_t lane;
} k1_pdm_lane_context_t;

typedef struct {
    uint32_t slot;
    uint32_t epoch;
    uint32_t sequence;
    uint64_t capture_start_us;
    uint64_t capture_end_us;
    int valid;
} k1_pdm_owner_t;

typedef struct {
    volatile uint32_t data_callbacks;
    volatile uint32_t error_callbacks;
    volatile uint32_t error_flags;
    uint32_t processed_slots;
    uint32_t processed_samples;
    uint32_t sample_hash;
    int32_t sample_min;
    int32_t sample_max;
    uint32_t sample_peak;
    uint64_t sample_square_sum;
    uint32_t sat_neg;
    uint32_t sat_pos;
    uint32_t packing_mismatch;
    uint32_t first_sat_raw;
} k1_pdm_lane_metrics_t;

static int32_t capture_buffer[K1_PDM_TARGET_LANE_COUNT]
                             [K1_PDM_STREAM_SLOT_COUNT]
                             [K1_PDM_TARGET_SLOT_ELEMENTS]
    __attribute__((aligned(32)));

static k1_pdm_stream_t capture_stream[K1_PDM_TARGET_LANE_COUNT];
static k1_pdm_owner_t capture_owner[K1_PDM_TARGET_LANE_COUNT];
static k1_pdm_lane_context_t capture_context[K1_PDM_TARGET_LANE_COUNT] = {
    {K1_PDM_TARGET_PROGRAMME_LANE},
    {K1_PDM_TARGET_MEASUREMENT_LANE},
};
static k1_pdm_lane_metrics_t capture_metrics[K1_PDM_TARGET_LANE_COUNT] = {
    {0u, 0u, 0u, 0u, 0u, 2166136261u, INT32_MAX, INT32_MIN, 0u, 0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u, 2166136261u, INT32_MAX, INT32_MIN, 0u, 0u, 0u, 0u, 0u, 0u},
};
#define K1_CAPTURE_META_CAP 5u
typedef struct {
    uint64_t source_begin;
    uint64_t source_end_exclusive;
    uint64_t sequence;
    uint32_t raw_epoch;
    uint64_t receipt_us;
    uint64_t receipt_cycles;
} k1_capture_block_meta_t;

static k1_asrc24_t capture_asrc;
static int16_t capture_pcm[K1_PDM_TARGET_SLOT_ELEMENTS];
static k1_capture_block_meta_t capture_meta[K1_CAPTURE_META_CAP];
static uint32_t capture_meta_head;
static uint32_t capture_meta_count;
static uint64_t capture_source_write_index;
static uint64_t capture_source_base;
static uint64_t capture_stream_epoch;
static uint8_t capture_stale_active;
static uint64_t capture_stale_until_us;
static uint32_t capture_stale_discards;
static uint64_t capture_injection_start_us;
static uint64_t capture_injection_end_us;
static uint64_t capture_hop_sequence;
static uint32_t capture_rate_segment;
static uint32_t capture_discontinuity_pending;
static uint32_t capture_discontinuity_reason;
static uint32_t capture_pair_epoch_drops;
static uint32_t capture_rate_samples;
static uint64_t capture_rate_t0_us;
static uint32_t capture_asrc_starved;
static uint32_t capture_ap_hops;
static uint32_t capture_asrc_push_drop;
static uint64_t capture_last_hop_us;
static uint32_t capture_last_hop_dt_us;
static uint32_t capture_last_hop_peak;
static uint32_t capture_last_hop_gain_q8;
static uint32_t capture_gain_clip_pos;
static uint32_t capture_gain_clip_neg;
static uint32_t capture_rate_locked;
static uint32_t capture_measured_hz;

static pdm_instance_ctrl_t capture_fall_pdm_ctrl;
static dmac_instance_ctrl_t capture_rise_dmac_ctrl;
static dmac_instance_ctrl_t capture_fall_dmac_ctrl;

static transfer_info_t capture_rise_transfer_info = {
    .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
    .transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
    .transfer_settings_word_b.irq = TRANSFER_IRQ_END,
    .transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
    .transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED,
    .transfer_settings_word_b.size = TRANSFER_SIZE_4_BYTE,
    .transfer_settings_word_b.mode = TRANSFER_MODE_BLOCK,
    .p_dest = NULL,
    .p_src = NULL,
    .num_blocks = 0u,
    .length = 0u,
};

static transfer_info_t capture_fall_transfer_info = {
    .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
    .transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
    .transfer_settings_word_b.irq = TRANSFER_IRQ_END,
    .transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
    .transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED,
    .transfer_settings_word_b.size = TRANSFER_SIZE_4_BYTE,
    .transfer_settings_word_b.mode = TRANSFER_MODE_BLOCK,
    .p_dest = NULL,
    .p_src = NULL,
    .num_blocks = 0u,
    .length = 0u,
};

static const dmac_extended_cfg_t capture_rise_dmac_extend = {
    .channel = K1_PDM_TARGET_RISE_DMA_CHANNEL,
    .irq = K1_PDM_TARGET_RISE_DMA_IRQ,
    .ipl = 12u,
    .offset = 0,
    .src_buffer_size = 0u,
    .activation_source = ELC_EVENT_PDM_DAT2,
    .p_callback = k1_pdm_target_dmac_isr,
    .p_callback_memory = NULL,
    .p_context = &g_pdm0_ctrl,
};

static const dmac_extended_cfg_t capture_fall_dmac_extend = {
    .channel = K1_PDM_TARGET_FALL_DMA_CHANNEL,
    .irq = K1_PDM_TARGET_FALL_DMA_IRQ,
    .ipl = 12u,
    .offset = 0,
    .src_buffer_size = 0u,
    .activation_source = ELC_EVENT_PDM_DAT0,
    .p_callback = k1_pdm_target_dmac_isr,
    .p_callback_memory = NULL,
    .p_context = &capture_fall_pdm_ctrl,
};

static const transfer_cfg_t capture_rise_transfer_cfg = {
    .p_info = &capture_rise_transfer_info,
    .p_extend = &capture_rise_dmac_extend,
};
static const transfer_cfg_t capture_fall_transfer_cfg = {
    .p_info = &capture_fall_transfer_info,
    .p_extend = &capture_fall_dmac_extend,
};
static const transfer_instance_t capture_rise_transfer = {
    .p_ctrl = &capture_rise_dmac_ctrl,
    .p_cfg = &capture_rise_transfer_cfg,
    .p_api = &g_transfer_on_dmac,
};
static const transfer_instance_t capture_fall_transfer = {
    .p_ctrl = &capture_fall_dmac_ctrl,
    .p_cfg = &capture_fall_transfer_cfg,
    .p_api = &g_transfer_on_dmac,
};

static pdm_cfg_t capture_rise_pdm_cfg;
static pdm_cfg_t capture_fall_pdm_cfg;
static pdm_extended_cfg_t capture_rise_pdm_extend;
static pdm_extended_cfg_t capture_fall_pdm_extend;
static volatile int capture_initialised;
static volatile int capture_running;
static volatile int capture_restart_requested;
static volatile int32_t capture_last_fsp_error;
static uint32_t capture_paired_slots;
static uint32_t capture_pair_skew_drops;
static uint32_t capture_startup_discard_pairs;
static uint32_t capture_discard_next_pair;
static uint64_t capture_max_pair_skew_us;
static uint64_t capture_first_start_us;
static uint64_t capture_last_end_us;
static uint32_t capture_last_cycle;
static uint64_t capture_cycle_high;
static uint32_t capture_rearm_denied;

static uint64_t k1_pdm_target_now_cycles(void) {
    const uint32_t primask = __get_PRIMASK();
    uint64_t cycles;
    __disable_irq();
    cycles = k1_cycle_extend(DWT->CYCCNT, &capture_last_cycle, &capture_cycle_high);
    if (primask == 0u) {
        __enable_irq();
    }
    return cycles;
}

static uint64_t k1_pdm_target_now_us(void) {
    return (k1_pdm_target_now_cycles() * 1000000ull) /
           (uint64_t) SystemCoreClock;
}

static void k1_pdm_target_reset_logical_stream(uint32_t reason) {
    const uint32_t discarded = capture_asrc.filled;
    const uint32_t prior_discarded = capture_asrc.discarded_samples;
    const uint32_t prior_rejected = capture_asrc.push_rejected;
    const uint32_t prior_consumed = capture_asrc.consumed_samples;
    const uint32_t hz = (capture_asrc.source_hz < K1_ASRC24_MIN_IN_HZ ||
                         capture_asrc.source_hz > K1_ASRC24_MAX_IN_HZ)
                            ? K1_PDM_TARGET_SAMPLE_RATE_HZ
                            : capture_asrc.source_hz;
    capture_stream_epoch += 1u;
    capture_hop_sequence = 0u;
    capture_source_write_index = 0u;
    capture_source_base = 0u;
    capture_meta_head = 0u;
    capture_meta_count = 0u;
    capture_rate_samples = 0u;
    capture_rate_t0_us = 0u;
    capture_rate_locked = 0u;
    capture_measured_hz = 0u;
    capture_discontinuity_pending = reason != 0u ? 1u : 0u;
    capture_discontinuity_reason = reason;
    k1_asrc24_reset(&capture_asrc, (uint32_t)capture_stream_epoch, hz);
    capture_asrc.discarded_samples = prior_discarded + discarded;
    capture_asrc.push_rejected = prior_rejected;
    capture_asrc.consumed_samples = prior_consumed;
}

static const k1_capture_block_meta_t *k1_pdm_target_meta_for(uint64_t abs_index) {
    uint32_t i;
    if (capture_meta_count == 0u) return 0;
    for (i = 0; i < capture_meta_count; ++i) {
        const uint32_t idx =
            (capture_meta_head + K1_CAPTURE_META_CAP - capture_meta_count + i) %
            K1_CAPTURE_META_CAP;
        const k1_capture_block_meta_t *meta = &capture_meta[idx];
        if (abs_index >= meta->source_begin &&
            abs_index < meta->source_end_exclusive) {
            return meta;
        }
    }
    return 0;
}

static void k1_pdm_target_retire_meta(void) {
    while (capture_meta_count > 0u) {
        const uint32_t idx =
            (capture_meta_head + K1_CAPTURE_META_CAP - capture_meta_count) %
            K1_CAPTURE_META_CAP;
        if (capture_meta[idx].source_end_exclusive > capture_source_base) {
            break;
        }
        capture_meta_count -= 1u;
    }
}

static void k1_pdm_target_invalidate_slot(uint32_t lane, uint32_t slot) {
    if ((SCB->CCR & SCB_CCR_DC_Msk) != 0u &&
        lane < K1_PDM_TARGET_LANE_COUNT &&
        slot < K1_PDM_STREAM_SLOT_COUNT) {
        SCB_InvalidateDCache_by_Addr((void *) &capture_buffer[lane][slot][0],
                                    (int32_t) sizeof(capture_buffer[lane][slot]));
        __DSB();
        __ISB();
    }
}

static void k1_pdm_target_request_restart(int32_t error) {
    if (error != (int32_t) FSP_SUCCESS) capture_last_fsp_error = error;
    capture_restart_requested = 1;
}

void pdm_callback(pdm_callback_args_t *args) {
    k1_pdm_lane_context_t *context;
    uint32_t lane;
    if (args == NULL || args->p_context == NULL) return;
    context = (k1_pdm_lane_context_t *) args->p_context;
    lane = context->lane;
    if (lane >= K1_PDM_TARGET_LANE_COUNT) return;

    if (args->event == PDM_EVENT_DATA) {
        /* Completions are owned by k1_pdm_target_dmac_isr so DMA is not
           rearmed into a consumer-held slot. */
        return;
    } else if (args->event == PDM_EVENT_ERROR) {
        capture_metrics[lane].error_callbacks++;
        capture_metrics[lane].error_flags |= (uint32_t) args->error;
        k1_pdm_target_request_restart((int32_t) FSP_ERR_OVERFLOW);
    }
}

static void k1_pdm_target_dmac_isr(dmac_callback_args_t *args) {
    pdm_instance_ctrl_t *pdm;
    uint32_t lane;
    uint32_t filling = 0u;
    int result;
    fsp_err_t error;
    transfer_instance_t *xfer;
    pdm_extended_cfg_t *extend;
    uint16_t blocks;
    void *dest;
    if (args == NULL || args->p_context == NULL) return;
    pdm = (pdm_instance_ctrl_t *) args->p_context;
    if (pdm == &g_pdm0_ctrl) lane = K1_PDM_TARGET_PROGRAMME_LANE;
    else if (pdm == &capture_fall_pdm_ctrl) lane = K1_PDM_TARGET_MEASUREMENT_LANE;
    else return;
    k1_pdm_target_invalidate_slot(lane, capture_stream[lane].active_slot);
    capture_metrics[lane].data_callbacks++;
    result = k1_pdm_stream_context_on_data(&capture_stream[lane],
                                           K1_PDM_TARGET_SLOT_ELEMENTS,
                                           k1_pdm_target_now_us());
    if (result == K1_PDM_STREAM_OVERFLOW || result == K1_PDM_STREAM_INVALID) {
        capture_rearm_denied++;
        k1_pdm_target_request_restart((int32_t) FSP_ERR_OVERFLOW);
        return;
    }
    if (k1_pdm_stream_context_filling_slot(&capture_stream[lane], &filling) !=
        K1_PDM_STREAM_OK) {
        capture_rearm_denied++;
        k1_pdm_target_request_restart((int32_t) FSP_ERR_OVERFLOW);
        return;
    }
    xfer = (transfer_instance_t *) pdm->p_cfg->p_transfer_rx;
    extend = (pdm_extended_cfg_t *) pdm->p_cfg->p_extend;
    if (xfer == NULL || extend == NULL || xfer->p_api == NULL) {
        k1_pdm_target_request_restart((int32_t) FSP_ERR_INVALID_ARGUMENT);
        return;
    }
    blocks = (uint16_t) (pdm->rx_int_count_max / (1u << extend->interrupt_threshold));
    dest = &capture_buffer[lane][filling][0];
    error = xfer->p_api->reset(xfer->p_ctrl,
                               (void *) &pdm->p_reg->PDDRR,
                               dest,
                               blocks);
    if (error != FSP_SUCCESS) {
        capture_last_fsp_error = (int32_t) error;
        k1_pdm_target_request_restart((int32_t) error);
        return;
    }
    pdm->p_read = dest;
}

static fsp_err_t k1_pdm_target_start_drivers(void) {
    fsp_err_t error;
    error = R_PDM_Start(&capture_fall_pdm_ctrl,
                        &capture_buffer[K1_PDM_TARGET_MEASUREMENT_LANE][0][0],
                        sizeof(capture_buffer[K1_PDM_TARGET_MEASUREMENT_LANE]),
                        K1_PDM_TARGET_SLOT_ELEMENTS);
    if (error != FSP_SUCCESS) return error;
    error = R_PDM_Start(&g_pdm0_ctrl,
                        &capture_buffer[K1_PDM_TARGET_PROGRAMME_LANE][0][0],
                        sizeof(capture_buffer[K1_PDM_TARGET_PROGRAMME_LANE]),
                        K1_PDM_TARGET_SLOT_ELEMENTS);
    if (error != FSP_SUCCESS) {
        (void) R_PDM_Stop(&capture_fall_pdm_ctrl);
        return error;
    }
    /* Retrigger both channel filters together after sequential FSP Open. */
    R_PDM->CH[K1_PDM_TARGET_FALL_CHANNEL].PDSTRTR = R_PDM_CH_PDSTRTR_STRTRG_Msk;
    R_PDM->CH[K1_PDM_TARGET_RISE_CHANNEL].PDSTRTR = R_PDM_CH_PDSTRTR_STRTRG_Msk;
    return FSP_SUCCESS;
}

static void k1_pdm_target_stop_drivers(void) {
    fsp_err_t error = R_PDM_Stop(&g_pdm0_ctrl);
    if (error != FSP_SUCCESS) capture_last_fsp_error = (int32_t) error;
    error = R_PDM_Stop(&capture_fall_pdm_ctrl);
    if (error != FSP_SUCCESS) capture_last_fsp_error = (int32_t) error;
    capture_running = 0;
    k1_pdm_stream_context_stop(&capture_stream[K1_PDM_TARGET_PROGRAMME_LANE]);
    k1_pdm_stream_context_stop(&capture_stream[K1_PDM_TARGET_MEASUREMENT_LANE]);
}

static int k1_pdm_target_acquire(uint32_t lane) {
    k1_pdm_owner_t *owner = &capture_owner[lane];
    if (owner->valid) return K1_PDM_STREAM_OK;
    if (k1_pdm_stream_context_acquire(&capture_stream[lane],
                                      &owner->slot,
                                      &owner->epoch,
                                      &owner->sequence,
                                      &owner->capture_start_us,
                                      &owner->capture_end_us) != K1_PDM_STREAM_OK) {
        return K1_PDM_STREAM_INVALID;
    }
    owner->valid = 1;
    return K1_PDM_STREAM_OK;
}

static void k1_pdm_target_release(uint32_t lane) {
    k1_pdm_owner_t *owner = &capture_owner[lane];
    if (!owner->valid) return;
    (void) k1_pdm_stream_context_release(&capture_stream[lane],
                                         owner->slot,
                                         owner->epoch,
                                         owner->sequence);
    owner->valid = 0;
}

static void k1_pdm_target_process_lane(uint32_t lane) {
    k1_pdm_lane_metrics_t *metrics = &capture_metrics[lane];
    k1_pdm_owner_t const *owner = &capture_owner[lane];
    uint32_t index;
    k1_pdm_target_invalidate_slot(lane, owner->slot);
    for (index = 0; index < K1_PDM_TARGET_SLOT_ELEMENTS; ++index) {
        const int32_t raw = capture_buffer[lane][owner->slot][index];
        k1_pdm_fifo16_view_t view;
        const uint8_t *bytes;
        int32_t signed_sample;
        uint32_t magnitude;
        uint64_t square;
        uint32_t byte;
        k1_pdm_fifo16_inspect((uint32_t) raw, &view);
        bytes = (const uint8_t *) &view.sample;
        signed_sample = (int32_t) view.sample;
        magnitude = signed_sample < 0
                  ? (uint32_t) (-signed_sample)
                  : (uint32_t) signed_sample;
        square = (uint64_t) magnitude * (uint64_t) magnitude;
        if (signed_sample < metrics->sample_min) metrics->sample_min = signed_sample;
        if (signed_sample > metrics->sample_max) metrics->sample_max = signed_sample;
        if (magnitude > metrics->sample_peak) metrics->sample_peak = magnitude;
        if (view.sat_neg) {
            if (metrics->sat_neg == 0u) metrics->first_sat_raw = view.raw;
            metrics->sat_neg++;
        }
        if (view.sat_pos) metrics->sat_pos++;
        if (view.packing_mismatch) metrics->packing_mismatch++;
        if (UINT64_MAX - metrics->sample_square_sum < square) {
            metrics->sample_square_sum = UINT64_MAX;
        } else {
            metrics->sample_square_sum += square;
        }
        for (byte = 0; byte < sizeof(view.sample); ++byte) {
            metrics->sample_hash ^= bytes[byte];
            metrics->sample_hash *= 16777619u;
        }
        if (lane == K1_PDM_TARGET_PROGRAMME_LANE) {
            capture_pcm[index] = view.sample;
        }
    }
    metrics->processed_slots++;
    metrics->processed_samples += K1_PDM_TARGET_SLOT_ELEMENTS;
        if (lane == K1_PDM_TARGET_PROGRAMME_LANE) {
        uint32_t peak = 1u;
        uint32_t gain_q8;
        uint32_t i;
        for (i = 0; i < K1_PDM_TARGET_SLOT_ELEMENTS; ++i) {
            const int32_t s = capture_pcm[i];
            const uint32_t mag = s < 0 ? (uint32_t)(-s) : (uint32_t)s;
            if (mag > peak) {
                peak = mag;
            }
        }
        gain_q8 = k1_pdm_loud_guard_q8(peak);
        capture_last_hop_gain_q8 = gain_q8;
        for (i = 0; i < K1_PDM_TARGET_SLOT_ELEMENTS; ++i) {
            const int32_t scaled =
                ((int32_t)capture_pcm[i] * (int32_t)gain_q8) >> 8;
            if (scaled > 32767) {
                capture_gain_clip_pos++;
            } else if (scaled < -32768) {
                capture_gain_clip_neg++;
            }
            capture_pcm[i] = k1_pdm_scale_sample(capture_pcm[i], gain_q8);
        }
        if (capture_stale_active) {
            capture_stale_discards += K1_PDM_TARGET_SLOT_ELEMENTS;
            capture_asrc.discarded_samples += K1_PDM_TARGET_SLOT_ELEMENTS;
        } else if (capture_meta_count >= K1_CAPTURE_META_CAP) {
            capture_asrc_push_drop++;
        } else if (k1_asrc24_push(&capture_asrc, capture_pcm,
                                  K1_PDM_TARGET_SLOT_ELEMENTS) != k1_asrc24_ok) {
            capture_asrc_push_drop++;
        } else {
            k1_capture_block_meta_t *meta = &capture_meta[capture_meta_head];
            meta->source_begin = capture_source_write_index;
            meta->source_end_exclusive =
                capture_source_write_index + K1_PDM_TARGET_SLOT_ELEMENTS;
            meta->sequence = owner->sequence;
            meta->raw_epoch = owner->epoch;
            meta->receipt_us = owner->capture_end_us;
            meta->receipt_cycles = k1_pdm_target_now_cycles();
            capture_source_write_index += K1_PDM_TARGET_SLOT_ELEMENTS;
            capture_meta_head = (capture_meta_head + 1u) % K1_CAPTURE_META_CAP;
            capture_meta_count += 1u;
            capture_rate_samples += K1_PDM_TARGET_SLOT_ELEMENTS;
            if (capture_rate_t0_us == 0u) {
                capture_rate_t0_us = owner->capture_end_us;
            }
        }
    }
}

static void k1_pdm_target_process_pair(void) {
    const k1_pdm_owner_t *programme = &capture_owner[K1_PDM_TARGET_PROGRAMME_LANE];
    const k1_pdm_owner_t *measurement = &capture_owner[K1_PDM_TARGET_MEASUREMENT_LANE];
    uint64_t skew;
    if (!programme->valid || !measurement->valid) return;

    if (programme->epoch != measurement->epoch) {
        k1_pdm_target_release(K1_PDM_TARGET_PROGRAMME_LANE);
        k1_pdm_target_release(K1_PDM_TARGET_MEASUREMENT_LANE);
        capture_pair_epoch_drops++;
        return;
    }

    if (programme->sequence != measurement->sequence) {
        if (programme->sequence < measurement->sequence) {
            k1_pdm_target_release(K1_PDM_TARGET_PROGRAMME_LANE);
        } else {
            k1_pdm_target_release(K1_PDM_TARGET_MEASUREMENT_LANE);
        }
        capture_pair_skew_drops++;
        return;
    }

    skew = programme->capture_end_us >= measurement->capture_end_us
         ? programme->capture_end_us - measurement->capture_end_us
         : measurement->capture_end_us - programme->capture_end_us;
    if (skew > capture_max_pair_skew_us) capture_max_pair_skew_us = skew;

    if (capture_discard_next_pair) {
        capture_discard_next_pair = 0u;
        capture_startup_discard_pairs++;
    } else {
        k1_pdm_target_process_lane(K1_PDM_TARGET_PROGRAMME_LANE);
        k1_pdm_target_process_lane(K1_PDM_TARGET_MEASUREMENT_LANE);
        if (capture_metrics[K1_PDM_TARGET_PROGRAMME_LANE].processed_slots == 1u) {
            capture_first_start_us = programme->capture_start_us;
        }
        capture_last_end_us = programme->capture_end_us >= measurement->capture_end_us
                            ? programme->capture_end_us
                            : measurement->capture_end_us;
        if (capture_rate_locked == 0u && capture_rate_t0_us != 0u) {
            const uint64_t dt_us = capture_last_end_us - capture_rate_t0_us;
            if (dt_us >= 2000000ull && capture_rate_samples > 0u) {
                const uint32_t hz =
                    (uint32_t)((capture_rate_samples * 1000000ull) / dt_us);
                if (k1_asrc24_set_rate(&capture_asrc, hz) == k1_asrc24_ok) {
                    capture_measured_hz = hz;
                    capture_rate_locked = 1u;
                    capture_rate_segment += 1u;
                }
            }
        }
    }
    capture_paired_slots++;
    k1_pdm_target_release(K1_PDM_TARGET_PROGRAMME_LANE);
    k1_pdm_target_release(K1_PDM_TARGET_MEASUREMENT_LANE);
}

int k1_pdm_target_initialise(void) {
    fsp_err_t error;
    uint64_t capture_start_us;
    uint32_t lane;
    if (capture_initialised) return 0;
    capture_last_cycle = DWT->CYCCNT;
    capture_cycle_high = 0u;
    for (lane = 0; lane < K1_PDM_TARGET_LANE_COUNT; ++lane) {
        if (k1_pdm_stream_context_configure(&capture_stream[lane],
                                            K1_PDM_TARGET_SLOT_ELEMENTS,
                                            K1_PDM_TARGET_SLOT_ELEMENTS) != K1_PDM_STREAM_OK) {
            return -1;
        }
    }

    capture_rise_pdm_extend = *(pdm_extended_cfg_t const *) g_pdm0_cfg.p_extend;
    capture_rise_pdm_extend.interrupt_threshold = PDM_INTERRUPT_THRESHOLD_8;
    capture_rise_pdm_extend.sincdec = K1_PDM_TARGET_SINCDEC_40K;
    capture_rise_pdm_extend.sincrng = K1_PDM_TARGET_SINCRNG_40K;
    capture_rise_pdm_cfg = g_pdm0_cfg;
    capture_rise_pdm_cfg.channel = K1_PDM_TARGET_RISE_CHANNEL;
    capture_rise_pdm_cfg.pcm_edge = PDM_INPUT_DATA_EDGE_RISE;
    capture_rise_pdm_cfg.p_extend = &capture_rise_pdm_extend;
    capture_rise_pdm_cfg.p_transfer_rx = &capture_rise_transfer;
    capture_rise_pdm_cfg.p_callback = pdm_callback;
    capture_rise_pdm_cfg.p_context = &capture_context[K1_PDM_TARGET_PROGRAMME_LANE];
    capture_rise_pdm_cfg.dat_irq = FSP_INVALID_VECTOR;
    capture_rise_pdm_cfg.dat_ipl = BSP_IRQ_DISABLED;

    capture_fall_pdm_extend = *(pdm_extended_cfg_t const *) g_pdm0_cfg.p_extend;
    capture_fall_pdm_extend.interrupt_threshold = PDM_INTERRUPT_THRESHOLD_8;
    capture_fall_pdm_extend.sincdec = K1_PDM_TARGET_SINCDEC_40K;
    capture_fall_pdm_extend.sincrng = K1_PDM_TARGET_SINCRNG_40K;
    capture_fall_pdm_cfg = g_pdm0_cfg;
    capture_fall_pdm_cfg.channel = K1_PDM_TARGET_FALL_CHANNEL;
    capture_fall_pdm_cfg.pcm_edge = PDM_INPUT_DATA_EDGE_FALL;
    capture_fall_pdm_cfg.p_extend = &capture_fall_pdm_extend;
    capture_fall_pdm_cfg.p_transfer_rx = &capture_fall_transfer;
    capture_fall_pdm_cfg.p_callback = pdm_callback;
    capture_fall_pdm_cfg.p_context = &capture_context[K1_PDM_TARGET_MEASUREMENT_LANE];
    capture_fall_pdm_cfg.dat_irq = FSP_INVALID_VECTOR;
    capture_fall_pdm_cfg.dat_ipl = BSP_IRQ_DISABLED;
    capture_fall_pdm_cfg.err_irq = PDM_ERR0_IRQn;

    error = R_PDM_Open(&capture_fall_pdm_ctrl, &capture_fall_pdm_cfg);
    if (error != FSP_SUCCESS) {
        capture_last_fsp_error = (int32_t) error;
        return -2;
    }
    error = R_PDM_Open(&g_pdm0_ctrl, &capture_rise_pdm_cfg);
    if (error != FSP_SUCCESS) {
        capture_last_fsp_error = (int32_t) error;
        (void) R_PDM_Close(&capture_fall_pdm_ctrl);
        return -3;
    }

    capture_start_us = k1_pdm_target_now_us();
    for (lane = 0; lane < K1_PDM_TARGET_LANE_COUNT; ++lane) {
        if (k1_pdm_stream_context_start(&capture_stream[lane],
                                        capture_start_us) != K1_PDM_STREAM_OK) {
            capture_last_fsp_error = (int32_t) FSP_ERR_INVALID_ARGUMENT;
            return -4;
        }
    }
    error = k1_pdm_target_start_drivers();
    if (error != FSP_SUCCESS) {
        capture_last_fsp_error = (int32_t) error;
        k1_pdm_stream_context_stop(&capture_stream[K1_PDM_TARGET_PROGRAMME_LANE]);
        k1_pdm_stream_context_stop(&capture_stream[K1_PDM_TARGET_MEASUREMENT_LANE]);
        return -5;
    }
    capture_discard_next_pair = 1u;
    capture_restart_requested = 0;
    capture_asrc_starved = 0u;
    capture_ap_hops = 0u;
    capture_asrc_push_drop = 0u;
    capture_last_hop_peak = 0u;
    capture_last_hop_gain_q8 = 0u;
    capture_gain_clip_pos = 0u;
    capture_gain_clip_neg = 0u;
    capture_stream_epoch = 0u;
    capture_rate_segment = 0u;
    k1_pdm_target_reset_logical_stream(0u);
    capture_running = 1;
    capture_initialised = 1;
    return 0;
}

void k1_pdm_target_poll_stale(uint64_t now_us) {
    if (!capture_stale_active) {
        return;
    }
    if (now_us < capture_stale_until_us) {
        return;
    }
    capture_stale_active = 0;
    capture_injection_end_us = now_us;
    k1_pdm_target_reset_logical_stream(120u);
}

int k1_pdm_target_begin_stale_test(uint64_t now_us) {
    if (capture_stale_active) {
        return -1;
    }
    capture_stale_active = 1;
    capture_stale_until_us = now_us + 120000ull;
    capture_injection_start_us = now_us;
    capture_injection_end_us = 0u;
    capture_stale_discards = 0u;
    return 0;
}

int k1_pdm_target_stale_active(void) { return capture_stale_active ? 1 : 0; }
uint32_t k1_pdm_target_stale_discards(void) { return capture_stale_discards; }
uint64_t k1_pdm_target_injection_start_us(void) { return capture_injection_start_us; }
uint64_t k1_pdm_target_injection_end_us(void) { return capture_injection_end_us; }

void k1_pdm_target_poll(void) {
    uint32_t lane;
    if (!capture_initialised) return;
    k1_pdm_target_poll_stale(k1_pdm_target_now_us());

    if (capture_restart_requested) {
        fsp_err_t error;
        uint64_t capture_start_us;
        k1_pdm_target_release(K1_PDM_TARGET_PROGRAMME_LANE);
        k1_pdm_target_release(K1_PDM_TARGET_MEASUREMENT_LANE);
        k1_pdm_target_stop_drivers();
        capture_start_us = k1_pdm_target_now_us();
        for (lane = 0; lane < K1_PDM_TARGET_LANE_COUNT; ++lane) {
            if (k1_pdm_stream_context_recover(&capture_stream[lane],
                                              capture_start_us) != K1_PDM_STREAM_OK) {
                return;
            }
        }
        error = k1_pdm_target_start_drivers();
        if (error != FSP_SUCCESS) {
            capture_last_fsp_error = (int32_t) error;
            return;
        }
        capture_discard_next_pair = 1u;
        capture_restart_requested = 0;
        k1_pdm_target_reset_logical_stream(1u);
        capture_running = 1;
        return;
    }

    for (;;) {
        const uint32_t before = capture_paired_slots;
        (void) k1_pdm_target_acquire(K1_PDM_TARGET_PROGRAMME_LANE);
        (void) k1_pdm_target_acquire(K1_PDM_TARGET_MEASUREMENT_LANE);
        k1_pdm_target_process_pair();
        if (capture_paired_slots == before) break;
    }
}

static int k1_pdm_target_lane_valid(uint32_t lane) {
    return lane < K1_PDM_TARGET_LANE_COUNT;
}

int k1_pdm_target_initialised(void) { return capture_initialised; }
int k1_pdm_target_running(void) { return capture_running; }
int32_t k1_pdm_target_last_fsp_error(void) { return capture_last_fsp_error; }
uint32_t k1_pdm_target_data_callbacks(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].data_callbacks : 0u;
}
uint32_t k1_pdm_target_error_callbacks(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].error_callbacks : 0u;
}
uint32_t k1_pdm_target_error_flags(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].error_flags : 0u;
}
uint32_t k1_pdm_target_processed_slots(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].processed_slots : 0u;
}
uint32_t k1_pdm_target_processed_samples(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].processed_samples : 0u;
}
uint32_t k1_pdm_target_sample_hash(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].sample_hash : 0u;
}
int32_t k1_pdm_target_sample_min(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) && capture_metrics[lane].processed_samples
         ? capture_metrics[lane].sample_min : 0;
}
int32_t k1_pdm_target_sample_max(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) && capture_metrics[lane].processed_samples
         ? capture_metrics[lane].sample_max : 0;
}
uint32_t k1_pdm_target_sample_peak(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].sample_peak : 0u;
}
uint64_t k1_pdm_target_sample_square_sum(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].sample_square_sum : 0u;
}
uint32_t k1_pdm_target_overflow_events(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_stream[lane].overflow_events : 0u;
}
uint32_t k1_pdm_target_drop_events(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_stream[lane].drop_events : 0u;
}
uint32_t k1_pdm_target_recovery_count(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_stream[lane].recovery_count : 0u;
}
uint64_t k1_pdm_target_first_capture_start_us(void) { return capture_first_start_us; }
uint64_t k1_pdm_target_last_capture_end_us(void) { return capture_last_end_us; }
uint32_t k1_pdm_target_paired_slots(void) { return capture_paired_slots; }
uint32_t k1_pdm_target_pair_skew_drops(void) { return capture_pair_skew_drops; }
uint32_t k1_pdm_target_startup_discard_pairs(void) { return capture_startup_discard_pairs; }
uint64_t k1_pdm_target_max_pair_skew_us(void) { return capture_max_pair_skew_us; }
uint32_t k1_pdm_target_rearm_denied(void) { return capture_rearm_denied; }
uint32_t k1_pdm_target_sat_neg(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].sat_neg : 0u;
}
uint32_t k1_pdm_target_sat_pos(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].sat_pos : 0u;
}
uint32_t k1_pdm_target_packing_mismatch(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].packing_mismatch : 0u;
}
uint32_t k1_pdm_target_first_sat_raw(uint32_t lane) {
    return k1_pdm_target_lane_valid(lane) ? capture_metrics[lane].first_sat_raw : 0u;
}
uint32_t k1_pdm_target_asrc_starved(void) { return capture_asrc_starved; }
uint32_t k1_pdm_target_ap_hops(void) { return capture_ap_hops; }
uint32_t k1_pdm_target_last_hop_dt_us(void) { return capture_last_hop_dt_us; }
uint32_t k1_pdm_target_last_hop_peak(void) { return capture_last_hop_peak; }
uint32_t k1_pdm_target_last_hop_gain_q8(void) { return capture_last_hop_gain_q8; }
uint32_t k1_pdm_target_gain_clip_pos(void) { return capture_gain_clip_pos; }
uint32_t k1_pdm_target_gain_clip_neg(void) { return capture_gain_clip_neg; }
uint32_t k1_pdm_target_measured_hz(void) { return capture_measured_hz; }
uint32_t k1_pdm_target_rate_locked(void) { return capture_rate_locked; }
uint32_t k1_pdm_target_asrc_consumed(void) { return capture_asrc.consumed_samples; }
uint32_t k1_pdm_target_asrc_discarded(void) { return capture_asrc.discarded_samples; }
uint32_t k1_pdm_target_push_rejected(void) { return capture_asrc.push_rejected; }
uint64_t k1_pdm_target_stream_epoch(void) { return capture_stream_epoch; }
uint32_t k1_pdm_target_pair_epoch_drops(void) { return capture_pair_epoch_drops; }

k1_audio_read_result_t k1_pdm_target_try_read_ap_hop(k1_audio_hop_t *out) {
    uint32_t required;
    uint32_t inc;
    uint64_t s;
    uint64_t f;
    uint64_t consume_count;
    uint64_t first_index;
    uint64_t last_index;
    uint64_t next_index;
    uint64_t support_last;
    const k1_capture_block_meta_t *first_meta;
    const k1_capture_block_meta_t *last_meta;
    const k1_capture_block_meta_t *support_meta;
    uint32_t hop_peak = 0u;
    uint32_t i;
    uint64_t ready_us;
    uint64_t q_index;
    uint64_t q_frac;
    uint64_t r;
    uint64_t delta_q16;
    uint64_t delta_us;
    int rc;
    if (out == 0) {
        return K1_AUDIO_READ_FAULT;
    }
    k1_pdm_target_poll_stale(k1_pdm_target_now_us());
    if (capture_discontinuity_pending) {
        memset(out, 0, sizeof(*out));
        out->discontinuity_reason = capture_discontinuity_reason;
        capture_discontinuity_pending = 0u;
        return K1_AUDIO_READ_DISCONTINUITY;
    }
    if (capture_stale_active) {
        return K1_AUDIO_READ_NOT_READY;
    }
    if (!capture_running) {
        return K1_AUDIO_READ_NOT_READY;
    }
    required = k1_asrc24_required_for_pull180(&capture_asrc);
    if (capture_asrc.filled < required) {
        return K1_AUDIO_READ_NOT_READY;
    }
    inc = k1_asrc24_phase_increment_q16(capture_asrc.source_hz);
    s = capture_asrc.src_index;
    f = capture_asrc.frac_q16;
    consume_count = s + ((f + 180ull * (uint64_t)inc) / 65536ull);
    first_index = capture_source_base + s;
    last_index = capture_source_base + s + ((f + 179ull * (uint64_t)inc) / 65536ull);
    next_index = capture_source_base + consume_count;
    support_last = last_index + 1ull;
    first_meta = k1_pdm_target_meta_for(first_index);
    last_meta = k1_pdm_target_meta_for(last_index);
    support_meta = k1_pdm_target_meta_for(support_last);
    if (first_meta == 0 || last_meta == 0 || support_meta == 0) {
        return K1_AUDIO_READ_FAULT;
    }
    if (k1_audio_hop_init_interval(out, capture_stream_epoch,
                                   capture_hop_sequence + 1u) != 0) {
        return K1_AUDIO_READ_FAULT;
    }
    rc = k1_asrc24_pull180(&capture_asrc, out->pcm);
    if (rc == k1_asrc24_err_not_ready) {
        return K1_AUDIO_READ_NOT_READY;
    }
    if (rc != k1_asrc24_ok) {
        return K1_AUDIO_READ_FAULT;
    }
    capture_source_base += consume_count;
    capture_hop_sequence += 1u;
    k1_pdm_target_retire_meta();
    ready_us = k1_pdm_target_now_us();
    out->raw_capture_epoch = last_meta->raw_epoch;
    out->source_identity = K1_PDM_TARGET_PROGRAMME_LANE;
    out->programme_lane = K1_PDM_TARGET_PROGRAMME_LANE;
    out->source_first_index = first_index;
    out->source_first_frac_q16 = (uint16_t)f;
    out->source_last_index = last_index;
    out->source_last_frac_q16 = (uint16_t)((f + 179ull * (uint64_t)inc) % 65536ull);
    out->source_next_index = next_index;
    out->source_next_frac_q16 = (uint16_t)((f + 180ull * (uint64_t)inc) % 65536ull);
    out->source_support_last_index = support_last;
    out->supporting_block_source_end_exclusive = support_meta->source_end_exclusive;
    out->source_block_first_sequence = first_meta->sequence;
    out->source_block_last_sequence = last_meta->sequence;
    out->source_rate_hz = capture_asrc.source_hz;
    out->rate_segment = capture_rate_segment;
    out->phase_increment_q16 = inc;
    out->supporting_dma_receipt_cycles = support_meta->receipt_cycles;
    out->supporting_dma_receipt_us = support_meta->receipt_us;
    out->descriptor_ready_us = ready_us;
    out->timestamp_provenance = K1_AUDIO_TS_DMA_ISR_BACKPROJECTION;
    out->physical_timestamp_valid = 0;
    out->uncertainty_known = 0;
    out->rate_state = capture_rate_locked ? K1_AUDIO_RATE_LOCKED
                                          : K1_AUDIO_RATE_CALIBRATING;
    q_index = last_index;
    q_frac = out->source_last_frac_q16;
    r = support_meta->source_end_exclusive - 1ull;
    out->capture_estimate_valid = 0;
    out->newest_sample_capture_estimate_us = 0;
    if (r >= q_index) {
        delta_q16 = (r - q_index) * 65536ull - q_frac;
        if (capture_asrc.source_hz != 0u &&
            delta_q16 <= (UINT64_MAX / 1000000ull)) {
            delta_us = (delta_q16 * 1000000ull) /
                       (65536ull * (uint64_t)capture_asrc.source_hz);
            if (support_meta->receipt_us >= delta_us) {
                out->newest_sample_capture_estimate_us =
                    support_meta->receipt_us - delta_us;
                if (out->newest_sample_capture_estimate_us <= ready_us) {
                    out->capture_estimate_valid = 1;
                }
            }
        }
    }
    for (i = 0; i < 180u; ++i) {
        const int32_t v = out->pcm[i];
        const uint32_t mag = v < 0 ? (uint32_t)(-v) : (uint32_t)v;
        if (mag > hop_peak) hop_peak = mag;
    }
    capture_last_hop_peak = hop_peak;
    if (capture_last_hop_us != 0u && ready_us >= capture_last_hop_us) {
        const uint64_t hop_dt = ready_us - capture_last_hop_us;
        capture_last_hop_dt_us = hop_dt > 0xffffffffull ? 0xffffffffu : (uint32_t) hop_dt;
    }
    capture_last_hop_us = ready_us;
    capture_ap_hops += 1u;
    return K1_AUDIO_READ_OK;
}

int k1_pdm_target_pull_ap_hop(int16_t out180[180]) {
    k1_audio_hop_t hop;
    if (out180 == 0) return -1;
#ifdef K1_LIVE_RUNTIME
    (void)hop;
    return -1;
#else
    if (k1_pdm_target_try_read_ap_hop(&hop) != K1_AUDIO_READ_OK) return -1;
    memcpy(out180, hop.pcm, 180u * sizeof(int16_t));
    return 0;
#endif
}

uint32_t k1_pdm_target_spare_slots(void) {
    uint32_t lane;
    uint32_t spare = K1_PDM_STREAM_SLOT_COUNT;
    if (!capture_running) return spare;
    for (lane = 0; lane < K1_PDM_TARGET_LANE_COUNT; ++lane) {
        uint32_t index;
        uint32_t free_slots = 0u;
        for (index = 0; index < K1_PDM_STREAM_SLOT_COUNT; ++index) {
            const uint32_t state = capture_stream[lane].slots[index].state;
            if (state == K1_PDM_SLOT_FREE || state == K1_PDM_SLOT_FILLING) {
                free_slots++;
            }
        }
        if (free_slots < spare) spare = free_slots;
    }
    return spare;
}
