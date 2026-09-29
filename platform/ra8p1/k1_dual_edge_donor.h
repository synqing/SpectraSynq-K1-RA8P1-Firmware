#pragma once
#include "core/visual/channel_render_state.h"
#include "core/visual/visual_audio_frame.h"
#include "core/visual/ws2816_pack.h"
namespace k1::titan {
struct DualEdgeDonorContinuity {
  std::uint64_t donor_next_us;
  std::uint64_t phase_next_us;
  float hue_position;
  float hue_shift_speed;
  float visual_gain;
};
void resetDualEdgeDonor() noexcept;
// Audio loss. Keeps the LED field, transport timers, hue and gain follower.
void invalidateDualEdgeDonorAudio() noexcept;
DualEdgeDonorContinuity dualEdgeDonorContinuity() noexcept;
void renderDualEdgeDonor(std::uint64_t now_us,const core::visual::VisualAudioFrameView* audio,
 core::visual::ChannelRenderState& a,core::visual::ChannelRenderState& b,
 core::visual::Pixel16* top,core::visual::Pixel16* bottom) noexcept;
}
