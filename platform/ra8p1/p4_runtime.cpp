/* Generic P4/E1 target workload. This is deliberately separate from K1 AP/VP. */
#include "p4_runtime.h"
#include "fixture_app.h"
#include "npu_load.h"
#include "semantic_sidecar.h"
#include "kernels.h"
#include "p4_fixture.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
constexpr std::uint32_t dsp_period_us=128000U;
constexpr std::uint32_t npu_period_us=50000U;
constexpr std::uint32_t npu_guard_us=2000U;
constexpr std::uint32_t allowed_lateness_us=100U;
constexpr std::uint32_t maximum_ulp=2U;
constexpr double goertzel_limit=1.0e-6;

template<unsigned Bins,unsigned WidthUs> struct Distribution {
    std::uint32_t bins[Bins+1]{};
    std::uint64_t sum_us=0;
    std::uint32_t count=0,maximum_us=0;
    void add(std::uint32_t value) {
        ++bins[value/WidthUs>Bins?Bins:value/WidthUs]; sum_us+=value; ++count;
        if(value>maximum_us) maximum_us=value;
    }
    std::uint32_t percentile(unsigned numerator) const {
        if(!count) return 0;
        const std::uint32_t wanted=(count*numerator+99)/100;
        std::uint32_t seen=0;
        for(unsigned i=0;i<=Bins;++i) if((seen+=bins[i])>=wanted) return i*WidthUs;
        return Bins*WidthUs;
    }
};

struct State {
    bool active=false,finished=false;
    std::uint32_t releases=0,mode=0,flags=0,last_cycle=0;
    std::uint32_t dsp_count=0,npu_count=0,numeric_failures=0,deadline_misses=0;
    std::uint32_t release_guard_failures=0,backlog_highwater=0,npu_failures=0,npu_output_failures=0;
    std::uint32_t maximum_ulp_seen=0,first_bad_bin=0xffffffffU;
    double maximum_goertzel_error=0;
    std::uint64_t elapsed=0,next_dsp=0,next_npu=0,end=0;
    std::uint64_t npu_cycles=0,npu_active=0,mac_active=0;
    Distribution<2000,100> dsp_time,npu_time,lateness;
    k1_semantic_state_t semantic{};
} state;

alignas(16) float pcm[2048];
alignas(16) p4_workspace_t workspace;
std::uint32_t clock_hz;

