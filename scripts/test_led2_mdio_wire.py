#!/usr/bin/env python3
"""Test actual LED2 production MDIO functions using an edge-driven PHY model.

Usage: python3 scripts/test_led2_mdio_wire.py --source platform/ra8p1/titan_led2_phy.c
Only the hardware GPIO API and DWT delay are replaced. K1_LED2_PHY_STUB is
not enabled. This checks software framing/ownership, not silicon or optics.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

PRELUDE = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef int fsp_err_t;
typedef int bsp_io_level_t;
typedef unsigned bsp_io_port_pin_t;
#define FSP_SUCCESS 0
#define BSP_IO_LEVEL_HIGH 1
#define BSP_IO_LEVEL_LOW 0
#define MDIO_PIN 0x0C0C
#define MDC_PIN 0x0C0B
#define MDIO_PHYADDR 1u
#define IOPORT_CFG_NMOS_ENABLE 0x40u
#define IOPORT_CFG_PORT_DIRECTION_OUTPUT 0x04u
#define IOPORT_CFG_PORT_DIRECTION_INPUT 0u
#define IOPORT_CFG_PORT_OUTPUT_HIGH 0x01u
#define IOPORT_CFG_PULLUP_ENABLE 0x10u
#define R_PFS_PORT_PIN_PmnPFS_PDR_Msk 0x00000004u
#define R_PFS_PORT_PIN_PmnPFS_ASEL_Msk 0x00008000u
#define R_PFS_PORT_PIN_PmnPFS_PMR_Msk 0x00010000u
#define R_PFS_PORT_PIN_PmnPFS_PSEL_Msk 0x1f000000u
#define R_BSP_PinAccessEnable() ((void)0)
#define R_BSP_PinAccessDisable() ((void)0)
typedef struct { volatile uint32_t PmnPFS; } fake_pfs_pin_t;
typedef struct { fake_pfs_pin_t PIN[16]; } fake_pfs_port_t;
typedef struct { fake_pfs_port_t PORT[16]; } fake_pfs_t;
static fake_pfs_t fake_pfs;
#define R_PFS (&fake_pfs)
#define trace_record(...) ((void)0)
static int g_ioport_ctrl, expected_io_error;
static uint32_t write_count, ta_zero_bitmap, mdio_pfs, mdc_pfs;
static uint8_t expected_ta_z, expected_ta_zero, pin_readback;
static uint64_t fake_now, pending_due;
static unsigned request_bits, rising_edges, data_edges, fake_latency;
static uint64_t request;
static int pin_mdc, host_output, host_one, phy_one, pending_one;
static int responder, protocol_errors, fake_present, fake_z;
static unsigned reply_index;
static uint16_t fake_word;
static int valid_write;
static unsigned input_attempts, mdio_reads, fail_read_n;
static unsigned pin_write_calls, fail_pin_write_n;
static int fail_input_count;

static void settle(void) {
  if (fake_now >= pending_due) phy_one = pending_one;
}
static void rising(void) {
  rising_edges++;
  settle();
  if (host_output) {
    request = (request << 1) | !!host_one;
    request_bits++;
    return;
  }
  if (!data_edges) {
    unsigned addr = 0, reg = 0, st = 0, op = 0;
    if (request_bits == 46) {
      uint64_t pre = request >> 14;
      st = (request >> 12) & 3;
      op = (request >> 10) & 3;
      addr = (request >> 5) & 31;
      reg = request & 31;
      responder = fake_present && pre == 0xffffffffULL && st == 1 && op == 2 &&
        addr == 1 && (reg == 2 || reg == 3);
      if (pre != 0xffffffffULL || st != 1 || op != 2 || addr != 1 || (reg != 2 && reg != 3)) protocol_errors++;
    } else if (request_bits == 64) {
      uint64_t pre = request >> 32;
      st = (request >> 30) & 3;
      op = (request >> 28) & 3;
      addr = (request >> 23) & 31;
      reg = (request >> 18) & 31;
      valid_write = pre == 0xffffffffULL && st == 1 && op == 1 && addr == 1 &&
        reg == 16 && ((request >> 16) & 3) == 2 && (request & 0xffff) == 0xa55a;
      if (!valid_write) protocol_errors++;
      responder = 0;
    } else {
      protocol_errors++;
      responder = 0;
    }
    reply_index = 0;
  }
  data_edges++;
  if (!responder) pending_one = 1;
  else if (reply_index == 0) pending_one = 0; /* ACK, after first released rising edge */
  else if (reply_index <= 16) pending_one = (fake_word >> (16 - reply_index)) & 1;
  else pending_one = 1; /* idle */
  reply_index++;
  pending_due = fake_now + fake_latency;
}
static int R_IOPORT_PinCfg(void *ctrl, unsigned pin, unsigned cfg) {
  (void)ctrl;
  if (pin != MDIO_PIN) return -1;
  int output = !!(cfg & IOPORT_CFG_PORT_DIRECTION_OUTPUT);
  if (!output) {
    input_attempts++;
    if (fail_input_count) {
      if (fail_input_count > 0) fail_input_count--;
      return -1; /* Configuration failure does not change pad direction. */
    }
  }
  if (output && !host_output) {
    if (responder && data_edges && reply_index < 18) protocol_errors++;
    request_bits = data_edges = reply_index = 0;
    request = 0;
    responder = 0;
    phy_one = pending_one = 1;
    pending_due = UINT64_MAX;
  }
  if (!output && host_output) {
    /* Z is diagnostic only. Its input sample may be either logic level. */
    phy_one = fake_present ? fake_z : 1;
    pending_due = UINT64_MAX;
  }
  host_output = output;
  host_one = !!(cfg & IOPORT_CFG_PORT_OUTPUT_HIGH);
  fake_pfs.PORT[12].PIN[12].PmnPFS = cfg;
  return 0;
}
static int R_IOPORT_PinWrite(void *ctrl, unsigned pin, int level) {
  (void)ctrl;
  pin_write_calls++;
  if (fail_pin_write_n && pin_write_calls == fail_pin_write_n) return -1;
  if (pin == MDIO_PIN) host_one = level;
  else if (pin == MDC_PIN) {
    if (!pin_mdc && level) rising();
    pin_mdc = level;
  } else return -1;
  return 0;
}
static int R_IOPORT_PinRead(void *ctrl, unsigned pin, int *level) {
  (void)ctrl;
  settle();
  if (pin == MDC_PIN) *level = pin_mdc;
  else if (pin == MDIO_PIN) {
    mdio_reads++;
    if (fail_read_n && mdio_reads == fail_read_n) return -1;
    *level = (host_output ? host_one : 1) & phy_one;
  }
  else return -1;
  return 0;
}
static void reset_model(uint16_t word, unsigned latency, int z, int present) {
  fake_now = 0; pending_due = UINT64_MAX;
  request_bits = rising_edges = data_edges = 0;
  request = 0; pin_mdc = 0; host_output = 0; host_one = phy_one = pending_one = 1;
  responder = protocol_errors = valid_write = 0; reply_index = 0;
  fake_word = word; fake_latency = latency; fake_z = z; fake_present = present;
  expected_ta_z = expected_ta_zero = 2; write_count = ta_zero_bitmap = 0;
  expected_io_error = 0;
  input_attempts = mdio_reads = fail_read_n = 0; fail_input_count = 0;
  pin_write_calls = fail_pin_write_n = 0;
  fake_pfs.PORT[12].PIN[11].PmnPFS = IOPORT_CFG_PORT_DIRECTION_OUTPUT;
}
'''

TESTS = r'''
static int failures;
#define CHECK(condition, label) do { if (!(condition)) { \
  fprintf(stderr, "FAIL %s (line %d)\n", label, __LINE__); failures++; } } while (0)
int main(void) {
  const uint16_t words[] = {0x001c, 0xc916, 0x8001, 0xa55a, 0x5aa5, 0x0001, 0xfffe};
  const unsigned latencies[] = {0, 300};
  unsigned cases = 0;
  for (unsigned l = 0; l < 2; l++) for (unsigned z = 0; z < 2; z++) for (unsigned w = 0; w < 7; w++) {
    reset_model(words[w], latencies[l], z, 1);
    uint16_t result = 0xdead;
    int rc = mdio_read(1, w == 0 ? 2 : 3, &result);
    if (rc || result != words[w] || host_output || pin_mdc || protocol_errors || rising_edges != 64) {
      fprintf(stderr, "FAIL read latency=%uns Z=%u word=%04x rc=%d result=%04x output=%d mdc=%d clocks=%u protocol_errors=%d\n",
        latencies[l], z, words[w], rc, result, host_output, pin_mdc, rising_edges, protocol_errors);
      failures++;
    }
    cases++;
  }
  reset_model(0x001c, 300, 1, 0);
  uint16_t result = 0xdead;
  int rc = mdio_read(1, 2, &result);
  CHECK(rc != 0, "absent PHY must fail");
  CHECK(result == 0xdead, "failed read must not publish data");
  CHECK(!host_output && !pin_mdc, "failed read leaves MDIO input and MDC low");
  CHECK(data_edges >= 33, "failed read drains at least 32 clocks after ACK");
  CHECK(ta_zero_bitmap == 0, "absent PHY cannot set ACK bitmap");
  CHECK(protocol_errors == 0, "absent PHY request framing remains valid");
  cases++;
  /* A successful transaction after failure must not depend on stale direction. */
  fake_present = 1; fake_word = 0xc916; fake_z = 0;
  rc = mdio_read(1, 3, &result);
  CHECK(rc == 0 && result == 0xc916, "read after absent PHY recovers exact leading-one word");
  CHECK(!host_output && !pin_mdc && !protocol_errors, "recovery leaves idle and no ownership violation");
  cases++;
  reset_model(0, 300, 1, 1);
  rc = mdio_write(1, 16, 0xa55a);
  CHECK(rc == 0 && valid_write, "write explicitly drives valid full frame from input idle");
  CHECK(!host_output && !pin_mdc && data_edges >= 1, "write leaves released idle with idle clock");
  CHECK(!protocol_errors, "write frame and ownership correct");
  const unsigned successful_write_calls = pin_write_calls;
  cases++;
  reset_model(0, 300, 1, 1);
  fake_pfs.PORT[12].PIN[11].PmnPFS = 0;
  rc = mdio_write(1, 16, 0xa55a);
  CHECK(rc != 0 && !host_output && !pin_mdc,
        "write rejects lost MDC GPIO ownership and leaves released idle");
  cases++;
  /* Every one-shot GPIO write failure in the production write frame must be
     retained and must leave the bus released with MDC low. */
  for (unsigned fault_call = 1; fault_call <= successful_write_calls; ++fault_call) {
    reset_model(0, 300, 1, 1);
    fail_pin_write_n = fault_call;
    rc = mdio_write(1, 16, 0xa55a);
    if (!rc || host_output || pin_mdc || pin_write_calls > successful_write_calls + 2) {
      fprintf(stderr, "FAIL write GPIO call=%u rc=%d output=%d mdc=%d calls=%u baseline=%u\n",
        fault_call, rc, host_output, pin_mdc, pin_write_calls, successful_write_calls);
      failures++;
    }
    cases++;
  }
  reset_model(0, 300, 1, 1);
  fail_input_count = 1;
  rc = mdio_write(1, 16, 0xa55a);
  CHECK(rc != 0 && input_attempts == 2 && !host_output && !pin_mdc,
        "write retries a failed release once and remains failed");
  cases++;
  reset_model(0, 300, 1, 1);
  fail_input_count = -1;
  rc = mdio_write(1, 16, 0xa55a);
  CHECK(rc != 0 && input_attempts == 2 && pin_mdc == 0,
        "write persistent release failure is bounded and lowers MDC");
  cases++;
  /* API faults are independent of PHY framing and must never commit invalid
     data to the caller. These are not fake no-ACK responses. */
  for (unsigned fault = 0; fault < 6; fault++) {
    reset_model(0xc916, 300, 1, fault == 4 ? 0 : 1);
    const char *label = "unknown";
    if (fault == 0) { label = "one input-release failure"; fail_input_count = 1; }
    if (fault == 1) { label = "ACK input read failure"; fail_read_n = 2; }
    if (fault == 2) { label = "data input read failure"; fail_read_n = 3; }
    if (fault == 3) { label = "final idle input read failure"; fail_read_n = 19; }
    if (fault == 4) { label = "failure-drain input read failure"; fail_read_n = 3; }
    if (fault == 5) { label = "persistent input-release failure"; fail_input_count = -1; }
    result = 0xdead;
    rc = mdio_read(1, 3, &result);
    if (!rc || result != 0xdead || expected_io_error != 1 || pin_mdc) {
      fprintf(stderr, "FAIL %s rc=%d result=%04x io=%d mdc=%d\n", label, rc, result, expected_io_error, pin_mdc);
      failures++;
    }
    if ((fault == 0 || fault == 5) && (rising_edges != 46 || mdio_reads != 0 || input_attempts < 2)) {
      fprintf(stderr, "FAIL %s: direction failure must abort sampling and make one bounded release attempt; clocks=%u reads=%u attempts=%u\n",
        label, rising_edges, mdio_reads, input_attempts);
      failures++;
    }
    if (fault == 0 && host_output) { fprintf(stderr, "FAIL transient release fault not cleaned up\n"); failures++; }
    if (fault == 5 && input_attempts > 2) { fprintf(stderr, "FAIL unbounded release retries\n"); failures++; }
    cases++;
  }
  reset_model(0x001c, 300, 1, 1);
  fake_pfs.PORT[12].PIN[11].PmnPFS = 0;
  result = 0xdead;
  rc = mdio_read(1, 2, &result);
  CHECK(rc != 0 && result == 0xdead && !host_output && !pin_mdc,
        "read rejects lost MDC GPIO ownership and publishes no data");
  cases++;
  if (failures) { fprintf(stderr, "K1_LED2_MDIO_WIRE=FAIL failures=%d cases=%u\n", failures, cases); return 1; }
  printf("K1_LED2_MDIO_WIRE=PASS cases=%u (host edge model; no silicon claim)\n", cases);
  return 0;
}
'''

def production_transport(path: Path) -> str:
    source = path.read_text()
    start = source.index("static int mdio_io_error;")
    end = source.index("\n#else\nstatic int stub_fault", start)
    body = source[start:end]
    # Replace only the physical-time seam; preserve production direction,
    # framing, turnaround, receive and cleanup functions verbatim.
    match = re.search(r"static void mdio_delay\(void\)\s*\{", body)
    if not match:
        raise ValueError("Missing production mdio_delay(): update time seam explicitly")
    depth, pos = 1, match.end()
    while depth:
        if body[pos] == "{": depth += 1
        elif body[pos] == "}": depth -= 1
        pos += 1
    body = body[:match.start()] + "static void mdio_delay(void) { fake_now += 2000u; }" + body[pos:]
    return body

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=Path("platform/ra8p1/titan_led2_phy.c"))
    parser.add_argument("--cc", default="cc")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="k1-led2-mdio-wire-") as tmp:
        c = Path(tmp) / "wire.c"
        exe = Path(tmp) / "wire"
        c.write_text(PRELUDE + production_transport(args.source) + TESTS)
        subprocess.run([args.cc, "-std=c11", "-O2", str(c), "-o", str(exe)], check=True)
        return subprocess.run([str(exe)], check=False).returncode

if __name__ == "__main__":
    raise SystemExit(main())
