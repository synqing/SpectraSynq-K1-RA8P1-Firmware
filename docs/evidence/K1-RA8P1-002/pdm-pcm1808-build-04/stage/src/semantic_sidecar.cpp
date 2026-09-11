#include "semantic_sidecar.h"
#include <cmath>
#include <cstring>

namespace {
void invalidate(k1_semantic_state_t *state) {
    state->valid=false;
    state->pending=false;
}
}

void k1_semantic_reset(k1_semantic_state_t *state) { std::memset(state,0,sizeof(*state)); }

bool k1_semantic_publish(k1_semantic_state_t *state, const k1_semantic_sample_t *sample,
                         uint64_t now_us, uint64_t maximum_age_us) {
    if(sample->source_id!=K1_SEMANTIC_SOURCE_ID) {
        ++state->invalid_identity; invalidate(state); return false;
    }
    for(float value:sample->values) if(!std::isfinite(value) || value<0.0f) {
        ++state->non_finite; invalidate(state); return false;
    }
    if(sample->available_us>now_us || sample->event_us>sample->available_us) {
        ++state->delayed; invalidate(state); return false;
    }
    if(now_us-sample->available_us>maximum_age_us) {
        ++state->stale; invalidate(state); return false;
    }
    if(state->ever_received && sample->sequence<=state->last_sequence) {
        ++state->out_of_order; invalidate(state); return false;
    }
    if(state->pending) ++state->queue_pressure;
    if(state->ever_received && !state->valid) ++state->recoveries;
    state->last_sequence=sample->sequence;
    state->last_available_us=sample->available_us;
    state->valid=true;
    state->pending=true;
    state->ever_received=true;
    ++state->accepted;
    return true;
}

void k1_semantic_loss(k1_semantic_state_t *state) { ++state->loss; invalidate(state); }
void k1_semantic_accelerator_error(k1_semantic_state_t *state) { ++state->accelerator_errors; invalidate(state); }
void k1_semantic_accelerator_timeout(k1_semantic_state_t *state) { ++state->accelerator_timeouts; invalidate(state); }

void k1_semantic_poll(k1_semantic_state_t *state, uint64_t now_us, uint64_t maximum_age_us) {
    if(state->valid && now_us-state->last_available_us>maximum_age_us) {
        ++state->age_timeouts;
        invalidate(state);
    }
}

void k1_semantic_consume(k1_semantic_state_t *state) {
    if(!state->valid) ++state->fallback_hops;
    state->pending=false;
}

void k1_semantic_failure_step(k1_semantic_state_t *state, uint32_t index) {
    constexpr uint64_t maximum_age_us=250000U;
    const uint64_t now_us=1000000U+(uint64_t(index)+1U)*7500U;
    k1_semantic_poll(state,now_us,maximum_age_us);
    auto publish=[&](uint32_t sequence) {
        const k1_semantic_sample_t value={K1_SEMANTIC_SOURCE_ID,sequence,now_us,now_us,
                                          {0.1f,0.2f,0.3f,0.4f}};
        return k1_semantic_publish(state,&value,now_us,maximum_age_us);
    };
    if(index==0) publish(1);
    else if(index==1) k1_semantic_loss(state);
    else if(index==2) publish(2);
    else if(index==3) { const k1_semantic_sample_t value={K1_SEMANTIC_SOURCE_ID,3,now_us,now_us+1U,{0.1f,0.2f,0.3f,0.4f}}; k1_semantic_publish(state,&value,now_us,maximum_age_us); }
    else if(index==4) publish(3);
    else if(index==5) { const k1_semantic_sample_t value={K1_SEMANTIC_SOURCE_ID,4,1,1,{0.1f,0.2f,0.3f,0.4f}}; k1_semantic_publish(state,&value,now_us,maximum_age_us); }
    else if(index==6) publish(4);
    else if(index==7) publish(4);
    else if(index==8) publish(5);
    else if(index==9) { const k1_semantic_sample_t value={0,6,now_us,now_us,{0.1f,0.2f,0.3f,0.4f}}; k1_semantic_publish(state,&value,now_us,maximum_age_us); }
    else if(index==10) publish(6);
    else if(index==11) { const k1_semantic_sample_t value={K1_SEMANTIC_SOURCE_ID,7,now_us,now_us,{NAN,0.2f,0.3f,0.4f}}; k1_semantic_publish(state,&value,now_us,maximum_age_us); }
    else if(index==12) publish(7);
    else if(index==13) { publish(8); publish(9); }
    else if(index==14) k1_semantic_accelerator_error(state);
    else if(index==15) publish(10);
    else if(index==16) k1_semantic_accelerator_timeout(state);
    else if(index==17) publish(11);
    else if(index==52) publish(12);
    else if(index>52 && (index-52)%20==0) publish(13+(index-72)/20);
    k1_semantic_consume(state);
}
