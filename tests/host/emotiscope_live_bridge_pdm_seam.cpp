// Host-only hardware queries for test_emotiscope_live_bridge.py.
// Hops are injected directly: no capture poll, guard gain or measured CPU cost
// exists. Zero gain is the production reset/unknown sentinel, not an algorithm.
#ifdef K1_PDM_TARGET
#include "pdm_target.h"
extern "C" uint32_t k1_pdm_target_last_hop_gain_q8(void) { return 0U; }
extern "C" void k1_pdm_target_take_hop_cost(k1_pdm_hop_cost_t* out) {
  if (out != nullptr) *out = {};
}
#endif
