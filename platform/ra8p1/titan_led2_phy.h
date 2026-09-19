#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Titan Mini HW V1.0 LED2 (U10 RTL8211F LED1 green / LED2 yellow).
   Schematic: MDIO PC12, MDC PC11, PHYRSTB PA07. PHY address strap 001.
   The pinned BSP's ETHERNET_* aliases name unrelated P414/P415/P708 pins;
   use the schematic-backed port values directly.
   GPIO Clause-22 MDIO; not FSP R_RMAC_PHY_Read/Write (unbounded wait). */

#define K1_LED2_PHYID_RTL8211F 0x001CC916u
#define K1_LED2_LCR_OWNED 0x6F60u
#define K1_LED2_EEELCR_OWNED 0x000Cu
#define K1_LED2_BMCR_LOOPBACK_100FD 0x6100u
#define K1_LED2_BMCR_POWERDOWN_100FD 0x6900u
#define K1_LED2_BMCR_OWNED 0xFF40u
#define K1_LED2_DWT_CLOCK_HZ 1000000000u
#define K1_LED2_PHY_TRACE_CAPACITY 96u
/* One Clause-22 frame at 4 us/MDC clock * 65 clocks plus GPIO software.
   400000 cycles is 400 us at the 1 GHz DWT used by this board. */
#define K1_LED2_FRAME_BUDGET_CYCLES 400000u
#define K1_LED2_TXN_IDLE 0u
#define K1_LED2_TXN_SAVE_PAGE 1u
#define K1_LED2_TXN_SELECT 2u
#define K1_LED2_TXN_WRITE_LCR 3u
#define K1_LED2_TXN_RESTORE 4u
#define K1_LED2_TXN_SAVE_PAGE2 5u
#define K1_LED2_TXN_SELECT2 6u
#define K1_LED2_TXN_READ_LCR 7u
#define K1_LED2_TXN_RESTORE2 8u
#define K1_LED2_TXN_CLEANUP 9u

enum {
  k1_led2_err_none = 0,
  k1_led2_err_identity = 1,
  k1_led2_err_mdio = 2,
  k1_led2_err_restore = 3,
  k1_led2_err_mode = 4,
  k1_led2_err_readback = 5,
  k1_led2_err_clock = 6
};

typedef struct {
  uint32_t sequence;
  uint32_t attempt;
  uint32_t reset_age_ms;
  uint16_t value;
  uint8_t address;
  uint8_t reg;
  uint8_t ack;
  uint8_t io_error;
  uint8_t value_valid;
} k1_led2_phy_trace_t;

enum {
  k1_led2_step_idle = 0,
  k1_led2_step_off1,
  k1_led2_step_green,
  k1_led2_step_off2,
  k1_led2_step_yellow,
  k1_led2_step_off3,
  k1_led2_step_both,
  k1_led2_step_off4,
  k1_led2_step_done
};

static inline uint16_t titan_led2_lcr_value(uint16_t saved_lcr, int green, int yellow) {
  const uint16_t g = green ? (uint16_t)0x0040u : (uint16_t)0x0100u;
  const uint16_t y = yellow ? (uint16_t)0x0800u : (uint16_t)0x2000u;
  return (uint16_t)((saved_lcr & (uint16_t)~K1_LED2_LCR_OWNED) | g | y);
}

int titan_led2_phy_init(uint32_t now_ms);
void titan_led2_phy_request(int green, int yellow);
int titan_led2_phy_poll(uint32_t now_ms, int allow_mdio);
int titan_led2_phy_service(uint32_t now_ms, uint32_t remaining_cycles);
int titan_led2_phy_start_experiment(uint32_t now_ms);
uint32_t titan_led2_phy_desired_version(void);
uint32_t titan_led2_phy_applied_version(void);
uint32_t titan_led2_phy_deferred(void);
uint32_t titan_led2_phy_txn_step(void);
int titan_led2_phy_capable(void);
int titan_led2_phy_mode_ok(void);
uint32_t titan_led2_phy_id(void);
uint32_t titan_led2_phy_error(void);
int titan_led2_phy_applied_green(void);
int titan_led2_phy_applied_yellow(void);
int titan_led2_phy_applied_valid(void);
int titan_led2_phy_txc_enabled(void);
int titan_led2_phy_txc_requested(void);
uint32_t titan_led2_phy_write_count(void);
int titan_led2_phy_experiment_step(void);
int titan_led2_phy_page_unknown(void);
uint32_t titan_led2_phy_last_cost_cycles(void);
uint16_t titan_led2_phy_last_id1(void);
uint16_t titan_led2_phy_last_id2(void);
uint8_t titan_led2_phy_last_addr(void);
uint8_t titan_led2_phy_expected_ta_z(void);
uint8_t titan_led2_phy_expected_ta_zero(void);
int titan_led2_phy_expected_io_error(void);
uint8_t titan_led2_phy_pin_readback(void);
uint32_t titan_led2_phy_ta_zero_bitmap(void);
int titan_led2_phy_register_readback_ok(void);
int titan_led2_phy_dwt_ok(void);
uint32_t titan_led2_phy_dwt_delta(void);
uint32_t titan_led2_phy_dwt_clock_hz(void);
uint32_t titan_led2_phy_mdio_pfs(void);
uint32_t titan_led2_phy_mdc_pfs(void);
uint32_t titan_led2_phy_reset_pfs(void);
uint32_t titan_led2_phy_txc_pfs(void);
uint32_t titan_led2_phy_attempt(void);
uint32_t titan_led2_phy_trace_snapshot(k1_led2_phy_trace_t *output, uint32_t capacity);
int titan_led2_phy_trace_latest(k1_led2_phy_trace_t *output);

#ifdef K1_LED2_PHY_STUB
void titan_led2_phy_stub_fail_restore(int enable);
uint16_t titan_led2_phy_stub_lcr(void);
uint16_t titan_led2_phy_stub_page(void);
uint16_t titan_led2_phy_stub_bmcr(void);
void titan_led2_phy_stub_reset(void);
void titan_led2_phy_stub_fail_read_after(uint8_t reg, uint32_t successful_matches);
void titan_led2_phy_stub_fail_write_after(uint8_t reg, uint32_t successful_matches);
void titan_led2_phy_stub_set_lcr_readback_xor(uint16_t mask);
void titan_led2_phy_stub_set_bmcr_readback_xor(uint16_t mask);
void titan_led2_phy_stub_set_bmcr(uint16_t value);
void titan_led2_phy_stub_set_phy_address(uint8_t address);
#endif

#ifdef __cplusplus
}
#endif
