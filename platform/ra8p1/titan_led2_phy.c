#include "titan_led2_phy.h"

#include <string.h>

#ifndef K1_LED2_PHY_STUB
#include <board.h>
#include <rtthread.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_ioport.h"
#endif

#define MDIO_PHYADDR 1u
#define REG_BMCR 0x00u
#define REG_BMSR 0x01u
#define REG_PHYID1 0x02u
#define REG_PHYID2 0x03u
#define REG_PAGE 0x1Fu
#define PAGE_LCR 0x0D04u
#define REG_LCR 0x10u
#define REG_EEELCR 0x11u
#define PAGE_PHYSR 0x0A43u
#define REG_PHYSR 0x1Au
#define HOLD_MS 500u
#define OBSERVE_MS 500u
#define PIN_READBACK_EXPECTED 0xFFu
#define TXC_PIN BSP_IO_PORT_03_PIN_09

static int capable;
static int mode_ok;
static int configured;
static int register_readback_ok;
static int page_unknown;
static int txc_enabled;
static int txc_requested;
static int dwt_ok;
static int expected_io_error;
static int desired_g, desired_y, applied_g, applied_y, applied_valid;
static int experiment_step;
static int txn_step;
static int txn_g, txn_y, txn_op_error;
static uint16_t txn_saved_page, txn_lcr, txn_readback;
static uint32_t desired_version, applied_version, txn_version;
static uint32_t deferred_count;
static uint32_t phy_id, last_error, write_count, last_cost, step_ms, observe_deadline, retry_ms;
static uint32_t ta_zero_bitmap;
static uint32_t dwt_delta, dwt_clock_hz, mdio_pfs, mdc_pfs, reset_pfs, txc_pfs;
static uint32_t reset_release_ms, probe_attempt, trace_sequence, trace_count, trace_next;
static uint16_t saved_page, saved_bmcr, saved_lcr, saved_eeelcr, last_id1, last_id2;
static uint8_t phy_addr = MDIO_PHYADDR, last_addr, expected_ta_z, expected_ta_zero, pin_readback;
static k1_led2_phy_trace_t trace_ring[K1_LED2_PHY_TRACE_CAPACITY];

#ifdef K1_LED2_PHY_STUB
static uint32_t stub_now_ms;
#endif

static uint32_t diagnostic_now_ms(void) {
#ifdef K1_LED2_PHY_STUB
  return stub_now_ms;
#else
  return (uint32_t)rt_tick_get();
#endif
}

static void trace_record(uint8_t addr, uint8_t reg, uint8_t ack, int io_error,
                         uint16_t value, int value_valid) {
  k1_led2_phy_trace_t *entry = &trace_ring[trace_next];
  entry->sequence = ++trace_sequence;
  entry->attempt = probe_attempt;
  entry->reset_age_ms = diagnostic_now_ms() - reset_release_ms;
  entry->value = value;
  entry->address = addr;
  entry->reg = reg;
  entry->ack = ack;
  entry->io_error = io_error ? 1u : 0u;
  entry->value_valid = value_valid ? 1u : 0u;
  trace_next = (trace_next + 1u) % K1_LED2_PHY_TRACE_CAPACITY;
  if (trace_count < K1_LED2_PHY_TRACE_CAPACITY) trace_count++;
}

#ifdef K1_LED2_PHY_STUB
static uint16_t stub_page;
static uint16_t stub_lcr = 0xC000u;
static uint16_t stub_eeelcr;
static uint16_t stub_bmcr = 0x0080u;
static int stub_fail_restore;
static uint16_t stub_physr = (1u << 2) | (1u << 4); /* link + 100 Mbps */
static uint16_t stub_bmsr = 0x0004u;
static uint16_t stub_lcr_readback_xor;
static uint16_t stub_bmcr_readback_xor;
static int stub_fail_read_reg = -1, stub_fail_write_reg = -1;
static uint32_t stub_fail_read_skip, stub_fail_write_skip;
static uint8_t stub_phy_addr = MDIO_PHYADDR;

void titan_led2_phy_stub_fail_restore(int enable) { stub_fail_restore = enable; }
uint16_t titan_led2_phy_stub_lcr(void) { return stub_lcr; }
uint16_t titan_led2_phy_stub_page(void) { return stub_page; }
uint16_t titan_led2_phy_stub_bmcr(void) { return stub_bmcr; }
void titan_led2_phy_stub_reset(void) {
  stub_page = 0;
  stub_lcr = 0xC000u;
  stub_eeelcr = 0;
  stub_bmcr = 0x0080u;
  stub_physr = (1u << 2) | (1u << 4);
  stub_bmsr = 0x0004u;
  stub_lcr_readback_xor = 0;
  stub_bmcr_readback_xor = 0;
  stub_fail_restore = 0;
  stub_fail_read_reg = -1;
  stub_fail_write_reg = -1;
  stub_fail_read_skip = 0;
  stub_fail_write_skip = 0;
  stub_phy_addr = MDIO_PHYADDR;
}
void titan_led2_phy_stub_fail_read_after(uint8_t reg, uint32_t successful_matches) {
  stub_fail_read_reg = reg;
  stub_fail_read_skip = successful_matches;
}
void titan_led2_phy_stub_fail_write_after(uint8_t reg, uint32_t successful_matches) {
  stub_fail_write_reg = reg;
  stub_fail_write_skip = successful_matches;
}
void titan_led2_phy_stub_set_lcr_readback_xor(uint16_t mask) { stub_lcr_readback_xor = mask; }
void titan_led2_phy_stub_set_bmcr_readback_xor(uint16_t mask) { stub_bmcr_readback_xor = mask; }
void titan_led2_phy_stub_set_bmcr(uint16_t value) { stub_bmcr = value; }
void titan_led2_phy_stub_set_phy_address(uint8_t address) { stub_phy_addr = address; }
#endif

