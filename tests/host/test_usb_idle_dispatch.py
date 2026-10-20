"""Execute the actual foreground USB block against a frame-starting poll.

Host scheduling proof only; latency and LED output require the physical run.
"""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def test_usb_dispatch_uses_idle_window_and_rechecks_command_side_start(tmp_path):
    entry = (ROOT / "platform/ra8p1/hal_entry.c").read_text()
    start = entry.index("        /* FSP EventGet advances")
    end = entry.index("#ifdef K1_PDM_TARGET", start)
    block = entry[start:end]
    # Dispatch must precede the real poll, not merely appear in a guarded block.
    assert entry.index("R_USB_Write", start) < entry.index("k1_fixture_poll(", start)
    source = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
enum { USB_STATUS_NONE, USB_STATUS_CONFIGURED, USB_STATUS_RESUME,
       USB_STATUS_REQUEST, USB_STATUS_READ_COMPLETE, USB_STATUS_WRITE_COMPLETE,
       USB_STATUS_DETACH, USB_STATUS_SUSPEND, USB_CLASS_PCDC, FSP_SUCCESS=0 };
typedef struct { unsigned data_size; } usb_event_info_t;
typedef int usb_status_t;
#define RT_TICK_PER_SECOND 1000
static int g_basic0_ctrl, next_event, busy, writes, reads, events, command_starts;
static bool attached=true, usb_configured=true, read_armed=true, write_pending=false;
static uint8_t usb_read[64], tx[64];
static size_t tx_size;
static unsigned mock_cycles;
static unsigned cycle_read(void) { mock_cycles+=100; return mock_cycles; }
typedef struct { unsigned ignored; } Dwt;
static Dwt dwt;
#define DWT (&dwt)
#define CYCCNT ignored + cycle_read()
static const unsigned SystemCoreClock=1000000;
static int rt_tick_get(void) { return 0; }
static bool k1_fixture_usb_service_allowed(void) { return !busy; }
static void R_USB_EventGet(usb_event_info_t *i, usb_status_t *e) {
    assert(!busy); ++events; *e=next_event; next_event=USB_STATUS_NONE; i->data_size=32;
}
static void k1_note_usb_event(uint32_t e) { (void)e; }
static void k1_status_led_usb(int a,int b,uint32_t c) { (void)a;(void)b;(void)c; }
static void k1_handle_request(usb_event_info_t *i) { (void)i; }
static void k1_fixture_consume(uint8_t *p,unsigned n,uint32_t t) {
    (void)p;(void)n;(void)t; tx_size=32; busy=command_starts;
}
static void k1_fixture_control_service(void) { assert(!busy); }
static bool k1_fixture_control_pending(void) { return false; }
static void k1_fixture_sent(void) { tx_size=0; }
static void k1_fixture_disconnect(void) { tx_size=0; }
static const uint8_t *k1_fixture_reply(size_t *n) { *n=tx_size; return tx; }
static int R_USB_Write(int *c,uint8_t *p,uint32_t n,int k) {
    (void)c;(void)p;(void)n;(void)k; assert(!busy); ++writes; return 0;
}
static int R_USB_Read(int *c,uint8_t *p,size_t n,int k) {
    (void)c;(void)p;(void)n;(void)k; assert(!busy); ++reads; return 0;
}
static void iteration(bool usb_ready, bool hold_usb) {
    uint32_t led_now=0;
''' + block + r'''
    busy=1; /* Palette poll starts the next frame. */
}
int main(void) {
    next_event=USB_STATUS_READ_COMPLETE;
    iteration(true,false);
    assert(events>1 && events<20 && writes==1 && write_pending && tx_size==32);
    int idle_events=events;
    iteration(true,false); /* Active frame excludes all foreground USB. */
    assert(events==idle_events && writes==1 && reads==0);
    busy=0; next_event=USB_STATUS_WRITE_COMPLETE;
    iteration(true,false);
    assert(!write_pending && tx_size==0 && reads==1 && read_armed);
    busy=0; command_starts=1; next_event=USB_STATUS_READ_COMPLETE;
    iteration(true,false); /* Mutating fixture starts GPT during consume. */
    assert(events==idle_events+2 && writes==1 && reads==1);
    busy=0; iteration(true,true); assert(events==idle_events+2 && writes==1);
    busy=0; iteration(false,false); assert(events==idle_events+2 && writes==1);
    /* EP0 work must drain even before CDC has a pending write. It remains
       bounded, cannot run during an active frame, and returns to the owner. */
    busy=0; attached=false; usb_configured=false; write_pending=false; mock_cycles=0;
    next_event=USB_STATUS_RESUME;
    tx_size=0; read_armed=false; command_starts=0;
    int before_enumeration=events;
    iteration(true,false);
    assert(events>before_enumeration+1 && events<before_enumeration+20);
    assert(!attached && !usb_configured);
    int after_enumeration=events;
    iteration(true,false);
    assert(events==after_enumeration); /* Active LED frame remains isolated. */
    busy=0; next_event=USB_STATUS_CONFIGURED;
    iteration(true,false);
    assert(attached && events==after_enumeration+1);
    return 0;
}
'''
    c = tmp_path / "usb.c"
    exe = tmp_path / "usb"
    c.write_text(source)
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", str(c), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)


def test_live_metrics_and_status_board_service_are_observational():
    source = (ROOT / "platform/ra8p1/hal_entry.c").read_text()
    metrics = source[source.index("size_t k1_platform_metrics"):source.index("void hal_entry")]
    assert "k1_pdm_target_poll(" not in metrics
    assert "k1_ws281x_gpt_dma_hw_pair_poll(" not in metrics
    assert "k1_ws281x_gpt_dma_hw_pair_busy(" not in metrics
    assert "k1_ws281x_gpt_dma_hw_pair_ready(" not in metrics
    # Onboard status includes MDIO bit-banging. It shares the idle admission.
    board = source[source.index("const uint32_t led_t0"):source.index("/* FSP EventGet advances")]
    assert board.index("if (k1_fixture_usb_service_allowed())") < board.index("k1_status_led_poll(")



def test_usb_irq_ownership_preserves_other_irqs_and_pending_bits(tmp_path):
    source = (ROOT / "platform/ra8p1/hal_entry.c").read_text()
    block = source[source.index("/* USB IRQ isolation."):source.index("void hal_entry(void)")]
    assert source.index("k1_usb_irq_suspend();",source.index("void hal_entry(void)")) < source.index("k1_fixture_poll(", source.index("void hal_entry(void)"))
    c = tmp_path/'irq.c'
    c.write_text(r"""
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#define K1_PALETTE_WS2816_GPT_PAIR 1
#define FSP_INVALID_VECTOR (-1)
typedef int IRQn_Type;
static struct {int irq,irq_r,irq_d0,irq_d1,hsirq,hsirq_d0,hsirq_d1,module_number;} g_basic0_cfg={17,18,19,20,21,22,23,0};
static unsigned enabled=0xffffffffu, pending=0xabcdefu;
static bool active;
static bool k1_fixture_usb_service_allowed(void) {return !active;}
static unsigned NVIC_GetEnableIRQ(int irq) {return (enabled>>irq)&1u;}
static void NVIC_DisableIRQ(int irq) {enabled &= ~(1u<<irq);}
static void NVIC_EnableIRQ(int irq) {assert(!active);enabled |= 1u<<irq;}
static void __DSB(void) {}
static void __ISB(void) {}
""" + block + r"""
int main(void) {
    enabled &= ~(1u<<19); /* Driver already left FIFO DMA IRQ disabled. */
    const unsigned original=enabled;
    k1_usb_irq_suspend();assert(usb_irq_suspended);
    assert((enabled & (15u<<17))==0);
    assert((enabled & ~(15u<<17))==(original & ~(15u<<17)));
    active=true;k1_usb_irq_suspend();k1_usb_irq_resume();
    assert((enabled & (15u<<17))==0 && usb_irq_suspended);
    assert(pending==0xabcdefu);
    active=false;k1_usb_irq_resume();assert(enabled==original && !usb_irq_suspended);
    k1_usb_irq_resume();assert(enabled==original);
    g_basic0_cfg.module_number=1;
    k1_usb_irq_suspend();assert((enabled & (7u<<21))==0);
    assert((enabled & ~(7u<<21))==(original & ~(7u<<21)));
    k1_usb_irq_resume();assert(enabled==original && pending==0xabcdefu);
    return 0;
}
""")
    exe=tmp_path/'irq'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)


def test_boot_usb_phase_is_bounded_and_services_post_configuration_requests(tmp_path):
    source=(ROOT/'platform/ra8p1/hal_entry.c').read_text()
    start=source.index('    const uint32_t usb_boot_start=')
    block=source[start:source.index('#endif',start)]
    assert 'k1_fixture_poll' not in block and 'pair_' not in block
    c=tmp_path/'boot.c'
    c.write_text(r"""
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
typedef struct { unsigned x; } usb_event_info_t;
typedef int usb_status_t;
enum { USB_STATUS_NONE, USB_STATUS_CONFIGURED, USB_STATUS_RESUME, USB_STATUS_REQUEST };
static struct { uint32_t CYCCNT; } clock_state;
#define DWT (&clock_state)
static const uint32_t SystemCoreClock=1000000;
static bool attached, usb_configured;
static int calls, requests, configure_at, resume_at;
static void R_USB_EventGet(usb_event_info_t *i, usb_status_t *event) {
    (void)i; ++calls; *event=calls==configure_at?USB_STATUS_CONFIGURED:calls==resume_at?USB_STATUS_RESUME:USB_STATUS_REQUEST;
}
static void k1_handle_request(usb_event_info_t *i) {(void)i;++requests;}
static void rt_thread_mdelay(int ms) {DWT->CYCCNT+=(uint32_t)ms*1000u;}
static void boot(bool usb_ready) {
"""+block+r"""
}
int main(void) {
    boot(false);assert(calls==0 && !attached);
    boot(true);assert(calls==500 && requests==500 && !attached);
    calls=0;requests=0;DWT->CYCCNT=UINT32_MAX-50000u;
    boot(true);assert(calls==500 && !attached); /* Counter wrap remains bounded. */
    calls=0;requests=0;configure_at=121;resume_at=1;
    boot(true);assert(calls==500 && attached && usb_configured);
    assert(requests==498); /* CDC class setup continues after CONFIGURED. */
    return 0;
}
""")
    exe=tmp_path/'boot'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
