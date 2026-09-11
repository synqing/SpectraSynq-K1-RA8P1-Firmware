#pragma once
#include <stdbool.h>
#include <stdint.h>

enum { K1_SEMANTIC_SOURCE_ID = 0x4b315331U };

typedef struct k1_semantic_sample {
    uint32_t source_id;
    uint32_t sequence;
    uint64_t event_us;
    uint64_t available_us;
    float values[4];
} k1_semantic_sample_t;

typedef struct k1_semantic_state {
    uint32_t last_sequence;
    uint64_t last_available_us;
    uint32_t accepted;
    uint32_t loss;
    uint32_t delayed;
    uint32_t stale;
    uint32_t out_of_order;
    uint32_t invalid_identity;
    uint32_t non_finite;
    uint32_t queue_pressure;
    uint32_t accelerator_errors;
    uint32_t accelerator_timeouts;
    uint32_t age_timeouts;
    uint32_t recoveries;
    uint32_t fallback_hops;
    bool valid;
    bool pending;
    bool ever_received;
} k1_semantic_state_t;

void k1_semantic_reset(k1_semantic_state_t *state);
bool k1_semantic_publish(k1_semantic_state_t *state, const k1_semantic_sample_t *sample,
                         uint64_t now_us, uint64_t maximum_age_us);
void k1_semantic_loss(k1_semantic_state_t *state);
void k1_semantic_accelerator_error(k1_semantic_state_t *state);
void k1_semantic_accelerator_timeout(k1_semantic_state_t *state);
void k1_semantic_poll(k1_semantic_state_t *state, uint64_t now_us, uint64_t maximum_age_us);
void k1_semantic_consume(k1_semantic_state_t *state);
void k1_semantic_failure_step(k1_semantic_state_t *state, uint32_t hop_index);