#ifndef K1_LED2_PHY_STUB
#define MDIO_PIN BSP_IO_PORT_12_PIN_12
#define MDC_PIN BSP_IO_PORT_12_PIN_11
#define RST_PIN BSP_IO_PORT_10_PIN_07

static int mdio_io_error;

#define PFS_GPIO_OWNERSHIP_MASK (R_PFS_PORT_PIN_PmnPFS_PDR_Msk | \
                                 R_PFS_PORT_PIN_PmnPFS_PMR_Msk | \
                                 R_PFS_PORT_PIN_PmnPFS_ASEL_Msk | \
                                 R_PFS_PORT_PIN_PmnPFS_PSEL_Msk)

static uint32_t pfs_read(unsigned port, unsigned pin) {
  return R_PFS->PORT[port].PIN[pin].PmnPFS;
}

static int pfs_is_gpio(uint32_t value, int output) {
  const uint32_t expected = output ? R_PFS_PORT_PIN_PmnPFS_PDR_Msk : 0u;
  return (value & PFS_GPIO_OWNERSHIP_MASK) == expected;
}

static void mdio_note_io(fsp_err_t result) {
  if (result != FSP_SUCCESS) mdio_io_error = 1;
}

static void mdio_delay(void) {
  uint32_t start = DWT->CYCCNT;
  while ((uint32_t)(DWT->CYCCNT - start) < 2000u) {
  }
}

static void mdc_write(bsp_io_level_t level) {
  mdio_note_io(R_IOPORT_PinWrite(&g_ioport_ctrl, MDC_PIN, level));
}

static void mdc_verify_output(void) {
  mdc_pfs = pfs_read(12u, 11u);
  if (!pfs_is_gpio(mdc_pfs, 1)) mdio_io_error = 1;
}

