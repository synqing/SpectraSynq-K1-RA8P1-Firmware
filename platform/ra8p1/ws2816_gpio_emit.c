#include "ws2816_gpio_emit.h"

#include "titan_led_pins.h"

#include <board.h>
#include "common_data.h"
#include "fixture_app.h"
#include "r_ioport.h"

static uint32_t clock_hz;
static int pins_ready;

/* Direct PCNTR3. Do not call through function pointers in the bit loop:
 * the first silicon emit measured 1352–1500 ns/bit from that overhead. */

#define DIN_A_PORT R_PORT6
#define DIN_A_MASK (1U << 1)
#define DIN_B_PORT R_PORT0
#define DIN_B_MASK (1U << 4)

static void configure_pins(void) {
    const uint32_t cfg = (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                         (uint32_t)IOPORT_CFG_PORT_OUTPUT_LOW |
                         (uint32_t)IOPORT_CFG_DRIVE_HIGH;
    R_BSP_PinAccessEnable();
    (void)R_IOPORT_PinCfg(&g_ioport_ctrl, (bsp_io_port_pin_t)K1_TITAN_DIN_A, cfg);
    (void)R_IOPORT_PinCfg(&g_ioport_ctrl, (bsp_io_port_pin_t)K1_TITAN_DIN_B, cfg);
    R_BSP_PinAccessDisable();
    pins_ready = 1;
}

static uint32_t ns_to_cycles(uint32_t ns) {
    if (clock_hz == 0U) return 0U;
    return (uint32_t)(((uint64_t)ns * clock_hz + 500000000ULL) / 1000000000ULL);
}

static void emit_direct(const uint8_t *lane_a, const uint8_t *lane_b,
                        packed_lane_completion_t *local, uint32_t t0h,
                        uint32_t t1h, uint32_t period) {
    uint32_t bit_min = 0xffffffffU;
    uint32_t bit_max = 0;
    uint32_t previous = DWT->CYCCNT;
    int have_previous = 0;
    const uint32_t t1h_use = t1h > t0h ? t1h : t0h;
    const uint32_t period_use = period > t1h_use ? period : t1h_use;
    for (size_t i = 0; i < K1_PACKED_BYTES_PER_LANE; ++i) {
        uint8_t ba = lane_a[i];
        uint8_t bb = lane_b[i];
        for (int bit = 7; bit >= 0; --bit) {
            const int one_a = ((ba >> bit) & 1U) != 0U;
            const int one_b = ((bb >> bit) & 1U) != 0U;
            const uint32_t start = DWT->CYCCNT;
            if (have_previous) {
                const uint32_t dt = start - previous;
                if (dt < bit_min) bit_min = dt;
                if (dt > bit_max) bit_max = dt;
            }
            previous = start;
            have_previous = 1;
            DIN_A_PORT->PCNTR3 = DIN_A_MASK;
            DIN_B_PORT->PCNTR3 = DIN_B_MASK;
            while ((DWT->CYCCNT - start) < t0h) {
            }
            if (!one_a) DIN_A_PORT->PCNTR3 = (DIN_A_MASK << 16);
            if (!one_b) DIN_B_PORT->PCNTR3 = (DIN_B_MASK << 16);
            while ((DWT->CYCCNT - start) < t1h_use) {
            }
            DIN_A_PORT->PCNTR3 = (DIN_A_MASK << 16);
            DIN_B_PORT->PCNTR3 = (DIN_B_MASK << 16);
            while ((DWT->CYCCNT - start) < period_use) {
            }
        }
    }
    local->bit_period_min_cycles =
        (have_previous && bit_min != 0xffffffffU) ? bit_min : 0;
    local->bit_period_max_cycles = bit_max;
}

void k1_ws2816_set_clock(uint32_t hz) { clock_hz = hz; }

packed_submit_result_t k1_ws2816_submit_packed_lanes(
    const uint8_t *lane_a, size_t a_bytes, const uint8_t *lane_b,
    size_t b_bytes, packed_lane_completion_t *completion) {
    packed_lane_completion_t local = {0};
    uint32_t irq;
    uint32_t submit;
    uint32_t transfer_done;
    uint32_t latch_start;
    uint32_t latch_ready;
    uint32_t t0h;
    uint32_t t1h;
    uint32_t period;
    uint32_t latch;
    if (lane_a == NULL || lane_b == NULL ||
        k1_ws2816_require_packed_lanes(a_bytes, b_bytes) != kPackedAccepted) {
        return kPackedWrongCount;
    }
    if (!pins_ready) configure_pins();
    t0h = ns_to_cycles(250U);
    t1h = ns_to_cycles(650U);
    period = ns_to_cycles(1250U);
    latch = ns_to_cycles(300000U);
    irq = __get_PRIMASK();
    __disable_irq();
    submit = DWT->CYCCNT;
    emit_direct(lane_a, lane_b, &local, t0h, t1h, period);
    transfer_done = DWT->CYCCNT;
    latch_start = transfer_done;
    while ((DWT->CYCCNT - latch_start) < latch) {
    }
    latch_ready = DWT->CYCCNT;
    if (!irq) {
        __enable_irq();
    }
    local.submit_cycles = submit;
    local.transfer_done_cycles = transfer_done;
    local.latch_ready_cycles = latch_ready;
    local.emit_cycles = transfer_done - submit;
    local.latch_cycles = latch_ready - transfer_done;
    if (completion != NULL) *completion = local;
    return kPackedAccepted;
}
