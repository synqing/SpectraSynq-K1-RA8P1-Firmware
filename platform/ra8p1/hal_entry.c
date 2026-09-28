/* Scalar M85 shell. CDC lifecycle/control handling follows verified Titan P2/P3. */
#include <stdbool.h>
#include <board.h>
#include <rtthread.h>
#include "hal_data.h"
#include "common_data.h"
#include "core_cm85.h"
#include <usb_pcdc/usb_pcdc.h>
#include <stdio.h>
#include "fixture_app.h"
#include "k1_status_led.h"
#include "titan_led2_phy.h"
#ifdef K1_NPU_LOAD
#include "npu_load.h"
#endif
#ifdef K1_PDM_CAPTURE
#include "pdm_capture.h"
#endif
#ifdef K1_PDM_TARGET
#include "pdm_target.h"
#endif
#ifdef K1_PALETTE_WS2816_GPT_PAIR
#include "ws281x_gpt_dma_hw_pair.h"
#elif defined(K1_PALETTE_GPT_DMA)
#include "ws281x_gpt_dma_hw.h"
#endif
#ifdef K1_PCM1808_TARGET
#include "pcm1808_target.h"
#endif
#ifdef K1_ARM_NUMERIC
extern int k1_arm_numeric_prepare(void);
#endif
#ifdef K1_USEFUL_NPU
extern int k1_useful_npu_probe(void);
#endif
#ifdef K1_COEXIST
extern int k1_coexist_probe(void);
#endif
static uint8_t usb_read[64];
static bool attached, usb_configured, read_armed, write_pending;
static uint32_t svc_pdm_cyc, svc_gpt_cyc, svc_usb_cyc;
static uint32_t max_pdm_gap_cyc, max_gpt_gap_cyc, max_usb_gap_cyc;
static void k1_mark_service(uint32_t *last, uint32_t *max_gap) {
    const uint32_t now = DWT->CYCCNT;
    if (*last != 0u) {
        const uint32_t gap = now - *last;
        if (gap > *max_gap) *max_gap = gap;
    }
    *last = now;
}
static uint32_t k1_gap_us(uint32_t cycles) {
    const uint32_t hz = SystemCoreClock;
    if (hz < 1000000u) return 0u;
    return cycles / (hz / 1000000u);
}
uint32_t k1_cycle_count(void) { return DWT->CYCCNT; }
size_t k1_platform_metrics(char* output, size_t capacity) {
#if defined(K1_PCM1808_TARGET) && !defined(K1_PDM_TARGET)
    return k1_pcm1808_target_metrics(output, capacity);
#else
    const rt_thread_t self=rt_thread_self();
    size_t untouched=0;
    const uint8_t* stack=(const uint8_t*)self->stack_addr;
    while(untouched<self->stack_size && stack[untouched]=='#') ++untouched;
    rt_size_t total=0,used=0,maximum=0;
    rt_memory_info(&total,&used,&maximum);
    /* Consistency check against RT-Thread ticks, not an externally calibrated
       oscillator measurement. This wait is outside all AP/render measurements. */
    const uint32_t first_tick=rt_tick_get(), first_cycle=DWT->CYCCNT;
#ifdef K1_PDM_TARGET
    /* Do not stall 100 ms on the USB request path. GPT timeout is ~14 ms. */
    /* Observation only: PDM progression belongs to the main loop. */
#if defined(K1_PALETTE_GPT_DMA) && !defined(K1_PALETTE_WS2816_GPT_PAIR)
    k1_ws281x_gpt_dma_hw_poll();
#endif
    /* Paired LED progress belongs to fixture_poll. Polling it here could
       start pending DMA immediately before this USB metrics response. */


#else
    rt_thread_mdelay(100);
#endif
    const uint32_t elapsed_cycles=DWT->CYCCNT-first_cycle;
    const uint32_t elapsed_ticks=rt_tick_get()-first_tick;
    const uint32_t scb_ccr=SCB->CCR;
#ifdef K1_PDM_TARGET
    const int n=snprintf(output,capacity,
        "{\"stack_bytes\":%lu,\"stack_untouched_bytes\":%lu,\"heap_total\":%lu,\"heap_used\":%lu,\"heap_maximum\":%lu,\"fpscr\":%lu,\"scb_ccr\":%lu,\"dcache_enabled\":%s,\"icache_enabled\":%s,\"clock_check_cycles\":%lu,\"clock_check_ticks\":%lu,\"tick_hz\":%lu,\"pdm_target\":{\"mpn\":\"LMD2718T261-OA1\",\"data_pin\":\"P502\",\"clock_pin\":\"P812\",\"sample_rate_hz\":%lu,\"profile\":\"ap_40k_asrc24\",\"working_source_sample_rate_hz\":24000,\"sample_rate_match\":false,\"slot_elements\":%lu,\"slot_duration_us\":%lu,\"shared_clock_and_data\":true,\"programme_lane\":0,\"acoustic_identity\":\"unproven\",\"initialised\":%s,\"running\":%s,\"last_fsp_error\":%ld,\"rearm_denied\":%lu,\"paired_slots\":%lu,\"pair_skew_drops\":%lu,\"startup_discard_pairs\":%lu,\"max_pair_skew_us\":%llu,\"first_capture_start_us\":%llu,\"last_capture_end_us\":%llu,\"asrc_starved\":%lu,\"ap_hops\":%lu,\"last_hop_dt_us\":%lu,\"last_hop_peak\":%lu,\"last_hop_gain_q8\":%lu,\"gain_clip_pos\":%lu,\"gain_clip_neg\":%lu,\"measured_hz\":%lu,\"rate_locked\":%lu,\"stream_epoch\":%llu,\"pair_epoch_drops\":%lu,\"asrc_consumed\":%lu,\"asrc_discarded\":%lu,\"push_rejected\":%lu,\"stale_discards\":%lu,\"max_pdm_gap_us\":%lu,\"max_gpt_gap_us\":%lu,\"max_usb_gap_us\":%lu,\"lanes\":[{\"microphone\":\"U14\",\"select\":\"LOW\",\"edge\":\"RISE\",\"pdm_channel\":2,\"dma_channel\":%lu,\"role\":\"programme\",\"data_callbacks\":%lu,\"error_callbacks\":%lu,\"error_flags\":%lu,\"processed_slots\":%lu,\"processed_samples\":%lu,\"sample_hash\":%lu,\"sample_min\":%ld,\"sample_max\":%ld,\"sample_peak\":%lu,\"sample_square_sum\":%llu,\"overflow_events\":%lu,\"drop_events\":%lu,\"recovery_count\":%lu,\"sat_neg\":%lu,\"sat_pos\":%lu,\"packing_mismatch\":%lu,\"first_sat_raw\":%lu},{\"microphone\":\"U13\",\"select\":\"HIGH\",\"edge\":\"FALL\",\"pdm_channel\":0,\"dma_channel\":%lu,\"role\":\"measurement\",\"data_callbacks\":%lu,\"error_callbacks\":%lu,\"error_flags\":%lu,\"processed_slots\":%lu,\"processed_samples\":%lu,\"sample_hash\":%lu,\"sample_min\":%ld,\"sample_max\":%ld,\"sample_peak\":%lu,\"sample_square_sum\":%llu,\"overflow_events\":%lu,\"drop_events\":%lu,\"recovery_count\":%lu,\"sat_neg\":%lu,\"sat_pos\":%lu,\"packing_mismatch\":%lu,\"first_sat_raw\":%lu}]}}",
        (unsigned long)self->stack_size,(unsigned long)untouched,(unsigned long)total,
        (unsigned long)used,(unsigned long)maximum,(unsigned long)__get_FPSCR(),
        (unsigned long)scb_ccr,
        (scb_ccr&SCB_CCR_DC_Msk)?"true":"false",
        (scb_ccr&SCB_CCR_IC_Msk)?"true":"false",
        (unsigned long)elapsed_cycles,(unsigned long)elapsed_ticks,(unsigned long)RT_TICK_PER_SECOND,
        (unsigned long)K1_PDM_TARGET_SAMPLE_RATE_HZ,(unsigned long)K1_PDM_TARGET_SLOT_ELEMENTS,
        (unsigned long)K1_PDM_TARGET_SLOT_DURATION_US,
        k1_pdm_target_initialised()?"true":"false",k1_pdm_target_running()?"true":"false",
        (long)k1_pdm_target_last_fsp_error(),
        (unsigned long)k1_pdm_target_rearm_denied(),
        (unsigned long)k1_pdm_target_paired_slots(),
        (unsigned long)k1_pdm_target_pair_skew_drops(),
        (unsigned long)k1_pdm_target_startup_discard_pairs(),
        (unsigned long long)k1_pdm_target_max_pair_skew_us(),
        (unsigned long long)k1_pdm_target_first_capture_start_us(),
        (unsigned long long)k1_pdm_target_last_capture_end_us(),
        (unsigned long)k1_pdm_target_asrc_starved(),
        (unsigned long)k1_pdm_target_ap_hops(),
        (unsigned long)k1_pdm_target_last_hop_dt_us(),
        (unsigned long)k1_pdm_target_last_hop_peak(),
        (unsigned long)k1_pdm_target_last_hop_gain_q8(),
        (unsigned long)k1_pdm_target_gain_clip_pos(),
        (unsigned long)k1_pdm_target_gain_clip_neg(),
        (unsigned long)k1_pdm_target_measured_hz(),
        (unsigned long)k1_pdm_target_rate_locked(),
        (unsigned long long)k1_pdm_target_stream_epoch(),
        (unsigned long)k1_pdm_target_pair_epoch_drops(),
        (unsigned long)k1_pdm_target_asrc_consumed(),
        (unsigned long)k1_pdm_target_asrc_discarded(),
        (unsigned long)k1_pdm_target_push_rejected(),
        (unsigned long)k1_pdm_target_stale_discards(),
        (unsigned long)k1_gap_us(max_pdm_gap_cyc),
        (unsigned long)k1_gap_us(max_gpt_gap_cyc),
        (unsigned long)k1_gap_us(max_usb_gap_cyc),
        (unsigned long)K1_PDM_TARGET_RISE_DMA_CHANNEL,
        (unsigned long)k1_pdm_target_data_callbacks(0u),
        (unsigned long)k1_pdm_target_error_callbacks(0u),
        (unsigned long)k1_pdm_target_error_flags(0u),
        (unsigned long)k1_pdm_target_processed_slots(0u),
        (unsigned long)k1_pdm_target_processed_samples(0u),
        (unsigned long)k1_pdm_target_sample_hash(0u),
        (long)k1_pdm_target_sample_min(0u),(long)k1_pdm_target_sample_max(0u),
        (unsigned long)k1_pdm_target_sample_peak(0u),
        (unsigned long long)k1_pdm_target_sample_square_sum(0u),
        (unsigned long)k1_pdm_target_overflow_events(0u),
        (unsigned long)k1_pdm_target_drop_events(0u),
        (unsigned long)k1_pdm_target_recovery_count(0u),
        (unsigned long)k1_pdm_target_sat_neg(0u),
        (unsigned long)k1_pdm_target_sat_pos(0u),
        (unsigned long)k1_pdm_target_packing_mismatch(0u),
        (unsigned long)k1_pdm_target_first_sat_raw(0u),
        (unsigned long)K1_PDM_TARGET_FALL_DMA_CHANNEL,
        (unsigned long)k1_pdm_target_data_callbacks(1u),
        (unsigned long)k1_pdm_target_error_callbacks(1u),
        (unsigned long)k1_pdm_target_error_flags(1u),
        (unsigned long)k1_pdm_target_processed_slots(1u),
        (unsigned long)k1_pdm_target_processed_samples(1u),
        (unsigned long)k1_pdm_target_sample_hash(1u),
        (long)k1_pdm_target_sample_min(1u),(long)k1_pdm_target_sample_max(1u),
        (unsigned long)k1_pdm_target_sample_peak(1u),
        (unsigned long long)k1_pdm_target_sample_square_sum(1u),
        (unsigned long)k1_pdm_target_overflow_events(1u),
        (unsigned long)k1_pdm_target_drop_events(1u),
        (unsigned long)k1_pdm_target_recovery_count(1u),
        (unsigned long)k1_pdm_target_sat_neg(1u),
        (unsigned long)k1_pdm_target_sat_pos(1u),
        (unsigned long)k1_pdm_target_packing_mismatch(1u),
        (unsigned long)k1_pdm_target_first_sat_raw(1u));
#else
    const int n=snprintf(output,capacity,
        "{\"stack_bytes\":%lu,\"stack_untouched_bytes\":%lu,\"heap_total\":%lu,\"heap_used\":%lu,\"heap_maximum\":%lu,\"fpscr\":%lu,\"scb_ccr\":%lu,\"dcache_enabled\":%s,\"icache_enabled\":%s,\"clock_check_cycles\":%lu,\"clock_check_ticks\":%lu,\"tick_hz\":%lu}",
        (unsigned long)self->stack_size,(unsigned long)untouched,(unsigned long)total,
        (unsigned long)used,(unsigned long)maximum,(unsigned long)__get_FPSCR(),
        (unsigned long)scb_ccr,
        (scb_ccr&SCB_CCR_DC_Msk)?"true":"false",
        (scb_ccr&SCB_CCR_IC_Msk)?"true":"false",
        (unsigned long)elapsed_cycles,(unsigned long)elapsed_ticks,(unsigned long)RT_TICK_PER_SECOND);
#endif
    return n>0 && (size_t)n<capacity?(size_t)n:0;
#endif
}
static void k1_handle_request(const usb_event_info_t *event_info) {
    static usb_pcdc_linecoding_t line_coding;
    uint16_t request = (uint16_t)(event_info->setup.request_type & USB_BREQUEST);
    if (USB_PCDC_SET_LINE_CODING == request)
        (void)R_USB_PeriControlDataGet(&g_basic0_ctrl,(uint8_t*)&line_coding,LINE_CODING_LENGTH);
    else if (USB_PCDC_GET_LINE_CODING == request)
        (void)R_USB_PeriControlDataSet(&g_basic0_ctrl,(uint8_t*)&line_coding,LINE_CODING_LENGTH);
    else if (USB_PCDC_SET_CONTROL_LINE_STATE == request)
        (void)R_USB_PeriControlStatusSet(&g_basic0_ctrl,USB_SETUP_STATUS_ACK);
}
/* USB IRQ isolation. Only this USB instance is deferred; GPT/DMAC/PDM IRQs
   remain enabled. Preserve pending bits and the driver's previous enable mask.
   The foreground owner can start a generation only after suspend returns. */
