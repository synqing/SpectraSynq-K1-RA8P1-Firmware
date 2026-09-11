#pragma once

#include <cstdint>

#include "core/visual/channel_render_state.h"
#include "core/visual/visual_audio_frame.h"

namespace k1::core::visual {

struct ProductRenderResult final {
  bool rendered;
  std::uint16_t effective_mode;
};

[[nodiscard]] ProductRenderResult renderProductChannel(
    ChannelRenderState& channel, const VisualAudioFrameView& visual,
    float delta_seconds) noexcept;

}  // namespace k1::core::visual
