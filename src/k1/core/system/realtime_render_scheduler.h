#pragma once

#include <cstdint>

namespace k1::core::system {

inline constexpr std::uint64_t kIdleRenderPeriodUs = 8333U;
inline constexpr std::uint64_t kAudioVisualCeilingUs = 8000U;
inline constexpr std::uint64_t kEffectBudgetUs = 2000U;
inline constexpr std::uint64_t kParallelLedWireBudgetUs = 4800U;
inline constexpr std::uint64_t kLatestAudioRenderStartUs =
    kAudioVisualCeilingUs - kEffectBudgetUs - kParallelLedWireBudgetUs;
inline constexpr std::uint64_t kMusicalRenderLateToleranceUs = 750U;
inline constexpr float kMinimumMusicalTargetConfidence = 0.55F;

enum class RenderTrigger : std::uint8_t {
  kNone = 0U,
  kAudioPublication,
  kMusicalTarget,
  kIdleCadence,
};

struct MusicalRenderTarget final {
  std::uint64_t epoch_id = 0U;
  std::uint64_t beat_index = 0U;
  std::uint64_t target_event_us = 0U;       // desired LED latch instant
  std::uint64_t source_publication_us = 0U; // latest audio evidence behind target
  std::uint32_t render_cost_us = 0U;
  std::uint32_t output_cost_us = 0U;
  float confidence = 0.0F;
};

struct RenderDecision final {
  RenderTrigger trigger = RenderTrigger::kNone;
  std::uint64_t source_publication_us = 0U;
  std::uint64_t target_event_us = 0U;
  std::uint64_t beat_index = 0U;
  float target_confidence = 0.0F;
};

// Shared-core scheduler. It owns policy only: no RT1062/Teensy/RA8P1 APIs.
// Platform adapters supply monotonic microseconds and measured/predicted output
// costs. This lets the same scheduler run behind RT1062 today and RA8P1 later.
class RealtimeRenderScheduler final {
 public:
  void noteAudioPublication(std::uint64_t publication_us) noexcept;
  [[nodiscard]] bool noteMusicalTarget(
      const MusicalRenderTarget& target) noexcept;
  [[nodiscard]] RenderDecision decide(std::uint64_t now_us) noexcept;
  void noteRendered(std::uint64_t render_started_us) noexcept;

  [[nodiscard]] std::uint32_t rejected_late_publications() const noexcept {
    return rejected_late_publications_;
  }
  [[nodiscard]] std::uint32_t idle_deadline_misses() const noexcept {
    return idle_deadline_misses_;
  }
  [[nodiscard]] std::uint32_t missed_musical_targets() const noexcept {
    return missed_musical_targets_;
  }
  [[nodiscard]] std::uint32_t rejected_musical_targets() const noexcept {
    return rejected_musical_targets_;
  }
  [[nodiscard]] std::uint32_t musical_reservations() const noexcept {
    return musical_reservations_;
  }

 private:
  [[nodiscard]] static std::uint64_t musicalStartUs(
      const MusicalRenderTarget& target) noexcept;

  MusicalRenderTarget pending_musical_{};
  std::uint64_t pending_publication_us_ = 0U;
  std::uint64_t next_idle_render_us_ = 0U;
  std::uint64_t last_musical_epoch_ = 0U;
  std::uint64_t last_musical_beat_ = 0U;
  std::uint32_t rejected_late_publications_ = 0U;
  std::uint32_t idle_deadline_misses_ = 0U;
  std::uint32_t missed_musical_targets_ = 0U;
  std::uint32_t rejected_musical_targets_ = 0U;
  std::uint32_t musical_reservations_ = 0U;
  bool audio_pending_ = false;
  bool musical_pending_ = false;
  bool have_last_musical_ = false;
};

}  // namespace k1::core::system