std::uint64_t cycles_for_us(std::uint32_t microseconds) {
    return (std::uint64_t(clock_hz)*microseconds+999999U)/1000000U;
}
std::uint32_t microseconds(std::uint64_t cycles) {
    if(!clock_hz) return 0xffffffffU;
    const std::uint64_t value=(cycles*1000000U+clock_hz-1)/clock_hz;
    return value>0xffffffffU?0xffffffffU:std::uint32_t(value);
}
std::uint32_t ulp_distance(float actual,std::uint32_t expected_bits) {
    std::uint32_t actual_bits; std::memcpy(&actual_bits,&actual,sizeof(actual_bits));
    return actual_bits>expected_bits?actual_bits-expected_bits:expected_bits-actual_bits;
}
void run_dsp() {
    const std::uint64_t lateness=state.elapsed-state.next_dsp;
    const std::uint32_t started=k1_cycle_count();
    const int result=p4_kernels(pcm,&workspace);
    const std::uint32_t work_cycles=k1_cycle_count()-started;
    const std::uint32_t work_us=microseconds(work_cycles);
    state.dsp_time.add(work_us); state.lateness.add(microseconds(lateness));
    if(lateness+work_cycles>cycles_for_us(dsp_period_us)) ++state.deadline_misses;
    if(lateness>cycles_for_us(allowed_lateness_us)) ++state.release_guard_failures;
    const std::uint32_t backlog=1U+std::uint32_t(lateness/cycles_for_us(dsp_period_us));
    if(backlog>state.backlog_highwater) state.backlog_highwater=backlog;
    bool valid=result==0;
    for(unsigned i=0;i<1025;++i) {
        const std::uint32_t expected=p4_rfft_mag_bits[i]^((state.flags&2U) && !state.dsp_count && i==1024?0x100U:0U);
        const std::uint32_t first=ulp_distance(workspace.rfft[i],expected);
        const std::uint32_t second=ulp_distance(workspace.hann_rfft[i],p4_hann_rfft_mag_bits[i]);
        if(first>state.maximum_ulp_seen) state.maximum_ulp_seen=first;
        if(second>state.maximum_ulp_seen) state.maximum_ulp_seen=second;
        if((first>maximum_ulp || second>maximum_ulp) && state.first_bad_bin==0xffffffffU) state.first_bad_bin=i;
        valid=valid && first<=maximum_ulp && second<=maximum_ulp;
    }
    double expected_goertzel; std::memcpy(&expected_goertzel,&p4_goertzel_bits,sizeof(expected_goertzel));
    const double error=std::fabs(workspace.goertzel_magnitude-expected_goertzel);
    if(error>state.maximum_goertzel_error) state.maximum_goertzel_error=error;
    valid=valid && std::isfinite(error) && error<=goertzel_limit;
    if(!valid) ++state.numeric_failures;
    if(state.flags&4U) k1_semantic_failure_step(&state.semantic,state.dsp_count);
    ++state.dsp_count; state.next_dsp+=cycles_for_us(dsp_period_us);
}
void run_npu() {
    k1_npu_measurement_t measurement{};
    k1_npu_invoke(state.npu_count&1U,&measurement);
    ++state.npu_count;
    state.npu_time.add(microseconds(measurement.wall_cycles));
    state.npu_cycles+=measurement.npu_cycles;
    state.npu_active+=measurement.npu_active;
    state.mac_active+=measurement.mac_active;
    if(measurement.invoke_status) ++state.npu_failures;
    if(!measurement.output_match) ++state.npu_output_failures;
}
void update_elapsed() {
    const std::uint32_t current=k1_cycle_count();
    state.elapsed+=std::uint32_t(current-state.last_cycle);
    state.last_cycle=current;
}
bool due_npu() { return state.next_npu<=state.end && state.elapsed>=state.next_npu; }
void finish_if_complete() {
    const bool dsp_done=state.mode==K1_P4_NPU_ALONE || state.dsp_count==state.releases;
    const bool npu_done=state.mode==K1_P4_DSP_ALONE || state.mode==K1_P4_SATURATION || state.next_npu>state.end;
    if(dsp_done && npu_done) { state.active=false; state.finished=true; }
}
}

void k1_p4_initialise(std::uint32_t hz) {
    clock_hz=hz;
    std::memcpy(pcm,p4_pcm_bits,sizeof(pcm));
}

bool k1_p4_start(std::uint32_t releases,std::uint32_t mode,std::uint32_t flags) {
    if(state.active || !clock_hz || !releases || releases>14063U || mode<1U || mode>4U || (flags&~6U) || !k1_npu_ready()) return false;
    std::memset(&state,0,sizeof(state));
    state.active=true; state.releases=releases; state.mode=mode; state.flags=flags;
    k1_semantic_reset(&state.semantic);
    state.last_cycle=k1_cycle_count();
    state.next_dsp=cycles_for_us(dsp_period_us);
    state.next_npu=cycles_for_us(npu_period_us);
    state.end=cycles_for_us(dsp_period_us)*releases;
    return true;
}

bool k1_p4_active() { return state.active; }

