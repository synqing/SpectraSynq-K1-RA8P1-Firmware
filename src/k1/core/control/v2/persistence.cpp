// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
#include "core/control/v2/persistence.h"

#include <cstring>

#include "core/control/v2/registry_view.h"

namespace k1::core::control::v2 {
namespace {

constexpr std::size_t kSlotsPerRegion = 3U;  // A, B, backup
constexpr std::size_t kBackupSlot = 2U;
constexpr std::size_t kChunk = 64U;

void put16(std::uint8_t* p, const std::uint16_t v) noexcept {
  p[0] = static_cast<std::uint8_t>(v);
  p[1] = static_cast<std::uint8_t>(v >> 8U);
}
void put32(std::uint8_t* p, const std::uint32_t v) noexcept {
  for (std::size_t i = 0U; i < 4U; ++i) p[i] = static_cast<std::uint8_t>(v >> (8U * i));
}
std::uint16_t get16(const std::uint8_t* p) noexcept {
  return static_cast<std::uint16_t>(p[0] | (p[1] << 8U));
}
std::uint32_t get32(const std::uint8_t* p) noexcept {
  return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8U) |
         (static_cast<std::uint32_t>(p[2]) << 16U) | (static_cast<std::uint32_t>(p[3]) << 24U);
}

// Incremental CRC-32 (IEEE, reflected) matching cv2::crc32Ieee for one-shot use.
std::uint32_t crcUpdate(std::uint32_t crc, const std::uint8_t* data, const std::size_t n) noexcept {
  for (std::size_t i = 0U; i < n; ++i) {
    crc ^= data[i];
    for (int b = 0; b < 8; ++b) crc = (crc >> 1U) ^ (0xEDB88320U & (0U - (crc & 1U)));
  }
  return crc;
}

bool newerThan(const std::uint32_t a, const std::uint32_t b) noexcept {
  return static_cast<std::int32_t>(a - b) > 0;
}

std::size_t slotOffset(const Region& r, const std::size_t slot) noexcept {
  return static_cast<std::size_t>(r.offset) + slot * static_cast<std::size_t>(r.slot_bytes);
}

bool canonicalValue(const cv2::ParameterDescriptor& d, const cv2::TypedValue& v) noexcept {
  if (v.kind != d.value_kind) return false;
  if (v.kind == cv2::ValueKind::kReal &&
      ((v.bits & 0x7F800000U) == 0x7F800000U || v.bits == 0x80000000U)) {
    return false;  // non-finite or negative zero
  }
  return legalValue(d, v);
}

}  // namespace

bool validLayout(const Layout& layout, const std::size_t store_size) noexcept {
  for (std::size_t i = 0U; i < kNamespaceCount; ++i) {
    const Region& a = layout.regions[i];
    const std::uint8_t id = static_cast<std::uint8_t>(a.ns);
    if (id == 0U || id > kNamespaceCount || a.slot_bytes <= kSlotHeaderBytes) return false;
    const std::size_t end = static_cast<std::size_t>(a.offset) + kSlotsPerRegion * a.slot_bytes;
    if (end > store_size) return false;
    for (std::size_t j = 0U; j < i; ++j) {
      const Region& b = layout.regions[j];
      const std::size_t b_end = static_cast<std::size_t>(b.offset) + kSlotsPerRegion * b.slot_bytes;
      if (b.ns == a.ns || (a.offset < b_end && b.offset < end)) return false;
    }
  }
  return true;
}

const Region* PersistentStore::region(const Namespace ns) const noexcept {
  if (!valid()) return nullptr;
  for (const Region& r : layout_.regions) {
    if (r.ns == ns) return &r;
  }
  return nullptr;
}

