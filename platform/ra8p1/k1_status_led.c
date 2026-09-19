#include "k1_status_led.h"
#include "titan_status_gpio.h"
#include "titan_led2_phy.h"
#include <string.h>
#include <stdio.h>

#define K1_LED_STALL_MS 250u
#define K1_LED_WAIT_LEASE_MS 30000u
#define K1_LED_WAIT_RENEW_MS 10000u
#define K1_LED_VERDICT_MS 60000u
#define K1_LED_REASON_MAX 48

struct Domain {
  uint32_t last_seq, last_ms, baseline, group_seq;
  int armed, fresh;
};

struct Status {
  uint8_t activity, display, cue, last_mask;
  uint8_t outcome, outcome_acked, identify;
  uint8_t warning, fatal, fatal_acked, stall_latched, stall_recoveries;
  uint8_t usb_configured, usb_session, boot_ok;
  uint8_t in_progress_group;
  uint32_t fail_flags, first_fail, completed, requested;
  uint32_t epoch_ms, group_ms, last_display;
  uint32_t wait_until, wait_reason, wait_last_set;
  uint32_t verdict_until, last_progress_ms, last_poll_ms, max_gap_ms;
  uint32_t max_service_cycles, last_gpio_error;
  uint32_t session, digest;
  uint64_t run_id;
  char reason[K1_LED_REASON_MAX];
  struct Domain hop, control;
};

static struct Status g;

static uint32_t elapsed_ms(uint32_t now, uint32_t epoch) { return now - epoch; }

static int in_iv(uint32_t t, uint16_t a, uint16_t b) { return t >= a && t < b; }

uint8_t k1_status_led_pattern_mask(uint8_t display, uint8_t cue, uint32_t elapsed, int *expired) {
  if (expired) *expired = 0;
  if (display == k1_led_display_boot_ok) {
    if (elapsed >= 720u) {
      if (expired) *expired = 1;
      return k1_led_colour_off;
    }
    return (in_iv(elapsed, 0, 180) || in_iv(elapsed, 360, 540)) ? k1_led_colour_green
                                                               : k1_led_colour_off;
  }
  if (display == k1_led_display_identify) {
    if (elapsed >= 900u) {
      if (expired) *expired = 1;
      return k1_led_colour_off;
    }
    return (in_iv(elapsed, 0, 120) || in_iv(elapsed, 300, 420) || in_iv(elapsed, 600, 720))
               ? k1_led_colour_white
               : k1_led_colour_off;
  }
  uint32_t cycle = 1000u;
  uint8_t colour = k1_led_colour_off;
  switch (display) {
  case k1_led_display_booting:
    cycle = 500u;
    colour = k1_led_colour_yellow;
    break;
  case k1_led_display_ready:
    cycle = 2000u;
    colour = k1_led_colour_blue;
    break;
  case k1_led_display_running:
    cycle = 1000u;
    colour = k1_led_colour_green;
    break;
  case k1_led_display_test_running:
    cycle = 1200u;
    colour = k1_led_colour_magenta;
    break;
  case k1_led_display_finalizing:
    cycle = 2000u;
    colour = k1_led_colour_cyan;
    break;
  case k1_led_display_test_pass:
    cycle = 2400u;
    colour = k1_led_colour_green;
    break;
  case k1_led_display_test_fail:
    cycle = 2400u;
    colour = k1_led_colour_red;
    break;
  case k1_led_display_test_incomplete:
    cycle = 2400u;
    colour = k1_led_colour_yellow;
    break;
  case k1_led_display_wait_operator:
    cycle = 2000u;
    colour = k1_led_colour_yellow;
    break;
  case k1_led_display_warning:
    cycle = 2000u;
    colour = k1_led_colour_yellow;
    break;
  case k1_led_display_fatal:
    cycle = 1000u;
    colour = k1_led_colour_red;
    break;
  default:
    return k1_led_colour_off;
  }
  const uint32_t t = elapsed % cycle;
  uint8_t mask = k1_led_colour_off;
  switch (display) {
  case k1_led_display_booting:
    if (in_iv(t, 0, 150)) mask = colour;
    break;
  case k1_led_display_ready:
    if (in_iv(t, 0, 80)) mask = colour;
    break;
  case k1_led_display_running:
    if (in_iv(t, 0, 80)) mask = colour;
    break;
  case k1_led_display_test_running:
  case k1_led_display_finalizing:
    if (in_iv(t, 0, 120) || in_iv(t, 300, 420)) mask = colour;
    break;
  case k1_led_display_test_pass:
  case k1_led_display_test_fail:
  case k1_led_display_test_incomplete:
    if (in_iv(t, 0, 120) || in_iv(t, 300, 420) || in_iv(t, 600, 720)) mask = colour;
    break;
  case k1_led_display_wait_operator:
    if (in_iv(t, 0, 800)) mask = colour;
    break;
  case k1_led_display_warning:
    if (in_iv(t, 0, 120) || in_iv(t, 300, 420)) mask = colour;
    break;
  case k1_led_display_fatal:
    if (in_iv(t, 0, 500)) mask = colour;
    break;
  default:
    break;
  }
  if (display == k1_led_display_test_fail) {
    if (cue == k1_led_cue_wait && in_iv(t, 1000, 1800)) mask = k1_led_colour_yellow;
    else if (cue == k1_led_cue_stall && (in_iv(t, 1000, 1120) || in_iv(t, 1300, 1420)))
      mask = k1_led_colour_yellow;
    else if (cue == k1_led_cue_progress && in_iv(t, 1000, 1120)) mask = k1_led_colour_magenta;
    else if (cue == k1_led_cue_finalizing && in_iv(t, 1000, 1120)) mask = k1_led_colour_cyan;
  }
  if (display == k1_led_display_warning && cue == k1_led_cue_progress && in_iv(t, 1000, 1120))
    mask = k1_led_colour_magenta;
  return mask;
}