void k1_p4_step() {
    if(!state.active) return;
    update_elapsed();
    if(state.mode!=K1_P4_NPU_ALONE && state.dsp_count<state.releases && state.elapsed>=state.next_dsp) {
        run_dsp(); finish_if_complete(); return;
    }
    if(state.mode==K1_P4_SATURATION) {
        if(state.dsp_count==state.releases) { finish_if_complete(); return; }
        const std::uint64_t until_dsp=state.next_dsp>state.elapsed?state.next_dsp-state.elapsed:0;
        if(until_dsp>cycles_for_us(npu_guard_us)) run_npu();
        return;
    }
    if(state.mode!=K1_P4_DSP_ALONE && due_npu()) {
        if(state.mode==K1_P4_CONCURRENT && state.dsp_count<state.releases) {
            const std::uint64_t until_dsp=state.next_dsp>state.elapsed?state.next_dsp-state.elapsed:0;
            if(until_dsp<=cycles_for_us(npu_guard_us)) return;
        }
        run_npu(); state.next_npu+=cycles_for_us(npu_period_us); finish_if_complete(); return;
    }
    finish_if_complete();
}

size_t k1_p4_status(char *output,size_t capacity) {
    const std::uint32_t elapsed_us=microseconds(state.elapsed);
    const std::uint64_t duty_ppm=elapsed_us?(state.npu_time.sum_us*1000000U)/elapsed_us:0;
    const int n=std::snprintf(output,capacity,
        "{\"profile\":\"generic-p4-seed0-v1\",\"active\":%s,\"finished\":%s,\"mode\":%lu,\"flags\":%lu,\"elapsed_us\":%lu,\"queue_capacity\":1,\"drops\":0,\"coalesces\":0,\"releases\":%lu,\"dsp_count\":%lu,\"npu_count\":%lu,\"numeric_failures\":%lu,\"deadline_misses\":%lu,\"release_guard_failures\":%lu,\"backlog_highwater\":%lu,\"npu_failures\":%lu,\"npu_output_failures\":%lu,\"maximum_ulp\":%lu,\"first_bad_bin\":%lu,\"maximum_goertzel_error\":%.9g,\"npu_cycles\":%llu,\"npu_active_cycles\":%llu,\"mac_active_cycles\":%llu,\"npu_wall_sum_us\":%llu,\"npu_duty_ppm\":%llu,\"dsp_mean_us\":%.3f,\"dsp_p50_us\":%lu,\"dsp_p95_us\":%lu,\"dsp_p99_us\":%lu,\"dsp_max_us\":%lu,\"npu_mean_us\":%.3f,\"npu_p99_us\":%lu,\"npu_max_us\":%lu,\"lateness_p99_us\":%lu,\"lateness_max_us\":%lu,\"semantic_valid\":%s,\"semantic_loss\":%lu,\"semantic_recoveries\":%lu}",
        state.active?"true":"false",state.finished?"true":"false",(unsigned long)state.mode,(unsigned long)state.flags,
        (unsigned long)elapsed_us,
        (unsigned long)state.releases,(unsigned long)state.dsp_count,(unsigned long)state.npu_count,
        (unsigned long)state.numeric_failures,(unsigned long)state.deadline_misses,(unsigned long)state.release_guard_failures,
        (unsigned long)state.backlog_highwater,(unsigned long)state.npu_failures,(unsigned long)state.npu_output_failures,
        (unsigned long)state.maximum_ulp_seen,(unsigned long)state.first_bad_bin,state.maximum_goertzel_error,
        (unsigned long long)state.npu_cycles,(unsigned long long)state.npu_active,(unsigned long long)state.mac_active,
        (unsigned long long)state.npu_time.sum_us,(unsigned long long)duty_ppm,
        state.dsp_time.count?double(state.dsp_time.sum_us)/state.dsp_time.count:0.0,
        (unsigned long)state.dsp_time.percentile(50),(unsigned long)state.dsp_time.percentile(95),
        (unsigned long)state.dsp_time.percentile(99),(unsigned long)state.dsp_time.maximum_us,
        state.npu_time.count?double(state.npu_time.sum_us)/state.npu_time.count:0.0,
        (unsigned long)state.npu_time.percentile(99),(unsigned long)state.npu_time.maximum_us,
        (unsigned long)state.lateness.percentile(99),(unsigned long)state.lateness.maximum_us,
        state.semantic.valid?"true":"false",(unsigned long)state.semantic.loss,(unsigned long)state.semantic.recoveries);
    return n>0 && std::size_t(n)<capacity?std::size_t(n):0;
}
