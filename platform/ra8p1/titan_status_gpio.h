#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum {
  k1_status_gpio_r = 1u,
  k1_status_gpio_g = 2u,
  k1_status_gpio_b = 4u,
  k1_status_gpio_port_mask = 0x0700u
};
int titan_status_gpio_init(void);
int titan_status_gpio_write(uint8_t logical_rgb);
uint32_t titan_status_gpio_last_physical(void);
int titan_status_gpio_last_error(void);
#ifdef __cplusplus
}
#endif
