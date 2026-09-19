#include "k1_double_probe.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
uint32_t k1_cycle_count(void);
#ifdef __cplusplus
}
#endif

static uint32_t g_calls;
static uint32_t g_cycles;

void k1_double_probe_reset(void)
{
    g_calls = 0;
    g_cycles = 0;
}

void k1_double_probe_snapshot(uint32_t *calls, uint32_t *cycles)
{
    if (calls) {
        *calls = g_calls;
    }
    if (cycles) {
        *cycles = g_cycles;
    }
}

static void note(uint32_t started)
{
    ++g_calls;
    g_cycles += k1_cycle_count() - started;
}

void k1_double_probe_inject_for_test(uint32_t calls)
{
    uint32_t i;
    for (i = 0; i < calls; ++i) {
        const uint32_t started = k1_cycle_count();
        note(started);
    }
}

#ifndef K1_DOUBLE_PROBE_HOST

#define WRAP_DD(name)                                                          \
    double __real_##name(double, double);                                      \
    double __wrap_##name(double a, double b)                                   \
    {                                                                          \
        const uint32_t started = k1_cycle_count();                             \
        const double result = __real_##name(a, b);                             \
        note(started);                                                         \
        return result;                                                         \
    }

#define WRAP_D_F(name)                                                         \
    float __real_##name(double);                                               \
    float __wrap_##name(double value)                                          \
    {                                                                          \
        const uint32_t started = k1_cycle_count();                             \
        const float result = __real_##name(value);                             \
        note(started);                                                         \
        return result;                                                         \
    }

#define WRAP_D_I(name)                                                         \
    int __real_##name(double);                                                 \
    int __wrap_##name(double value)                                            \
    {                                                                          \
        const uint32_t started = k1_cycle_count();                             \
        const int result = __real_##name(value);                               \
        note(started);                                                         \
        return result;                                                         \
    }

#define WRAP_CMP(name)                                                         \
    int __real_##name(double, double);                                         \
    int __wrap_##name(double a, double b)                                      \
    {                                                                          \
        const uint32_t started = k1_cycle_count();                             \
        const int result = __real_##name(a, b);                                \
        note(started);                                                         \
        return result;                                                         \
    }

WRAP_DD(__aeabi_dadd)
WRAP_DD(__aeabi_dsub)
WRAP_DD(__aeabi_dmul)
WRAP_DD(__aeabi_ddiv)
WRAP_DD(__aeabi_drsub)
WRAP_D_F(__aeabi_d2f)
WRAP_D_I(__aeabi_d2iz)
WRAP_CMP(__aeabi_dcmpeq)
WRAP_CMP(__aeabi_dcmplt)
WRAP_CMP(__aeabi_dcmple)
WRAP_CMP(__aeabi_dcmpge)
WRAP_CMP(__aeabi_dcmpgt)
WRAP_CMP(__aeabi_dcmpun)

unsigned long long __real___aeabi_d2ulz(double);
unsigned long long __wrap___aeabi_d2ulz(double value)
{
    const uint32_t started = k1_cycle_count();
    const unsigned long long result = __real___aeabi_d2ulz(value);
    note(started);
    return result;
}

double __real___aeabi_i2d(int);
double __wrap___aeabi_i2d(int value)
{
    const uint32_t started = k1_cycle_count();
    const double result = __real___aeabi_i2d(value);
    note(started);
    return result;
}

double __real___aeabi_ui2d(unsigned);
double __wrap___aeabi_ui2d(unsigned value)
{
    const uint32_t started = k1_cycle_count();
    const double result = __real___aeabi_ui2d(value);
    note(started);
    return result;
}

double __real___aeabi_l2d(long long);
double __wrap___aeabi_l2d(long long value)
{
    const uint32_t started = k1_cycle_count();
    const double result = __real___aeabi_l2d(value);
    note(started);
    return result;
}

#endif