#ifdef K1_PALETTE_WS2816_GPT_PAIR
static bool usb_irq_suspended;
static uint8_t usb_irq_saved;
static IRQn_Type k1_usb_instance_irq(unsigned index) {
    const IRQn_Type fs[4]={g_basic0_cfg.irq,g_basic0_cfg.irq_r,
                          g_basic0_cfg.irq_d0,g_basic0_cfg.irq_d1};
    const IRQn_Type hs[4]={g_basic0_cfg.hsirq,g_basic0_cfg.hsirq_d0,
                          g_basic0_cfg.hsirq_d1,FSP_INVALID_VECTOR};
    return g_basic0_cfg.module_number==0 ? fs[index] : hs[index];
}
static void k1_usb_irq_suspend(void) {
    if (usb_irq_suspended) return;
    usb_irq_saved=0;
    for (unsigned i=0;i<4;i++) {
        const IRQn_Type irq=k1_usb_instance_irq(i);
        if ((int32_t)irq<0) continue;
        if (NVIC_GetEnableIRQ(irq)) {
            usb_irq_saved|=(uint8_t)(1u<<i);
            NVIC_DisableIRQ(irq);
        }
    }
    __DSB(); __ISB();
    usb_irq_suspended=true;
}
static void k1_usb_irq_resume(void) {
    if (!usb_irq_suspended || !k1_fixture_usb_service_allowed()) return;
    usb_irq_suspended=false;
    for (unsigned i=0;i<4;i++) {
        const IRQn_Type irq=k1_usb_instance_irq(i);
        if ((int32_t)irq>=0 && (usb_irq_saved & (1u<<i))) NVIC_EnableIRQ(irq);
    }
}
#endif

