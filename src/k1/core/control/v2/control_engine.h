// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
//
// Engine-side authoritative control state for the K1 control contract v2.
// Portable, fixed capacity, allocation-free, no I/O and no JSON: safe to call
// from the engine's control task at a safe control boundary.
//
// Law:
// - apply() validates every operation of a transaction before mutating
//   anything; a transaction that changes state commits exactly one new
//   revision. Rendering reads committed state only.
// - Each target carries the commit revision that last changed it. Ordinary
//   absolute writes name the target revision they expect, so independent
//   A/B edits never reject each other while a stale same-target write gets an
//   explicit kRevisionConflict with the actual base and revision. Relative
//   deltas commute and may omit the check (they then apply to the current base).
// - Identity: engine epoch + commit revision. Before the revision would wrap
//   the engine opens a new epoch (revision restarts at 1, all identities
//   before it are invalid); epoch exhaustion refuses mutations explicitly.
//   Per writer, transaction ids strictly increase within a session; a new
//   session is opened by the writer before its ids could wrap.
// - Retries are idempotent: the same (writer, session, transaction id) with
//   the same content returns the original receipt without a second effect;
//   different content under that identity is kIdentityReuse.
// - A gesture holds a per-target lease. Preset recall, blackout and external
//   sync may override it; the overridden gesture is then superseded (never
//   replayed) and rearms from the returned base. A terminal op releases the
//   lease; an idle lease times out.
// - Base and effective are separate: receipts and snapshots carry accepted
//   bases; EffectiveObservation carries the response output and any mode
//   modulation (Liveiness) without touching bases.
#pragma once

#include <cstddef>
#include <cstdint>

#include "contract/control_v2/control_v2_objects.h"
#include "core/control/v2/registry_view.h"
#include "core/control/v2/response_dynamics.h"

namespace k1::core::control::v2 {

enum class Admission : std::uint8_t {
  kImplementedOnly = 0U,  // production: planned semantics answer kUnavailable
  kDeclared = 1U,         // bring-up/host: planned semantics store a base, reported inactive
};

struct EngineConfig final {
  cv2::Platform platform{cv2::Platform::kRt1062};
  Admission admission{Admission::kImplementedOnly};
  std::uint32_t initial_epoch{1U};     // persisted boot counter supplies this
  std::uint32_t initial_revision{0U};
  std::uint64_t lease_timeout_us{0U};  // 0 = the platform's declared timeout
  std::uint32_t observation_valid_for_us{100000U};
};

// Liveiness/mode modulation hook (MOD/VP implement; CTL owns the seam). It
// receives the effective control value and may produce a material value; it
// can never write a base.
struct ModulationInput final {
  cv2::TargetRef target{};
  cv2::TypedValue effective{};
  float liveiness{0.5F};
  bool liveiness_enabled{false};
  std::uint16_t mode_id{0U};
};
struct ModulatorHook final {
  std::uint8_t adapter_version{0U};
  bool (*modulate)(void* context, const ModulationInput& input, cv2::TypedValue& material) noexcept{nullptr};
  void* context{nullptr};
};

class ControlEngine final {
 public:
  static constexpr std::size_t kMaxLeases = 16U;
  static constexpr std::size_t kMaxWriters = 4U;
  static constexpr std::size_t kDuplicateWindow = 16U;
  static constexpr std::size_t kTargets = gen::kTargetCount;

  [[nodiscard]] bool reset(const EngineConfig& config, std::uint64_t now_us) noexcept;
  void apply(const cv2::ControlTransaction& txn, std::uint64_t now_us, cv2::ApplyReceipt& out) noexcept;
  // Profile activation (the engine only knows the identity). Revisions must
  // strictly increase; activation retires every gesture lease.
  [[nodiscard]] bool activateProfile(const cv2::ProfileIdentity& identity, std::uint64_t now_us) noexcept;
  void advance(std::uint64_t now_us) noexcept;  // lease expiry
  void snapshot(std::uint64_t now_us, cv2::StateSnapshot& out) const noexcept;
  [[nodiscard]] bool observe(const cv2::TargetRef& target, std::uint64_t now_us,
                             cv2::EffectiveObservation& out) const noexcept;
  void setModulator(const ModulatorHook* hook) noexcept { modulator_ = hook; }

