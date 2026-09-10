#include "ws281x_diag.h"
#include "titan_led_pins.h"
#include <board.h>
#include "common_data.h"
#include "r_ioport.h"

static uint32_t cycles_for_ns(uint32_t ns, uint32_t hz) {
    return (uint32_t)(((uint64_t)ns * hz + 999999999ULL) / 1000000000ULL);
}

int k1_ws281x_diag_emit(const uint8_t *bytes, size_t size, uint32_t profile,
                       uint32_t pin, uint32_t hz,
                       k1_ws281x_diag_result_t *result) {
    k1_ws281x_diag_timing_t timing;
    k1_ws281x_diag_result_t local = {0};
    if (!bytes || !result || !size || size > K1_WS281X_DIAG_MAX_BYTES || pin > 1u ||
        hz < 48000000u || hz > 1200000000u ||
        !k1_ws281x_diag_profile(profile, &timing) ||
        size % timing.bytes_per_pixel ||
        size / timing.bytes_per_pixel > K1_WS281X_DIAG_MAX_PIXELS ||
        !(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) return -1;

    const unsigned port_number = pin == 0u ? 6u : 0u;
    const unsigned pin_number = pin == 0u ? 1u : 4u;
    const bsp_io_port_pin_t selected = (bsp_io_port_pin_t)
        (pin == 0u ? K1_TITAN_DIN_A : K1_TITAN_DIN_B);
    volatile uint32_t *const pfs = &R_PFS->PORT[port_number].PIN[pin_number].PmnPFS;
    volatile uint32_t *const output = pin == 0u ? &R_PORT6->PCNTR3 : &R_PORT0->PCNTR3;
    const uint32_t high = 1u << pin_number, low = high << 16;
    const uint32_t config = IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                           IOPORT_CFG_PORT_OUTPUT_LOW | IOPORT_CFG_DRIVE_HIGH;
    local.pfs_before = *pfs;
    R_BSP_PinAccessEnable();
    const fsp_err_t error = R_IOPORT_PinCfg(&g_ioport_ctrl, selected, config);
    R_BSP_PinAccessDisable();
    local.pin_config_error = (int32_t)error;
    local.pfs_after = *pfs;
    *result = local;
    const uint32_t mode_mask = R_PFS_PORT_PIN_PmnPFS_PDR_Msk |
                              R_PFS_PORT_PIN_PmnPFS_PMR_Msk |
                              R_PFS_PORT_PIN_PmnPFS_ASEL_Msk;
    if (error != FSP_SUCCESS ||
        (local.pfs_after & mode_mask) != R_PFS_PORT_PIN_PmnPFS_PDR_Msk) return -2;

    const uint32_t t0h = cycles_for_ns(timing.t0h_ns, hz);
    const uint32_t t1h = cycles_for_ns(timing.t1h_ns, hz);
    const uint32_t period = cycles_for_ns(timing.period_ns, hz);
    const uint32_t reset = cycles_for_ns(timing.reset_us * 1000u, hz);
    *output = low;
    __DSB();
    uint32_t started = DWT->CYCCNT;
    while ((uint32_t)(DWT->CYCCNT - started) < reset) {}

    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    const uint32_t emit_start = DWT->CYCCNT;
    uint32_t next = emit_start, previous = 0, bit_min = UINT32_MAX, bit_max = 0;
    int have_previous = 0;
    for (size_t byte = 0; byte < size; ++byte) {
        for (int bit = 7; bit >= 0; --bit) {
            const uint32_t high_time = (bytes[byte] & (1u << bit)) ? t1h : t0h;
            while ((int32_t)(DWT->CYCCNT - next) < 0) {}
            const uint32_t rise_marker = DWT->CYCCNT;
            *output = high;
            while ((uint32_t)(DWT->CYCCNT - rise_marker) < high_time) {}
            *output = low;
            /* Statistics run in the low phase, not between timestamp and rise. */
            if (have_previous) {
                const uint32_t interval = rise_marker - previous;
                if (interval < bit_min) bit_min = interval;
                if (interval > bit_max) bit_max = interval;
            }
            previous = rise_marker;
            have_previous = 1;
            next = rise_marker + period;
        }
    }
    while ((int32_t)(DWT->CYCCNT - next) < 0) {}
    *output = low;
    __DSB();
    const uint32_t ended = DWT->CYCCNT;
    __set_PRIMASK(primask);
    /* Reset remains low with the caller's interrupt state restored. */
    while ((uint32_t)(DWT->CYCCNT - ended) < reset) {}
    local.emit_cycles = ended - emit_start;
    local.latch_cycles = DWT->CYCCNT - ended;
    local.bit_period_min_cycles = bit_min == UINT32_MAX ? 0u : bit_min;
    local.bit_period_max_cycles = bit_max;
    local.pfs_after = *pfs;
    *result = local;
    return 0;
}
