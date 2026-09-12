#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

enum {
  k1_led_colour_off = 0,
  k1_led_colour_red = 1,
  k1_led_colour_green = 2,
  k1_led_colour_yellow = 3,
  k1_led_colour_blue = 4,
  k1_led_colour_magenta = 5,
  k1_led_colour_cyan = 6,
  k1_led_colour_white = 7
};

enum k1_led_activity {
  k1_led_activity_booting = 0,
  k1_led_activity_ready,
  k1_led_activity_running,
  k1_led_activity_test,
  k1_led_activity_finalizing,
  k1_led_activity_wait
};

enum k1_led_outcome {
  k1_led_outcome_none = 0,
  k1_led_outcome_pending,
  k1_led_outcome_pass,
  k1_led_outcome_fail,
  k1_led_outcome_incomplete
};

enum k1_led_display {
  k1_led_display_booting = 0,
  k1_led_display_boot_ok,
  k1_led_display_ready,
  k1_led_display_running,
  k1_led_display_test_running,
  k1_led_display_finalizing,
  k1_led_display_test_pass,
  k1_led_display_test_fail,
  k1_led_display_test_incomplete,
  k1_led_display_wait_operator,
  k1_led_display_warning,
  k1_led_display_fatal,
  k1_led_display_identify
};

enum k1_led_cue {
  k1_led_cue_none = 0,
  k1_led_cue_wait,
  k1_led_cue_stall,
  k1_led_cue_progress,
  k1_led_cue_finalizing
};

enum {
  k1_led_fail_crc = 1u,
  k1_led_fail_deadline = 2u,
  k1_led_fail_guard = 4u,
  k1_led_fail_render = 8u,
  k1_led_fail_verdict_timeout = 16u,
  k1_led_fail_cancel = 32u
};

void k1_status_led_init(uint32_t now_ms);
void k1_status_led_set_activity(uint8_t activity, uint32_t now_ms);
void k1_status_led_boot_ok(uint32_t now_ms);
void k1_status_led_observe_control(uint32_t now_ms);
void k1_status_led_observe_hop(uint32_t sequence, uint32_t now_ms);
void k1_status_led_test_begin(uint64_t run_id, uint32_t now_ms);
void k1_status_led_test_fail(uint32_t flag, const char *reason, uint32_t now_ms);
void k1_status_led_execution_end(uint32_t completed, uint32_t fail_flags, uint32_t now_ms);
int k1_status_led_verdict(uint64_t run_id, uint8_t outcome, uint32_t digest, uint32_t now_ms);
int k1_status_led_ack(uint64_t run_id, uint32_t now_ms);
int k1_status_led_ack_recovery(uint32_t now_ms);
int k1_status_led_wait(uint32_t reason, uint32_t now_ms);
void k1_status_led_wait_clear(uint32_t now_ms);
int k1_status_led_identify(uint32_t now_ms);
void k1_status_led_fault(int fatal, const char *reason, uint32_t now_ms);
void k1_status_led_usb(int configured, int session, uint32_t now_ms);
void k1_status_led_poll(uint32_t now_ms, int emit_gpio);
uint8_t k1_status_led_display(void);
uint8_t k1_status_led_pattern_mask(uint8_t display, uint8_t cue, uint32_t elapsed, int *expired);
int k1_status_led_snapshot(char *output, uint32_t capacity);
uint32_t k1_status_led_max_service_cycles(void);
void k1_status_led_add_service_cycles(uint32_t cycles);

#ifdef __cplusplus
}
#endif
