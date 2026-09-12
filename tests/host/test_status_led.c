#include "k1_status_led.h"
#include "titan_status_gpio.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern uint32_t titan_status_gpio_stub_port(void);

static uint32_t port_unrelated(void) {
  return titan_status_gpio_stub_port() & ~k1_status_gpio_port_mask;
}

static void assert_colour(uint8_t logical) {
  const uint32_t physical = titan_status_gpio_last_physical();
  uint32_t on = 0;
  if (logical & 1) on |= (1u << 9);
  if (logical & 2) on |= (1u << 8);
  if (logical & 4) on |= (1u << 10);
  assert(physical == ((~on) & 0x0700u));
  assert(port_unrelated() == 0);
}

int main(void) {
  int expired = 0;
  assert(k1_status_led_pattern_mask(k1_led_display_booting, 0, 0, &expired) == k1_led_colour_yellow);
  assert(k1_status_led_pattern_mask(k1_led_display_booting, 0, 150, &expired) == k1_led_colour_off);
  assert(k1_status_led_pattern_mask(k1_led_display_ready, 0, 0, &expired) == k1_led_colour_blue);
  assert(k1_status_led_pattern_mask(k1_led_display_ready, 0, 80, &expired) == k1_led_colour_off);
  assert(k1_status_led_pattern_mask(k1_led_display_running, 0, 0, &expired) == k1_led_colour_green);
  assert(k1_status_led_pattern_mask(k1_led_display_test_running, 0, 0, &expired) == k1_led_colour_magenta);
  assert(k1_status_led_pattern_mask(k1_led_display_test_running, 0, 120, &expired) == k1_led_colour_off);
  assert(k1_status_led_pattern_mask(k1_led_display_test_running, 0, 300, &expired) == k1_led_colour_magenta);
  assert(k1_status_led_pattern_mask(k1_led_display_finalizing, 0, 300, &expired) == k1_led_colour_cyan);
  assert(k1_status_led_pattern_mask(k1_led_display_test_pass, 0, 600, &expired) == k1_led_colour_green);
  assert(k1_status_led_pattern_mask(k1_led_display_test_fail, 0, 0, &expired) == k1_led_colour_red);
  assert(k1_status_led_pattern_mask(k1_led_display_test_fail, k1_led_cue_wait, 1000, &expired) ==
         k1_led_colour_yellow);
  assert(k1_status_led_pattern_mask(k1_led_display_test_fail, k1_led_cue_progress, 1000, &expired) ==
         k1_led_colour_magenta);
  assert(k1_status_led_pattern_mask(k1_led_display_test_fail, k1_led_cue_finalizing, 1000, &expired) ==
         k1_led_colour_cyan);
  assert(k1_status_led_pattern_mask(k1_led_display_test_incomplete, 0, 300, &expired) ==
         k1_led_colour_yellow);
  assert(k1_status_led_pattern_mask(k1_led_display_wait_operator, 0, 0, &expired) == k1_led_colour_yellow);
  assert(k1_status_led_pattern_mask(k1_led_display_fatal, 0, 0, &expired) == k1_led_colour_red);
  assert(k1_status_led_pattern_mask(k1_led_display_fatal, 0, 500, &expired) == k1_led_colour_off);
  k1_status_led_pattern_mask(k1_led_display_boot_ok, 0, 720, &expired);
  assert(expired == 1);
  k1_status_led_pattern_mask(k1_led_display_identify, 0, 900, &expired);
  assert(expired == 1);
  /* wrap: elapsed 2^32-10 + 20 from epoch handled by uint32 subtract in poll, not here */

  k1_status_led_init(0);
  assert_colour(k1_led_colour_yellow);
  assert(k1_status_led_display() == k1_led_display_booting);

  k1_status_led_boot_ok(10);
  k1_status_led_poll(10, 1);
  assert(k1_status_led_display() == k1_led_display_boot_ok);
  k1_status_led_poll(10 + 720, 1);
  assert(k1_status_led_display() == k1_led_display_ready);
  assert_colour(k1_led_colour_blue); /* t=0 of ready after epoch reset */

  const uint8_t before = k1_status_led_display();
  k1_status_led_set_activity(k1_led_activity_ready, 800);
  k1_status_led_poll(850, 1);
  assert(k1_status_led_display() == before || k1_status_led_display() == k1_led_display_ready);

  assert(k1_status_led_identify(900) == 0);
  k1_status_led_poll(900, 1);
  assert(k1_status_led_display() == k1_led_display_identify);
  assert_colour(k1_led_colour_white);

  k1_status_led_fault(1, "usb_open", 1000);
  k1_status_led_poll(1000, 1);
  assert(k1_status_led_display() == k1_led_display_fatal);
  assert(k1_status_led_identify(1001) != 0);
  assert(k1_status_led_ack_recovery(1002) != 0);
  assert_colour(k1_led_colour_red);

  k1_status_led_init(0);
  k1_status_led_boot_ok(10);
  k1_status_led_poll(800, 1);
  k1_status_led_test_begin(42, 1000);
  k1_status_led_observe_hop(0, 1000);
  k1_status_led_observe_hop(1, 1008);
  k1_status_led_poll(1008, 1);
  assert(k1_status_led_display() == k1_led_display_test_running);
  k1_status_led_test_fail(k1_led_fail_guard, "release_guard", 1010);
  k1_status_led_observe_hop(2, 1016);
  k1_status_led_poll(1016, 1);
  assert(k1_status_led_display() == k1_led_display_test_fail);
  k1_status_led_observe_hop(3, 1020);
  k1_status_led_poll(1020, 1);
  assert(k1_status_led_display() == k1_led_display_test_fail);
  assert(k1_status_led_verdict(42, k1_led_outcome_pass, 1, 2000) != 0);
  k1_status_led_execution_end(6000, k1_led_fail_guard, 50000);
  k1_status_led_poll(50000, 1);
  assert(k1_status_led_display() == k1_led_display_test_fail);
  assert(k1_status_led_ack(99, 50001) != 0);
  assert(k1_status_led_ack(42, 50002) == 0);
  k1_status_led_poll(50002, 1);
  assert(k1_status_led_display() != k1_led_display_test_pass);

  k1_status_led_init(0);
  k1_status_led_boot_ok(10);
  k1_status_led_poll(800, 1);
  k1_status_led_test_begin(7, 1000);
  for (uint32_t hop = 1; hop <= 8; ++hop) k1_status_led_observe_hop(hop, 1000 + hop * 8);
  k1_status_led_execution_end(8, 0, 2000);
  k1_status_led_poll(2000, 1);
  assert(k1_status_led_display() == k1_led_display_finalizing);
  assert(k1_status_led_verdict(7, k1_led_outcome_pass, 99, 2100) == 0);
  k1_status_led_poll(2100, 1);
  assert(k1_status_led_display() == k1_led_display_test_pass);
  assert(k1_status_led_verdict(7, k1_led_outcome_pass, 99, 2110) == 0);

  k1_status_led_init(0);
  k1_status_led_boot_ok(10);
  k1_status_led_poll(800, 1);
  k1_status_led_test_begin(1, 1000);
  k1_status_led_observe_hop(1, 1008);
  k1_status_led_execution_end(1, 0, 2000);
  k1_status_led_poll(2000 + 60000, 1);
  assert(k1_status_led_display() == k1_led_display_test_incomplete);

  k1_status_led_init(0);
  k1_status_led_boot_ok(10);
  k1_status_led_poll(800, 1);
  assert(k1_status_led_wait(3, 900) == 0);
  k1_status_led_poll(900, 1);
  assert(k1_status_led_display() == k1_led_display_wait_operator);
  k1_status_led_poll(900 + 30000, 1);
  assert(k1_status_led_display() != k1_led_display_wait_operator);

  char json[1024];
  assert(k1_status_led_snapshot(json, sizeof json) == 0);
  assert(strstr(json, "\"schema\":1"));
  puts("K1_STATUS_LED=PASS");
  return 0;
}
