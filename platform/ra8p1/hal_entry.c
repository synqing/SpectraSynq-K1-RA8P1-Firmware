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
static uint8_t usb_read[64];
static bool attached, read_armed, write_pending;
uint32_t k1_cycle_count(void) { return DWT->CYCCNT; }
size_t k1_platform_metrics(char* output, size_t capacity) {
    const rt_thread_t self=rt_thread_self();
    size_t untouched=0;
    const uint8_t* stack=(const uint8_t*)self->stack_addr;
    while(untouched<self->stack_size && stack[untouched]=='#') ++untouched;
    rt_size_t total=0,used=0,maximum=0;
    rt_memory_info(&total,&used,&maximum);
    /* Consistency check against RT-Thread ticks, not an externally calibrated
       oscillator measurement. This wait is outside all AP/render measurements. */
    const uint32_t first_tick=rt_tick_get(), first_cycle=DWT->CYCCNT;
    rt_thread_mdelay(100);
    const uint32_t elapsed_cycles=DWT->CYCCNT-first_cycle;
    const uint32_t elapsed_ticks=rt_tick_get()-first_tick;
    const int n=snprintf(output,capacity,
        "{\"stack_bytes\":%lu,\"stack_untouched_bytes\":%lu,\"heap_total\":%lu,\"heap_used\":%lu,\"heap_maximum\":%lu,\"fpscr\":%lu,\"clock_check_cycles\":%lu,\"clock_check_ticks\":%lu,\"tick_hz\":%lu}",
        (unsigned long)self->stack_size,(unsigned long)untouched,(unsigned long)total,
        (unsigned long)used,(unsigned long)maximum,(unsigned long)__get_FPSCR(),
        (unsigned long)elapsed_cycles,(unsigned long)elapsed_ticks,(unsigned long)RT_TICK_PER_SECOND);
    return n>0 && (size_t)n<capacity?(size_t)n:0;
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
#if BSP_CFG_DCACHE_ENABLED
    SCB_DisableDCache(); __DSB(); __ISB();
#endif
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT=0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    uint8_t uid[16]; const uint8_t* raw=R_BSP_UniqueIdGet()->unique_id_bytes;
    for(unsigned i=0;i<16;++i) uid[i]=raw[15-i];
    k1_fixture_initialise(uid,SystemCoreClock,R_CPU_CTRL->CPU1ACTCSR);
    /* No SecondaryCoreStart and no RM_ETHOSU_Open in the scalar image. */
    if(FSP_SUCCESS!=R_USB_Open(&g_basic0_ctrl,&g_basic0_cfg)) return;
    for(;;) {
        usb_event_info_t info={0}; usb_status_t event=USB_STATUS_NONE;
        (void)R_USB_EventGet(&info,&event);
        switch(event) {
        case USB_STATUS_CONFIGURED: case USB_STATUS_RESUME: attached=true; break;
        case USB_STATUS_REQUEST: k1_handle_request(&info); break;
        case USB_STATUS_READ_COMPLETE:
            read_armed=false;
            k1_fixture_consume(usb_read,info.data_size,(uint32_t)((uint64_t)rt_tick_get()*1000U/RT_TICK_PER_SECOND));
            break;
        case USB_STATUS_WRITE_COMPLETE: write_pending=false; k1_fixture_sent(); break;
        case USB_STATUS_DETACH: case USB_STATUS_SUSPEND:
            attached=false; read_armed=false; write_pending=false; k1_fixture_disconnect(); break;
        default: break;
        }
        k1_fixture_poll((uint32_t)((uint64_t)rt_tick_get()*1000U/RT_TICK_PER_SECOND));
        size_t size; const uint8_t* reply=k1_fixture_reply(&size);
        if(attached && size && !write_pending &&
           FSP_SUCCESS==R_USB_Write(&g_basic0_ctrl,(uint8_t*)reply,(uint32_t)size,USB_CLASS_PCDC)) write_pending=true;
        if(attached && !size && !read_armed &&
           FSP_SUCCESS==R_USB_Read(&g_basic0_ctrl,usb_read,sizeof(usb_read),USB_CLASS_PCDC)) read_armed=true;
        rt_thread_mdelay(1);
    }
}
