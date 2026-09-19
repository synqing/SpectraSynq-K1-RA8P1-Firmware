#include "pdm_target.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    if (K1_PDM_TARGET_SINCDEC_40K != 49u) abort();
    if (K1_PDM_TARGET_SINCRNG_40K == 5u) abort();
    if (K1_PDM_TARGET_SINCRNG_40K != 10u) abort();
    printf("PDM_SINCRNG_PASS sincdec=%u sincrng=%u\n",
           (unsigned)K1_PDM_TARGET_SINCDEC_40K,
           (unsigned)K1_PDM_TARGET_SINCRNG_40K);
    return 0;
}