void hal_entry(void) {
#if BSP_CFG_DCACHE_ENABLED && !defined(K1_KEEP_DCACHE_ENABLED)
    SCB_DisableDCache(); __DSB(); __ISB();
#endif
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT=0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    uint8_t uid[16]; const uint8_t* raw=R_BSP_UniqueIdGet()->unique_id_bytes;
    for(unsigned i=0;i<16;++i) uid[i]=raw[15-i];
#ifdef K1_NPU_LOAD
    (void)k1_npu_initialise();
#endif
#ifdef K1_PDM_CAPTURE
    /* Accounting probe only. Does not open R_PDM or replace the CDC fixture. */
    (void)k1_pdm_configure(16000u, 1u, 4u, 2u, 2u, 4000u);
    k1_pdm_start();
#endif
#ifdef K1_ARM_NUMERIC
    /* Host-equivalent prepare only. Does not claim executed Arm numerics. */
    (void)k1_arm_numeric_prepare();
#endif
#ifdef K1_USEFUL_NPU
    /* Admission only. Does not invoke Ethos-U or replace the CDC fixture. */
    (void)k1_useful_npu_probe();
#endif
#ifdef K1_COEXIST
    /* Pair bookkeeping only. Does not open USB, PDM, or Ethos-U. */
    (void)k1_coexist_probe();
#endif
    k1_status_led_init((uint32_t)rt_tick_get());
    (void)titan_led2_phy_init((uint32_t)rt_tick_get());
    k1_fixture_initialise(uid,SystemCoreClock,R_CPU_CTRL->CPU1ACTCSR);
    /* No SecondaryCoreStart and no RM_ETHOSU_Open in the scalar image. */
    const bool usb_ready = FSP_SUCCESS==R_USB_Open(&g_basic0_ctrl,&g_basic0_cfg);
    if(!usb_ready) {
        k1_status_led_fault(1,"usb_open",(uint32_t)rt_tick_get());
        #ifndef K1_LIVE_EMIT
        for(;;) k1_status_led_poll((uint32_t)rt_tick_get(),1);
#endif
    }
    k1_status_led_boot_ok((uint32_t)rt_tick_get());
#if defined(K1_PALETTE_WS2816_GPT_PAIR) && defined(K1_LIVE_EMIT)
    /* Bounded startup phase only: let USB enumerate before the first LED IRQ
       deferral. No host is required; autonomous audio/render starts after a
       500 ms admission interval (individual USB events are non-preemptible).
       USB 2.0 Connect Timing ECN, section 7.1.7.3: host debounce alone is
       at least 100 ms, followed by reset and at least 10 ms recovery. The
       former 100 ms window could expire before enumeration even began.
       CONFIGURED precedes CDC class setup requests; service the full bounded
       window so early configuration does not cut off that handover. An absent
       host never blocks product startup beyond this fixed interval.
       Runtime admission and transmitter timing remain unchanged. */
    const uint32_t usb_boot_start=DWT->CYCCNT;
    while (usb_ready &&
           (uint32_t)(DWT->CYCCNT-usb_boot_start)<SystemCoreClock/2U) {
        usb_event_info_t boot_info={0}; usb_status_t boot_event=USB_STATUS_NONE;
        (void)R_USB_EventGet(&boot_info,&boot_event);
        if (boot_event==USB_STATUS_CONFIGURED)
            attached=usb_configured=true;
        else if (boot_event==USB_STATUS_RESUME)
            attached=usb_configured;
        else if (boot_event==USB_STATUS_REQUEST) k1_handle_request(&boot_info);
        rt_thread_mdelay(1);
    }
#endif
    for(;;) {
#ifdef K1_PALETTE_WS2816_GPT_PAIR
        if (usb_ready) k1_usb_irq_resume();
#endif
        k1_fixture_schedule_step();
        const uint32_t remaining=k1_fixture_release_remaining_cycles();
        const bool hold_usb=k1_fixture_schedule_active() && remaining<(SystemCoreClock/1250U);
        const bool hold_yield=k1_fixture_schedule_active() && remaining<(SystemCoreClock/900U);
        const uint32_t led_now=(uint32_t)rt_tick_get();
        const uint32_t led_t0=DWT->CYCCNT;
        /* RGB is cheap and independent of USB hold. LED2 MDIO is admitted
           only when remaining slack covers one Clause-22 frame. */
        if (k1_fixture_usb_service_allowed()) {
            const uint32_t status_budget=SystemCoreClock/10000U;
            k1_status_led_set_led2_budget(k1_fixture_schedule_active() && remaining<status_budget
                                         ? remaining : status_budget);
            k1_status_led_poll(led_now, 1);
            k1_status_led_add_service_cycles(DWT->CYCCNT-led_t0);
        }
        /* FSP EventGet advances one queued USB interrupt per call. Drain a
           reply only in idle time. Stop admitting work after 1 ms; an individual
           event is non-preemptible, so this is not a worst-case execution bound.
           See staged FSP r_usb_basic.c / usb_pstd_pcd_task(). */
        const uint32_t usb_start=DWT->CYCCNT;
        do {
        usb_event_info_t info={0}; usb_status_t event=USB_STATUS_NONE;
        if(usb_ready && !hold_usb && k1_fixture_usb_service_allowed()) {
            k1_fixture_control_service();
            (void)R_USB_EventGet(&info,&event);
            k1_note_usb_event((uint32_t)event);
            switch(event) {
            case USB_STATUS_CONFIGURED:
                attached=usb_configured=true;
                k1_status_led_usb(1,1,led_now);
                break;
            case USB_STATUS_RESUME:
                /* Resume is a bus event, not proof of SET_CONFIGURATION. */
                attached=usb_configured;
                k1_status_led_usb(attached,attached,led_now);
                break;
            case USB_STATUS_REQUEST: k1_handle_request(&info); break;
            case USB_STATUS_READ_COMPLETE:
                read_armed=false;
                k1_fixture_consume(usb_read,info.data_size,(uint32_t)((uint64_t)rt_tick_get()*1000U/RT_TICK_PER_SECOND));
                break;
            case USB_STATUS_WRITE_COMPLETE: write_pending=false; k1_fixture_sent(); break;
            case USB_STATUS_DETACH:
                usb_configured=false;
                /* fall through */
            case USB_STATUS_SUSPEND:
                attached=false; read_armed=false; write_pending=false; k1_fixture_disconnect();
                k1_status_led_usb(0,0,led_now);
                break;
            default: break;
            }
        }
        /* Dispatch before palette polling can start the next LED frame.
           Recheck: a consumed diagnostic command may itself start a frame. */
        size_t size; const uint8_t* reply=k1_fixture_reply(&size);
        if(usb_ready && !hold_usb && k1_fixture_usb_service_allowed()) {
            if(attached && size && !write_pending &&
               FSP_SUCCESS==R_USB_Write(&g_basic0_ctrl,(uint8_t*)reply,(uint32_t)size,USB_CLASS_PCDC)) write_pending=true;
            if(attached && !size && !read_armed && !k1_fixture_control_pending() &&
               FSP_SUCCESS==R_USB_Read(&g_basic0_ctrl,usb_read,sizeof(usb_read),USB_CLASS_PCDC)) read_armed=true;
        }
            /* Enumeration has no CDC write_pending flag. Drain its internal
               FSP work within the same idle-only time budget; otherwise each
               EP0 stage receives just one task step before the next LED poll.
               Configured runtime reply admission is unchanged. */
            if(!usb_ready || hold_usb || !k1_fixture_usb_service_allowed() ||
               (attached && !write_pending)) break;
        } while((uint32_t)(DWT->CYCCNT-usb_start)<SystemCoreClock/1000U);
#ifdef K1_PDM_TARGET
        /* Product audio starts as soon as this loop can consume it. USB is
           an optional control connection, never the power-on permission. */
#ifdef K1_LIVE_EMIT
        if(!k1_pdm_target_initialised())
#else
        /* Offline fixtures retain the enumeration-first capture contract. */
        if(attached && !k1_pdm_target_initialised())
#endif
            (void)k1_pdm_target_initialise();
#endif
#ifdef K1_PCM1808_TARGET
        /* External PCM1808 supplies BCLK/LRCK. Do not arm SSIE1 until USB is
           configured and this loop can continuously drain 7.5 ms hops. */
        if(attached && !k1_pcm1808_target_initialised())
            (void)k1_pcm1808_target_initialise();
#endif
#ifdef K1_PDM_TARGET
        k1_pdm_target_poll();
        k1_mark_service(&svc_pdm_cyc, &max_pdm_gap_cyc);
#endif
#ifdef K1_PALETTE_WS2816_GPT_PAIR
        if (usb_ready) k1_usb_irq_suspend();
#endif
        k1_fixture_poll((uint32_t)((uint64_t)rt_tick_get()*1000U/RT_TICK_PER_SECOND));
#if defined(K1_PALETTE_GPT_DMA) || defined(K1_PALETTE_WS2816_GPT_PAIR)
        k1_mark_service(&svc_gpt_cyc, &max_gpt_gap_cyc);
#endif
        k1_mark_service(&svc_usb_cyc, &max_usb_gap_cyc);
#ifdef K1_PCM1808_TARGET
        k1_pcm1808_target_poll();
#endif
        if(!k1_fixture_schedule_active()) {
#ifdef K1_PDM_TARGET
            if(!k1_pdm_target_running()) rt_thread_mdelay(1);
#else
            rt_thread_mdelay(1);
#endif
        }
        else if(!hold_yield) rt_thread_yield();
    }
}
