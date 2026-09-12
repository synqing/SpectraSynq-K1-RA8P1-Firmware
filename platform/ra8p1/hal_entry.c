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
#ifdef K1_NPU_LOAD
#include "npu_load.h"
#endif
#ifdef K1_PDM_CAPTURE
#include "pdm_capture.h"
#endif
#ifdef K1_PDM_TARGET
#include "pdm_target.h"
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
static bool attached, read_armed, write_pending;
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
    /* Keep the two 7.5 ms DMAC rings drained while this diagnostic samples the
       RT-Thread clock. Observability must not manufacture a PDM overflow. */
    for(unsigned i=0;i<100;++i) {
        rt_thread_mdelay(1);
        k1_pdm_target_poll();
    }
#else
    rt_thread_mdelay(100);
#endif
    const uint32_t elapsed_cycles=DWT->CYCCNT-first_cycle;
    const uint32_t elapsed_ticks=rt_tick_get()-first_tick;
    const uint32_t scb_ccr=SCB->CCR;
#ifdef K1_PDM_TARGET
    const int n=snprintf(output,capacity,
        "{\"stack_bytes\":%lu,\"stack_untouched_bytes\":%lu,\"heap_total\":%lu,\"heap_used\":%lu,\"heap_maximum\":%lu,\"fpscr\":%lu,\"scb_ccr\":%lu,\"dcache_enabled\":%s,\"icache_enabled\":%s,\"clock_check_cycles\":%lu,\"clock_check_ticks\":%lu,\"tick_hz\":%lu,\"pdm_target\":{\"sample_rate_hz\":%lu,\"working_source_sample_rate_hz\":12800,\"sample_rate_match\":false,\"slot_elements\":%lu,\"slot_duration_us\":%lu,\"shared_clock_and_data\":true,\"programme_lane\":0,\"initialised\":%s,\"running\":%s,\"last_fsp_error\":%ld,\"paired_slots\":%lu,\"pair_skew_drops\":%lu,\"startup_discard_pairs\":%lu,\"max_pair_skew_us\":%llu,\"first_capture_start_us\":%llu,\"last_capture_end_us\":%llu,\"lanes\":[{\"microphone\":\"IM1\",\"select\":\"HIGH\",\"edge\":\"RISE\",\"pdm_channel\":2,\"dma_channel\":0,\"role\":\"programme\",\"data_callbacks\":%lu,\"error_callbacks\":%lu,\"error_flags\":%lu,\"processed_slots\":%lu,\"processed_samples\":%lu,\"sample_hash\":%lu,\"sample_min\":%ld,\"sample_max\":%ld,\"sample_peak\":%lu,\"sample_square_sum\":%llu,\"overflow_events\":%lu,\"drop_events\":%lu,\"recovery_count\":%lu},{\"microphone\":\"IM2\",\"select\":\"LOW\",\"edge\":\"FALL\",\"pdm_channel\":0,\"dma_channel\":1,\"role\":\"measurement\",\"data_callbacks\":%lu,\"error_callbacks\":%lu,\"error_flags\":%lu,\"processed_slots\":%lu,\"processed_samples\":%lu,\"sample_hash\":%lu,\"sample_min\":%ld,\"sample_max\":%ld,\"sample_peak\":%lu,\"sample_square_sum\":%llu,\"overflow_events\":%lu,\"drop_events\":%lu,\"recovery_count\":%lu}]}}",
        (unsigned long)self->stack_size,(unsigned long)untouched,(unsigned long)total,
        (unsigned long)used,(unsigned long)maximum,(unsigned long)__get_FPSCR(),
        (unsigned long)scb_ccr,
        (scb_ccr&SCB_CCR_DC_Msk)?"true":"false",
        (scb_ccr&SCB_CCR_IC_Msk)?"true":"false",
        (unsigned long)elapsed_cycles,(unsigned long)elapsed_ticks,(unsigned long)RT_TICK_PER_SECOND,
        (unsigned long)K1_PDM_TARGET_SAMPLE_RATE_HZ,(unsigned long)K1_PDM_TARGET_SLOT_ELEMENTS,
        (unsigned long)K1_PDM_TARGET_SLOT_DURATION_US,
        k1_pdm_target_initialised()?"true":"false",k1_pdm_target_running()?"true":"false",
        (long)k1_pdm_target_last_fsp_error(),(unsigned long)k1_pdm_target_paired_slots(),
        (unsigned long)k1_pdm_target_pair_skew_drops(),
        (unsigned long)k1_pdm_target_startup_discard_pairs(),
        (unsigned long long)k1_pdm_target_max_pair_skew_us(),
        (unsigned long long)k1_pdm_target_first_capture_start_us(),
        (unsigned long long)k1_pdm_target_last_capture_end_us(),
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
        (unsigned long)k1_pdm_target_recovery_count(1u));
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
    k1_fixture_initialise(uid,SystemCoreClock,R_CPU_CTRL->CPU1ACTCSR);
    /* No SecondaryCoreStart and no RM_ETHOSU_Open in the scalar image. */
    if(FSP_SUCCESS!=R_USB_Open(&g_basic0_ctrl,&g_basic0_cfg)) {
        k1_status_led_fault(1,"usb_open",(uint32_t)rt_tick_get());
        for(;;) k1_status_led_poll((uint32_t)rt_tick_get(),1);
    }
    k1_status_led_boot_ok((uint32_t)rt_tick_get());
    for(;;) {
        k1_fixture_schedule_step();
        const uint32_t remaining=k1_fixture_release_remaining_cycles();
        const bool hold_usb=k1_fixture_schedule_active() && remaining<(SystemCoreClock/1250U);
        const bool hold_yield=k1_fixture_schedule_active() && remaining<(SystemCoreClock/900U);
        const uint32_t led_now=(uint32_t)rt_tick_get();
        const uint32_t led_t0=DWT->CYCCNT;
        k1_status_led_poll(led_now, hold_usb?0:1);
        k1_status_led_add_service_cycles(DWT->CYCCNT-led_t0);
        usb_event_info_t info={0}; usb_status_t event=USB_STATUS_NONE;
        if(!hold_usb) {
            (void)R_USB_EventGet(&info,&event);
            k1_note_usb_event((uint32_t)event);
            switch(event) {
            case USB_STATUS_CONFIGURED: case USB_STATUS_RESUME:
                attached=true;
                k1_status_led_usb(1,1,led_now);
                break;
            case USB_STATUS_REQUEST: k1_handle_request(&info); break;
            case USB_STATUS_READ_COMPLETE:
                read_armed=false;
                k1_fixture_consume(usb_read,info.data_size,(uint32_t)((uint64_t)rt_tick_get()*1000U/RT_TICK_PER_SECOND));
                break;
            case USB_STATUS_WRITE_COMPLETE: write_pending=false; k1_fixture_sent(); break;
            case USB_STATUS_DETACH: case USB_STATUS_SUSPEND:
                attached=false; read_armed=false; write_pending=false; k1_fixture_disconnect();
                k1_status_led_usb(0,0,led_now);
                break;
            default: break;
            }
        }
#ifdef K1_PDM_TARGET
        /* USB enumeration can occupy more than the bounded 15 ms PDM ring.
           Start capture only once CONFIGURED has been observed and this loop
           can immediately become its active consumer. */
        if(attached && !k1_pdm_target_initialised())
            (void)k1_pdm_target_initialise();
#endif
#ifdef K1_PCM1808_TARGET
        /* External PCM1808 supplies BCLK/LRCK. Do not arm SSIE1 until USB is
           configured and this loop can continuously drain 7.5 ms hops. */
        if(attached && !k1_pcm1808_target_initialised())
            (void)k1_pcm1808_target_initialise();
#endif
        k1_fixture_poll((uint32_t)((uint64_t)rt_tick_get()*1000U/RT_TICK_PER_SECOND));
#ifdef K1_PDM_TARGET
        k1_pdm_target_poll();
#endif
#ifdef K1_PCM1808_TARGET
        k1_pcm1808_target_poll();
#endif
        size_t size; const uint8_t* reply=k1_fixture_reply(&size);
        if(!hold_usb) {
            if(attached && size && !write_pending &&
               FSP_SUCCESS==R_USB_Write(&g_basic0_ctrl,(uint8_t*)reply,(uint32_t)size,USB_CLASS_PCDC)) write_pending=true;
            if(attached && !size && !read_armed &&
               FSP_SUCCESS==R_USB_Read(&g_basic0_ctrl,usb_read,sizeof(usb_read),USB_CLASS_PCDC)) read_armed=true;
        }
        if(!k1_fixture_schedule_active()) rt_thread_mdelay(1);
        else if(!hold_yield) rt_thread_yield();
    }
}
