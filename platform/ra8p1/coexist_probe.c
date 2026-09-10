/* Paired audio/NPU coexistence bookkeeping. Host-safe: no FSP, USB, PDM, Ethos-U.
   Load-only [127,117,123] cannot satisfy useful-NPU treatment.
   Physical pair stays NOT_RUN until WP13 physical PDM and WP14 useful U55
   CURRENT_TARGET cells exist. G4 O2/O3 already FAILED; do not relabel unrun.
   7500 us deadline is not widened. 6000 us p99 stays unscored. */
#include <stdint.h>
#include <string.h>

#define K1_COEXIST_DEADLINE_US 7500u
#define K1_COEXIST_P99_ALLOCATION_US 6000u
#define K1_COEXIST_MAX_EVENTS 16u
#define K1_COEXIST_G4_O2_MISSES 2005u
#define K1_COEXIST_G4_O3_MISSES 2006u

#define K1_COEXIST_TREATMENT_AUDIO_ONLY 0
#define K1_COEXIST_TREATMENT_USEFUL_NPU 1

#define K1_COEXIST_OK 0
#define K1_COEXIST_MISMATCH_INPUT 1
#define K1_COEXIST_MISMATCH_PROFILE 2
#define K1_COEXIST_MISMATCH_BUILD 3
#define K1_COEXIST_MISMATCH_CLOCK 4
#define K1_COEXIST_TREATMENT_NOT_DECLARED 5
#define K1_COEXIST_LOAD_ONLY 6
#define K1_COEXIST_USEFUL_U55_NOT_ADMITTED 7
#define K1_COEXIST_PHYSICAL_PDM_NOT_RUN 8
#define K1_COEXIST_DEADLINE_WIDENED 9
#define K1_COEXIST_G4_RERUN_AS_UNRUN 10
#define K1_COEXIST_P99_INVENTED 11
#define K1_COEXIST_MISSING_RAW_EVENTS 12
#define K1_COEXIST_BAD_EVENT 13

#define K1_COEXIST_BLOCK_LOAD_ONLY (1u << 0)
#define K1_COEXIST_BLOCK_U55 (1u << 1)
#define K1_COEXIST_BLOCK_PDM (1u << 2)
#define K1_COEXIST_BLOCK_G4_UNRUN (1u << 3)
#define K1_COEXIST_BLOCK_P99 (1u << 4)
#define K1_COEXIST_BLOCK_DEADLINE (1u << 5)

typedef struct {
    uint32_t release_us;
    uint32_t complete_us;
    uint32_t overhead_us;
    uint32_t duration_us;
    uint32_t missed;
} k1_coexist_event_t;

static uint8_t s_input[32];
static uint8_t s_profile[32];
static uint8_t s_build[32];
static uint32_t s_sample_rate_hz;
static uint32_t s_clock_hz;
static uint32_t s_hop_period_us;
static uint32_t s_deadline_us;
static int s_identity_set;
static int s_treatment = -1;
static int s_last_decline = K1_COEXIST_PHYSICAL_PDM_NOT_RUN;
static int s_admitted;
static uint32_t s_blockers;
static uint32_t s_event_count;
static k1_coexist_event_t s_events[K1_COEXIST_MAX_EVENTS];
static uint32_t s_window_start_us;
static uint32_t s_window_end_us;
static uint32_t s_overhead_sum_us;
static int s_physical_not_run = 1;

static int k1_digest_equal(const uint8_t *left, const uint8_t *right) {
    if (left == NULL || right == NULL) return 0;
    return memcmp(left, right, 32) == 0;
}

static int k1_digest_nonzero(const uint8_t *digest) {
    unsigned i;
    if (digest == NULL) return 0;
    for (i = 0; i < 32u; ++i) if (digest[i] != 0u) return 1;
    return 0;
}

static void k1_copy_digest(uint8_t *dst, const uint8_t *src) {
    if (dst == NULL) return;
    if (src == NULL) {
        memset(dst, 0, 32);
        return;
    }
    memcpy(dst, src, 32);
}

static uint32_t k1_coexist_rank_index(uint32_t n, uint32_t p) {
    uint32_t idx;
    if (n == 0u) return 0u;
    idx = (p * n + 99u) / 100u;
    if (idx < 1u) idx = 1u;
    if (idx > n) idx = n;
    return idx - 1u;
}