PersistentStore::SlotState PersistentStore::inspect(const Region& r, const std::size_t slot) const noexcept {
  SlotState s{};
  std::uint8_t h[kSlotHeaderBytes];
  const std::size_t base = slotOffset(r, slot);
  if (!store_.read(base, h, sizeof(h))) return s;
  if (get32(h) != kSlotMagic || h[4] != static_cast<std::uint8_t>(r.ns) || h[5] != 0U || get16(h + 10) != 0U ||
      get32(h + 24) != 0U || cv2::crc32Ieee(h, 28U) != get32(h + 28)) {
    return s;
  }
  s.version = {get16(h + 6), get16(h + 8)};
  s.generation = get32(h + 12);
  s.length = get32(h + 16);
  s.payload_crc = get32(h + 20);
  if (s.generation == 0U || s.length > r.slot_bytes - kSlotHeaderBytes) return s;
  s.header_ok = true;
  std::uint32_t crc = 0xFFFFFFFFU;
  std::uint8_t chunk[kChunk];
  for (std::size_t done = 0U; done < s.length;) {
    const std::size_t n = s.length - done < kChunk ? s.length - done : kChunk;
    if (!store_.read(base + kSlotHeaderBytes + done, chunk, n)) return s;
    crc = crcUpdate(crc, chunk, n);
    done += n;
  }
  s.complete = (crc ^ 0xFFFFFFFFU) == s.payload_crc;
  return s;
}

int PersistentStore::newest(const SlotState* states) const noexcept {
  int best = -1;
  for (int i = 0; i < 2; ++i) {
    if (states[i].complete && (best < 0 || newerThan(states[i].generation, states[best].generation))) best = i;
  }
  return best;
}

LoadStatus PersistentStore::load(const Namespace ns, const std::uint16_t supported_major, std::uint8_t* out,
                                 const std::size_t capacity, LoadInfo& info) const noexcept {
  const Region* r = region(ns);
  if (r == nullptr) return LoadStatus::kBadNamespace;
  const SlotState states[2] = {inspect(*r, 0U), inspect(*r, 1U)};
  const int best = newest(states);
  if (best < 0) return LoadStatus::kEmpty;
  const SlotState& s = states[best];
  if (s.version.major > supported_major) return LoadStatus::kFutureVersion;
  if (s.length > capacity || (s.length > 0U && out == nullptr)) return LoadStatus::kBufferTooSmall;
  if (s.length > 0U && !store_.read(slotOffset(*r, static_cast<std::size_t>(best)) + kSlotHeaderBytes, out, s.length)) {
    return LoadStatus::kStoreError;
  }
  if (cv2::crc32Ieee(out, s.length) != s.payload_crc) return LoadStatus::kStoreError;  // changed under us
  info = LoadInfo{s.version, s.generation, s.length, static_cast<std::uint8_t>(best)};
  return LoadStatus::kOk;
}

SaveStatus PersistentStore::save(const Namespace ns, const SchemaVersion version, const std::uint16_t supported_major,
                                 const std::uint8_t* payload, const std::size_t length) noexcept {
  const Region* r = region(ns);
  if (r == nullptr || version.major == 0U || version.major > supported_major) return SaveStatus::kBadNamespace;
  if (length > r->slot_bytes - kSlotHeaderBytes || (length > 0U && payload == nullptr)) return SaveStatus::kTooLarge;
  const SlotState states[2] = {inspect(*r, 0U), inspect(*r, 1U)};
  for (const SlotState& s : states) {
    if (s.complete && s.version.major > supported_major) return SaveStatus::kFutureVersionPresent;
  }
  const int best = newest(states);
  const std::size_t target = best == 0 ? 1U : 0U;
  std::uint32_t generation = best < 0 ? 1U : states[best].generation + 1U;
  if (generation == 0U) generation = 1U;
  const std::size_t base = slotOffset(*r, target);
  if (length > 0U && !store_.write(base + kSlotHeaderBytes, payload, length)) return SaveStatus::kStoreError;
  std::uint8_t h[kSlotHeaderBytes]{};
  put32(h, kSlotMagic);
  h[4] = static_cast<std::uint8_t>(ns);
  put16(h + 6, version.major);
  put16(h + 8, version.minor);
  put32(h + 12, generation);
  put32(h + 16, static_cast<std::uint32_t>(length));
  put32(h + 20, cv2::crc32Ieee(payload, length));
  put32(h + 28, cv2::crc32Ieee(h, 28U));
  if (!store_.write(base, h, sizeof(h))) return SaveStatus::kStoreError;
  const SlotState written = inspect(*r, target);
  return written.complete && written.generation == generation ? SaveStatus::kOk : SaveStatus::kStoreError;
}

