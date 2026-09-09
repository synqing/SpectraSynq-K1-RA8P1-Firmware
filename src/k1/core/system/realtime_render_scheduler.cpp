#include "core/system/realtime_render_scheduler.h"

#include <cmath>
#include <limits>

namespace k1::core::system {
namespace {

void saturatingIncrement(std::uint32_t& value) noexcept {
  if (value != std::numeric_limits<std::uint32_t>::max()) ++value;
}

}  // namespace

std::uint64_t RealtimeRenderScheduler::musicalStartUs(
    const MusicalRenderTarget& target) noexcept {
  const std::uint64_t lead =
      static_cast<std::uint64_t>(target.render_cost_us) + target.output_cost_us;
  return target.target_event_us > lead ? target.target_event_us - lead : 0U;
}

void RealtimeRenderScheduler::noteAudioPublication(
    const std::uint64_t publication_us) noexcept {
  pending_publication_us_ = publication_us;
  audio_pending_ = true;
}

bool RealtimeRenderScheduler::noteMusicalTarget(
    const MusicalRenderTarget& target) noexcept {
  if (!std::isfinite(target.confidence) ||
      target.confidence < kMinimumMusicalTargetConfidence ||
      target.target_event_us == 0U || target.output_cost_us == 0U) {
    saturatingIncrement(rejected_musical_targets_);
    return false;
  }
  if (have_last_musical_ && target.epoch_id == last_musical_epoch_ &&
      target.beat_index <= last_musical_beat_) {
    // Do not render the same predicted beat twice when AudioPipeline publishes
    // it on multiple AP frames.
    return false;
  }
  if (musical_pending_ && target.epoch_id == pending_musical_.epoch_id &&
      target.beat_index < pending_musical_.beat_index) {
    return false;
  }
  // Updating the same pending beat is allowed: a newer media/monotonic fit may
  // sharpen its deadline before it fires.
  pending_musical_ = target;
  musical_pending_ = true;
  return true;
}

RenderDecision RealtimeRenderScheduler::decide(
    const std::uint64_t now_us) noexcept {
  if (musical_pending_) {
    const std::uint64_t start_us = musicalStartUs(pending_musical_);
    if (now_us >= start_us) {
      const std::uint64_t late_by = now_us - start_us;
      if (late_by <= kMusicalRenderLateToleranceUs &&
          now_us <= pending_musical_.target_event_us) {
        const MusicalRenderTarget target = pending_musical_;
        musical_pending_ = false;
        have_last_musical_ = true;
        last_musical_epoch_ = target.epoch_id;
        last_musical_beat_ = target.beat_index;
        return {RenderTrigger::kMusicalTarget,
                target.source_publication_us,
                target.target_event_us,
                target.beat_index,
                target.confidence};
      }
      saturatingIncrement(missed_musical_targets_);
      musical_pending_ = false;
    } else {
      const std::uint64_t busy_budget =
          static_cast<std::uint64_t>(pending_musical_.render_cost_us) +
          pending_musical_.output_cost_us;
      // Protect the future musical deadline from a lower-priority render that
      // would still be on the wire when the musical render needs to begin.
      if (busy_budget >= start_us - now_us) {
        saturatingIncrement(musical_reservations_);
        return {};
      }
    }
  }

  if (audio_pending_) {
    audio_pending_ = false;
    if (now_us >= pending_publication_us_ &&
        (now_us - pending_publication_us_) <= kLatestAudioRenderStartUs) {
      return {RenderTrigger::kAudioPublication, pending_publication_us_, 0U, 0U,
              0.0F};
    }
    saturatingIncrement(rejected_late_publications_);
  }

  if (next_idle_render_us_ == 0U || now_us >= next_idle_render_us_) {
    if (next_idle_render_us_ != 0U) {
      const std::uint64_t missed =
          (now_us - next_idle_render_us_) / kIdleRenderPeriodUs;
      const std::uint64_t room =
          std::numeric_limits<std::uint32_t>::max() - idle_deadline_misses_;
      idle_deadline_misses_ += static_cast<std::uint32_t>(
          missed < room ? missed : room);
    }
    return {RenderTrigger::kIdleCadence, now_us, 0U, 0U, 0.0F};
  }
  return {};
}

void RealtimeRenderScheduler::noteRendered(
    const std::uint64_t render_started_us) noexcept {
  next_idle_render_us_ = render_started_us + kIdleRenderPeriodUs;
}

}  // namespace k1::core::system
