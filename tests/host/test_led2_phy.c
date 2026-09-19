#include "titan_led2_phy.h"
#include "k1_status_led.h"
#include <assert.h>
#include <stdio.h>

static void initialise_ready(uint32_t now_ms) {
  titan_led2_phy_stub_reset();
  assert(titan_led2_phy_init(now_ms) == 0);
  assert(titan_led2_phy_poll(now_ms, 1) == 0);
  assert(titan_led2_phy_capable() == 1);
  assert(titan_led2_phy_mode_ok() == 1);
  assert(titan_led2_phy_register_readback_ok() == 1);
  assert(titan_led2_phy_dwt_ok() == 1);
  assert(titan_led2_phy_dwt_clock_hz() == K1_LED2_DWT_CLOCK_HZ);
  assert(titan_led2_phy_experiment_step() == k1_led2_step_idle);
  assert(titan_led2_phy_start_experiment(now_ms) == 0);
  assert(titan_led2_phy_poll(now_ms, 1) == 0);
}

int main(void) {
  uint8_t g = 1, y = 1;
  assert(titan_led2_lcr_value(0, 0, 0) == 0x2100u);
  assert(titan_led2_lcr_value(0, 1, 0) == 0x2040u);
  assert(titan_led2_lcr_value(0, 0, 1) == 0x0900u);
  assert(titan_led2_lcr_value(0, 1, 1) == 0x0840u);
  assert((titan_led2_lcr_value(0xFFFFu, 0, 0) & K1_LED2_LCR_OWNED) == 0x2100u);
  assert((titan_led2_lcr_value(0xFFFFu, 1, 0) & K1_LED2_LCR_OWNED) == 0x2040u);
  assert((titan_led2_lcr_value(0xFFFFu, 0, 1) & K1_LED2_LCR_OWNED) == 0x0900u);
  assert((titan_led2_lcr_value(0xFFFFu, 1, 1) & K1_LED2_LCR_OWNED) == 0x0840u);

  k1_status_led2_channels(k1_led_display_wait_operator, 0, 0, 1, &g, &y);
  assert(g == 0 && y == 1);
  k1_status_led2_channels(k1_led_display_wait_operator, 0, 800, 1, &g, &y);
  assert(g == 0 && y == 0);
  k1_status_led2_channels(k1_led_display_warning, 0, 0, 1, &g, &y);
  assert(g == 0 && y == 1);
  k1_status_led2_channels(k1_led_display_warning, 0, 300, 1, &g, &y);
  assert(y == 1);
  k1_status_led2_channels(k1_led_display_running, 0, 0, 1, &g, &y);
  assert(g == 1 && y == 0);
  k1_status_led2_channels(k1_led_display_running, 0, 100, 1, &g, &y);
  assert(g == 0 && y == 0);
  k1_status_led2_channels(k1_led_display_running, 0, 0, 0, &g, &y);
  assert(g == 0 && y == 0);

  titan_led2_phy_stub_reset();
  assert(titan_led2_phy_init(0) == 0);
  assert(titan_led2_phy_id() == K1_LED2_PHYID_RTL8211F);
  assert(titan_led2_phy_poll(0, 1) == 0);
  assert(titan_led2_phy_mode_ok() == 1);
  assert(titan_led2_phy_register_readback_ok() == 1);
  assert(titan_led2_phy_txc_requested() == 1);
  assert(titan_led2_phy_txc_enabled() == 0); /* Clock is not independently verified. */
  assert((titan_led2_phy_stub_bmcr() & K1_LED2_BMCR_OWNED) ==
         (K1_LED2_BMCR_LOOPBACK_100FD & K1_LED2_BMCR_OWNED));
  assert((titan_led2_phy_stub_bmcr() & (uint16_t)~K1_LED2_BMCR_OWNED) == 0x0080u);
  assert(titan_led2_phy_experiment_step() == k1_led2_step_idle);
  assert(titan_led2_phy_start_experiment(0) == 0);
  assert(titan_led2_phy_experiment_step() == k1_led2_step_off1);
  assert(titan_led2_phy_poll(10, 1) == 0);
  assert(titan_led2_phy_applied_green() == 0 && titan_led2_phy_applied_yellow() == 0);
  const uint32_t writes = titan_led2_phy_write_count();
  assert(titan_led2_phy_poll(20, 1) == 0);
  assert(titan_led2_phy_write_count() == writes);

  assert(titan_led2_phy_poll(500, 1) == 0);
  assert(titan_led2_phy_experiment_step() == k1_led2_step_green);
  assert(titan_led2_phy_poll(500, 1) == 0);
  assert(titan_led2_phy_applied_green() == 1 && titan_led2_phy_applied_yellow() == 0);
  assert((titan_led2_phy_stub_lcr() & K1_LED2_LCR_OWNED) == 0x2040u);

  assert(titan_led2_phy_poll(1000, 1) == 0);
  assert(titan_led2_phy_poll(1000, 1) == 0);
  assert(titan_led2_phy_applied_green() == 0 && titan_led2_phy_applied_yellow() == 0);

  assert(titan_led2_phy_poll(1500, 1) == 0);
  assert(titan_led2_phy_poll(1500, 1) == 0);
  assert(titan_led2_phy_applied_green() == 0 && titan_led2_phy_applied_yellow() == 1);
  assert((titan_led2_phy_stub_lcr() & K1_LED2_LCR_OWNED) == 0x0900u);

  assert(titan_led2_phy_poll(2000, 1) == 0);
  assert(titan_led2_phy_poll(2500, 1) == 0);
  assert(titan_led2_phy_poll(2500, 1) == 0);
  assert(titan_led2_phy_applied_green() == 1 && titan_led2_phy_applied_yellow() == 1);
  assert((titan_led2_phy_stub_lcr() & K1_LED2_LCR_OWNED) == 0x0840u);

  assert(titan_led2_phy_poll(3000, 1) == 0);
  assert(titan_led2_phy_poll(3000, 1) == 0);
  assert(titan_led2_phy_applied_green() == 0 && titan_led2_phy_applied_yellow() == 0);
  assert(titan_led2_phy_poll(3500, 1) == 0);
  assert(titan_led2_phy_experiment_step() == k1_led2_step_done);
  assert(titan_led2_phy_stub_page() != 0x0D04u);

  /* Software reset is a controlled BMCR bit and must not be preserved into
     either loopback write; unrelated collision-test state remains intact. */
  titan_led2_phy_stub_reset();
  titan_led2_phy_stub_set_bmcr(0x8080u);
  assert(titan_led2_phy_init(3600) == 0);
  assert((titan_led2_phy_stub_bmcr() & 0x8000u) == 0);
  assert((titan_led2_phy_stub_bmcr() & 0x0080u) != 0);

  k1_led2_phy_trace_t trace[K1_LED2_PHY_TRACE_CAPACITY];
  const uint32_t trace_count = titan_led2_phy_trace_snapshot(trace, K1_LED2_PHY_TRACE_CAPACITY);
  int saw_id1 = 0, saw_id2 = 0;
  for (uint32_t i = 0; i < trace_count; ++i) {
    assert(trace[i].sequence != 0);
    if (trace[i].attempt == 1 && trace[i].address == 1 && trace[i].reg == 2 &&
        trace[i].ack == 0 && trace[i].io_error == 0 && trace[i].value_valid &&
        trace[i].value == 0x001Cu) saw_id1 = 1;
    if (trace[i].attempt == 1 && trace[i].address == 1 && trace[i].reg == 3 &&
        trace[i].ack == 0 && trace[i].io_error == 0 && trace[i].value_valid &&
        trace[i].value == 0xC916u) saw_id2 = 1;
  }
  assert(saw_id1 && saw_id2);

  /* A failed operation still restores the original page and keeps its first error. */
  initialise_ready(4000);
  assert(titan_led2_phy_poll(4500, 1) == 0);
  titan_led2_phy_stub_fail_write_after(0x10u, 0);
  assert(titan_led2_phy_poll(4500, 1) != 0);
  assert(titan_led2_phy_error() == k1_led2_err_mdio);
  assert(titan_led2_phy_stub_page() == 0);
  assert(titan_led2_phy_page_unknown() == 0);
  assert(titan_led2_phy_applied_valid() == 0);

  /* If both the operation and restoration fail, retain the operation error and
     separately expose page uncertainty. */
  initialise_ready(5000);
  assert(titan_led2_phy_poll(5500, 1) == 0);
  titan_led2_phy_stub_fail_write_after(0x10u, 0);
  titan_led2_phy_stub_fail_restore(1);
  assert(titan_led2_phy_poll(5500, 1) != 0);
  assert(titan_led2_phy_error() == k1_led2_err_mdio);
  assert(titan_led2_phy_page_unknown() == 1);

  /* A restoration-only failure is not mislabelled as an operation failure. */
  initialise_ready(6000);
  assert(titan_led2_phy_poll(6500, 1) == 0);
  titan_led2_phy_stub_fail_write_after(0x1Fu, 1);
  assert(titan_led2_phy_poll(6500, 1) != 0);
  assert(titan_led2_phy_error() == k1_led2_err_restore);
  assert(titan_led2_phy_page_unknown() == 1);

  /* Requested channels are not applied until the owned LCR bits read back. */
  initialise_ready(7000);
  assert(titan_led2_phy_poll(7500, 1) == 0);
  titan_led2_phy_stub_set_lcr_readback_xor(0x0040u);
  assert(titan_led2_phy_poll(7500, 1) != 0);
  assert(titan_led2_phy_error() == k1_led2_err_readback);
  assert(titan_led2_phy_applied_valid() == 0);
  titan_led2_phy_stub_set_lcr_readback_xor(0);

  /* Mode-observation I/O failures are immediate and cannot become READY from
     zero/stale register values. */
  titan_led2_phy_stub_reset();
  assert(titan_led2_phy_init(8000) == 0);
  titan_led2_phy_stub_fail_read_after(0x01u, 0);
  assert(titan_led2_phy_poll(8000, 1) != 0);
  assert(titan_led2_phy_error() == k1_led2_err_mdio);
  assert(titan_led2_phy_mode_ok() == 0);

  /* BMCR configuration is page-zero scoped and verified before capability can
     advance to mode observation. */
  titan_led2_phy_stub_reset();
  titan_led2_phy_stub_set_bmcr_readback_xor(0x0100u);
  assert(titan_led2_phy_init(9000) != 0);
  assert(titan_led2_phy_error() == k1_led2_err_readback);
  assert(titan_led2_phy_capable() == 0);
  assert(titan_led2_phy_mode_ok() == 0);

  /* A coherent responder on another address is evidence of a strap mismatch,
     never permission to change the schematic address contract. */
  titan_led2_phy_stub_reset();
  titan_led2_phy_stub_set_phy_address(2);
  assert(titan_led2_phy_init(10000) != 0);
  assert(titan_led2_phy_error() == k1_led2_err_identity);
  assert(titan_led2_phy_id() == K1_LED2_PHYID_RTL8211F);
  assert(titan_led2_phy_last_addr() == 2);
  assert(titan_led2_phy_capable() == 0);

  /* Slack below one Clause-22 frame must not start MDIO; one-frame slack
     must not complete a multi-register apply. */
  titan_led2_phy_stub_reset();
  assert(titan_led2_phy_init(11000) == 0);
  assert(titan_led2_phy_poll(11000, 1) == 0);
  assert(titan_led2_phy_mode_ok() == 1);
  assert(titan_led2_phy_experiment_step() == k1_led2_step_idle);
  titan_led2_phy_request(1, 0);
  {
    const uint32_t deferred = titan_led2_phy_deferred();
    assert(titan_led2_phy_service(11000, 0) == 0);
    assert(titan_led2_phy_deferred() == deferred + 1u);
    assert(titan_led2_phy_applied_green() == 0);
    assert(titan_led2_phy_txn_step() == K1_LED2_TXN_IDLE ||
           titan_led2_phy_applied_green() == 0);
    assert(titan_led2_phy_service(11000, K1_LED2_FRAME_BUDGET_CYCLES) == 0);
    assert(titan_led2_phy_txn_step() != K1_LED2_TXN_IDLE);
    assert(titan_led2_phy_applied_valid() == 0);
    assert(titan_led2_phy_service(11000, 0xffffffffu) == 0);
    assert(titan_led2_phy_applied_green() == 1);
    assert(titan_led2_phy_applied_version() == titan_led2_phy_desired_version());
  }

  /* Capability becoming ready must not launch the diagnostic colour sequence. */
  titan_led2_phy_stub_reset();
  assert(titan_led2_phy_init(12000) == 0);
  assert(titan_led2_phy_poll(12000, 1) == 0);
  assert(titan_led2_phy_mode_ok() == 1);
  assert(titan_led2_phy_experiment_step() == k1_led2_step_idle);

  puts("K1_LED2_PHY=PASS");
  return 0;
}