static void mdio_output(void) {
  mdio_note_io(R_IOPORT_PinCfg(&g_ioport_ctrl, MDIO_PIN,
                               (uint32_t)IOPORT_CFG_NMOS_ENABLE |
                                   (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                                   (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH |
                                   (uint32_t)IOPORT_CFG_PULLUP_ENABLE));
  mdio_pfs = pfs_read(12u, 12u);
  if (!pfs_is_gpio(mdio_pfs, 1)) mdio_io_error = 1;
}

static void mdio_input(void) {
  mdio_note_io(R_IOPORT_PinCfg(&g_ioport_ctrl, MDIO_PIN,
                               (uint32_t)IOPORT_CFG_PORT_DIRECTION_INPUT |
                                   (uint32_t)IOPORT_CFG_PULLUP_ENABLE));
  mdio_pfs = pfs_read(12u, 12u);
  if (!pfs_is_gpio(mdio_pfs, 0)) mdio_io_error = 1;
}

static void mdio_write_level(int one) {
  mdio_note_io(
      R_IOPORT_PinWrite(&g_ioport_ctrl, MDIO_PIN, one ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW));
}

static int mdio_read_bit(void) {
  bsp_io_level_t level = BSP_IO_LEVEL_HIGH;
  mdio_note_io(R_IOPORT_PinRead(&g_ioport_ctrl, MDIO_PIN, &level));
  return level == BSP_IO_LEVEL_HIGH;
}

static void note_pin_level(bsp_io_port_pin_t pin, bsp_io_level_t expected, uint8_t mask) {
  /* Allow pad/pull-up settling before the diagnostic PIDR sample.
     Ten microseconds nominal at 1 GHz; this is not continuity proof. */
  for (unsigned settle = 0; settle < 5u; ++settle) mdio_delay();
  bsp_io_level_t level = expected == BSP_IO_LEVEL_HIGH ? BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH;
  mdio_note_io(R_IOPORT_PinRead(&g_ioport_ctrl, pin, &level));
  if (level == expected) pin_readback = (uint8_t)(pin_readback | mask);
}

static void mdio_bit_out(int one) {
  mdio_write_level(one);
  mdio_delay();
  mdc_write(BSP_IO_LEVEL_HIGH);
  mdio_delay();
  mdc_write(BSP_IO_LEVEL_LOW);
}

static int mdio_bit_in(void) {
  mdio_delay();
  mdc_write(BSP_IO_LEVEL_HIGH);
  mdio_delay();
  const int bit = mdio_read_bit();
  mdc_write(BSP_IO_LEVEL_LOW);
  return bit;
}

static void mdio_bits_out(uint32_t value, unsigned count) {
  while (count--) mdio_bit_out((int)((value >> count) & 1u));
}

static uint16_t mdio_bits_in(unsigned count) {
  uint16_t value = 0;
  while (count--) {
    value = (uint16_t)((value << 1) | (uint16_t)mdio_bit_in());
  }
  return value;
}

static int mdio_write(uint8_t addr, uint8_t reg, uint16_t data) {
  unsigned i;
  mdio_io_error = 0;
  R_BSP_PinAccessEnable();
  mdc_verify_output();
  mdc_write(BSP_IO_LEVEL_LOW);
  mdio_output();
  for (i = 0; i < 32u; ++i) mdio_bit_out(1);
  mdio_bits_out(0x1u, 2); /* 01 start */
  mdio_bits_out(0x1u, 2); /* 01 write */
  mdio_bits_out(addr, 5);
  mdio_bits_out(reg, 5);
  mdio_bits_out(0x2u, 2); /* TA 10 */
  mdio_bits_out(data, 16);
  mdio_input();
  if (mdio_io_error) {
    /* One bounded release retry; retain the original failure. */
    mdio_input();
  } else {
    (void)mdio_bit_in();
  }
  mdc_write(BSP_IO_LEVEL_LOW);
  mdc_verify_output();
  R_BSP_PinAccessDisable();
  write_count++;
  return mdio_io_error ? -1 : 0;
}

static int mdio_read(uint8_t addr, uint8_t reg, uint16_t *data) {
  unsigned i;
  mdio_io_error = 0;
  R_BSP_PinAccessEnable();
  mdc_verify_output();
  mdc_write(BSP_IO_LEVEL_LOW);
  mdio_output();
  for (i = 0; i < 32u; ++i) mdio_bit_out(1);
  mdio_bits_out(0x1u, 2); /* 01 start */
  mdio_bits_out(0x2u, 2); /* 10 read */
  mdio_bits_out(addr, 5);
  mdio_bits_out(reg, 5);
  mdio_input();
  if (mdio_io_error) {
    /* Do not clock/sample a read when releasing MDIO failed. One bounded
       release retry may recover the pad, but the transaction still fails. */
    mdio_input();
    mdc_write(BSP_IO_LEVEL_LOW);
    if (addr == MDIO_PHYADDR) {
      expected_ta_z = 2;
      expected_ta_zero = 2;
      expected_io_error = mdio_io_error;
    }
    mdc_verify_output();
    R_BSP_PinAccessDisable();
    trace_record(addr, reg, 2u, mdio_io_error, 0u, 0);
    return -1;
  }
  const int ta_z = mdio_read_bit();
  const int ta_zero = mdio_bit_in();
  if (!mdio_io_error && !ta_zero) ta_zero_bitmap |= (uint32_t)1u << addr;
  if (addr == MDIO_PHYADDR) {
    expected_ta_z = (uint8_t)ta_z;
    expected_ta_zero = (uint8_t)ta_zero;
    expected_io_error = mdio_io_error;
  }
  /* Samples occur late after rising MDC. The unclocked pre-ACK level is
     diagnostic only; the first clocked late sample is the PHY ACK, followed
     by 16 data samples. A second clocked TA sample would consume D15. */
  if (mdio_io_error || ta_zero) {
    for (i = 0; i < 32u; ++i) (void)mdio_bit_in();
    if (addr == MDIO_PHYADDR) expected_io_error = mdio_io_error;
    mdc_write(BSP_IO_LEVEL_LOW);
    R_BSP_PinAccessDisable();
    trace_record(addr, reg, (uint8_t)(ta_zero ? 1u : 0u), mdio_io_error, 0u, 0);
    return -1;
  }
  const uint16_t value = mdio_bits_in(16);
  (void)mdio_bit_in();
  if (addr == MDIO_PHYADDR) expected_io_error = mdio_io_error;
  mdc_write(BSP_IO_LEVEL_LOW);
  mdc_verify_output();
  R_BSP_PinAccessDisable();
  if (mdio_io_error) {
    trace_record(addr, reg, 0u, mdio_io_error, 0u, 0);
    return -1;
  }
  *data = value;
  trace_record(addr, reg, 0u, 0, value, 1);
  return 0;
}
#else
static int stub_fault(int *reg_filter, uint32_t *skip, uint8_t reg) {
  if (*reg_filter != (int)reg) return 0;
  if (*skip) {
    (*skip)--;
    return 0;
  }
  *reg_filter = -1;
  return 1;
}

static int mdio_write(uint8_t addr, uint8_t reg, uint16_t data) {
  write_count++;
  if (addr != stub_phy_addr) return 0;
  if (stub_fault(&stub_fail_write_reg, &stub_fail_write_skip, reg)) return -1;
  if (reg == REG_PAGE) {
    if (stub_fail_restore && data == 0) return -1;
    stub_page = data;
    return 0;
  }
  if (stub_page == PAGE_LCR && reg == REG_LCR) {
    stub_lcr = data;
    return 0;
  }
  if (stub_page == PAGE_LCR && reg == REG_EEELCR) {
    stub_eeelcr = data;
    return 0;
  }
  if (stub_page == 0 && reg == REG_BMCR) stub_bmcr = data;
  return 0;
}

static int mdio_read(uint8_t addr, uint8_t reg, uint16_t *data) {
  uint16_t value = 0;
  if (addr != stub_phy_addr) {
    trace_record(addr, reg, 1u, 0, 0u, 0);
    return -1;
  }
  if (stub_fault(&stub_fail_read_reg, &stub_fail_read_skip, reg)) {
    trace_record(addr, reg, 0u, 1, 0u, 0);
    return -1;
  }
  if (reg == REG_PHYID1) {
    value = (uint16_t)(K1_LED2_PHYID_RTL8211F >> 16);
  } else if (reg == REG_PHYID2) {
    value = (uint16_t)K1_LED2_PHYID_RTL8211F;
  } else if (reg == REG_PAGE) {
    value = stub_page;
  } else if (stub_page == 0 && reg == REG_BMCR) {
    value = (uint16_t)(stub_bmcr ^ stub_bmcr_readback_xor);
  } else if (stub_page == 0 && reg == REG_BMSR) {
    value = stub_bmsr;
  } else if (stub_page == PAGE_LCR && reg == REG_LCR) {
    value = (uint16_t)(stub_lcr ^ stub_lcr_readback_xor);
  } else if (stub_page == PAGE_LCR && reg == REG_EEELCR) {
    value = stub_eeelcr;
  } else if (stub_page == PAGE_PHYSR && reg == REG_PHYSR) {
    value = stub_physr;
  }
  *data = value;
  trace_record(addr, reg, 0u, 0, value, 1);
  return 0;
}
#endif

static int paged_op(uint16_t page, uint8_t reg, uint16_t *value, int write) {
  uint16_t current = 0;
  int operation_error = 0;
  if (page_unknown) {
    last_error = k1_led2_err_restore;
    return -1;
  }
  if (mdio_read(phy_addr, REG_PAGE, &current)) {
    page_unknown = 1;
    last_error = k1_led2_err_mdio;
    return -1;
  }
  if (mdio_write(phy_addr, REG_PAGE, page)) {
    operation_error = 1;
  } else if (write) {
    operation_error = mdio_write(phy_addr, reg, *value) != 0;
  } else {
    operation_error = mdio_read(phy_addr, reg, value) != 0;
  }
  const int restore_error = mdio_write(phy_addr, REG_PAGE, current) != 0;
  if (restore_error) page_unknown = 1;
  else page_unknown = 0;
  if (operation_error) {
    /* The operation is the first failure. Page uncertainty is reported
       separately if cleanup also fails. */
    last_error = k1_led2_err_mdio;
    return -1;
  }
  if (restore_error) {
    last_error = k1_led2_err_restore;
    return -1;
  }
  return 0;
}

static int standard_op(uint8_t reg, uint16_t *value, int write) {
  return paged_op(0u, reg, value, write);
}

static int apply_channels(int green, int yellow) __attribute__((unused));
static int apply_channels(int green, int yellow) {
  uint16_t lcr = titan_led2_lcr_value(saved_lcr, green, yellow);
  uint16_t readback = 0;
  applied_valid = 0;
  register_readback_ok = 0;
  if (paged_op(PAGE_LCR, REG_LCR, &lcr, 1)) return -1;
  if (paged_op(PAGE_LCR, REG_LCR, &readback, 0)) return -1;
  if ((readback & K1_LED2_LCR_OWNED) != (lcr & K1_LED2_LCR_OWNED)) {
    register_readback_ok = 0;
    last_error = k1_led2_err_readback;
    return -1;
  }
  applied_g = green;
  applied_y = yellow;
  applied_valid = 1;
  applied_version = txn_version ? txn_version : desired_version;
  register_readback_ok = 1;
  last_error = k1_led2_err_none;
  return 0;
}

static void led2_txn_begin(void) {
  txn_g = desired_g;
  txn_y = desired_y;
  txn_version = desired_version;
  txn_lcr = titan_led2_lcr_value(saved_lcr, txn_g, txn_y);
  txn_op_error = 0;
  txn_readback = 0;
  applied_valid = 0;
  register_readback_ok = 0;
  txn_step = K1_LED2_TXN_SAVE_PAGE;
}

static int led2_txn_fail(uint32_t error) {
  last_error = error;
  capable = 0;
  configured = 0;
  mode_ok = 0;
  register_readback_ok = 0;
  applied_valid = 0;
  txn_step = K1_LED2_TXN_IDLE;
  return -1;
}

static int led2_txn_frame(void) {
  if (txn_step == K1_LED2_TXN_IDLE) return 0;
  if (page_unknown && txn_step != K1_LED2_TXN_CLEANUP) {
    last_error = k1_led2_err_restore;
    txn_step = K1_LED2_TXN_IDLE;
    return -1;
  }
  switch (txn_step) {
  case K1_LED2_TXN_SAVE_PAGE:
    if (mdio_read(phy_addr, REG_PAGE, &txn_saved_page))
      return led2_txn_fail(k1_led2_err_mdio);
    txn_step = K1_LED2_TXN_SELECT;
    return 0;
  case K1_LED2_TXN_SELECT:
    if (mdio_write(phy_addr, REG_PAGE, PAGE_LCR)) {
      txn_op_error = 1;
      txn_step = K1_LED2_TXN_CLEANUP;
      return 0;
    }
    txn_step = K1_LED2_TXN_WRITE_LCR;
    return 0;
  case K1_LED2_TXN_WRITE_LCR:
    if (mdio_write(phy_addr, REG_LCR, txn_lcr)) txn_op_error = 1;
    txn_step = K1_LED2_TXN_RESTORE;
    return 0;
  case K1_LED2_TXN_RESTORE:
    if (mdio_write(phy_addr, REG_PAGE, txn_saved_page)) {
      page_unknown = 1;
      return led2_txn_fail(txn_op_error ? k1_led2_err_mdio : k1_led2_err_restore);
    }
    page_unknown = 0;
    if (txn_op_error) return led2_txn_fail(k1_led2_err_mdio);
    if (desired_version != txn_version) {
      led2_txn_begin();
      return 0;
    }
    txn_step = K1_LED2_TXN_SAVE_PAGE2;
    return 0;
  case K1_LED2_TXN_SAVE_PAGE2:
    if (mdio_read(phy_addr, REG_PAGE, &txn_saved_page))
      return led2_txn_fail(k1_led2_err_mdio);
    txn_step = K1_LED2_TXN_SELECT2;
    return 0;
  case K1_LED2_TXN_SELECT2:
    if (mdio_write(phy_addr, REG_PAGE, PAGE_LCR)) {
      txn_op_error = 1;
      txn_step = K1_LED2_TXN_CLEANUP;
      return 0;
    }
    txn_step = K1_LED2_TXN_READ_LCR;
    return 0;
  case K1_LED2_TXN_READ_LCR:
    if (mdio_read(phy_addr, REG_LCR, &txn_readback)) txn_op_error = 1;
    txn_step = K1_LED2_TXN_RESTORE2;
    return 0;
  case K1_LED2_TXN_RESTORE2:
    if (mdio_write(phy_addr, REG_PAGE, txn_saved_page)) {
      page_unknown = 1;
      return led2_txn_fail(txn_op_error ? k1_led2_err_mdio : k1_led2_err_restore);
    }
    page_unknown = 0;
    if (txn_op_error) return led2_txn_fail(k1_led2_err_mdio);
    if ((txn_readback & K1_LED2_LCR_OWNED) != (txn_lcr & K1_LED2_LCR_OWNED))
      return led2_txn_fail(k1_led2_err_readback);
    applied_g = txn_g;
    applied_y = txn_y;
    applied_valid = 1;
    applied_version = txn_version;
    register_readback_ok = 1;
    last_error = k1_led2_err_none;
    txn_step = K1_LED2_TXN_IDLE;
    if (desired_version != txn_version) led2_txn_begin();
    return 0;
  case K1_LED2_TXN_CLEANUP:
    if (mdio_write(phy_addr, REG_PAGE, txn_saved_page)) {
      page_unknown = 1;
      return led2_txn_fail(txn_op_error ? k1_led2_err_mdio : k1_led2_err_restore);
    }
    page_unknown = 0;
    return led2_txn_fail(txn_op_error ? k1_led2_err_mdio : k1_led2_err_restore);
  default:
    txn_step = K1_LED2_TXN_IDLE;
    return 0;
  }
}

static int led2_apply_needed(void) {
  if (page_unknown) return 1;
  if (txn_step != K1_LED2_TXN_IDLE) return 1;
  if (!applied_valid) return 1;
  if (desired_g != applied_g || desired_y != applied_y) return 1;
  if (desired_version != applied_version) return 1;
  return 0;
}

static int identify_phy(void) {
  uint8_t slot;
  int found_other = 0;
  probe_attempt++;
#ifndef K1_LED2_PHY_STUB
  {
    bsp_io_level_t reset_level = BSP_IO_LEVEL_LOW;
    bsp_io_level_t mdc_level = BSP_IO_LEVEL_HIGH;
    R_BSP_PinAccessEnable();
    reset_pfs = pfs_read(10u, 7u);
    mdc_pfs = pfs_read(12u, 11u);
    const fsp_err_t reset_result = R_IOPORT_PinRead(&g_ioport_ctrl, RST_PIN, &reset_level);
    const fsp_err_t mdc_result = R_IOPORT_PinRead(&g_ioport_ctrl, MDC_PIN, &mdc_level);
    R_BSP_PinAccessDisable();
    if (reset_result != FSP_SUCCESS || mdc_result != FSP_SUCCESS ||
        !pfs_is_gpio(reset_pfs, 1) || !pfs_is_gpio(mdc_pfs, 1) ||
        reset_level != BSP_IO_LEVEL_HIGH || mdc_level != BSP_IO_LEVEL_LOW) {
      last_error = k1_led2_err_mdio;
      return -1;
    }
  }
#endif
  ta_zero_bitmap = 0;
  last_id1 = 0;
  last_id2 = 0;
  last_addr = MDIO_PHYADDR;
  phy_id = 0;
  for (slot = 0; slot < 32u; ++slot) {
    const uint8_t addr = slot == 0u ? MDIO_PHYADDR : (uint8_t)(slot - (slot <= MDIO_PHYADDR));
    uint16_t id1 = 0, id2 = 0, again1 = 0, again2 = 0;
    uint16_t page_readback = 0xffffu;
    int page_zero_ok = addr != MDIO_PHYADDR;
    /* Clause-22 writes do not ACK. Selecting page zero before identity makes
       standard-register interpretation explicit; the repeated ID reads are
       the evidence that a device actually responded. */
    if (mdio_write(addr, REG_PAGE, 0u)) {
      last_error = k1_led2_err_mdio;
      return -1;
    }
    if (addr == MDIO_PHYADDR) {
      page_zero_ok = !mdio_read(addr, REG_PAGE, &page_readback) && page_readback == 0u;
      if (page_zero_ok) page_unknown = 0;
    }
    const int first1 = mdio_read(addr, REG_PHYID1, &id1);
    const int first2 = mdio_read(addr, REG_PHYID2, &id2);
    if (addr == MDIO_PHYADDR) {
      last_id1 = id1;
      last_id2 = id2;
    }
    if (first1 || first2) continue;
    const int second1 = mdio_read(addr, REG_PHYID1, &again1);
    const int second2 = mdio_read(addr, REG_PHYID2, &again2);
    if (second1 || second2) continue;
    if (id1 == 0 || id1 == 0xffffu || id2 == 0 || id2 == 0xffffu) continue;
    if (id1 != again1 || id2 != again2) continue;
    const uint32_t candidate = ((uint32_t)id1 << 16) | id2;
    if (addr == MDIO_PHYADDR) {
      last_id1 = id1;
      last_id2 = id2;
      last_addr = addr;
      phy_id = candidate;
      phy_addr = MDIO_PHYADDR;
      if (page_zero_ok &&
          (candidate & 0xfffffff0u) == (K1_LED2_PHYID_RTL8211F & 0xfffffff0u)) {
        last_error = k1_led2_err_none;
        return 0;
      }
    } else if (!found_other) {
      /* Preserve the first coherent off-contract responder for diagnosis, but
         never silently move the board contract away from address 1. */
      found_other = 1;
      last_id1 = id1;
      last_id2 = id2;
      last_addr = addr;
      phy_id = candidate;
    }
  }
  last_error = k1_led2_err_identity;
  return -1;
}

static int prepare_txc_route(void) {
  txc_enabled = 0;
#ifndef K1_LED2_PHY_STUB
  R_BSP_PinAccessEnable();
  const fsp_err_t result = R_IOPORT_PinCfg(
      &g_ioport_ctrl, TXC_PIN,
      (uint32_t)IOPORT_CFG_DRIVE_MID | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN |
          (uint32_t)IOPORT_PERIPHERAL_ETHER_RGMII);
  R_BSP_PinAccessDisable();
  txc_pfs = pfs_read(3u, 9u);
  const uint32_t expected = R_PFS_PORT_PIN_PmnPFS_PMR_Msk |
                            (uint32_t)IOPORT_PERIPHERAL_ETHER_RGMII;
  const uint32_t mask = R_PFS_PORT_PIN_PmnPFS_PMR_Msk |
                        R_PFS_PORT_PIN_PmnPFS_ASEL_Msk |
                        R_PFS_PORT_PIN_PmnPFS_PSEL_Msk;
  if (result != FSP_SUCCESS || (txc_pfs & mask) != expected) {
    last_error = k1_led2_err_clock;
    return -1;
  }
  R_BSP_MODULE_START(FSP_IP_ETHER, 0);
#else
  txc_pfs = 0x18010000u;
#endif
  /* This proves only that the board-backed P309 RGMII1_TXC route was requested
     and the Ethernet module stop was released. The clock remains unverified
     until mode/status evidence or a clock measurement says otherwise. */
  txc_requested = 1;
  return 0;
}

static int configure_loopback(void) {
  uint16_t eeelcr = 0;
  uint16_t lcr = 0;
  uint16_t readback = 0;
  uint16_t bmcr = 0;
  configured = 0;
  register_readback_ok = 0;
  applied_valid = 0;
  mode_ok = 0;
  if (prepare_txc_route()) return -1;
  if (mdio_read(phy_addr, REG_PAGE, &saved_page)) {
    page_unknown = 1;
    last_error = k1_led2_err_mdio;
    return -1;
  }
  if (standard_op(REG_BMCR, &saved_bmcr, 0)) return -1;
  if (paged_op(PAGE_LCR, REG_LCR, &saved_lcr, 0)) return -1;
  if (paged_op(PAGE_LCR, REG_EEELCR, &saved_eeelcr, 0)) return -1;
  bmcr = (uint16_t)((saved_bmcr & (uint16_t)~K1_LED2_BMCR_OWNED) |
                    (K1_LED2_BMCR_POWERDOWN_100FD & K1_LED2_BMCR_OWNED));
  if (standard_op(REG_BMCR, &bmcr, 1)) return -1;
  if (standard_op(REG_BMCR, &readback, 0)) return -1;
  if ((readback & K1_LED2_BMCR_OWNED) != (bmcr & K1_LED2_BMCR_OWNED)) {
    last_error = k1_led2_err_readback;
    return -1;
  }
  bmcr = (uint16_t)((saved_bmcr & (uint16_t)~K1_LED2_BMCR_OWNED) |
                    (K1_LED2_BMCR_LOOPBACK_100FD & K1_LED2_BMCR_OWNED));
  if (standard_op(REG_BMCR, &bmcr, 1)) return -1;
  readback = 0;
  if (standard_op(REG_BMCR, &readback, 0)) return -1;
  if ((readback & K1_LED2_BMCR_OWNED) != (bmcr & K1_LED2_BMCR_OWNED)) {
    last_error = k1_led2_err_readback;
    return -1;
  }
  eeelcr = (uint16_t)(saved_eeelcr & (uint16_t)~K1_LED2_EEELCR_OWNED);
  if (paged_op(PAGE_LCR, REG_EEELCR, &eeelcr, 1)) return -1;
  readback = 0;
  if (paged_op(PAGE_LCR, REG_EEELCR, &readback, 0)) return -1;
  if ((readback & K1_LED2_EEELCR_OWNED) != (eeelcr & K1_LED2_EEELCR_OWNED)) {
    last_error = k1_led2_err_readback;
    return -1;
  }
  lcr = titan_led2_lcr_value(saved_lcr, 0, 0);
  if (paged_op(PAGE_LCR, REG_LCR, &lcr, 1)) return -1;
  readback = 0;
  if (paged_op(PAGE_LCR, REG_LCR, &readback, 0)) return -1;
  if ((readback & K1_LED2_LCR_OWNED) != (lcr & K1_LED2_LCR_OWNED)) {
    last_error = k1_led2_err_readback;
    return -1;
  }
  applied_g = 0;
  applied_y = 0;
  applied_valid = 1;
  configured = 1;
  register_readback_ok = 1;
  last_error = k1_led2_err_none;
  return 0;
}

static int observe_mode(uint16_t *bmsr, uint16_t *physr) {
  uint16_t first = 0, bmcr = 0;
  if (standard_op(REG_BMCR, &bmcr, 0)) return -1;
  if ((bmcr & K1_LED2_BMCR_OWNED) !=
      (K1_LED2_BMCR_LOOPBACK_100FD & K1_LED2_BMCR_OWNED)) {
    register_readback_ok = 0;
    last_error = k1_led2_err_readback;
    return -1;
  }
  if (standard_op(REG_BMSR, &first, 0)) return -1;
  if (standard_op(REG_BMSR, bmsr, 0)) return -1;
  return paged_op(PAGE_PHYSR, REG_PHYSR, physr, 0);
}

static int validate_dwt_counter(void) {
#ifndef K1_LED2_PHY_STUB
  dwt_clock_hz = SystemCoreClock;
  if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) ||
      dwt_clock_hz != K1_LED2_DWT_CLOCK_HZ) return -1;
  const uint32_t started = DWT->CYCCNT;
  for (volatile unsigned i = 0; i < 64u; ++i) __NOP();
  dwt_delta = DWT->CYCCNT - started;
  if (!dwt_delta) return -1;
#else
  dwt_clock_hz = K1_LED2_DWT_CLOCK_HZ;
  dwt_delta = 64u;
#endif
  dwt_ok = 1;
  return 0;
}

