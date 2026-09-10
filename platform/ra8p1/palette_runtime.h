#pragma once
#include <cstddef>
#include <cstdint>
#include "core/visual/channel_render_state.h"
#include "core/visual/visual_audio_frame.h"

namespace k1::titan {
inline constexpr std::uint32_t kPaletteCatalogueOpcode = 15U;
inline constexpr std::uint32_t kPaletteConfigureOpcode = 16U;
inline constexpr std::uint32_t kPaletteStatusOpcode = 17U;
inline constexpr std::uint32_t kPaletteFrameOpcode = 18U;
inline constexpr std::uint32_t kPalettePeriodUs = 8333U;
// Flags: active, automatic catalogue cycle, physical bench output.
struct PaletteConfig {
  std::uint32_t version = 1U;
  std::uint32_t palette_a = 0U, palette_b = 1U;
  std::uint32_t mode_a = 0U, mode_b = 0U; // 0 = native palette preview.
  std::uint32_t flags = 0U, brightness = 24U, output_channel = 0U;
};
class PaletteRuntime {
 public:
  bool configure(const PaletteConfig& config, std::uint64_t now_us) noexcept;
  bool step(std::uint64_t now_us,
            const core::visual::VisualAudioFrameView* audio) noexcept;
  void stop() noexcept { config_.flags = 0U; }
  bool active() const noexcept { return (config_.flags & 1U) != 0U; }
  bool emitEnabled() const noexcept { return (config_.flags & 4U) != 0U; }
  const PaletteConfig& config() const noexcept { return config_; }
  const core::visual::ChannelRenderState& channel(unsigned i) const noexcept {
    return i ? b_ : a_;
  }
  std::size_t packBenchGrb(std::uint8_t* destination, std::size_t capacity,
                         unsigned pixels = 128U) const noexcept;
  std::size_t catalogueJson(char* out, std::size_t capacity) const noexcept;
  std::size_t statusJson(char* out, std::size_t capacity) const noexcept;
  void recordEmit(int result, std::uint32_t cycles) noexcept;
 private:
  core::visual::ChannelRenderState a_{core::visual::PixelChannelId::kChannelA};
  core::visual::ChannelRenderState b_{core::visual::PixelChannelId::kChannelB};
  PaletteConfig config_{};
  std::uint64_t next_us_ = 0U, last_us_ = 0U, cycle_start_us_ = 0U;
  std::uint64_t frames_ = 0U, skipped_ = 0U, emitted_ = 0U, emit_errors_ = 0U;
  std::uint32_t last_emit_cycles_ = 0U, maximum_emit_cycles_ = 0U;
  bool waiting_for_audio_ = false;
};
} // namespace k1::titan
