#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Campaign admission for one profile-3 pair fixture.
   A request is two 480-byte lanes and one generation. Malformed, partial,
   stale, wrong-epoch and in-flight requests are rejected here, before any
   transmitter call. This file does not touch GPT, DMA or GPIO. */

#define K1_PAIR_FIXTURE_OPCODE 27u
#define K1_PAIR_FIXTURE_VERSION 1u
#define K1_PAIR_FIXTURE_PROFILE 3u
#define K1_PAIR_FIXTURE_HEADER_BYTES 24u
#define K1_PAIR_FIXTURE_LANE_BYTES 480u
#define K1_PAIR_FIXTURE_BYTES (K1_PAIR_FIXTURE_HEADER_BYTES + (2u * K1_PAIR_FIXTURE_LANE_BYTES))
#define K1_PAIR_FIXTURE_KIND_MAP 1u
#define K1_PAIR_FIXTURE_KIND_BLACK 2u

enum {
    K1_PAIR_FIXTURE_OK = 0,
    K1_PAIR_FIXTURE_SHAPE = 1,
    K1_PAIR_FIXTURE_PARTIAL = 2,
    K1_PAIR_FIXTURE_BAD_PROFILE = 3,
    K1_PAIR_FIXTURE_GENERATION = 4,
    K1_PAIR_FIXTURE_EPOCH = 5,
    K1_PAIR_FIXTURE_STALE = 6,
    K1_PAIR_FIXTURE_IN_FLIGHT = 7,
    K1_PAIR_FIXTURE_KIND = 8,
    K1_PAIR_FIXTURE_NOT_BLACK = 9
};

typedef struct {
    uint32_t last_generation;
    uint32_t expected_epoch;
    uint32_t in_flight;
} k1_pair_fixture_gate_t;

typedef struct {
    const uint8_t *lane0;
    const uint8_t *lane1;
    uint32_t generation;
    uint32_t epoch;
    uint32_t profile;
    uint32_t kind;
} k1_pair_fixture_view_t;

int k1_pair_fixture_admit(const k1_pair_fixture_gate_t *gate,
                          const uint8_t *request, size_t size,
                          k1_pair_fixture_view_t *out);

#ifdef __cplusplus
}
#endif
