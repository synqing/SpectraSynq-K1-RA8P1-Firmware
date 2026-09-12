#include "titan_status_gpio.h"
#ifndef K1_STATUS_GPIO_STUB
#include <board.h>
#include "common_data.h"
#include "r_ioport.h"
#endif

#define K1_STATUS_BIT_G (1u << 8)
#define K1_STATUS_BIT_R (1u << 9)
#define K1_STATUS_BIT_B (1u << 10)

static uint32_t last_physical = k1_status_gpio_port_mask;
static int last_error;
#ifdef K1_STATUS_GPIO_STUB
static uint32_t stub_port = k1_status_gpio_port_mask;
#endif

static uint32_t physical_from_logical(uint8_t logical) {
  uint32_t on = 0;
  if (logical & k1_status_gpio_r) on |= K1_STATUS_BIT_R;
  if (logical & k1_status_gpio_g) on |= K1_STATUS_BIT_G;
  if (logical & k1_status_gpio_b) on |= K1_STATUS_BIT_B;
  return (~on) & k1_status_gpio_port_mask;
}

int titan_status_gpio_init(void) {
  last_error = 0;
  last_physical = k1_status_gpio_port_mask;
#ifdef K1_STATUS_GPIO_STUB
  stub_port = k1_status_gpio_port_mask;
  return 0;
#else
  const uint32_t cfg = (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                       (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH |
                       (uint32_t)IOPORT_CFG_DRIVE_HIGH;
  R_BSP_PinAccessEnable();
  fsp_err_t error = R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_08, cfg);
  if (error == FSP_SUCCESS)
    error = R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_09, cfg);
  if (error == FSP_SUCCESS)
    error = R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_10, cfg);
  if (error == FSP_SUCCESS)
    error = R_IOPORT_PortWrite(&g_ioport_ctrl, BSP_IO_PORT_01, k1_status_gpio_port_mask,
                               k1_status_gpio_port_mask);
  R_BSP_PinAccessDisable();
  last_error = (int)error;
  return last_error;
#endif
}

int titan_status_gpio_write(uint8_t logical_rgb) {
  const uint32_t physical = physical_from_logical(logical_rgb);
  if (physical == last_physical) return last_error;
#ifdef K1_STATUS_GPIO_STUB
  stub_port = (stub_port & ~k1_status_gpio_port_mask) | physical;
  last_physical = physical;
  last_error = 0;
  return 0;
#else
  R_BSP_PinAccessEnable();
  const fsp_err_t error =
      R_IOPORT_PortWrite(&g_ioport_ctrl, BSP_IO_PORT_01, physical, k1_status_gpio_port_mask);
  R_BSP_PinAccessDisable();
  if (error == FSP_SUCCESS) last_physical = physical;
  last_error = (int)error;
  return last_error;
#endif
}

uint32_t titan_status_gpio_last_physical(void) { return last_physical; }
int titan_status_gpio_last_error(void) { return last_error; }
#ifdef K1_STATUS_GPIO_STUB
uint32_t titan_status_gpio_stub_port(void) { return stub_port; }
#endif
