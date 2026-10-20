#pragma once
// TEST IMAGE ONLY (K1_V9_RELEASE_PUPPET). A host-timed puppet for the R01+R03
// release, so the Captain can judge the drop on the device before a live
// trigger exists (P6 drop-cue feasibility, 15 Oct 2026: no causal loudness cue
// separates drops on the room microphone).
//
// Board 0's speed carries one code. 0.501 arms both edges together (one plate);
// the drop lands hold_lead_s after the arm, with the riser hold before it. The
// code is not a speed: the renderer keeps motion at 0.500 while it is present.
// Product images do not define K1_V9_RELEASE_PUPPET and never read this.
#include <cmath>
#include <cstdint>

#include "core/visual/inhouse_waveform.h"

namespace k1::core::visual {

struct V9PuppetState final {
  bool armed = false;
  std::uint64_t arm_us = 0U;
  int last_code = 0;
};

inline constexpr float kV9PuppetBaseSpeed = 0.5F;

// 1 for the arm code (0.501 within ±0.0002), else 0.
inline int v9PuppetCode(float speed) noexcept {
  if (!std::isfinite(speed)) return 0;
  const float d = (speed - kV9PuppetBaseSpeed) * 1000.0F;
  return std::fabs(d - 1.0F) < 0.2F ? 1 : 0;
}

// Arms on the rising edge of the code only. Returns true on a fresh arm.
inline bool v9PuppetUpdate(V9PuppetState& s, float speed, std::uint64_t now_us) noexcept {
  const int code = v9PuppetCode(speed);
  const bool rising = code == 1 && s.last_code != 1;
  s.last_code = code;
  if (rising) {
    s.armed = true;
    s.arm_us = now_us;
  }
  return rising;
}

// Seconds since the arm; a large negative value before any arm.
inline float v9PuppetClock(const V9PuppetState& s, std::uint64_t now_us) noexcept {
  if (!s.armed || now_us < s.arm_us) return -1000.0F;
  return static_cast<float>(now_us - s.arm_us) * 1.0e-6F;
}

#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
// The whole device glue, shared by palette_runtime.cpp and the host probe so
// the scored path and the flashed path are one piece of code. Call it for each
// board before that board renders. On a fresh arm both renderers get the scored
// R01+R03 front (p6_effect_probe "r01+r03"), with the drop hold_lead_s after the
// arm. Returns true while board 0's speed carries the code, so the caller keeps
// motion at kV9PuppetBaseSpeed for board 0.
inline bool v9PuppetStep(V9PuppetState& s, InhouseWaveformRenderer (&renderer)[2], unsigned board,
                         float primary_speed, std::uint64_t now_us) noexcept {
  if (v9PuppetUpdate(s, primary_speed, now_us)) {
    InhouseV9Front front{};
    front.enabled = true;
    front.hold_contraction = true;
    front.drop_s[0] = front.hold_lead_s;
    front.drop_count = 1U;
    for (auto& r : renderer) (void)r.setV9HostFront(front);
  }
  renderer[board & 1U].setV9HostClock(v9PuppetClock(s, now_us));
  return v9PuppetCode(primary_speed) != 0;
}
#endif

}  // namespace k1::core::visual