bool PersistentStore::preserveNewest(const Namespace ns) noexcept {
  const Region* r = region(ns);
  if (r == nullptr) return false;
  const SlotState states[2] = {inspect(*r, 0U), inspect(*r, 1U)};
  const int best = newest(states);
  if (best < 0) return false;
  const std::size_t from = slotOffset(*r, static_cast<std::size_t>(best));
  const std::size_t to = slotOffset(*r, kBackupSlot);
  const std::size_t total = kSlotHeaderBytes + states[best].length;
  bool identical = true;
  std::uint8_t a[kChunk];
  std::uint8_t b[kChunk];
  for (std::size_t done = 0U; done < total && identical;) {
    const std::size_t n = total - done < kChunk ? total - done : kChunk;
    if (!store_.read(from + done, a, n) || !store_.read(to + done, b, n)) return false;
    identical = std::memcmp(a, b, n) == 0;
    done += n;
  }
  if (identical) return true;
  // Payload first, header last: a torn backup is simply invalid, never mistaken.
  for (std::size_t done = kSlotHeaderBytes; done < total;) {
    const std::size_t n = total - done < kChunk ? total - done : kChunk;
    if (!store_.read(from + done, a, n) || !store_.write(to + done, a, n)) return false;
    done += n;
  }
  if (!store_.read(from, a, kSlotHeaderBytes) || !store_.write(to, a, kSlotHeaderBytes)) return false;
  for (std::size_t done = 0U; done < total;) {
    const std::size_t n = total - done < kChunk ? total - done : kChunk;
    if (!store_.read(from + done, a, n) || !store_.read(to + done, b, n) || std::memcmp(a, b, n) != 0) return false;
    done += n;
  }
  return true;
}

LoadStatus PersistentStore::loadBackup(const Namespace ns, std::uint8_t* out, const std::size_t capacity,
                                       LoadInfo& info) const noexcept {
  const Region* r = region(ns);
  if (r == nullptr) return LoadStatus::kBadNamespace;
  const SlotState s = inspect(*r, kBackupSlot);
  if (!s.complete) return LoadStatus::kEmpty;
  if (s.length > capacity) return LoadStatus::kBufferTooSmall;
  if (s.length > 0U && !store_.read(slotOffset(*r, kBackupSlot) + kSlotHeaderBytes, out, s.length)) {
    return LoadStatus::kStoreError;
  }
  info = LoadInfo{s.version, s.generation, s.length, static_cast<std::uint8_t>(kBackupSlot)};
  return LoadStatus::kOk;
}

// --- control-state records ------------------------------------------------------------

std::size_t encodeControlState(const ControlStateImage& image, std::uint8_t* out, const std::size_t capacity) noexcept {
  const std::size_t size = 4U + kControlStateEntryBytes * image.count;
  if (out == nullptr || image.count > cv2::kMaxSnapshotEntries || capacity < size) return 0U;
  std::memset(out, 0, size);
  put16(out, image.count);
  for (std::size_t i = 0U; i < image.count; ++i) {
    std::uint8_t* e = out + 4U + kControlStateEntryBytes * i;
    const ControlStateImage::Entry& in = image.entries[i];
    put16(e, static_cast<std::uint16_t>(in.target.semantic_id | (static_cast<std::uint16_t>(in.target.channel) << 12U)));
    e[4] = static_cast<std::uint8_t>(in.value.kind);
    put32(e + 8, in.value.bits);
  }
  return size;
}

namespace {
bool addEntry(ControlStateImage& out, const cv2::TargetRef target, const cv2::TypedValue value) noexcept {
  const int index = targetIndex(target);
  if (index == kNoIndex || !canonicalValue(targetDescriptor(static_cast<std::size_t>(index)), value)) return false;
  for (std::size_t i = 0U; i < out.count; ++i) {
    if (out.entries[i].target == target) {
      out.entries[i].value = value;  // last record wins
      return true;
    }
  }
  if (out.count >= cv2::kMaxSnapshotEntries) return false;
  out.entries[out.count++] = {target, value};
  return true;
}
}  // namespace