int titan_led2_phy_init(uint32_t now_ms) {
#ifdef K1_LED2_PHY_STUB
  stub_now_ms = now_ms;
#endif
  capable = 0;
  mode_ok = 0;
  configured = 0;
  register_readback_ok = 0;
  page_unknown = 0;
  txc_enabled = 0;
  txc_requested = 0;
  dwt_ok = 0;
  dwt_delta = 0;
  dwt_clock_hz = 0;
  mdio_pfs = 0;
  mdc_pfs = 0;
  reset_pfs = 0;
  txc_pfs = 0;
  applied_valid = 0;
  experiment_step = k1_led2_step_idle;
  txn_step = K1_LED2_TXN_IDLE;
  txn_op_error = 0;
  desired_version = 1u;
  applied_version = 0;
  txn_version = 0;
  deferred_count = 0;
  desired_g = 0;
  desired_y = 0;
  applied_g = 0;
  applied_y = 0;
  last_error = k1_led2_err_none;
  phy_id = 0;
  ta_zero_bitmap = 0;
  probe_attempt = 0;
  trace_sequence = 0;
  trace_count = 0;
  trace_next = 0;
  memset(trace_ring, 0, sizeof(trace_ring));
  expected_ta_z = 2;
  expected_ta_zero = 2;
  expected_io_error = 0;
  pin_readback = 0;
  if (validate_dwt_counter()) {
    last_error = k1_led2_err_clock;
    return -1;
  }
#ifndef K1_LED2_PHY_STUB
  mdio_io_error = 0;
  R_BSP_PinAccessEnable();
  mdio_note_io(R_IOPORT_PinCfg(&g_ioport_ctrl, MDC_PIN,
                               (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                                   (uint32_t)IOPORT_CFG_PORT_OUTPUT_LOW |
                                   (uint32_t)IOPORT_CFG_DRIVE_MID));
  mdc_pfs = pfs_read(12u, 11u);
  if (!pfs_is_gpio(mdc_pfs, 1)) mdio_io_error = 1;
  mdio_output();
  mdio_note_io(R_IOPORT_PinCfg(&g_ioport_ctrl, RST_PIN,
                               (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                                   (uint32_t)IOPORT_CFG_PORT_OUTPUT_LOW |
                                   (uint32_t)IOPORT_CFG_DRIVE_HIGH));
  reset_pfs = pfs_read(10u, 7u);
  if (!pfs_is_gpio(reset_pfs, 1)) mdio_io_error = 1;
  note_pin_level(RST_PIN, BSP_IO_LEVEL_LOW, 0x01u);
  note_pin_level(MDC_PIN, BSP_IO_LEVEL_LOW, 0x04u);
  mdc_write(BSP_IO_LEVEL_HIGH);
  note_pin_level(MDC_PIN, BSP_IO_LEVEL_HIGH, 0x08u);
  mdc_write(BSP_IO_LEVEL_LOW);
  note_pin_level(MDC_PIN, BSP_IO_LEVEL_LOW, 0x10u);
  mdio_write_level(1);
  note_pin_level(MDIO_PIN, BSP_IO_LEVEL_HIGH, 0x20u);
  mdio_write_level(0);
  note_pin_level(MDIO_PIN, BSP_IO_LEVEL_LOW, 0x40u);
  mdio_write_level(1);
  note_pin_level(MDIO_PIN, BSP_IO_LEVEL_HIGH, 0x80u);
  R_BSP_PinAccessDisable();
  rt_thread_mdelay(20);
  R_BSP_PinAccessEnable();
  mdio_note_io(R_IOPORT_PinWrite(&g_ioport_ctrl, RST_PIN, BSP_IO_LEVEL_HIGH));
  note_pin_level(RST_PIN, BSP_IO_LEVEL_HIGH, 0x02u);
  reset_pfs = pfs_read(10u, 7u);
  R_BSP_PinAccessDisable();
  if (mdio_io_error || pin_readback != PIN_READBACK_EXPECTED) {
    last_error = k1_led2_err_mdio;
    return -1;
  }
  reset_release_ms = diagnostic_now_ms();
  /* Conservative diagnostic startup margin, not a claimed silicon minimum.
     Subsequent identity retries leave reset continuously deasserted. */
  rt_thread_mdelay(200);
#else
  pin_readback = PIN_READBACK_EXPECTED;
  reset_release_ms = now_ms;
#endif
  retry_ms = diagnostic_now_ms() + 500u;
  if (identify_phy()) return -1;
  capable = 1;
  if (configure_loopback()) {
    capable = 0;
    configured = 0;
    mode_ok = 0;
    applied_valid = 0;
    return -1;
  }
  observe_deadline = diagnostic_now_ms() + OBSERVE_MS;
  return 0;
}

void titan_led2_phy_request(int green, int yellow) {
  const int g = green ? 1 : 0;
  const int y = yellow ? 1 : 0;
  if (g != desired_g || y != desired_y) {
    desired_version += 1u;
    if (desired_version == 0u) desired_version = 1u;
  }
  desired_g = g;
  desired_y = y;
}

int titan_led2_phy_start_experiment(uint32_t now_ms) {
  if (!capable || !configured || !mode_ok || !register_readback_ok) return -1;
  experiment_step = k1_led2_step_off1;
  step_ms = now_ms;
  titan_led2_phy_request(0, 0);
  return 0;
}

static int experiment_channels(int step, int *green, int *yellow) {
  *green = 0;
  *yellow = 0;
  if (step == k1_led2_step_green) *green = 1;
  else if (step == k1_led2_step_yellow) *yellow = 1;
  else if (step == k1_led2_step_both) {
    *green = 1;
    *yellow = 1;
  }
  return 0;
}

int titan_led2_phy_service(uint32_t now_ms, uint32_t remaining_cycles) {
#ifndef K1_LED2_PHY_STUB
  uint32_t t0;
#endif
#ifdef K1_LED2_PHY_STUB
  stub_now_ms = now_ms;
#endif
  if (remaining_cycles < K1_LED2_FRAME_BUDGET_CYCLES) {
    if (led2_apply_needed()) deferred_count += 1u;
    return 0;
  }
  if (!capable) {
    if ((int32_t)(now_ms - retry_ms) < 0) return last_error ? -1 : 0;
    retry_ms = now_ms + 500u;
    if (identify_phy()) return -1;
    capable = 1;
    if (configure_loopback()) {
      capable = 0;
      configured = 0;
      mode_ok = 0;
      applied_valid = 0;
      return -1;
    }
    observe_deadline = now_ms + OBSERVE_MS;
  }
#ifndef K1_LED2_PHY_STUB
  t0 = DWT->CYCCNT;
#endif
  if (!mode_ok) {
    uint16_t bmsr = 0, physr = 0;
    if ((int32_t)(now_ms - observe_deadline) < 0) {
      if (observe_mode(&bmsr, &physr)) {
        capable = 0;
        configured = 0;
        mode_ok = 0;
        applied_valid = 0;
        retry_ms = now_ms + 500u;
#ifndef K1_LED2_PHY_STUB
        last_cost = DWT->CYCCNT - t0;
#endif
        return -1;
      }
      if ((bmsr & 0x0004u) && ((physr >> 4) & 0x3u) == 1u) {
        mode_ok = 1;
        register_readback_ok = 1;
        last_error = k1_led2_err_none;
        /* Application mode applies the current request. Diagnostic colour
           sequences require an explicit start_experiment. */
      }
    } else if (!mode_ok) {
      last_error = k1_led2_err_mode;
      capable = 0;
      configured = 0;
      register_readback_ok = 0;
      applied_valid = 0;
      retry_ms = now_ms + 500u;
    }
    if (!mode_ok) {
#ifndef K1_LED2_PHY_STUB
      last_cost = DWT->CYCCNT - t0;
#endif
      return -1;
    }
  }
  if (experiment_step >= k1_led2_step_off1 && experiment_step < k1_led2_step_done) {
    int green = 0, yellow = 0;
    experiment_channels(experiment_step, &green, &yellow);
    titan_led2_phy_request(green, yellow);
    if ((int32_t)(now_ms - step_ms) >= (int32_t)HOLD_MS) {
      experiment_step++;
      step_ms = now_ms;
    }
  }
  if (txn_step == K1_LED2_TXN_IDLE && led2_apply_needed()) led2_txn_begin();
  {
    const int unlimited = remaining_cycles == 0xffffffffu;
    uint32_t slack = remaining_cycles;
    while (txn_step != K1_LED2_TXN_IDLE &&
           (unlimited || slack >= K1_LED2_FRAME_BUDGET_CYCLES)) {
      const int rc = led2_txn_frame();
      if (!unlimited) slack -= K1_LED2_FRAME_BUDGET_CYCLES;
      if (rc != 0) {
        retry_ms = now_ms + 500u;
#ifndef K1_LED2_PHY_STUB
        last_cost = DWT->CYCCNT - t0;
#endif
        return rc;
      }
      if (!unlimited) break;
    }
    if (txn_step != K1_LED2_TXN_IDLE && !unlimited) deferred_count += 1u;
  }
#ifndef K1_LED2_PHY_STUB
  last_cost = DWT->CYCCNT - t0;
#endif
  return 0;
}

int titan_led2_phy_poll(uint32_t now_ms, int allow_mdio) {
  if (!allow_mdio) return 0;
  return titan_led2_phy_service(now_ms, 0xffffffffu);
}

int titan_led2_phy_capable(void) { return capable; }
int titan_led2_phy_mode_ok(void) { return mode_ok; }
uint32_t titan_led2_phy_id(void) { return phy_id; }
uint32_t titan_led2_phy_error(void) { return last_error; }
int titan_led2_phy_applied_green(void) { return applied_g; }
int titan_led2_phy_applied_yellow(void) { return applied_y; }
int titan_led2_phy_applied_valid(void) { return applied_valid; }
int titan_led2_phy_txc_enabled(void) { return txc_enabled; }
int titan_led2_phy_txc_requested(void) { return txc_requested; }
uint32_t titan_led2_phy_write_count(void) { return write_count; }
int titan_led2_phy_experiment_step(void) { return experiment_step; }
uint32_t titan_led2_phy_desired_version(void) { return desired_version; }
uint32_t titan_led2_phy_applied_version(void) { return applied_version; }
uint32_t titan_led2_phy_deferred(void) { return deferred_count; }
uint32_t titan_led2_phy_txn_step(void) { return (uint32_t)txn_step; }
int titan_led2_phy_page_unknown(void) { return page_unknown; }
uint32_t titan_led2_phy_last_cost_cycles(void) { return last_cost; }
uint16_t titan_led2_phy_last_id1(void) { return last_id1; }
uint16_t titan_led2_phy_last_id2(void) { return last_id2; }
uint8_t titan_led2_phy_last_addr(void) { return last_addr; }
uint8_t titan_led2_phy_expected_ta_z(void) { return expected_ta_z; }
uint8_t titan_led2_phy_expected_ta_zero(void) { return expected_ta_zero; }
int titan_led2_phy_expected_io_error(void) { return expected_io_error; }
uint8_t titan_led2_phy_pin_readback(void) { return pin_readback; }
uint32_t titan_led2_phy_ta_zero_bitmap(void) { return ta_zero_bitmap; }
int titan_led2_phy_register_readback_ok(void) { return register_readback_ok; }
int titan_led2_phy_dwt_ok(void) { return dwt_ok; }
uint32_t titan_led2_phy_dwt_delta(void) { return dwt_delta; }
uint32_t titan_led2_phy_dwt_clock_hz(void) { return dwt_clock_hz; }
uint32_t titan_led2_phy_mdio_pfs(void) { return mdio_pfs; }
uint32_t titan_led2_phy_mdc_pfs(void) { return mdc_pfs; }
uint32_t titan_led2_phy_reset_pfs(void) { return reset_pfs; }
uint32_t titan_led2_phy_txc_pfs(void) { return txc_pfs; }
uint32_t titan_led2_phy_attempt(void) { return probe_attempt; }

uint32_t titan_led2_phy_trace_snapshot(k1_led2_phy_trace_t *output, uint32_t capacity) {
  if (!output || !capacity) return 0;
  const uint32_t count = trace_count < capacity ? trace_count : capacity;
  const uint32_t oldest = trace_count < K1_LED2_PHY_TRACE_CAPACITY ? 0u : trace_next;
  const uint32_t skip = trace_count - count;
  for (uint32_t i = 0; i < count; ++i) {
    output[i] = trace_ring[(oldest + skip + i) % K1_LED2_PHY_TRACE_CAPACITY];
  }
  return count;
}

int titan_led2_phy_trace_latest(k1_led2_phy_trace_t *output) {
  if (!output || !trace_count) return -1;
  const uint32_t index = trace_next ? trace_next - 1u : K1_LED2_PHY_TRACE_CAPACITY - 1u;
  *output = trace_ring[index];
  return 0;
}