static void k1_sort_u32(uint32_t *values, uint32_t n) {
    uint32_t i, j, key;
    for (i = 1u; i < n; ++i) {
        key = values[i];
        j = i;
        while (j > 0u && values[j - 1u] > key) {
            values[j] = values[j - 1u];
            --j;
        }
        values[j] = key;
    }
}

void k1_coexist_reset(void) {
    memset(s_input, 0, sizeof(s_input));
    memset(s_profile, 0, sizeof(s_profile));
    memset(s_build, 0, sizeof(s_build));
    s_sample_rate_hz = 0;
    s_clock_hz = 0;
    s_hop_period_us = 0;
    s_deadline_us = K1_COEXIST_DEADLINE_US;
    s_identity_set = 0;
    s_treatment = -1;
    s_last_decline = K1_COEXIST_PHYSICAL_PDM_NOT_RUN;
    s_admitted = 0;
    s_blockers = 0;
    s_event_count = 0;
    memset(s_events, 0, sizeof(s_events));
    s_window_start_us = 0;
    s_window_end_us = 0;
    s_overhead_sum_us = 0;
    s_physical_not_run = 1;
}

int k1_coexist_bind(const uint8_t *input_sha256,
                    const uint8_t *profile_sha256,
                    const uint8_t *build_sha256,
                    uint32_t sample_rate_hz,
                    uint32_t clock_hz,
                    uint32_t hop_period_us,
                    uint32_t deadline_us) {
    k1_coexist_reset();
    if (deadline_us != K1_COEXIST_DEADLINE_US) {
        s_last_decline = K1_COEXIST_DEADLINE_WIDENED;
        s_blockers = K1_COEXIST_BLOCK_DEADLINE;
        return s_last_decline;
    }
    if (hop_period_us != K1_COEXIST_DEADLINE_US) {
        s_last_decline = K1_COEXIST_MISMATCH_CLOCK;
        return s_last_decline;
    }
    if (sample_rate_hz == 0u) {
        s_last_decline = K1_COEXIST_MISMATCH_CLOCK;
        return s_last_decline;
    }
    if (!k1_digest_nonzero(input_sha256) || !k1_digest_nonzero(profile_sha256) ||
        !k1_digest_nonzero(build_sha256)) {
        s_last_decline = K1_COEXIST_MISMATCH_INPUT;
        return s_last_decline;
    }
    k1_copy_digest(s_input, input_sha256);
    k1_copy_digest(s_profile, profile_sha256);
    k1_copy_digest(s_build, build_sha256);
    s_sample_rate_hz = sample_rate_hz;
    s_clock_hz = clock_hz;
    s_hop_period_us = hop_period_us;
    s_deadline_us = deadline_us;
    s_identity_set = 1;
    s_last_decline = K1_COEXIST_OK;
    return s_last_decline;
}

int k1_coexist_set_treatment(int treatment) {
    if (treatment != K1_COEXIST_TREATMENT_AUDIO_ONLY &&
        treatment != K1_COEXIST_TREATMENT_USEFUL_NPU) {
        s_treatment = -1;
        s_last_decline = K1_COEXIST_TREATMENT_NOT_DECLARED;
        return s_last_decline;
    }
    s_treatment = treatment;
    return K1_COEXIST_OK;
}

int k1_coexist_pair_comparable(const uint8_t *a_input,
                               const uint8_t *a_profile,
                               const uint8_t *a_build,
                               uint32_t a_clock,
                               int a_treatment,
                               const uint8_t *b_input,
                               const uint8_t *b_profile,
                               const uint8_t *b_build,
                               uint32_t b_clock,
                               int b_treatment) {
    const int declared =
        (a_treatment == K1_COEXIST_TREATMENT_AUDIO_ONLY &&
         b_treatment == K1_COEXIST_TREATMENT_USEFUL_NPU) ||
        (a_treatment == K1_COEXIST_TREATMENT_USEFUL_NPU &&
         b_treatment == K1_COEXIST_TREATMENT_AUDIO_ONLY);
    if (!declared) {
        s_last_decline = K1_COEXIST_TREATMENT_NOT_DECLARED;
        return s_last_decline;
    }
    if (!k1_digest_equal(a_input, b_input)) {
        s_last_decline = K1_COEXIST_MISMATCH_INPUT;
        return s_last_decline;
    }
    if (!k1_digest_equal(a_profile, b_profile)) {
        s_last_decline = K1_COEXIST_MISMATCH_PROFILE;
        return s_last_decline;
    }
    if (!k1_digest_equal(a_build, b_build)) {
        s_last_decline = K1_COEXIST_MISMATCH_BUILD;
        return s_last_decline;
    }
    if (a_clock != b_clock) {
        s_last_decline = K1_COEXIST_MISMATCH_CLOCK;
        return s_last_decline;
    }
    s_last_decline = K1_COEXIST_OK;
    return s_last_decline;
}

