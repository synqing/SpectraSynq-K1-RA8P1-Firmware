#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void k1_fixture_initialise(const uint8_t uid[16], uint32_t clock_hz, uint32_t cpu_wait);
void k1_fixture_consume(const uint8_t* bytes, size_t count, uint32_t tick_ms);
void k1_fixture_poll(uint32_t tick_ms);
void k1_fixture_disconnect(void);
const uint8_t* k1_fixture_reply(size_t* count);
void k1_fixture_sent(void);
uint32_t k1_cycle_count(void);
size_t k1_platform_metrics(char* output, size_t capacity);
void k1_fixture_schedule_step(void);
bool k1_fixture_schedule_active(void);
uint32_t k1_fixture_release_remaining_cycles(void);
void k1_note_usb_event(uint32_t event);
#ifdef __cplusplus
}
#endif