static void set_reason(const char *reason) {
  if (!reason) {
    g.reason[0] = 0;
    return;
  }
  size_t n = 0;
  while (reason[n] && n + 1 < K1_LED_REASON_MAX) {
    g.reason[n] = reason[n];
    ++n;
  }
  g.reason[n] = 0;
}

static int healthy_idle(void) {
  const int badged = ((g.outcome == k1_led_outcome_fail ||
                       g.outcome == k1_led_outcome_incomplete) &&
                      !g.outcome_acked);
  return !g.fatal && !g.warning && !badged && g.activity != k1_led_activity_test &&
         g.activity != k1_led_activity_finalizing && g.activity != k1_led_activity_wait;
}

static void cancel_identify(void) { g.identify = 0; }

static void resolve(uint32_t now_ms) {
  uint8_t display;
  uint8_t cue = k1_led_cue_none;
  const int fail_unacked = (g.outcome == k1_led_outcome_fail) && !g.outcome_acked;
  const int incomplete_unacked = (g.outcome == k1_led_outcome_incomplete) && !g.outcome_acked;
  const int pass_unacked = (g.outcome == k1_led_outcome_pass) && !g.outcome_acked;
  const int test_live = g.activity == k1_led_activity_test;
  const int finalizing = g.activity == k1_led_activity_finalizing;
  const int progressing = g.hop.fresh && (now_ms - g.last_progress_ms) < K1_LED_STALL_MS;
  if (g.fatal && !g.fatal_acked) {
    display = k1_led_display_fatal;
  } else if (fail_unacked || (g.outcome == k1_led_outcome_fail && test_live)) {
    display = k1_led_display_test_fail;
    if (g.activity == k1_led_activity_wait) cue = k1_led_cue_wait;
    else if (g.warning && g.stall_latched) cue = k1_led_cue_stall;
    else if (test_live && progressing) cue = k1_led_cue_progress;
    else if (finalizing) cue = k1_led_cue_finalizing;
  } else if (incomplete_unacked) {
    display = k1_led_display_test_incomplete;
  } else if (g.warning) {
    display = k1_led_display_warning;
    if (test_live && progressing) cue = k1_led_cue_progress;
  } else if (g.activity == k1_led_activity_wait) {
    display = k1_led_display_wait_operator;
  } else if (finalizing) {
    display = k1_led_display_finalizing;
  } else if (test_live) {
    display = k1_led_display_test_running;
  } else if (pass_unacked) {
    display = k1_led_display_test_pass;
  } else if (g.activity == k1_led_activity_booting) {
    display = k1_led_display_booting;
  } else if (g.boot_ok) {
    display = k1_led_display_boot_ok;
  } else if (g.activity == k1_led_activity_running) {
    display = k1_led_display_running;
  } else {
    display = k1_led_display_ready;
  }
  if (g.identify && (display == k1_led_display_ready || display == k1_led_display_running) &&
      healthy_idle()) {
    display = k1_led_display_identify;
  }
  if (display != g.display || cue != g.cue) {
    g.epoch_ms = now_ms;
    g.in_progress_group = 0;
  }
  g.display = display;
  g.cue = cue;
}

