#include "core/visual/channel_render_state.h"

#include <cstddef>

namespace k1::core::visual {

ChannelRenderState::ChannelRenderState(const PixelChannelId channel) noexcept
    : channel_(channel) {}

void ChannelRenderState::prepareAudio(
    const contract::AudioFeaturesV1& shared) noexcept {
  applyAudioFocus(shared, audio_focus_, focused_audio_);
}

void ChannelRenderState::clearFrame() noexcept {
  for (Pixel8& pixel : frame_) {
    pixel = Pixel8{};
  }
}

}  // namespace k1::core::visual
