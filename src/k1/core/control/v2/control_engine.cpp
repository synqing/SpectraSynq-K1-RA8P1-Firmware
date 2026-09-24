// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
#include "core/control/v2/control_engine.h"

#include <cstring>

namespace k1::core::control::v2 {
namespace {

std::size_t minSize(const std::size_t a, const std::size_t b) noexcept { return a < b ? a : b; }

double clampDouble(const double x, const double lo, const double hi) noexcept {
  return x < lo ? lo : (x > hi ? hi : x);
}

int enableTarget(const cv2::ParameterDescriptor& d, const cv2::Channel channel) noexcept {
  return d.enable_semantic_id == 0U ? kNoIndex : targetIndex({d.enable_semantic_id, channel});
}

}  // namespace

bool ControlEngine::reset(const EngineConfig& config, const std::uint64_t now_us) noexcept {
  const gen::PlatformProfile* platform = platformProfile(config.platform);
  if (platform == nullptr || !platform->engine || config.initial_epoch == 0U ||
      platform->max_targets < kTargets) {
    return false;
  }
  config_ = config;
  platform_ = platform;
  lease_timeout_us_ = config.lease_timeout_us != 0U ? config.lease_timeout_us : platform->lease_timeout_us;
  epoch_ = config.initial_epoch;
  revision_ = config.initial_revision;
  exhausted_ = false;
  profile_active_ = false;
  profile_ = cv2::ProfileIdentity{};
  resetTargets(now_us);
  for (Writer& w : writers_) w = Writer{};
  for (CacheEntry& c : cache_) c.used = false;
  cache_next_ = 0U;
  return true;
}

void ControlEngine::resetTargets(const std::uint64_t now_us) noexcept {
  for (std::size_t i = 0U; i < kTargets; ++i) {
    const cv2::ParameterDescriptor& d = targetDescriptor(i);
    TargetState& t = targets_[i];
    t = TargetState{};
    t.base = d.default_value;
    responseReset(t.response, d.default_value.asNumber(), now_us);
  }
  for (Lease& l : leases_) l = Lease{};
}

cv2::Support ControlEngine::support(const std::uint16_t semantic_id) const noexcept {
  const int index = parameterIndex(semantic_id);
  return index == kNoIndex ? cv2::Support::kUnsupported : platform_->parameter_support[index].support;
}

bool ControlEngine::leaseLive(const Lease& lease, const std::uint64_t now_us) const noexcept {
  return lease.active && (now_us < lease.last_activity_us ||
                          now_us - lease.last_activity_us <= lease_timeout_us_);
}

int ControlEngine::findLease(const std::size_t target, const std::uint64_t now_us) const noexcept {
  for (std::size_t i = 0U; i < kMaxLeases; ++i) {
    if (leaseLive(leases_[i], now_us) && leases_[i].target == target) {
      return static_cast<int>(i);
    }
  }
  return kNoIndex;
}

std::size_t ControlEngine::activeLeases(const std::uint64_t now_us) const noexcept {
  std::size_t n = 0U;
  for (const Lease& l : leases_) n += leaseLive(l, now_us) ? 1U : 0U;
  return n;
}

cv2::InactiveReason ControlEngine::inactiveReason(const std::size_t index) const noexcept {
  const cv2::ParameterDescriptor& d = targetDescriptor(index);
  const cv2::TargetRef target = targetAt(index);
  switch (support(d.semantic_id)) {
    case cv2::Support::kImplemented: break;
    case cv2::Support::kPlanned: return cv2::InactiveReason::kPlatformPlanned;
    case cv2::Support::kUnadvertised: return cv2::InactiveReason::kUnadvertised;
    case cv2::Support::kNotApplicable: return cv2::InactiveReason::kNotApplicable;
    case cv2::Support::kUnsupported: return cv2::InactiveReason::kPlatformUnsupported;
  }
  const int enable = enableTarget(d, target.channel);
  if (enable != kNoIndex && targets_[enable].base.bits == 0U) {
    return cv2::InactiveReason::kFeatureDisabled;
  }
  if (d.semantic_id == gen::kLiveiness) {
    const int mode = targetIndex({gen::kMode, target.channel});
    bool admitted = false;
    for (std::size_t i = 0U; mode != kNoIndex && i < platform_->mode_count; ++i) {
      admitted = admitted || (platform_->modes[i].mode_id == targets_[mode].base.bits &&
                              platform_->modes[i].support == cv2::Support::kImplemented);
    }
    if (!admitted) {
      return cv2::InactiveReason::kModeUnsupported;
    }
  }
  if (d.semantic_id == gen::kGlobalMaster) {
    const int blackout = targetIndex({gen::kBlackout, cv2::Channel::kGlobal});
    if (blackout != kNoIndex && targets_[blackout].base.bits != 0U) {
      return cv2::InactiveReason::kBlackoutActive;
    }
  }
  return cv2::InactiveReason::kNone;
}

void ControlEngine::fillResult(const std::size_t index, cv2::ReceiptOpResult& r,
                               const std::uint64_t now_us) const noexcept {
  r.target = targetAt(index);
  r.target_revision = targets_[index].revision;
  r.accepted_base = targets_[index].base;
  r.inactive_reason = inactiveReason(index);
  r.flags = static_cast<std::uint8_t>(
      (r.inactive_reason == cv2::InactiveReason::kNone ? cv2::kStateFlagActive : 0U) |
      (findLease(index, now_us) != kNoIndex ? cv2::kStateFlagLeaseHeld : 0U));
}

void ControlEngine::reject(const cv2::ControlTransaction& txn, const cv2::Outcome outcome,
                           const cv2::Reason reason, const std::size_t failing,
                           const std::uint64_t now_us, cv2::ApplyReceipt& out) const noexcept {
  out = cv2::ApplyReceipt{};
  out.writer_id = txn.writer_id;
  out.outcome = outcome;
  out.reason = reason;
  out.session_epoch = txn.session_epoch;
  out.transaction_id = txn.transaction_id;
  out.engine_epoch = epoch_;
  out.commit_revision = revision_;
  out.engine_time_us = now_us;
  const bool ops_known = txn.op_count <= cv2::kMaxTransactionOps;
  out.op_count = ops_known ? txn.op_count : 0U;
  out.failing_op_index = failing < out.op_count ? static_cast<std::uint8_t>(failing) : 0xFFU;
  for (std::size_t i = 0U; i < out.op_count; ++i) {
    cv2::ReceiptOpResult& r = out.results[i];
    const int index = targetIndex(txn.ops[i].target);
    if (index == kNoIndex) {
      r.target = txn.ops[i].target;
      r.inactive_reason = cv2::InactiveReason::kPlatformUnsupported;
    } else {
      fillResult(static_cast<std::size_t>(index), r, now_us);
    }
    const bool failed_here = i == failing;
    r.outcome = failed_here ? outcome : cv2::Outcome::kRejected;
    r.reason = failed_here ? reason : cv2::Reason::kTransactionAborted;
    if (r.outcome == cv2::Outcome::kAccepted) {  // never: a rejection names a failure
      r.outcome = cv2::Outcome::kRejected;
    }
  }
}

void ControlEngine::remember(const cv2::ControlTransaction& txn, const std::uint8_t digest[16],
                             const cv2::ApplyReceipt& receipt) noexcept {
  const std::size_t window = minSize(kDuplicateWindow, platform_->duplicate_window);
  if (window == 0U) {
    return;
  }
  CacheEntry& e = cache_[cache_next_ % window];
  cache_next_ = (cache_next_ + 1U) % window;
  e.used = true;
  e.writer = txn.writer_id;
  e.session = txn.session_epoch;
  e.transaction = txn.transaction_id;
  std::memcpy(e.digest, digest, sizeof(e.digest));
  e.receipt = receipt;
}

bool ControlEngine::openRevision() noexcept {
  if (revision_ < 0xFFFFFFFFU) {
    ++revision_;
    return true;
  }
  if (epoch_ == 0xFFFFFFFFU) {
    exhausted_ = true;
    return false;
  }
  // The revision would wrap: open a new epoch before any identity can alias.
  ++epoch_;
  revision_ = 1U;
  for (TargetState& t : targets_) {
    t.revision = 0U;
    t.last_writer = Identity{};
    t.superseded = Identity{};
  }
  for (Lease& l : leases_) l = Lease{};
  for (Writer& w : writers_) w = Writer{};
  for (CacheEntry& c : cache_) c.used = false;
  return true;
}

void ControlEngine::apply(const cv2::ControlTransaction& txn, const std::uint64_t now_us,
                          cv2::ApplyReceipt& out) noexcept {
  if (platform_ == nullptr || cv2::validate(txn) != cv2::CodecStatus::kOk) {
    reject(txn, cv2::Outcome::kRejected, cv2::Reason::kMalformed, 0xFFU, now_us, out);
    return;
  }
  if (exhausted_) {
    reject(txn, cv2::Outcome::kRejected, cv2::Reason::kEpochExhausted, 0xFFU, now_us, out);
    return;
  }
  if (txn.engine_epoch != epoch_) {
    reject(txn, cv2::Outcome::kRejected, cv2::Reason::kEpochMismatch, 0xFFU, now_us, out);
    return;
  }
  if (txn.op_count > platform_->max_transaction_ops) {
    reject(txn, cv2::Outcome::kRejected, cv2::Reason::kCapacityExceeded, 0xFFU, now_us, out);
    return;
  }
  // Writer and session identity.
  Writer* writer = nullptr;
  const std::size_t writer_limit = minSize(kMaxWriters, platform_->max_writers);
  for (std::size_t i = 0U; i < writer_limit; ++i) {
    if (writers_[i].used && writers_[i].id == txn.writer_id) writer = &writers_[i];
  }
  for (std::size_t i = 0U; writer == nullptr && i < writer_limit; ++i) {
    if (!writers_[i].used) {
      writers_[i] = Writer{true, txn.writer_id, 0U, 0U};
      writer = &writers_[i];
    }
  }
  if (writer == nullptr) {
    reject(txn, cv2::Outcome::kRejected, cv2::Reason::kCapacityExceeded, 0xFFU, now_us, out);
    return;
  }
  if (txn.session_epoch < writer->session) {
    reject(txn, cv2::Outcome::kRejected, cv2::Reason::kStaleSession, 0xFFU, now_us, out);
    return;
  }
  std::uint8_t encoded[cv2::kMaxControlTransactionBytes];
  std::uint8_t full_digest[cv2::kDigestBytes];
  const cv2::EncodeResult enc = cv2::encode(txn, encoded, sizeof(encoded));
  cv2::sha256(encoded, enc.size, full_digest);
  if (txn.session_epoch == writer->session) {
    for (const CacheEntry& e : cache_) {
      if (e.used && e.writer == txn.writer_id && e.session == txn.session_epoch &&
          e.transaction == txn.transaction_id) {
        if (std::memcmp(e.digest, full_digest, sizeof(e.digest)) == 0) {
          out = e.receipt;  // idempotent retry: original outcome, no second effect
        } else {
          reject(txn, cv2::Outcome::kRejected, cv2::Reason::kIdentityReuse, 0xFFU, now_us, out);
        }
        return;
      }
    }
    if (txn.transaction_id <= writer->last_transaction) {
      reject(txn, cv2::Outcome::kExpired, cv2::Reason::kDuplicateWindowExpired, 0xFFU, now_us, out);
      return;
    }
  } else {
    // A newer session: sequence restarts; the writer's older leases end.
    writer->session = txn.session_epoch;
    writer->last_transaction = 0U;
    for (Lease& l : leases_) {
      if (l.active && l.holder.writer == txn.writer_id) l.active = false;
    }
  }
  writer->last_transaction = txn.transaction_id;  // identity consumed from here on
  if (txn.deadline_us != 0U && now_us > txn.deadline_us) {
    reject(txn, cv2::Outcome::kExpired, cv2::Reason::kDeadlineExpired, 0xFFU, now_us, out);
    remember(txn, full_digest, out);
    return;
  }
  if (txn.profile_revision != 0U &&
      (!profile_active_ || profile_.profile_revision != txn.profile_revision)) {
    reject(txn, cv2::Outcome::kRejected, cv2::Reason::kProfileRevisionMismatch, 0xFFU, now_us, out);
    remember(txn, full_digest, out);
    return;
  }
  // Validate every operation before touching anything.
  std::size_t index[cv2::kMaxTransactionOps];
  cv2::TypedValue next[cv2::kMaxTransactionOps];
  int own_lease[cv2::kMaxTransactionOps];
  std::size_t leases_needed = 0U;
  bool changes = false;
  const bool override_leases = (txn.flags & cv2::kTxnFlagOverrideLeases) != 0U;
  for (std::size_t i = 0U; i < txn.op_count; ++i) {
    const cv2::ControlOp& op = txn.ops[i];
    auto fail = [&](const cv2::Outcome o, const cv2::Reason r) {
      reject(txn, o, r, i, now_us, out);
      remember(txn, full_digest, out);
    };
    const int t = targetIndex(op.target);
    if (t == kNoIndex) {
      fail(cv2::Outcome::kRejected,
           findParameter(op.target.semantic_id) != nullptr ? cv2::Reason::kScopeMismatch : cv2::Reason::kUnknownTarget);
      return;
    }
    index[i] = static_cast<std::size_t>(t);
    const cv2::ParameterDescriptor& d = targetDescriptor(index[i]);
    const cv2::Support sup = support(d.semantic_id);
    if (sup == cv2::Support::kUnadvertised) {
      fail(cv2::Outcome::kUnavailable, cv2::Reason::kUnadvertisedTarget);
      return;
    }
    if (sup != cv2::Support::kImplemented &&
        !(sup == cv2::Support::kPlanned && config_.admission == Admission::kDeclared)) {
      fail(cv2::Outcome::kUnavailable, cv2::Reason::kUnsupportedOnPlatform);
      return;
    }
    TargetState& ts = targets_[index[i]];
    if (op.op_kind == cv2::OpKind::kSetAbsolute) {
      if (op.value.kind != d.value_kind) {
        fail(cv2::Outcome::kRejected, cv2::Reason::kTypeMismatch);
        return;
      }
      if (!legalValue(d, op.value)) {
        fail(cv2::Outcome::kRejected, cv2::Reason::kOutOfDomain);
        return;
      }
      next[i] = op.value;
    } else {
      if ((d.flags & cv2::kParamFlagRelativeAdmitted) == 0U || op.value.kind != d.value_kind ||
          (platform_->input_modality_mask & cv2::kModalityMaskRelative) == 0U) {
        fail(cv2::Outcome::kRejected, cv2::Reason::kDeltaNotAdmitted);
        return;
      }
      if (d.value_kind == cv2::ValueKind::kInteger) {
        const std::int64_t sum = static_cast<std::int64_t>(ts.base.asInteger()) + op.value.asInteger();
        const std::int64_t lo = d.legal_min.asInteger();
        const std::int64_t hi = d.legal_max.asInteger();
        next[i] = cv2::TypedValue::integer(static_cast<std::int32_t>(sum < lo ? lo : (sum > hi ? hi : sum)));
      } else {
        next[i] = cv2::TypedValue::real(static_cast<float>(clampDouble(
            static_cast<double>(ts.base.asReal()) + static_cast<double>(op.value.asReal()),
            d.legal_min.asNumber(), d.legal_max.asNumber())));
      }
    }
    if (!policyAdmitted(d, op.response_policy_id, platform_->response_policy_mask)) {
      fail(cv2::Outcome::kRejected, cv2::Reason::kResponsePolicyNotAdmitted);
      return;
    }
    const std::uint32_t gesture = op.gesture_id;
    if (ts.superseded.matches(txn.writer_id, txn.session_epoch, gesture)) {
      fail(cv2::Outcome::kSuperseded, cv2::Reason::kLeaseSuperseded);
      return;
    }
    const int lease = findLease(index[i], now_us);
    own_lease[i] = lease != kNoIndex &&
                           leases_[lease].holder.matches(txn.writer_id, txn.session_epoch, gesture)
                       ? lease
                       : kNoIndex;
    if (lease != kNoIndex && own_lease[i] == kNoIndex && !override_leases) {
      fail(cv2::Outcome::kRejected, cv2::Reason::kLeaseHeld);
      return;
    }
    if (own_lease[i] == kNoIndex && !override_leases) {
      const bool self_continuation = ts.last_writer.matches(txn.writer_id, txn.session_epoch, gesture);
      if ((op.op_flags & cv2::kOpFlagCheckExpectedRevision) != 0U) {
        if (op.expected_target_revision != ts.revision && !self_continuation) {
          fail(cv2::Outcome::kRejected, cv2::Reason::kRevisionConflict);
          return;
        }
      } else if (!self_continuation && op.op_kind == cv2::OpKind::kSetAbsolute) {
        // Deltas commute, so they may omit the check; absolute writes may not.
        fail(cv2::Outcome::kRejected, cv2::Reason::kExpectedRevisionRequired);
        return;
      }
    }
    if (gesture != 0U && (op.op_flags & cv2::kOpFlagTerminal) == 0U && own_lease[i] == kNoIndex) {
      ++leases_needed;
    }
    changes = changes || next[i] != ts.base;
  }
  std::size_t free_leases = 0U;
  const std::size_t lease_limit = minSize(kMaxLeases, platform_->max_concurrent_leases);
  for (std::size_t i = 0U; i < lease_limit; ++i) {
    free_leases += leaseLive(leases_[i], now_us) ? 0U : 1U;
  }
  if (override_leases) {  // leases this transaction breaks become free
    for (std::size_t i = 0U; i < txn.op_count; ++i) {
      const int lease = findLease(index[i], now_us);
      free_leases += (lease != kNoIndex && own_lease[i] == kNoIndex && static_cast<std::size_t>(lease) < lease_limit) ? 1U : 0U;
    }
  }
  if (leases_needed > free_leases) {
    reject(txn, cv2::Outcome::kRejected, cv2::Reason::kCapacityExceeded, 0U, now_us, out);
    remember(txn, full_digest, out);
    return;
  }
  if (changes && revision_ == 0xFFFFFFFFU && epoch_ == 0xFFFFFFFFU) {
    exhausted_ = true;
    reject(txn, cv2::Outcome::kRejected, cv2::Reason::kEpochExhausted, 0xFFU, now_us, out);
    return;
  }
  // Commit: one revision for the whole transaction.
  if (changes) {
    const std::uint32_t epoch_before = epoch_;
    (void)openRevision();
    if (epoch_ != epoch_before) {  // new epoch: re-register this writer
      writers_[0] = Writer{true, txn.writer_id, txn.session_epoch, txn.transaction_id};
      writer = &writers_[0];
    }
  }
  for (std::size_t i = 0U; i < txn.op_count; ++i) {
    const cv2::ControlOp& op = txn.ops[i];
    TargetState& ts = targets_[index[i]];
    const cv2::ParameterDescriptor& d = targetDescriptor(index[i]);
    if (override_leases) {
      const int lease = findLease(index[i], now_us);
      if (lease != kNoIndex && lease != own_lease[i]) {
        ts.superseded = leases_[lease].holder;
        leases_[lease].active = false;
      } else if (ts.last_writer.gesture != 0U &&
                 !ts.last_writer.matches(txn.writer_id, txn.session_epoch, op.gesture_id)) {
        ts.superseded = ts.last_writer;
      }
    }
    if (next[i] != ts.base) {
      ts.base = next[i];
      ts.revision = revision_;
      if (d.value_kind == cv2::ValueKind::kReal) {
        responseRetarget(ts.response, next[i].asNumber(), resolveResponse(op.response_policy_id, d), now_us);
      } else {
        responseReset(ts.response, next[i].asNumber(), now_us);
      }
    }
    ts.last_writer = Identity{txn.writer_id, txn.session_epoch, op.gesture_id};
    if (op.gesture_id != 0U) {
      int lease = own_lease[i] != kNoIndex && leaseLive(leases_[own_lease[i]], now_us) ? own_lease[i] : kNoIndex;
      if ((op.op_flags & cv2::kOpFlagTerminal) != 0U) {
        if (lease != kNoIndex) leases_[lease].active = false;
      } else {
        for (std::size_t k = 0U; lease == kNoIndex && k < lease_limit; ++k) {
          if (!leaseLive(leases_[k], now_us)) lease = static_cast<int>(k);
        }
        if (lease != kNoIndex) {
          leases_[lease] = Lease{true, static_cast<std::uint16_t>(index[i]), ts.last_writer, now_us};
        }
      }
    }
  }
  out = cv2::ApplyReceipt{};
  out.writer_id = txn.writer_id;
  out.outcome = cv2::Outcome::kAccepted;
  out.reason = cv2::Reason::kNone;
  out.session_epoch = txn.session_epoch;
  out.transaction_id = txn.transaction_id;
  out.engine_epoch = epoch_;
  out.commit_revision = revision_;
  out.engine_time_us = now_us;
  out.op_count = txn.op_count;
  for (std::size_t i = 0U; i < txn.op_count; ++i) {
    fillResult(index[i], out.results[i], now_us);
    out.results[i].outcome = cv2::Outcome::kAccepted;
    out.results[i].reason = cv2::Reason::kNone;
  }
  remember(txn, full_digest, out);
}

bool ControlEngine::activateProfile(const cv2::ProfileIdentity& identity, const std::uint64_t now_us) noexcept {
  (void)now_us;
  if (platform_ == nullptr || exhausted_ || cv2::validate(identity) != cv2::CodecStatus::kOk ||
      (profile_active_ && identity.profile_revision <= profile_.profile_revision)) {
    return false;
  }
  if (!openRevision()) {
    return false;
  }
  for (Lease& l : leases_) l.active = false;  // old-profile gestures are retired
  profile_ = identity;
  profile_active_ = true;
  return true;
}

void ControlEngine::advance(const std::uint64_t now_us) noexcept {
  for (Lease& l : leases_) {
    if (l.active && !leaseLive(l, now_us)) l.active = false;
  }
}

void ControlEngine::snapshot(const std::uint64_t now_us, cv2::StateSnapshot& out) const noexcept {
  out = cv2::StateSnapshot{};
  out.engine_epoch = epoch_;
  out.snapshot_revision = revision_;
  out.profile_revision = profile_active_ ? profile_.profile_revision : 0U;
  std::memcpy(out.registry_sha256, gen::kRegistrySha256, cv2::kDigestBytes);
  if (profile_active_) {
    std::memcpy(out.profile_sha256, profile_.profile_sha256, cv2::kDigestBytes);
  }
  out.engine_time_us = now_us;
  out.entry_count = static_cast<std::uint16_t>(kTargets);
  for (std::size_t i = 0U; i < kTargets; ++i) {
    cv2::SnapshotEntry& e = out.entries[i];
    cv2::ReceiptOpResult r{};
    fillResult(i, r, now_us);
    e.target = r.target;
    e.target_revision = r.target_revision;
    e.base = r.accepted_base;
    e.flags = r.flags;
    e.inactive_reason = r.inactive_reason;
  }
}

bool ControlEngine::observe(const cv2::TargetRef& target, const std::uint64_t now_us,
                            cv2::EffectiveObservation& out) const noexcept {
  const int index = targetIndex(target);
  if (platform_ == nullptr || index == kNoIndex) {
    return false;
  }
  const cv2::ParameterDescriptor& d = targetDescriptor(static_cast<std::size_t>(index));
  const TargetState& t = targets_[index];
  out = cv2::EffectiveObservation{};
  out.target = target;
  out.engine_epoch = epoch_;
  out.base_revision = t.revision;
  out.observed_time_us = now_us;
  out.valid_for_us = config_.observation_valid_for_us;
  const cv2::Support sup = support(d.semantic_id);
  if (sup != cv2::Support::kImplemented &&
      !(sup == cv2::Support::kPlanned && config_.admission == Admission::kDeclared)) {
    out.availability = cv2::Availability::kUnsupported;
    return true;
  }
  out.availability = cv2::Availability::kAvailable;
  if (d.value_kind == cv2::ValueKind::kReal) {
    out.effective = cv2::TypedValue::real(static_cast<float>(responseEvaluate(t.response, now_us)));
    if (!responseSettled(t.response, now_us)) out.modulation_sources |= cv2::kModulationResponse;
  } else {
    out.effective = t.base;
  }
  if (d.semantic_id == gen::kGlobalMaster && inactiveReason(static_cast<std::size_t>(index)) ==
                                                 cv2::InactiveReason::kBlackoutActive) {
    out.effective = cv2::TypedValue::integer(0);  // blackout is immediate, never eased
    out.modulation_sources |= cv2::kModulationBlackout;
  }
  if (modulator_ != nullptr && modulator_->modulate != nullptr && target.channel != cv2::Channel::kGlobal) {
    const int live = targetIndex({gen::kLiveiness, target.channel});
    const int live_on = targetIndex({gen::kLiveinessOn, target.channel});
    const int mode = targetIndex({gen::kMode, target.channel});
    ModulationInput in{};
    in.target = target;
    in.effective = out.effective;
    in.liveiness = live != kNoIndex ? targets_[live].base.asReal() : 0.5F;
    in.liveiness_enabled = live_on != kNoIndex && targets_[live_on].base.bits != 0U;
    in.mode_id = mode != kNoIndex ? static_cast<std::uint16_t>(targets_[mode].base.bits) : 0U;
    cv2::TypedValue material{};
    if (in.liveiness_enabled && modulator_->modulate(modulator_->context, in, material) &&
        cv2::validate(cv2::EffectiveObservation{out.target, out.engine_epoch, out.base_revision,
                                                out.effective, out.observed_time_us, out.valid_for_us,
                                                out.time_domain, out.availability,
                                                out.modulation_sources, material}) == cv2::CodecStatus::kOk) {
      out.material = material;
      out.modulation_sources |= cv2::kModulationLiveiness;
    }
  }
  return true;
}

cv2::TypedValue ControlEngine::base(const cv2::TargetRef& target) const noexcept {
  const int index = targetIndex(target);
  return index == kNoIndex ? cv2::TypedValue{} : targets_[index].base;
}

std::uint32_t ControlEngine::targetRevision(const cv2::TargetRef& target) const noexcept {
  const int index = targetIndex(target);
  return index == kNoIndex ? 0U : targets_[index].revision;
}

cv2::ControlTransaction transactionFromIntent(const cv2::GestureIntent& g) noexcept {
  cv2::ControlTransaction t{};
  t.writer_id = g.writer_id;
  t.source = cv2::TransactionSource::kGesture;
  t.session_epoch = g.session_epoch;
  t.transaction_id = g.sequence;
  t.engine_epoch = g.engine_epoch;
  t.profile_revision = g.profile_revision;
  t.atomicity = cv2::Atomicity::kSingleTarget;
  t.op_count = 1U;
  cv2::ControlOp& op = t.ops[0];
  op.target = g.target;
  op.op_kind = g.intent_kind == cv2::IntentKind::kRelativeDelta ? cv2::OpKind::kApplyDelta
                                                                 : cv2::OpKind::kSetAbsolute;
  op.op_flags = static_cast<std::uint8_t>(
      cv2::kOpFlagCheckExpectedRevision |
      ((g.flags & cv2::kIntentFlagTerminal) != 0U ? cv2::kOpFlagTerminal : 0U));
  op.response_policy_id = g.response_policy_id;
  op.expected_target_revision = g.expected_target_revision;
  op.gesture_id = g.gesture_id;
  op.value = g.value;
  return t;
}

}  // namespace k1::core::control::v2