static void note_transition(uint32_t now_ms) { resolve(now_ms); }

void k1_status_led_init(uint32_t now_ms) {
  memset(&g, 0, sizeof(g));
  g.session = now_ms ^ 0xA5A5u;
  g.last_mask = 0xffu;
  g.activity = k1_led_activity_booting;
  set_reason("booting");
  (void)titan_status_gpio_init();
  g.last_gpio_error = (uint32_t)titan_status_gpio_last_error();
  g.epoch_ms = now_ms;
  g.last_poll_ms = now_ms;
  g.display = k1_led_display_booting;
  (void)titan_status_gpio_write(k1_led_colour_yellow);
  g.last_mask = k1_led_colour_yellow;
}

void k1_status_led_set_activity(uint8_t activity, uint32_t now_ms) {
  if (g.activity == activity) return;
  g.activity = activity;
  if (activity == k1_led_activity_test) {
    g.hop.armed = 1;
    g.hop.baseline = g.hop.last_seq;
    g.hop.group_seq = g.hop.last_seq;
    g.hop.fresh = 0;
    g.last_progress_ms = now_ms;
  }
  if (activity != k1_led_activity_wait) g.wait_until = 0;
  cancel_identify();
  note_transition(now_ms);
}

void k1_status_led_boot_ok(uint32_t now_ms) {
  g.boot_ok = 1;
  if (g.activity == k1_led_activity_booting) g.activity = k1_led_activity_ready;
  set_reason("boot_ok");
  note_transition(now_ms);
}

void k1_status_led_observe_control(uint32_t now_ms) {
  g.control.last_seq += 1u;
  g.control.last_ms = now_ms;
  g.control.fresh = 1;
}

static int seq_advanced(uint32_t last, uint32_t baseline) {
  return (int32_t)(last - baseline) > 0;
}

void k1_status_led_observe_hop(uint32_t sequence, uint32_t now_ms) {
  if (!g.hop.armed) {
    g.hop.baseline = sequence;
    g.hop.group_seq = sequence;
    g.hop.armed = 1;
  }
  if (sequence != g.hop.last_seq) {
    if (seq_advanced(sequence, g.hop.last_seq) || g.hop.last_seq == 0) {
      if (g.warning && g.stall_latched) g.stall_recoveries += 1u;
      g.last_progress_ms = now_ms;
      g.hop.fresh = 1;
    }
    g.hop.last_seq = sequence;
    g.hop.last_ms = now_ms;
  }
}

void k1_status_led_test_begin(uint64_t run_id, uint32_t now_ms) {
  if (g.activity == k1_led_activity_test || g.activity == k1_led_activity_finalizing) return;
  g.run_id = run_id ? run_id : ((uint64_t)g.session << 32) | now_ms;
  g.outcome = k1_led_outcome_pending;
  g.outcome_acked = 0;
  g.fail_flags = 0;
  g.first_fail = 0;
  g.completed = 0;
  g.requested = 0;
  g.digest = 0;
  g.verdict_until = 0;
  g.warning = 0;
  g.stall_latched = 0;
  g.stall_recoveries = 0;
  g.hop.armed = 1;
  g.hop.fresh = 0;
  g.hop.baseline = g.hop.last_seq;
  g.hop.group_seq = g.hop.last_seq;
  set_reason("test_running");
  cancel_identify();
  k1_status_led_set_activity(k1_led_activity_test, now_ms);
}

void k1_status_led_test_fail(uint32_t flag, const char *reason, uint32_t now_ms) {
  if (!g.first_fail) g.first_fail = flag;
  g.fail_flags |= flag;
  g.outcome = k1_led_outcome_fail;
  g.outcome_acked = 0;
  if (reason) set_reason(reason);
  cancel_identify();
  note_transition(now_ms);
}

