#include "pair_fixture_admit.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void expect(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "FAIL %s\n", name);
        failures += 1;
    }
}

static void fill_request(uint8_t *request, uint32_t profile, uint32_t generation,
                         uint32_t epoch, uint32_t kind, uint8_t lane_mark)
{
    memset(request, 0, K1_PAIR_FIXTURE_BYTES);
    request[0] = K1_PAIR_FIXTURE_VERSION;
    request[4] = (uint8_t)profile;
    request[8] = (uint8_t)generation;
    request[12] = (uint8_t)epoch;
    request[16] = (uint8_t)kind;
    if (lane_mark) {
        request[K1_PAIR_FIXTURE_HEADER_BYTES] = lane_mark;
        request[K1_PAIR_FIXTURE_HEADER_BYTES + K1_PAIR_FIXTURE_LANE_BYTES] = lane_mark;
    }
}

static k1_pair_fixture_gate_t open_gate(void)
{
    k1_pair_fixture_gate_t gate;
    gate.last_generation = 0;
    gate.expected_epoch = 1;
    gate.in_flight = 0;
    return gate;
}

int main(void)
{
    uint8_t request[K1_PAIR_FIXTURE_BYTES];
    k1_pair_fixture_view_t view;
    k1_pair_fixture_gate_t gate = open_gate();
    uint8_t one_lane[K1_PAIR_FIXTURE_HEADER_BYTES + K1_PAIR_FIXTURE_LANE_BYTES];

    fill_request(request, 3, 1, 1, K1_PAIR_FIXTURE_KIND_MAP, 0x5a);
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_OK,
           "map admitted");
    expect(view.lane0 && view.lane0[0] == 0x5a && view.generation == 1u, "map view");

    memset(one_lane, 0, sizeof one_lane);
    expect(k1_pair_fixture_admit(&gate, one_lane, sizeof one_lane, &view) == K1_PAIR_FIXTURE_PARTIAL,
           "one lane is partial");
    expect(view.lane0 == 0 && view.lane1 == 0, "partial leaves no lanes");

    expect(k1_pair_fixture_admit(&gate, request, sizeof request - 1u, &view) == K1_PAIR_FIXTURE_SHAPE,
           "short request");
    expect(k1_pair_fixture_admit(&gate, request, 0, &view) == K1_PAIR_FIXTURE_SHAPE, "empty request");

    fill_request(request, 1, 1, 1, K1_PAIR_FIXTURE_KIND_MAP, 1);
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_BAD_PROFILE,
           "wrong profile");

    fill_request(request, 3, 0, 1, K1_PAIR_FIXTURE_KIND_MAP, 1);
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_GENERATION,
           "zero generation");

    fill_request(request, 3, 1, 0, K1_PAIR_FIXTURE_KIND_MAP, 1);
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_EPOCH,
           "zero epoch");

    fill_request(request, 3, 1, 2, K1_PAIR_FIXTURE_KIND_MAP, 1);
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_EPOCH,
           "wrong epoch");

    gate.last_generation = 4;
    fill_request(request, 3, 4, 1, K1_PAIR_FIXTURE_KIND_MAP, 1);
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_STALE,
           "replayed generation");
    fill_request(request, 3, 3, 1, K1_PAIR_FIXTURE_KIND_MAP, 1);
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_STALE,
           "older generation");

    gate = open_gate();
    gate.in_flight = 1;
    fill_request(request, 3, 2, 1, K1_PAIR_FIXTURE_KIND_MAP, 1);
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_IN_FLIGHT,
           "in flight");
    expect(view.lane0 == 0, "in flight does not publish lanes");

    gate = open_gate();
    fill_request(request, 3, 2, 1, 9, 1);
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_KIND,
           "bad kind");

    fill_request(request, 3, 2, 1, K1_PAIR_FIXTURE_KIND_BLACK, 1);
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_NOT_BLACK,
           "black kind with light");

    fill_request(request, 3, 2, 1, K1_PAIR_FIXTURE_KIND_BLACK, 0);
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_OK,
           "black pair admitted");
    expect(view.kind == K1_PAIR_FIXTURE_KIND_BLACK, "black kind retained");

    fill_request(request, 3, 2, 1, K1_PAIR_FIXTURE_KIND_MAP, 1);
    request[20] = 1;
    expect(k1_pair_fixture_admit(&gate, request, sizeof request, &view) == K1_PAIR_FIXTURE_SHAPE,
           "reserved must be zero");

    if (failures) {
        fprintf(stderr, "PAIR_FIXTURE_ADMIT_HOST=FAIL %d\n", failures);
        return 1;
    }
    printf("PAIR_FIXTURE_ADMIT_HOST=PASS\n");
    return 0;
}
