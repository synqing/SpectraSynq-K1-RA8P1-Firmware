#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct k1_npu_measurement {
    int32_t invoke_status;
    uint32_t wall_cycles;
    uint64_t npu_cycles;
    uint32_t npu_active;
    uint32_t mac_active;
    bool output_match;
} k1_npu_measurement_t;
bool k1_npu_initialise(void);
bool k1_npu_ready(void);
void k1_npu_invoke(uint32_t case_index, k1_npu_measurement_t *measurement);
#ifdef __cplusplus
}
#endif
