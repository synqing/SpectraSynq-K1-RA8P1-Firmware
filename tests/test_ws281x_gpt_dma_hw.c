/* Run the production driver. These are register/FSP seam tests, not silicon. */
#include <assert.h>
#include <stdio.h>
#define K1_RA8P1_TARGET 1
#define K1_WS281X_HW_TEST 1
#include "../platform/ra8p1/ws281x_gpt_dma_hw.c"
_Static_assert(sizeof(k1_ws281x_hw_diag_t)==728u,"v1 layout changed");
_Static_assert(sizeof(k1_ws281x_hw_diag_v2_t)==1420u,"v2 layout changed");
static const uint8_t wire[]={0x80,0x55,0xaa}; /* unlike the old tests, bit0 != bit1 */
static void fresh(void) {
 memset(&gpt6_ctrl,0,sizeof(gpt6_ctrl));memset(&gpt0_ctrl,0,sizeof(gpt0_ctrl));
 memset(&dmac_ctrl,0,sizeof(dmac_ctrl));memset(gpt_regs,0,sizeof(gpt_regs));
 memset(&dma_regs,0,sizeof(dma_regs));memset(irq_contexts,0,sizeof(irq_contexts));
 memset(snapshots,0,sizeof(snapshots));memset(&elc,0,sizeof(elc));
 memset(&fault_witness,0,sizeof(fault_witness));witness_prepared=0;dma_global.DMCTL=0;
 last_prepare_cycles=maximum_prepare_cycles=0;
 initialised=opened=attempts=state=owned=fault=first_fault=completions=errors=0;
 dma_complete=waveform_complete=dma_irqs=stop_irqs=0;
 elc_started=fail_start=fail_reset=irq_mask=0;mstp.MSTPCRC=0xffffffff;DWT->CYCCNT=100;
}
static void submit(void) {
 assert(k1_ws281x_gpt_dma_hw_init()); assert(elc_started);
 assert(k1_ws281x_gpt_dma_hw_submit(wire,sizeof(wire),1)==0);
 assert(gpt6_ctrl.p_reg->GTCCR[0]==duty[0]);
 assert(gpt6_ctrl.p_reg->GTCCR[2]==duty[1]); assert(duty[0]!=duty[1]);
 assert(gpt6_ctrl.p_reg->GTIOR&(1u<<4));
 assert(dmac_ctrl.p_reg->DMCRA==22);
}
static void done_dma(void) {dma_regs.DMCRA=0;dmac_done(NULL);}
static void stop_irq(void) {timer_callback_args_t a={.event=TIMER_EVENT_CYCLE_END};gpt_regs[6].GTCR=0;gpt0_overflow(&a);}
int main(int argc,char **argv) {
 fresh();submit();assert(elc.ELSR[0].HA_b.ELS==0x1b7&&elc.ELSR[1].HA_b.ELS==0x187);
 assert(k1_ws281x_gpt_dma_hw_submit(wire,3,1)==K1_WS281X_SUBMIT_BUSY);
 done_dma();assert(completions==0);stop_irq();assert(completions==0);
 DWT->CYCCNT+=reset_cycles;assert(k1_ws281x_gpt_dma_hw_poll()==K1_WS281X_TX_READY);assert(completions==1&&dma_irqs==1);
 k1_ws281x_hw_diag_t d;irq_mask=1;k1_ws281x_gpt_dma_hw_snapshot(&d);assert(irq_mask==1&&d.frames==1&&d.snapshots[3].gpt6_gtcr==0);
 assert(d.version==1&&d.bytes==728);
 k1_ws281x_hw_diag_v2_t v2; k1_ws281x_gpt_dma_hw_snapshot_v2(&v2);
 assert(irq_mask==1&&v2.header.version==2&&v2.header.bytes==sizeof(v2));
 assert(v2.first_fault.valid==0&&v2.first_fault.payload_bytes==0);
 fresh();submit();dma_regs.DMCRA=0;stop_irq();DWT->CYCCNT+=reset_cycles;k1_ws281x_gpt_dma_hw_poll();assert(completions==0);dmac_done(NULL);k1_ws281x_gpt_dma_hw_poll();assert(completions==1); /* IRQ order may reverse */
 fresh();submit();stop_irq();assert(first_fault==K1_WS281X_FAULT_DMA&&completions==0);assert(snapshots[4].dmac_count==22);
 assert(k1_ws281x_gpt_dma_hw_submit(wire,3,1)==K1_WS281X_SUBMIT_UNAVAILABLE);assert(errors==1); /* no retry storm */
 for (unsigned remaining=1;remaining<=3;++remaining) {
  fresh();submit();dma_regs.DMCRA=remaining;stop_irq();
  assert(first_fault==K1_WS281X_FAULT_DMA&&dma_complete==0);
  assert(snapshots[4].dmac_count==remaining&&snapshots[4].dmac_enable==1);
  DWT->CYCCNT+=reset_cycles;assert(k1_ws281x_gpt_dma_hw_poll()==K1_WS281X_TX_FAULT);
  assert(completions==0&&errors==1&&dma_regs.DMCNT==0);
  dmac_done(NULL);assert(completions==0&&dma_irqs==0&&errors==1);
 }
 /* A later failed submission must not report the first successful frame. */
 fresh();submit();done_dma();stop_irq();DWT->CYCCNT+=reset_cycles;k1_ws281x_gpt_dma_hw_poll();
 uint8_t later[]={24,177,0}; assert(k1_ws281x_gpt_dma_hw_submit(later,sizeof(later),1)==0);
 memset(later,255,sizeof(later)); /* Caller storage may change after submit. */
 dma_regs.DMCRA=2;dma_regs.DMSTS=0x80;dma_global.DMCTL=1;
 DWT->CYCCNT+=30000;stop_irq();k1_ws281x_gpt_dma_hw_snapshot_v2(&v2);
 assert(v2.first_fault.valid==1&&v2.first_fault.frame_id==2&&v2.first_fault.terminal.attempt==2);
 assert(v2.first_fault.payload_bytes==3&&v2.first_fault.packed_grb[0]==24&&v2.first_fault.packed_grb[1]==177&&v2.first_fault.packed_grb[2]==0);
 assert(v2.first_fault.expected_dma_words==22&&v2.first_fault.terminal.dmac_count==2);
 assert(v2.first_fault.dma_source_start==(uint32_t)(uintptr_t)&duty[2]);
 assert(v2.first_fault.dmsts==0x80&&v2.first_fault.dmctl==1&&v2.first_fault.elapsed_cycles==30000);
 assert(v2.first_fault.start_requested==1&&v2.first_fault.dma_complete==0&&v2.first_fault.waveform_complete==0);
 if (argc==2&&!strcmp(argv[1],"--witness")) {
  assert(fwrite(&v2,sizeof(v2),1,stdout)==1);return 0;
 }
 k1_ws281x_first_fault_witness_t saved=v2.first_fault;
 assert(k1_ws281x_gpt_dma_hw_submit(wire,3,1)==K1_WS281X_SUBMIT_UNAVAILABLE);
 dma_regs.DMCRA=0;DWT->CYCCNT+=100;dmac_done(NULL);k1_ws281x_gpt_dma_hw_snapshot_v2(&v2);
 assert(!memcmp(&saved,&v2.first_fault,sizeof(saved)));
 /* submit's initial poll may discover a fault: preserve it before returning. */
 fresh();submit();DWT->CYCCNT+=timeout_cycles+1;
 assert(k1_ws281x_gpt_dma_hw_submit(wire,3,1)==K1_WS281X_SUBMIT_UNAVAILABLE);
 k1_ws281x_gpt_dma_hw_snapshot_v2(&v2);
 assert(v2.first_fault.valid&&v2.first_fault.frame_id==1&&v2.first_fault.terminal.fault==K1_WS281X_FAULT_TIMEOUT);
 fresh();submit();done_dma();timer_callback_args_t a={0};gpt0_overflow(&a);assert(first_fault==K1_WS281X_FAULT_MISSING_STOP&&completions==0);
 fresh();submit();DWT->CYCCNT+=timeout_cycles+1;k1_ws281x_gpt_dma_hw_poll();assert(first_fault==K1_WS281X_FAULT_TIMEOUT);assert(snapshots[4].gpt6_gtcr==1&&gpt_regs[6].GTCR==0);
 fresh();irq_contexts[63]=&dummy;assert(!k1_ws281x_gpt_dma_hw_init());assert(first_fault==K1_WS281X_FAULT_RESOURCE);
 fresh();submit();irq_contexts[74]=&dummy;done_dma();stop_irq();DWT->CYCCNT+=reset_cycles;k1_ws281x_gpt_dma_hw_poll();assert(k1_ws281x_gpt_dma_hw_submit(wire,3,1)==K1_WS281X_SUBMIT_UNAVAILABLE);
 fresh();assert(k1_ws281x_gpt_dma_hw_init());fail_start=1;assert(k1_ws281x_gpt_dma_hw_submit(wire,3,1)==K1_WS281X_SUBMIT_UNAVAILABLE);assert(first_fault==K1_WS281X_FAULT_RESOURCE);
 fresh();assert(k1_ws281x_gpt_dma_hw_init());fail_reset=1;assert(k1_ws281x_gpt_dma_hw_submit(wire,3,1)==K1_WS281X_SUBMIT_UNAVAILABLE);
 fresh();submit();k1_ws281x_gpt_dma_hw_abort_low();assert(completions==0&&!k1_ws281x_gpt_dma_hw_ready());dmac_done(NULL);assert(dma_irqs==0);
 puts("K1_GPT_PRODUCTION_DRIVER_HOST=PASS completion_guard=true v1_bytes=728 v2_bytes=1420 immutable_fault_frame=true");
}