int k1_coexist_record_event(uint32_t release_us, uint32_t complete_us, uint32_t overhead_us) {
    k1_coexist_event_t *event;
    uint32_t duration;
    if (s_event_count >= K1_COEXIST_MAX_EVENTS) {
        s_last_decline = K1_COEXIST_BAD_EVENT;
        return s_last_decline;
    }
    if (complete_us < release_us) {
        s_last_decline = K1_COEXIST_BAD_EVENT;
        return s_last_decline;
    }
    duration = complete_us - release_us;
    event = &s_events[s_event_count];
    event->release_us = release_us;
    event->complete_us = complete_us;
    event->overhead_us = overhead_us;
    event->duration_us = duration;
    event->missed = duration > s_deadline_us ? 1u : 0u;
    if (s_event_count == 0u || release_us < s_window_start_us) s_window_start_us = release_us;
    if (complete_us > s_window_end_us) s_window_end_us = complete_us;
    s_overhead_sum_us += overhead_us;
    s_event_count += 1u;
    return K1_COEXIST_OK;
}

uint32_t k1_coexist_event_count(void) { return s_event_count; }

uint32_t k1_coexist_duration_at(uint32_t index) {
    if (index >= s_event_count) return 0u;
    return s_events[index].duration_us;
}

uint32_t k1_coexist_release_at(uint32_t index) {
    if (index >= s_event_count) return 0u;
    return s_events[index].release_us;
}

uint32_t k1_coexist_complete_at(uint32_t index) {
    if (index >= s_event_count) return 0u;
    return s_events[index].complete_us;
}

uint32_t k1_coexist_overhead_at(uint32_t index) {
    if (index >= s_event_count) return 0u;
    return s_events[index].overhead_us;
}

uint32_t k1_coexist_missed_at(uint32_t index) {
    if (index >= s_event_count) return 0u;
    return s_events[index].missed;
}

uint32_t k1_coexist_deadline_misses(void) {
    uint32_t i, misses = 0;
    for (i = 0; i < s_event_count; ++i) misses += s_events[i].missed;
    return misses;
}

uint32_t k1_coexist_percentile(uint32_t p) {
    uint32_t copy[K1_COEXIST_MAX_EVENTS];
    uint32_t i;
    if (s_event_count == 0u || p > 100u) return 0u;
    for (i = 0; i < s_event_count; ++i) copy[i] = s_events[i].duration_us;
    k1_sort_u32(copy, s_event_count);
    return copy[k1_coexist_rank_index(s_event_count, p)];
}

uint32_t k1_coexist_max_us(void) {
    uint32_t i, maximum = 0;
    for (i = 0; i < s_event_count; ++i) {
        if (s_events[i].duration_us > maximum) maximum = s_events[i].duration_us;
    }
    return maximum;
}

