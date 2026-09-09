/* Identified smoke-graph load generator. It is not a semantic implementation. */
#include "npu_load.h"
#include <string.h>
#include "hal_data.h"
#include "common_data.h"
#include "core_cm85.h"
#include "pmu_ethosu.h"
#include "sub_0001_invoke.h"
#include "sub_0001_tensors.h"
#include "npu_inputs.h"

static bool ready;

bool k1_npu_initialise(void) {
    ready = FSP_SUCCESS == RM_ETHOSU_Open(&g_rm_ethosu0_ctrl, &g_rm_ethosu0_cfg);
    if (ready) {
        ETHOSU_PMU_Enable(&g_ethosu0);
        ETHOSU_PMU_Set_EVTYPER(&g_ethosu0, 0U, ETHOSU_PMU_NPU_ACTIVE);
        ETHOSU_PMU_Set_EVTYPER(&g_ethosu0, 1U, ETHOSU_PMU_MAC_ACTIVE);
        ETHOSU_PMU_CNTR_Enable(&g_ethosu0,
            ETHOSU_PMU_CCNT_Msk | ETHOSU_PMU_CNT1_Msk | ETHOSU_PMU_CNT2_Msk);
    }
    return ready;
}

bool k1_npu_ready(void) { return ready; }

void k1_npu_invoke(uint32_t case_index, k1_npu_measurement_t *measurement) {
    memset(measurement, 0, sizeof(*measurement));
    measurement->invoke_status = -1;
    if (!ready || case_index >= K1_NPU_INPUT_COUNT) return;
    memcpy(sub_0001_arena + sub_0001_address_logmel_70085_10209_70057,
           k1_npu_inputs[case_index], K1_NPU_INPUT_BYTES);
    ETHOSU_PMU_CYCCNT_Reset(&g_ethosu0);
    ETHOSU_PMU_EVCNTR_ALL_Reset(&g_ethosu0);
    const uint32_t started = DWT->CYCCNT;
    measurement->invoke_status = sub_0001_invoke(false);
    measurement->wall_cycles = DWT->CYCCNT - started;
    measurement->npu_cycles = ETHOSU_PMU_Get_CCNTR(&g_ethosu0);
    measurement->npu_active = ETHOSU_PMU_Get_EVCNTR(&g_ethosu0, 0U);
    measurement->mac_active = ETHOSU_PMU_Get_EVCNTR(&g_ethosu0, 1U);
    measurement->output_match = measurement->invoke_status == 0 &&
        memcmp(sub_0001_arena + sub_0001_address_activity_70065_10212,
               k1_npu_expected[case_index], 3U) == 0;
}