void k1_status_led_execution_end(uint32_t completed, uint32_t fail_flags, uint32_t now_ms) {
  g.completed = completed;
  g.fail_flags |= fail_flags;
  if (fail_flags && !g.first_fail) g.first_fail = fail_flags;
  if (g.fail_flags) {
    g.outcome = k1_led_outcome_fail;
    if (!g.reason[0]) set_reason("required_check");
    g.outcome_acked = 0;
    cancel_identify();
    k1_status_led_set_activity(k1_led_activity_ready, now_ms);
    return;
  }
  if (g.outcome != k1_led_outcome_fail) {
    g.outcome = k1_led_outcome_pending;
    g.verdict_until = now_ms + K1_LED_VERDICT_MS;
    set_reason("finalizing");
  }
  g.outcome_acked = 0;
  cancel_identify();
  k1_status_led_set_activity(k1_led_activity_finalizing, now_ms);
}

int k1_status_led_verdict(uint64_t run_id, uint8_t outcome, uint32_t digest, uint32_t now_ms) {
  if (g.activity == k1_led_activity_test) return -1;
  if (run_id && g.run_id && run_id != g.run_id) return -2;
  if (g.outcome == k1_led_outcome_fail && outcome == k1_led_outcome_pass) return -3;
  if (g.outcome == k1_led_outcome_pass && outcome == k1_led_outcome_pass && g.digest == digest)
    return 0;
  if (g.outcome == k1_led_outcome_incomplete && outcome == k1_led_outcome_pass) return -4;
  g.digest = digest;
  if (g.outcome != k1_led_outcome_fail) g.outcome = outcome;
  if (g.activity == k1_led_activity_finalizing) g.activity = k1_led_activity_ready;
  g.verdict_until = 0;
  g.outcome_acked = 0;
  if (g.outcome == k1_led_outcome_pass) set_reason("test_pass");
  else if (g.outcome == k1_led_outcome_fail) set_reason("test_fail");
  else set_reason("test_incomplete");
  note_transition(now_ms);
  return 0;
}

int k1_status_led_ack(uint64_t run_id, uint32_t now_ms) {
  if (g.activity == k1_led_activity_test || g.activity == k1_led_activity_finalizing) return -1;
  if (run_id && g.run_id && run_id != g.run_id) return -2;
  if (g.outcome != k1_led_outcome_pass && g.outcome != k1_led_outcome_fail &&
      g.outcome != k1_led_outcome_incomplete)
    return -3;
  g.outcome_acked = 1;
  note_transition(now_ms);
  return 0;
}

int k1_status_led_ack_recovery(uint32_t now_ms) {
  if (g.fatal) return -1;
  g.fatal_acked = 1;
  note_transition(now_ms);
  return 0;
}

int k1_status_led_wait(uint32_t reason, uint32_t now_ms) {
  if (g.wait_until && (now_ms - g.wait_last_set) < K1_LED_WAIT_RENEW_MS) return -1;
  g.wait_reason = reason;
  g.wait_until = now_ms + K1_LED_WAIT_LEASE_MS;
  g.wait_last_set = now_ms;
  cancel_identify();
  k1_status_led_set_activity(k1_led_activity_wait, now_ms);
  set_reason("wait_operator");
  return 0;
}

void k1_status_led_wait_clear(uint32_t now_ms) {
  g.wait_until = 0;
  if (g.activity == k1_led_activity_wait)
    g.activity = (g.outcome == k1_led_outcome_pending) ? k1_led_activity_finalizing
                                                       : k1_led_activity_ready;
  note_transition(now_ms);
}

int k1_status_led_identify(uint32_t now_ms) {
  resolve(now_ms);
  if (g.display != k1_led_display_ready && g.display != k1_led_display_running &&
      g.display != k1_led_display_identify)
    return -1;
  if (!healthy_idle()) return -1;
  g.identify = 1;
  note_transition(now_ms);
  return 0;
}

void k1_status_led_fault(int fatal, const char *reason, uint32_t now_ms) {
  if (fatal) {
    g.fatal = 1;
    g.fatal_acked = 0;
  } else {
    g.warning = 1;
  }
  set_reason(reason);
  cancel_identify();
  note_transition(now_ms);
}