int k1_coexist_has_distribution(void) { return s_event_count > 0u ? 1 : 0; }
int k1_coexist_p99_scored(void) { return 0; }
uint32_t k1_coexist_p99_allocation_us(void) { return K1_COEXIST_P99_ALLOCATION_US; }
uint32_t k1_coexist_deadline_us(void) { return s_deadline_us ? s_deadline_us : K1_COEXIST_DEADLINE_US; }
uint32_t k1_coexist_hop_period_us(void) { return s_hop_period_us; }
uint32_t k1_coexist_sample_rate_hz(void) { return s_sample_rate_hz; }
uint32_t k1_coexist_clock_hz(void) { return s_clock_hz; }
uint32_t k1_coexist_window_start_us(void) { return s_window_start_us; }
uint32_t k1_coexist_window_end_us(void) { return s_window_end_us; }
uint32_t k1_coexist_overhead_sum_us(void) { return s_overhead_sum_us; }
int k1_coexist_identity_set(void) { return s_identity_set; }
int k1_coexist_treatment(void) { return s_treatment; }
int k1_coexist_last_decline(void) { return s_last_decline; }
int k1_coexist_admitted(void) { return s_admitted; }
uint32_t k1_coexist_blockers(void) { return s_blockers; }
int k1_coexist_physical_not_run(void) { return s_physical_not_run; }
uint32_t k1_coexist_g4_o2_misses(void) { return K1_COEXIST_G4_O2_MISSES; }
uint32_t k1_coexist_g4_o3_misses(void) { return K1_COEXIST_G4_O3_MISSES; }
int k1_coexist_g4_already_failed(void) { return 1; }

const char *k1_coexist_g4_status_text(void) { return "FAILED"; }
const char *k1_coexist_g6_text(void) { return "NO_QUALIFYING_CANDIDATE"; }

int k1_coexist_refuse_widen(uint32_t claimed_deadline_us) {
    return claimed_deadline_us == K1_COEXIST_DEADLINE_US ? 0 : 1;
}

int k1_coexist_is_load_only_output(const uint8_t *raw, uint32_t length) {
    static const uint8_t expected[3] = {127u, 117u, 123u};
    if (raw == NULL || length != 3u) return 0;
    return memcmp(raw, expected, 3) == 0;
}

void k1_coexist_copy_input(uint8_t *out) { k1_copy_digest(out, s_input); }
void k1_coexist_copy_profile(uint8_t *out) { k1_copy_digest(out, s_profile); }
void k1_coexist_copy_build(uint8_t *out) { k1_copy_digest(out, s_build); }

int k1_coexist_admit(int physical_pdm_complete,
                     int useful_u55_admitted,
                     int load_only,
                     int g4_as_unrun,
                     int claimed_p99_pass,
                     uint32_t claimed_deadline_us) {
    s_admitted = 0;
    s_blockers = 0;
    s_physical_not_run = 1;
    if (claimed_deadline_us != K1_COEXIST_DEADLINE_US) {
        s_blockers |= K1_COEXIST_BLOCK_DEADLINE;
        s_last_decline = K1_COEXIST_DEADLINE_WIDENED;
        return s_last_decline;
    }
    if (claimed_p99_pass) {
        s_blockers |= K1_COEXIST_BLOCK_P99;
        s_last_decline = K1_COEXIST_P99_INVENTED;
        return s_last_decline;
    }
    if (g4_as_unrun) {
        s_blockers |= K1_COEXIST_BLOCK_G4_UNRUN;
        s_last_decline = K1_COEXIST_G4_RERUN_AS_UNRUN;
        return s_last_decline;
    }
    if (load_only) {
        s_blockers |= K1_COEXIST_BLOCK_LOAD_ONLY;
        s_last_decline = K1_COEXIST_LOAD_ONLY;
        return s_last_decline;
    }
    if (!useful_u55_admitted) s_blockers |= K1_COEXIST_BLOCK_U55;
    if (!physical_pdm_complete) s_blockers |= K1_COEXIST_BLOCK_PDM;
    if (!useful_u55_admitted) {
        s_last_decline = K1_COEXIST_USEFUL_U55_NOT_ADMITTED;
        return s_last_decline;
    }
    if (!physical_pdm_complete) {
        s_last_decline = K1_COEXIST_PHYSICAL_PDM_NOT_RUN;
        return s_last_decline;
    }
    /* Identity/gate only. This module never opens a board. */
    s_admitted = 1;
    s_physical_not_run = 1;
    s_last_decline = K1_COEXIST_OK;
    return s_last_decline;
}

int k1_coexist_probe(void) {
    static const uint8_t load_only[3] = {127u, 117u, 123u};
    (void)load_only;
    /* Default campaign state: load-only NPU, no physical PDM, no useful U55. */
    return k1_coexist_admit(0, 0, 1, 0, 0, K1_COEXIST_DEADLINE_US);
}

int k1_coexist_acquire(void) {
    /* Never opens USB, flash, serial, audio, or Ethos-U. */
    s_physical_not_run = 1;
    if (!s_admitted) return -1;
    return -1;
}