  [[nodiscard]] std::uint32_t epoch() const noexcept { return epoch_; }
  [[nodiscard]] std::uint32_t commitRevision() const noexcept { return revision_; }
  [[nodiscard]] bool epochExhausted() const noexcept { return exhausted_; }
  [[nodiscard]] cv2::TypedValue base(const cv2::TargetRef& target) const noexcept;
  [[nodiscard]] std::uint32_t targetRevision(const cv2::TargetRef& target) const noexcept;
  [[nodiscard]] std::size_t activeLeases(std::uint64_t now_us) const noexcept;

 private:
  struct Identity final {
    std::uint16_t writer{0U};
    std::uint32_t session{0U};
    std::uint32_t gesture{0U};
    [[nodiscard]] bool matches(std::uint16_t w, std::uint32_t s, std::uint32_t g) const noexcept {
      return g != 0U && writer == w && session == s && gesture == g;
    }
  };
  struct TargetState final {
    cv2::TypedValue base{};
    std::uint32_t revision{0U};
    ResponseState response{};
    Identity last_writer{};
    Identity superseded{};
  };
  struct Lease final {
    bool active{false};
    std::uint16_t target{0U};
    Identity holder{};
    std::uint64_t last_activity_us{0U};
  };
  struct Writer final {
    bool used{false};
    std::uint16_t id{0U};
    std::uint32_t session{0U};
    std::uint32_t last_transaction{0U};
  };
  struct CacheEntry final {
    bool used{false};
    std::uint16_t writer{0U};
    std::uint32_t session{0U};
    std::uint32_t transaction{0U};
    std::uint8_t digest[16]{};
    cv2::ApplyReceipt receipt{};
  };

  [[nodiscard]] bool leaseLive(const Lease& lease, std::uint64_t now_us) const noexcept;
  [[nodiscard]] int findLease(std::size_t target, std::uint64_t now_us) const noexcept;
  [[nodiscard]] cv2::InactiveReason inactiveReason(std::size_t target) const noexcept;
  [[nodiscard]] cv2::Support support(std::uint16_t semantic_id) const noexcept;
  [[nodiscard]] bool openRevision() noexcept;
  void resetTargets(std::uint64_t now_us) noexcept;
  void fillResult(std::size_t target, cv2::ReceiptOpResult& result, std::uint64_t now_us) const noexcept;
  void reject(const cv2::ControlTransaction& txn, cv2::Outcome outcome, cv2::Reason reason,
              std::size_t failing, std::uint64_t now_us, cv2::ApplyReceipt& out) const noexcept;
  void remember(const cv2::ControlTransaction& txn, const std::uint8_t digest[16],
                const cv2::ApplyReceipt& receipt) noexcept;

  EngineConfig config_{};
  const gen::PlatformProfile* platform_{nullptr};
  std::uint64_t lease_timeout_us_{0U};
  std::uint32_t epoch_{1U};
  std::uint32_t revision_{0U};
  bool exhausted_{false};
  bool profile_active_{false};
  cv2::ProfileIdentity profile_{};
  TargetState targets_[kTargets]{};
  Lease leases_[kMaxLeases]{};
  Writer writers_[kMaxWriters]{};
  CacheEntry cache_[kDuplicateWindow]{};
  std::size_t cache_next_{0U};
  const ModulatorHook* modulator_{nullptr};
};

// Converts a captured gesture intent into its single-op engine transaction
// (transaction id = intent sequence; absolute -> set, relative -> delta).
[[nodiscard]] cv2::ControlTransaction transactionFromIntent(const cv2::GestureIntent& intent) noexcept;

}  // namespace k1::core::control::v2
