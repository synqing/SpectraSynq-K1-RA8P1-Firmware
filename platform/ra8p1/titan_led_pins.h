#pragma once

// Match bsp_io.h BSP_IO_PORT_06_PIN_01 / BSP_IO_PORT_00_PIN_04 without
// pulling FSP into this header (K1Scalar CPPPATH does not include bsp_io.h).
#define K1_TITAN_DIN_A 0x0601u /* P601, U18 pin 7 */
#define K1_TITAN_DIN_B 0x0004u /* P004, U18 pin 16; GPIO only, no GPT */
#define K1_TITAN_DIN_B_GPT 0x0604u /* P604, U18 pin 12; pair GPT8 / GTIOC8B */