void k1_status_led_usb(int configured, int session, uint32_t now_ms) {
  g.usb_configured = configured ? 1 : 0;
  g.usb_session = session ? 1 : 0;
  (void)now_ms;
}

void k1_status_led2_channels(uint8_t display, uint8_t cue, uint32_t elapsed, int progress_ok,
                             uint8_t *green, uint8_t *yellow) {
  *green = 0;
  *yellow = 0;
  if (display == k1_led_display_wait_operator) {
    *yellow = (uint8_t)((elapsed % 2000u) < 800u);
    return;
  }
  if (display == k1_led_display_warning || cue == k1_led_cue_stall) {
    const uint32_t t = elapsed % 2000u;
    *yellow = (uint8_t)((t < 120u) || (t >= 300u && t < 420u));
    return;
  }
  if (progress_ok) *green = (uint8_t)((elapsed % 1000u) < 100u);
}

static int progress_eligible(void) {
  if (g.display != k1_led_display_running && g.display != k1_led_display_test_running) return 1;
  if (!g.hop.armed) return 0;
  if (!seq_advanced(g.hop.last_seq, g.hop.group_seq)) return 0;
  if (!g.hop.fresh) return 0;
  return 1;
}

static uint32_t led2_budget_cycles = 0xffffffffu;

void k1_status_led_set_led2_budget(uint32_t remaining_cycles) {
  led2_budget_cycles = remaining_cycles;
}

void k1_status_led_poll(uint32_t now_ms, int emit_gpio) {
  if (g.last_poll_ms) {
    const uint32_t gap = now_ms - g.last_poll_ms;
    if (gap > g.max_gap_ms) g.max_gap_ms = gap;
  }
  g.last_poll_ms = now_ms;
  k1_status_led_observe_control(now_ms);
  if (g.wait_until && (int32_t)(now_ms - g.wait_until) >= 0) k1_status_led_wait_clear(now_ms);
  if (g.verdict_until && g.activity == k1_led_activity_finalizing &&
      (int32_t)(now_ms - g.verdict_until) >= 0) {
    if (g.outcome != k1_led_outcome_fail) {
      g.outcome = k1_led_outcome_incomplete;
      g.fail_flags |= k1_led_fail_verdict_timeout;
      set_reason("verdict_timeout");
    }
    g.activity = k1_led_activity_ready;
    g.verdict_until = 0;
  }
  if (g.activity == k1_led_activity_test && g.hop.armed && g.hop.fresh == 0 &&
      (now_ms - g.last_progress_ms) > K1_LED_STALL_MS && seq_advanced(g.hop.last_seq, g.hop.baseline)) {
    /* baseline established but no further hops */
  }
  if (g.activity == k1_led_activity_test && g.hop.armed &&
      (now_ms - g.last_progress_ms) > K1_LED_STALL_MS && seq_advanced(g.hop.last_seq, g.hop.baseline)) {
    if (!g.warning) {
      g.warning = 1;
      g.stall_latched = 1;
      g.stall_recoveries = 0;
      set_reason("progress_stall_hop");
    }
  }
  if (g.warning && g.stall_latched && g.stall_recoveries >= 2u && !g.fatal) {
    g.warning = 0;
    g.stall_recoveries = 0;
  }
  resolve(now_ms);
  int expired = 0;
  uint32_t elapsed = elapsed_ms(now_ms, g.epoch_ms);
  uint8_t mask = k1_status_led_pattern_mask(g.display, g.cue, elapsed, &expired);
  if (expired) {
    if (g.display == k1_led_display_boot_ok) g.boot_ok = 0;
    if (g.display == k1_led_display_identify) g.identify = 0;
    resolve(now_ms);
    elapsed = elapsed_ms(now_ms, g.epoch_ms);
    mask = k1_status_led_pattern_mask(g.display, g.cue, elapsed, 0);
  }
  if (g.display == k1_led_display_running || g.display == k1_led_display_test_running) {
    uint32_t cycle = (g.display == k1_led_display_running) ? 1000u : 1200u;
    uint32_t group = elapsed / cycle;
    if (!g.in_progress_group) {
      if (progress_eligible()) {
        g.in_progress_group = 1;
        g.group_ms = group;
        g.hop.group_seq = g.hop.last_seq;
        g.hop.fresh = 0;
      } else {
        mask = k1_led_colour_off;
      }
    } else if (group != g.group_ms) {
      g.in_progress_group = 0;
      if (progress_eligible()) {
        g.in_progress_group = 1;
        g.group_ms = group;
        g.hop.group_seq = g.hop.last_seq;
        g.hop.fresh = 0;
      } else {
        mask = k1_led_colour_off;
      }
    }
  }
  {
    uint8_t led2_g = 0, led2_y = 0;
    int progress_ok = 0;
    if (g.display == k1_led_display_running || g.display == k1_led_display_test_running)
      progress_ok = g.in_progress_group && progress_eligible();
    else if (g.display != k1_led_display_fatal && g.hop.fresh &&
             seq_advanced(g.hop.last_seq, g.hop.group_seq))
      progress_ok = 1;
    k1_status_led2_channels(g.display, g.cue, elapsed, progress_ok, &led2_g, &led2_y);
    titan_led2_phy_request(led2_g, led2_y);
  }
  if (emit_gpio) {
    (void)titan_status_gpio_write(mask);
    g.last_gpio_error = (uint32_t)titan_status_gpio_last_error();
    g.last_mask = mask;
  }
  (void)titan_led2_phy_service(now_ms, led2_budget_cycles);
}

