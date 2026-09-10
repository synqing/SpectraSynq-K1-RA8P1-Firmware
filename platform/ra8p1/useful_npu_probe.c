/* Useful-U55 admission only. Host-safe: no FSP, no Ethos-U invoke, no USB.
   Load-only / constant [127,117,123] cannot pass useful inference.
   Missing original same-compilation model/goldens/allocation keeps G6
   as NO_QUALIFYING_CANDIDATE. Licence UNKNOWN is not cleared. A different
   student is not selected. */
#include <stdint.h>
#include <string.h>

#define K1_USEFUL_NPU_DECLINE_OK 0
#define K1_USEFUL_NPU_DECLINE_LOAD_ONLY 1
#define K1_USEFUL_NPU_DECLINE_CONSTANT_OUTPUT 2
#define K1_USEFUL_NPU_DECLINE_MISSING_MODEL 3
#define K1_USEFUL_NPU_DECLINE_MISSING_GOLDENS 4
#define K1_USEFUL_NPU_DECLINE_MISSING_ALLOC 5
#define K1_USEFUL_NPU_DECLINE_MISSING_OPERATOR 6
#define K1_USEFUL_NPU_DECLINE_MISSING_PERMISSIONS 7
#define K1_USEFUL_NPU_DECLINE_LICENCE_UNKNOWN 8
#define K1_USEFUL_NPU_DECLINE_NOT_INPUT_DEPENDENT 9
#define K1_USEFUL_NPU_DECLINE_DIFFERENT_STUDENT 10

static const uint8_t k_load_only_expected[3] = {127u, 117u, 123u};
static uint8_t s_identity[32];
static int s_identity_set;
static int s_last_decline = K1_USEFUL_NPU_DECLINE_MISSING_MODEL;
static int s_admitted;
static int s_g6_no_qualifying = 1;
static int s_licence_unknown = 1;
static int s_invoked;

static int k1_digest_nonzero(const uint8_t *digest) {
    unsigned i;
    if (digest == NULL) return 0;
    for (i = 0; i < 32u; ++i) if (digest[i] != 0u) return 1;
    return 0;
}

void k1_useful_npu_bind_identity(const uint8_t *model_sha256,
                                 const uint8_t *weights_sha256,
                                 const uint8_t *input_sha256,
                                 uint8_t *out) {
    unsigned i;
    uint8_t mix = 0;
    if (out == NULL) return;
    memset(out, 0, 32);
    for (i = 0; i < 32u; ++i) {
        const uint8_t m = model_sha256 ? model_sha256[i] : 0u;
        const uint8_t w = weights_sha256 ? weights_sha256[i] : 0u;
        const uint8_t n = input_sha256 ? input_sha256[i] : 0u;
        mix = (uint8_t)(mix + m + w + n + (uint8_t)i);
        out[i] = (uint8_t)(m ^ w ^ n ^ mix);
    }
}

int k1_useful_npu_identity_equal(const uint8_t *left, const uint8_t *right) {
    if (left == NULL || right == NULL) return 0;
    return memcmp(left, right, 32) == 0;
}

int k1_useful_npu_is_load_only_output(const uint8_t *raw, uint32_t length) {
    if (raw == NULL || length != 3u) return 0;
    return memcmp(raw, k_load_only_expected, 3) == 0;
}

int k1_useful_npu_admit(int has_same_compilation_model,
                        int has_allocation_report,
                        int has_operator_report,
                        int has_golden_inputs,
                        int has_golden_outputs,
                        int has_code_perm,
                        int has_weight_perm,
                        int has_data_perm,
                        int licence_cleared,
                        int load_only,
                        int constant_output,
                        int input_dependent,
                        int different_student,
                        const uint8_t *model_sha256,
                        const uint8_t *weights_sha256,
                        const uint8_t *input_sha256,
                        const uint8_t *expected_raw,
                        uint32_t expected_length) {
    s_admitted = 0;
    s_invoked = 0;
    s_identity_set = 0;
    memset(s_identity, 0, sizeof(s_identity));
    s_g6_no_qualifying = 1;
    s_licence_unknown = licence_cleared ? 0 : 1;

    if (different_student) {
        s_last_decline = K1_USEFUL_NPU_DECLINE_DIFFERENT_STUDENT;
        return s_last_decline;
    }
    if (load_only) {
        s_last_decline = K1_USEFUL_NPU_DECLINE_LOAD_ONLY;
        return s_last_decline;
    }
    if (constant_output || k1_useful_npu_is_load_only_output(expected_raw, expected_length)) {
        s_last_decline = K1_USEFUL_NPU_DECLINE_CONSTANT_OUTPUT;
        return s_last_decline;
    }
    if (!has_same_compilation_model || !k1_digest_nonzero(model_sha256)) {
        s_last_decline = K1_USEFUL_NPU_DECLINE_MISSING_MODEL;
        return s_last_decline;
    }
    if (!has_golden_inputs || !has_golden_outputs) {
        s_last_decline = K1_USEFUL_NPU_DECLINE_MISSING_GOLDENS;
        return s_last_decline;
    }
    if (!has_allocation_report) {
        s_last_decline = K1_USEFUL_NPU_DECLINE_MISSING_ALLOC;
        return s_last_decline;
    }
    if (!has_operator_report) {
        s_last_decline = K1_USEFUL_NPU_DECLINE_MISSING_OPERATOR;
        return s_last_decline;
    }
    if (!has_code_perm || !has_weight_perm || !has_data_perm) {
        s_last_decline = K1_USEFUL_NPU_DECLINE_MISSING_PERMISSIONS;
        return s_last_decline;
    }
    if (!licence_cleared) {
        s_last_decline = K1_USEFUL_NPU_DECLINE_LICENCE_UNKNOWN;
        return s_last_decline;
    }
    if (!input_dependent) {
        s_last_decline = K1_USEFUL_NPU_DECLINE_NOT_INPUT_DEPENDENT;
        return s_last_decline;
    }

    k1_useful_npu_bind_identity(model_sha256, weights_sha256, input_sha256, s_identity);
    s_identity_set = 1;
    /* G6 stays no-qualifying until a same-compilation useful candidate is
       admitted AND executed. This function never invokes the U55. */
    s_last_decline = K1_USEFUL_NPU_DECLINE_OK;
    s_admitted = 1;
    s_g6_no_qualifying = 1;
    return s_last_decline;
}

int k1_useful_npu_probe(void) {
    static const uint8_t load_only[3] = {127u, 117u, 123u};
    return k1_useful_npu_admit(0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
                               NULL, NULL, NULL, load_only, 3u);
}

int k1_useful_npu_invoke(void) {
    /* Admission never opens Ethos-U. A future authorised path would still
       refuse here unless admit() returned OK and a CURRENT_TARGET owner
       exists. This module does not invoke. */
    s_invoked = 0;
    if (!s_admitted) return -1;
    return -1;
}

int k1_useful_npu_admitted(void) { return s_admitted; }
int k1_useful_npu_last_decline(void) { return s_last_decline; }
int k1_useful_npu_g6_no_qualifying(void) { return s_g6_no_qualifying; }
int k1_useful_npu_licence_unknown(void) { return s_licence_unknown; }
int k1_useful_npu_invoked(void) { return s_invoked; }
int k1_useful_npu_identity_set(void) { return s_identity_set; }

void k1_useful_npu_copy_identity(uint8_t *out) {
    if (out == NULL) return;
    memcpy(out, s_identity, 32);
}

const char *k1_useful_npu_g6_text(void) {
    return "NO_QUALIFYING_CANDIDATE";
}