bool decodeControlState(const std::uint8_t* in, const std::size_t length, ControlStateImage& out,
                        std::size_t& dropped) noexcept {
  if (in == nullptr || length < 4U) return false;
  const std::size_t count = get16(in);
  if (get16(in + 2) != 0U || count > cv2::kMaxSnapshotEntries || length != 4U + kControlStateEntryBytes * count) {
    return false;
  }
  ControlStateImage image{};
  dropped = 0U;
  for (std::size_t i = 0U; i < count; ++i) {
    const std::uint8_t* e = in + 4U + kControlStateEntryBytes * i;
    const std::uint16_t key = get16(e);
    const std::uint8_t channel = static_cast<std::uint8_t>(key >> 12U);
    const cv2::TargetRef target{static_cast<std::uint16_t>(key & 0x0FFFU), static_cast<cv2::Channel>(channel)};
    const cv2::TypedValue value{static_cast<cv2::ValueKind>(e[4]), get32(e + 8)};
    const bool reserved_clear = get16(e + 2) == 0U && e[5] == 0U && e[6] == 0U && e[7] == 0U;
    if (!reserved_clear || channel > 2U || !addEntry(image, target, value)) ++dropped;
  }
  out = image;
  return true;
}

bool migrateLegacyControlState(const std::uint8_t* in, const std::size_t length, ControlStateImage& out,
                               std::size_t& dropped) noexcept {
  if (in == nullptr || length < 4U) return false;
  const std::size_t count = get16(in);
  if (get16(in + 2) != 0U || length != 4U + kLegacyRecordBytes * count) return false;
  ControlStateImage image{};
  dropped = 0U;
  for (std::size_t i = 0U; i < count; ++i) {
    const std::uint8_t* rec = in + 4U + kLegacyRecordBytes * i;
    const gen::V1AliasEntry* alias = v1Alias(rec[0]);
    const cv2::ParameterDescriptor* d = alias != nullptr ? findParameter(alias->semantic_id) : nullptr;
    if (d == nullptr || rec[1] != 0U || rec[2] != 0U || rec[3] != 0U) {
      ++dropped;
      continue;
    }
    const cv2::TypedValue value{d->value_kind, get32(rec + 4)};
    if (!addEntry(image, {alias->semantic_id, alias->channel}, value)) ++dropped;
  }
  out = image;
  return true;
}

ControlLoadReport loadControlState(PersistentStore& store, const MigrationPolicy& policy,
                                   ControlStateImage& out) noexcept {
  ControlLoadReport report{};
  static std::uint8_t payload[kMaxControlStatePayload];
  static std::uint8_t encoded[kMaxControlStatePayload];
  static ControlStateImage image;
  LoadInfo info{};
  report.status = store.load(Namespace::kControlState, kControlStateVersion.major, payload, sizeof(payload), info);
  if (report.status != LoadStatus::kOk) return report;
  report.loaded = info.version;
  if (info.version.major == kControlStateVersion.major) {
    if (!decodeControlState(payload, info.length, image, report.dropped)) {
      report.status = LoadStatus::kStoreError;
      return report;
    }
    out = image;
    return report;
  }
  if (info.version.major != kLegacyControlStateMajor) {
    report.status = LoadStatus::kFutureVersion;  // unknown older major: never guessed at
    return report;
  }
  // Preserve the original bytes before anything is migrated.
  report.original_preserved = store.preserveNewest(Namespace::kControlState);
  if (!report.original_preserved) {
    report.status = LoadStatus::kMigrationBackupFailed;
    return report;
  }
  if (!migrateLegacyControlState(payload, info.length, image, report.dropped)) {
    report.status = LoadStatus::kStoreError;
    return report;
  }
  report.migrated = true;
  for (std::size_t i = 0U; policy.admitted != nullptr && i < policy.count; ++i) {
    const NamedMigration& m = policy.admitted[i];
    if (m.name != nullptr && m.apply != nullptr && m.apply(payload, info.length, image)) ++report.named_applied;
  }
  const std::size_t size = encodeControlState(image, encoded, sizeof(encoded));
  report.migrated_saved =
      size != 0U && store.save(Namespace::kControlState, kControlStateVersion, kControlStateVersion.major, encoded,
                               size) == SaveStatus::kOk;
  out = image;  // the migrated state is valid even if the re-save tears (next boot repeats it)
  return report;
}

SaveStatus saveControlState(PersistentStore& store, const ControlStateImage& image) noexcept {
  static std::uint8_t encoded[kMaxControlStatePayload];
  const std::size_t size = encodeControlState(image, encoded, sizeof(encoded));
  if (size == 0U) return SaveStatus::kTooLarge;
  return store.save(Namespace::kControlState, kControlStateVersion, kControlStateVersion.major, encoded, size);
}

}  // namespace k1::core::control::v2