uint8_t k1_status_led_display(void) { return g.display; }

uint32_t k1_status_led_max_service_cycles(void) { return g.max_service_cycles; }

void k1_status_led_add_service_cycles(uint32_t cycles) {
  if (cycles > g.max_service_cycles) g.max_service_cycles = cycles;
}

int k1_status_led_slim_snapshot(char *output, uint32_t capacity) {
  static const char *const names[] = {"BOOTING",        "BOOT_OK",         "READY",
                                      "RUNNING",        "TEST_RUNNING",    "FINALIZING",
                                      "TEST_PASS",      "TEST_FAIL",       "TEST_INCOMPLETE",
                                      "WAIT_OPERATOR",  "WARNING",         "FATAL",
                                      "IDENTIFY"};
  const char *disp = (g.display < 13) ? names[g.display] : "UNKNOWN";
  const int n = snprintf(output, capacity,
                         "{\"schema\":1,\"slim\":true,\"display\":\"%s\",\"activity\":%u,"
                         "\"outcome\":%u,\"fail_flags\":%lu,\"hop_seq\":%lu,\"led2_ok\":%s}",
                         disp, (unsigned)g.activity, (unsigned)g.outcome,
                         (unsigned long)g.fail_flags, (unsigned long)g.hop.last_seq,
                         titan_led2_phy_capable() && titan_led2_phy_mode_ok() ? "true"
                                                                              : "false");
  return (n > 0 && (uint32_t)n < capacity) ? 0 : -1;
}

