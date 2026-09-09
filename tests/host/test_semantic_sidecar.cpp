#include "semantic_sidecar.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static k1_semantic_sample_t sample(unsigned sequence,std::uint64_t now) {
    return {K1_SEMANTIC_SOURCE_ID,sequence,now,now,{0.1f,0.2f,0.3f,0.4f}};
}

int main() {
    constexpr std::uint64_t age=250000;
    k1_semantic_state_t state{};
    auto value=sample(1,1000000);
    assert(k1_semantic_publish(&state,&value,1000000,age));
    k1_semantic_consume(&state);
    k1_semantic_loss(&state);
    value=sample(2,1000100); assert(k1_semantic_publish(&state,&value,1000100,age));
    value=sample(3,1000200); assert(k1_semantic_publish(&state,&value,1000200,age));
    assert(state.queue_pressure==1 && state.recoveries==1);
    k1_semantic_consume(&state);
    value=sample(3,1000300); assert(!k1_semantic_publish(&state,&value,1000300,age));
    value=sample(4,1000400); value.source_id=0; assert(!k1_semantic_publish(&state,&value,1000400,age));
    value=sample(4,1000500); value.values[3]=NAN; assert(!k1_semantic_publish(&state,&value,1000500,age));
    value=sample(4,1000600); value.available_us=1000700; assert(!k1_semantic_publish(&state,&value,1000600,age));
    value=sample(4,1); assert(!k1_semantic_publish(&state,&value,1000000,age));
    value=sample(4,1000800); assert(k1_semantic_publish(&state,&value,1000800,age));
    k1_semantic_consume(&state);
    k1_semantic_poll(&state,1250801,age); assert(!state.valid && state.age_timeouts==1);
    k1_semantic_accelerator_error(&state);
    k1_semantic_accelerator_timeout(&state);
    k1_semantic_consume(&state);
    assert(state.loss==1 && state.delayed==1 && state.stale==1 && state.out_of_order==1);
    assert(state.invalid_identity==1 && state.non_finite==1 && state.accelerator_errors==1);
    assert(state.accelerator_timeouts==1 && state.fallback_hops==1);
    k1_semantic_reset(&state);
    for(unsigned hop=0;hop<6000;++hop) k1_semantic_failure_step(&state,hop);
    assert(state.valid && state.accepted==309 && state.loss==1 && state.delayed==1);
    assert(state.stale==1 && state.out_of_order==1 && state.invalid_identity==1);
    assert(state.non_finite==1 && state.queue_pressure==1 && state.accelerator_errors==1);
    assert(state.accelerator_timeouts==1 && state.age_timeouts==1 && state.recoveries==9);
    assert(state.fallback_hops==9);
    std::puts("K1_SEMANTIC_SIDECAR=PASS identity=PASS finite=PASS age=PASS ordering=PASS pressure=PASS timeout=PASS recovery=PASS");
}
