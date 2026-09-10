/* Scalar/MVE numeric kernels and reusable fixtures.
   Host-safe: no FSP/RT-Thread/USB. Host C and Python cannot pass as
   executed Arm. CROSS_COMPILED disassembly is not CURRENT_TARGET.
   Campaign finite tolerance is the frozen 1e-05 absolute bound; it is
   not widened after mismatch. Specials are classified, not scored with
   that bound. */
#include <stdint.h>
#include <string.h>

#if defined(__ARM_FEATURE_MVE)
#include <arm_mve.h>
#endif

#define K1_ARM_NUMERIC_MAX_COUNT 256u

static float s_left[K1_ARM_NUMERIC_MAX_COUNT];
static float s_right[K1_ARM_NUMERIC_MAX_COUNT];
static uint32_t s_count;
static float s_scalar;
static float s_chunked;
static int s_prepared;

static float k1_arm_from_bits(uint32_t bits) {
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

uint32_t k1_arm_f32_bits(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

/* 0 finite, 1 nan, 2 +inf, 3 -inf, 4 -0, 5 +0, 6 denormal */
int k1_arm_classify(float value) {
    const uint32_t bits = k1_arm_f32_bits(value);
    const uint32_t exp = (bits >> 23) & 0xffu;
    const uint32_t sig = bits & 0x7fffffu;
    if (exp == 0xffu) return sig ? 1 : ((bits >> 31) ? 3 : 2);
    if (exp == 0u && sig == 0u) return (bits >> 31) ? 4 : 5;
    if (exp == 0u) return 6;
    return 0;
}

__attribute__((noinline)) float k1_arm_dot_scalar(const float *a, const float *b, uint32_t count) {
    float sum = 0.0f;
    uint32_t i;
    if (a == NULL || b == NULL) return k1_arm_from_bits(0x7fc00000u);
    for (i = 0; i < count; ++i) sum += a[i] * b[i];
    return sum;
}

__attribute__((noinline)) float k1_arm_dot_chunked(const float *a, const float *b, uint32_t count) {
    float lane[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float sum;
    uint32_t i = 0;
    if (a == NULL || b == NULL) return k1_arm_from_bits(0x7fc00000u);
    for (; i + 4u <= count; i += 4u) {
        lane[0] += a[i] * b[i];
        lane[1] += a[i + 1u] * b[i + 1u];
        lane[2] += a[i + 2u] * b[i + 2u];
        lane[3] += a[i + 3u] * b[i + 3u];
    }
    sum = (lane[0] + lane[1]) + (lane[2] + lane[3]);
    for (; i < count; ++i) sum += a[i] * b[i];
    return sum;
}

#if defined(__ARM_FEATURE_MVE)
__attribute__((noinline)) float k1_arm_dot_mve(const float *a, const float *b, uint32_t count) {
    float32x4_t acc = vdupq_n_f32(0.0f);
    float lanes[4];
    float result;
    uint32_t i = 0;
    if (a == NULL || b == NULL) return k1_arm_from_bits(0x7fc00000u);
    for (; i + 4u <= count; i += 4u) {
        acc = vfmaq(acc, vld1q(a + i), vld1q(b + i));
    }
    vst1q(lanes, acc);
    result = (lanes[0] + lanes[1]) + (lanes[2] + lanes[3]);
    for (; i < count; ++i) result += a[i] * b[i];
    return result;
}
#endif

int k1_arm_fill_cancellation(float *a, float *b, uint32_t count) {
    uint32_t i;
    if (a == NULL || b == NULL || count == 0u || count > K1_ARM_NUMERIC_MAX_COUNT) return -1;
    for (i = 0; i < count; ++i) {
        const uint32_t lane = i % 3u;
        a[i] = (lane == 0u) ? 1.0e8f : ((lane == 1u) ? 1.0f : -1.0e8f);
        b[i] = 1.0f;
    }
    return 0;
}

int k1_arm_fill_magnitude(float *a, float *b, uint32_t count) {
    uint32_t i;
    if (a == NULL || b == NULL || count == 0u || count > K1_ARM_NUMERIC_MAX_COUNT) return -1;
    for (i = 0; i < count; ++i) {
        if ((i & 1u) == 0u) {
            a[i] = 1.0e8f;
            b[i] = 1.0e-20f;
        } else {
            a[i] = 1.0e-20f;
            b[i] = 1.0e8f;
        }
    }
    return 0;
}

int k1_arm_fill_tail(float *a, float *b, uint32_t *count) {
    static const float left[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    static const float right[5] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    if (a == NULL || b == NULL || count == NULL) return -1;
    memcpy(a, left, sizeof(left));
    memcpy(b, right, sizeof(right));
    *count = 5u;
    return 0;
}

int k1_arm_fill_special(uint32_t case_id, float *a, float *b, uint32_t *count) {
    if (a == NULL || b == NULL || count == NULL) return -1;
    *count = 1u;
    b[0] = 1.0f;
    switch (case_id) {
    case 0u: /* NaN * 1 */
        a[0] = k1_arm_from_bits(0x7fc00000u);
        break;
    case 1u: /* +Inf * 2 */
        a[0] = k1_arm_from_bits(0x7f800000u);
        b[0] = 2.0f;
        break;
    case 2u: /* -Inf * 2 */
        a[0] = k1_arm_from_bits(0xff800000u);
        b[0] = 2.0f;
        break;
    case 3u: /* +Inf * 0 */
        a[0] = k1_arm_from_bits(0x7f800000u);
        b[0] = 0.0f;
        break;
    case 4u: /* +0 * 1 */
        a[0] = k1_arm_from_bits(0x00000000u);
        break;
    case 5u: /* -0 * 1 */
        a[0] = k1_arm_from_bits(0x80000000u);
        break;
    case 6u: /* denormal * 2 */
        a[0] = k1_arm_from_bits(0x00000001u);
        b[0] = 2.0f;
        break;
    default:
        return -1;
    }
    return 0;
}

int k1_arm_special_case_count(void) { return 7; }

int k1_arm_numeric_prepare(void) {
    uint32_t tail_count = 0;
    if (k1_arm_fill_cancellation(s_left, s_right, 12u) != 0) return -1;
    s_count = 12u;
    s_scalar = k1_arm_dot_scalar(s_left, s_right, s_count);
    s_chunked = k1_arm_dot_chunked(s_left, s_right, s_count);
    (void)k1_arm_fill_tail(s_left, s_right, &tail_count);
    s_prepared = 1;
    return 0;
}

int k1_arm_numeric_prepared(void) { return s_prepared; }
uint32_t k1_arm_numeric_count(void) { return s_count; }
float k1_arm_numeric_scalar(void) { return s_scalar; }
float k1_arm_numeric_chunked(void) { return s_chunked; }
int k1_arm_binary_executed(void) { return 0; }
uint32_t k1_arm_numeric_max_count(void) { return K1_ARM_NUMERIC_MAX_COUNT; }
