#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef int fsp_err_t; typedef int bsp_io_port_pin_t;
#define FSP_SUCCESS 0
#define BSP_IRQ_DISABLED 255
#define FSP_INVALID_VECTOR -1
#define GPT0_COUNTER_OVERFLOW_IRQn 63
#define DMAC2_INT_IRQn 74
#define VECTOR_NUMBER_DMAC2_INT 74
#define GPT_PIN_LEVEL_LOW 0
#define GPT_SOURCE_NONE 0
#define GPT_SOURCE_GPT_A (1u<<16)
#define GPT_SOURCE_GPT_B (1u<<17)
#define GPT_CAPTURE_FILTER_NONE 0
#define GPT_GTIOC_POLARITY_NORMAL 0
#define TIMER_MODE_PWM 1
#define TIMER_MODE_PERIODIC 0
#define TIMER_SOURCE_DIV_1 0
#define TIMER_EVENT_CYCLE_END 0
#define TRANSFER_ADDR_MODE_FIXED 0
#define TRANSFER_ADDR_MODE_INCREMENTED 1
#define TRANSFER_REPEAT_AREA_SOURCE 0
#define TRANSFER_IRQ_END 1
#define TRANSFER_CHAIN_MODE_DISABLED 0
#define TRANSFER_SIZE_4_BYTE 2
#define TRANSFER_MODE_NORMAL 0
#define ELC_EVENT_GPT6_COUNTER_OVERFLOW 0x1bd
#define ELC_EVENT_GPT6_CAPTURE_COMPARE_A 0x1b7
#define ELC_EVENT_GPT0_COUNTER_OVERFLOW 0x187
#define ELC_PERIPHERAL_GPT_A 0
#define ELC_PERIPHERAL_GPT_B 1
#define FSP_IP_ELC 1
#define IOPORT_CFG_PERIPHERAL_PIN (1u<<16)
#define IOPORT_PERIPHERAL_GPT1 (3u<<24)
#define IOPORT_CFG_DRIVE_HIGH (3u<<10)
#define IOPORT_CFG_PORT_DIRECTION_OUTPUT (1u<<2)
#define IOPORT_CFG_PORT_OUTPUT_LOW 0
#define R_PFS_PORT_PIN_PmnPFS_PMR_Msk (1u<<16)
#define R_GPT0_GTCR_CST_Msk 1u
#define SCB_CCR_DC_Msk (1u<<16)
typedef struct {uint32_t GTCR,GTCNT,GTPR,GTCCR[6],GTBER,GTIOR,GTPSR,GTUPSR;} gpt_regs_t;
typedef struct {uint32_t DMCRA,DMSAR,DMDAR,DMCNT,DMTMD,DMINT,DMREQ,DMSTS;} dma_regs_t;
static struct {uint32_t DMCTL;} dma_global;
#define R_DMA (&dma_global)
typedef struct {int event;} timer_callback_args_t;
typedef struct {void *p_context;} dmac_callback_args_t;
typedef struct {
 struct {bool output_enabled;int stop_level;} gtioca,gtiocb;
 uint32_t start_source,stop_source,clear_source,capture_a_source,capture_b_source,count_up_source,count_down_source;
 int capture_filter_gtioca,capture_filter_gtiocb,capture_a_ipl,capture_b_ipl,compare_match_c_ipl,compare_match_d_ipl,compare_match_e_ipl,compare_match_f_ipl;
 int capture_a_irq,capture_b_irq,compare_match_c_irq,compare_match_d_irq,compare_match_e_irq,compare_match_f_irq;
 uint32_t compare_match_value[6],compare_match_status;void *p_pwm_cfg;
 struct {uint32_t gtior;} gtior_setting;int gtioca_polarity,gtiocb_polarity;
} gpt_extended_cfg_t;
typedef struct {int mode;uint32_t period_counts,duty_cycle_counts,source_div,channel;void (*p_callback)(timer_callback_args_t*);void *p_context;gpt_extended_cfg_t *p_extend;int cycle_end_ipl,cycle_end_irq;} timer_cfg_t;
typedef struct {gpt_regs_t *p_reg;const timer_cfg_t *p_cfg;} gpt_instance_ctrl_t;
typedef struct {uint32_t clock_frequency;} timer_info_t;
typedef struct {struct {int dest_addr_mode,repeat_area,irq,chain_mode,src_addr_mode,size,mode;} transfer_settings_word_b;void *p_src,*p_dest;uint16_t length,num_blocks;} transfer_info_t;
typedef struct {int channel,irq,ipl,offset,src_buffer_size,activation_source;void (*p_callback)(dmac_callback_args_t*);void *p_callback_memory,*p_context;} dmac_extended_cfg_t;
typedef struct {transfer_info_t *p_info;const dmac_extended_cfg_t *p_extend;} transfer_cfg_t;
typedef struct {dma_regs_t *p_reg;} dmac_instance_ctrl_t;
static struct {uint32_t CYCCNT;} dwt;
static struct {uint32_t CCR;} scb;
static struct {struct {struct {struct {uint32_t PmnPFS;} PIN[16];} PORT[16];} unused;} dummy;
static struct {struct {struct {uint32_t PmnPFS;} PIN[16];} PORT[16];} mock_pfs;
static struct {uint32_t PCNTR3;} port6;
static struct {struct {struct {uint32_t ELS;} HA_b;} ELSR[32];struct {uint32_t ELCON;} ELCR_b;} elc;
static struct {uint32_t DELSR[8];} icu;
static struct {uint32_t MSTPCRC;} mstp;
#define DWT (&dwt)
#define SCB (&scb)
#define R_PFS (&mock_pfs)
#define R_PORT6 (&port6)
#define R_ELC (&elc)
#define R_ICU (&icu)
#define R_MSTP (&mstp)
static uint32_t SystemCoreClock=1000000000u,irq_mask;
static int g_ioport_ctrl,elc_started,fail_start,fail_reset;
static void *irq_contexts[128];
static gpt_regs_t gpt_regs[7]; static dma_regs_t dma_regs;
static void module_start(int ip,int channel) {(void)ip;(void)channel;elc_started=1;mstp.MSTPCRC=0;}
#define R_BSP_MODULE_START module_start
static void __DSB(void) {} static void __DMB(void) {} static void __ISB(void) {}
static uint32_t __get_PRIMASK(void) {return irq_mask;}
static void __disable_irq(void) {irq_mask=1;}
static void __set_PRIMASK(uint32_t x) {irq_mask=x;}
static void SCB_CleanDCache_by_Addr(void*p,int32_t n) {(void)p;(void)n;}
static void R_BSP_PinAccessEnable(void) {} static void R_BSP_PinAccessDisable(void) {}
static void *R_FSP_IsrContextGet(int n) {return irq_contexts[n];}
static uint32_t NVIC_GetPendingIRQ(int n) {(void)n;return 0;}
static void R_BSP_IrqClearPending(int n) {(void)n;}
static int R_IOPORT_PinCfg(void*c,bsp_io_port_pin_t p,uint32_t config) {(void)c;(void)p;mock_pfs.PORT[6].PIN[1].PmnPFS=config;return 0;}
static int R_GPT_Open(gpt_instance_ctrl_t*c,const timer_cfg_t*q) {c->p_reg=&gpt_regs[q->channel];c->p_cfg=q;c->p_reg->GTPR=q->period_counts-1;c->p_reg->GTUPSR=q->p_extend->count_up_source;if(q->cycle_end_irq>=0)irq_contexts[q->cycle_end_irq]=c;return 0;}
static int R_GPT_Close(gpt_instance_ctrl_t*c) {(void)c;return 0;}
static int R_GPT_InfoGet(gpt_instance_ctrl_t*c,timer_info_t*i) {(void)c;i->clock_frequency=300000000;return 0;}
static int R_GPT_Stop(gpt_instance_ctrl_t*c) {c->p_reg->GTCR=0;return 0;}
static int R_GPT_Reset(gpt_instance_ctrl_t*c) {c->p_reg->GTCNT=0;c->p_reg->GTCCR[0]=c->p_reg->GTCCR[2];return fail_reset;}
static int R_GPT_PeriodSet(gpt_instance_ctrl_t*c,uint32_t p) {c->p_reg->GTPR=p-1;return 0;}
static int R_GPT_Enable(gpt_instance_ctrl_t*c) {c->p_reg->GTPSR=c->p_cfg->p_extend->stop_source;return 0;}
static int R_GPT_Start(gpt_instance_ctrl_t*c) {if(fail_start)return 9;c->p_reg->GTCR=1;return 0;}
static int R_DMAC_Open(dmac_instance_ctrl_t*c,const transfer_cfg_t*q) {c->p_reg=&dma_regs;irq_contexts[q->p_extend->irq]=c;icu.DELSR[q->p_extend->channel]=q->p_extend->activation_source;return 0;}
static int R_DMAC_Close(dmac_instance_ctrl_t*c) {(void)c;return 0;}
static int R_DMAC_Disable(dmac_instance_ctrl_t*c) {c->p_reg->DMCNT=0;return 0;}
static int R_DMAC_Enable(dmac_instance_ctrl_t*c) {c->p_reg->DMCNT=1;return 0;}
static int R_DMAC_Reset(dmac_instance_ctrl_t*c,const void*s,void*d,uint16_t n) {c->p_reg->DMCRA=n;c->p_reg->DMSAR=(uint32_t)(uintptr_t)s;c->p_reg->DMDAR=(uint32_t)(uintptr_t)d;return R_DMAC_Enable(c);}
