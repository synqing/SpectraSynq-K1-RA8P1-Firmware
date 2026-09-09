#include "p4_runtime.h"
#include "npu_load.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

static std::uint32_t cycles;
extern "C" std::uint32_t k1_cycle_count() { cycles+=1000U; return cycles; }
extern "C" bool k1_npu_ready() { return true; }
extern "C" bool k1_npu_initialise() { return true; }
extern "C" void k1_npu_invoke(std::uint32_t, k1_npu_measurement_t* result) {
    std::memset(result,0,sizeof(*result)); result->wall_cycles=100;
    result->npu_cycles=80; result->npu_active=60; result->mac_active=40; result->output_match=true;
}
static std::string finish() {
    for(unsigned i=0;i<200000 && k1_p4_active();++i) k1_p4_step();
    assert(!k1_p4_active());
    char output[4096]; const auto count=k1_p4_status(output,sizeof(output)); assert(count);
    return std::string(output,count);
}
int main() {
    k1_p4_initialise(1000000U);
    assert(!k1_p4_start(0,K1_P4_DSP_ALONE,0));
    assert(!k1_p4_start(1,0,0));
    cycles=0xffff0000U;
    assert(k1_p4_start(1,K1_P4_DSP_ALONE,0));
    auto status=finish();
    assert(status.find("\"dsp_count\":1")!=std::string::npos);
    assert(status.find("\"numeric_failures\":0")!=std::string::npos);
    assert(k1_p4_start(1,K1_P4_DSP_ALONE,2));
    status=finish(); assert(status.find("\"numeric_failures\":1")!=std::string::npos);
    assert(k1_p4_start(2,K1_P4_CONCURRENT,0));
    status=finish();
    assert(status.find("\"dsp_count\":2")!=std::string::npos);
    assert(status.find("\"npu_count\":5")!=std::string::npos);
    assert(status.find("\"npu_failures\":0")!=std::string::npos);
    std::puts("GENERIC_P4_RUNTIME=PASS numeric=PASS mutation=PASS schedule=PASS npu=PASS cycle_wrap=PASS");
}
