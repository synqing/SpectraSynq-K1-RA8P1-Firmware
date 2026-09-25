#include "pair_fixture_admit.h"

static uint32_t read_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void clear_view(k1_pair_fixture_view_t *out)
{
    out->lane0 = 0;
    out->lane1 = 0;
    out->generation = 0;
    out->epoch = 0;
    out->profile = 0;
    out->kind = 0;
}

static int lanes_are_black(const uint8_t *lane0, const uint8_t *lane1)
{
    size_t index;
    for (index = 0; index < K1_PAIR_FIXTURE_LANE_BYTES; ++index) {
        if (lane0[index] != 0u || lane1[index] != 0u) {
            return 0;
        }
    }
    return 1;
}

int k1_pair_fixture_admit(const k1_pair_fixture_gate_t *gate,
                          const uint8_t *request, size_t size,
                          k1_pair_fixture_view_t *out)
{
    uint32_t version;
    uint32_t profile;
    uint32_t generation;
    uint32_t epoch;
    uint32_t kind;
    uint32_t reserved;
    const uint8_t *lane0;
    const uint8_t *lane1;

    if (out) {
        clear_view(out);
    }
    if (!gate || !out || !request || size == 0u) {
        return K1_PAIR_FIXTURE_SHAPE;
    }
    if (size == (size_t)K1_PAIR_FIXTURE_HEADER_BYTES + K1_PAIR_FIXTURE_LANE_BYTES) {
        return K1_PAIR_FIXTURE_PARTIAL;
    }
    if (size != K1_PAIR_FIXTURE_BYTES) {
        return K1_PAIR_FIXTURE_SHAPE;
    }
    version = read_u32(request);
    profile = read_u32(request + 4);
    generation = read_u32(request + 8);
    epoch = read_u32(request + 12);
    kind = read_u32(request + 16);
    reserved = read_u32(request + 20);
    if (version != K1_PAIR_FIXTURE_VERSION || reserved != 0u) {
        return K1_PAIR_FIXTURE_SHAPE;
    }
    if (profile != K1_PAIR_FIXTURE_PROFILE) {
        return K1_PAIR_FIXTURE_BAD_PROFILE;
    }
    if (generation == 0u) {
        return K1_PAIR_FIXTURE_GENERATION;
    }
    if (epoch == 0u || gate->expected_epoch == 0u || epoch != gate->expected_epoch) {
        return K1_PAIR_FIXTURE_EPOCH;
    }
    if (kind != K1_PAIR_FIXTURE_KIND_MAP && kind != K1_PAIR_FIXTURE_KIND_BLACK) {
        return K1_PAIR_FIXTURE_KIND;
    }
    if (generation <= gate->last_generation) {
        return K1_PAIR_FIXTURE_STALE;
    }
    if (gate->in_flight) {
        return K1_PAIR_FIXTURE_IN_FLIGHT;
    }
    lane0 = request + K1_PAIR_FIXTURE_HEADER_BYTES;
    lane1 = lane0 + K1_PAIR_FIXTURE_LANE_BYTES;
    if (kind == K1_PAIR_FIXTURE_KIND_BLACK && !lanes_are_black(lane0, lane1)) {
        return K1_PAIR_FIXTURE_NOT_BLACK;
    }
    out->lane0 = lane0;
    out->lane1 = lane1;
    out->generation = generation;
    out->epoch = epoch;
    out->profile = profile;
    out->kind = kind;
    return K1_PAIR_FIXTURE_OK;
}