int k1_status_led_snapshot(char *output, uint32_t capacity) {
  static const char *const names[] = {"BOOTING",        "BOOT_OK",         "READY",
                                      "RUNNING",        "TEST_RUNNING",    "FINALIZING",
                                      "TEST_PASS",      "TEST_FAIL",       "TEST_INCOMPLETE",
                                      "WAIT_OPERATOR",  "WARNING",         "FATAL",
                                      "IDENTIFY"};
  static const char *const outcomes[] = {"none", "pending", "pass", "fail", "incomplete"};
  const char *disp = (g.display < 13) ? names[g.display] : "UNKNOWN";
  const char *outc = (g.outcome < 5) ? outcomes[g.outcome] : "unknown";
  k1_led2_phy_trace_t led2_trace = {0};
  const int have_led2_trace = titan_led2_phy_trace_latest(&led2_trace) == 0;
  const int n = snprintf(
      output, capacity,
      "{\"schema\":1,\"display\":\"%s\",\"activity\":%u,\"outcome\":\"%s\",\"outcome_acked\":%s,"
      "\"run_id\":%llu,\"fail_flags\":%lu,\"first_fail\":%lu,\"completed\":%lu,\"reason\":\"%s\","
      "\"warning\":%s,\"fatal\":%s,\"usb_configured\":%s,\"usb_session\":%s,\"wait_reason\":%lu,"
      "\"identify\":%s,\"hop_seq\":%lu,\"gpio_error\":%lu,\"max_gap_ms\":%lu,\"max_service_cycles\":%lu,"
      "\"cue\":%u,\"logical_rgb\":%u,"
      "\"led2\":{\"ok\":%s,\"id\":%lu,\"id1\":%u,\"id2\":%u,\"addr\":%u,"
      "\"g\":%u,\"y\":%u,\"applied_valid\":%s,\"reg_readback\":%s,\"err\":%lu,"
      "\"txc\":%s,\"txc_requested\":%s,\"txc_state\":\"unverified\",\"step\":%u,"
      "\"page_unknown\":%s,\"taz\":%u,\"ta0\":%u,\"io\":%u,\"pins\":%u,"
      "\"ta_bitmap\":%lu,\"attempt\":%lu,\"dwt_ok\":%s,\"dwt_hz\":%lu,"
      "\"dwt_delta\":%lu,\"mdio_pfs\":%lu,"
      "\"mdc_pfs\":%lu,\"reset_pfs\":%lu,\"txc_pfs\":%lu,"
      "\"trace\":{\"valid\":%s,\"seq\":%lu,\"attempt\":%lu,\"reset_age_ms\":%lu,"
      "\"addr\":%u,\"reg\":%u,\"ack\":%u,\"io\":%u,\"value\":%u,\"value_valid\":%s}}}",
      disp, (unsigned)g.activity, outc, g.outcome_acked ? "true" : "false",
      (unsigned long long)g.run_id, (unsigned long)g.fail_flags, (unsigned long)g.first_fail,
      (unsigned long)g.completed, g.reason[0] ? g.reason : "",
      g.warning ? "true" : "false", g.fatal ? "true" : "false",
      g.usb_configured ? "true" : "false", g.usb_session ? "true" : "false",
      (unsigned long)g.wait_reason, g.identify ? "true" : "false", (unsigned long)g.hop.last_seq,
      (unsigned long)g.last_gpio_error, (unsigned long)g.max_gap_ms,
      (unsigned long)g.max_service_cycles, (unsigned)g.cue, (unsigned)g.last_mask,
      titan_led2_phy_capable() && titan_led2_phy_mode_ok() &&
              titan_led2_phy_register_readback_ok() && titan_led2_phy_applied_valid() &&
              !titan_led2_phy_page_unknown()
          ? "true"
          : "false",
      (unsigned long)titan_led2_phy_id(), (unsigned)titan_led2_phy_last_id1(),
      (unsigned)titan_led2_phy_last_id2(), (unsigned)titan_led2_phy_last_addr(),
      (unsigned)titan_led2_phy_applied_green(),
      (unsigned)titan_led2_phy_applied_yellow(), titan_led2_phy_applied_valid() ? "true" : "false",
      titan_led2_phy_register_readback_ok() ? "true" : "false", (unsigned long)titan_led2_phy_error(),
      titan_led2_phy_txc_enabled() ? "true" : "false",
      titan_led2_phy_txc_requested() ? "true" : "false", (unsigned)titan_led2_phy_experiment_step(),
      titan_led2_phy_page_unknown() ? "true" : "false", (unsigned)titan_led2_phy_expected_ta_z(),
      (unsigned)titan_led2_phy_expected_ta_zero(),
      (unsigned)titan_led2_phy_expected_io_error(), (unsigned)titan_led2_phy_pin_readback(),
      (unsigned long)titan_led2_phy_ta_zero_bitmap(), (unsigned long)titan_led2_phy_attempt(),
      titan_led2_phy_dwt_ok() ? "true" : "false",
      (unsigned long)titan_led2_phy_dwt_clock_hz(), (unsigned long)titan_led2_phy_dwt_delta(),
      (unsigned long)titan_led2_phy_mdio_pfs(),
      (unsigned long)titan_led2_phy_mdc_pfs(), (unsigned long)titan_led2_phy_reset_pfs(),
      (unsigned long)titan_led2_phy_txc_pfs(), have_led2_trace ? "true" : "false",
      (unsigned long)led2_trace.sequence, (unsigned long)led2_trace.attempt,
      (unsigned long)led2_trace.reset_age_ms, (unsigned)led2_trace.address,
      (unsigned)led2_trace.reg, (unsigned)led2_trace.ack, (unsigned)led2_trace.io_error,
      (unsigned)led2_trace.value, led2_trace.value_valid ? "true" : "false");
  return (n > 0 && (uint32_t)n < capacity) ? 0 : -1;
}
