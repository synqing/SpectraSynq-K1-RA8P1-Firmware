// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
//
// Power-safe persistence for the K1 control contract v2 over an abstract byte
// store. Portable: no filesystem, flash driver or allocation; the platform
// supplies a ByteStore (NVS/flash/EEPROM adapter) through hal/.
//
// Law:
// - Each namespace owns a separate region: slot A, slot B and a backup slot.
//   A save writes the payload, then the 32-byte header (header CRC last), into
//   the slot not holding the newest valid record, so a write torn at any byte
//   leaves the previous record loadable. Load takes the newest slot whose
//   header and payload CRCs both hold; generations compare in serial order.
// - A record whose schema major is newer than the reader supports is never
//   interpreted: load reports kFutureVersion (the caller keeps its defaults)
//   and every save to that namespace is refused, so newer data survives a
//   downgrade untouched.
// - Migration of an older major first preserves the original slot bytes
//   verbatim in the backup slot (read back and verified); only then is the
//   migrated record saved. Behavioural remaps (Mood -> Liveiness) happen only
//   through a named migration the caller explicitly admits; the default set
//   is empty, so legacy Mood stays on its legacy alias.
#pragma once

#include <cstddef>
#include <cstdint>

#include "contract/control_v2/control_v2_objects.h"

namespace k1::core::control::v2 {

namespace cv2 = ::k1::control_v2;

class ByteStore {
 public:
  [[nodiscard]] virtual std::size_t size() const noexcept = 0;
  [[nodiscard]] virtual bool read(std::size_t offset, std::uint8_t* out, std::size_t length) const noexcept = 0;
  // May fail part-way (power loss); a false return means the bytes written so
  // far are unknown.
  [[nodiscard]] virtual bool write(std::size_t offset, const std::uint8_t* in, std::size_t length) noexcept = 0;

 protected:
  ~ByteStore() = default;
};

enum class Namespace : std::uint8_t {
  kControlState = 1U,  // accepted bases
  kProfile = 2U,       // compiled profile bytes (console / standalone)
  kCalibration = 3U,   // input calibration
  kSelection = 4U,     // active profile / page selection
};
inline constexpr std::size_t kNamespaceCount = 4U;

inline constexpr std::uint32_t kSlotMagic = 0x5043314BU;  // "K1CP" little-endian
inline constexpr std::size_t kSlotHeaderBytes = 32U;
// Slot header (little-endian):
//   0 u32 magic  4 u8 namespace  5 u8 reserved  6 u16 schema_major
//   8 u16 schema_minor  10 u16 reserved  12 u32 generation (>= 1)
//  16 u32 payload_length  20 u32 payload_crc32  24 u32 reserved
//  28 u32 header_crc32 over bytes 0..27

struct Region final {
  Namespace ns{Namespace::kControlState};
  std::uint32_t offset{0U};
  std::uint32_t slot_bytes{0U};  // header + payload capacity; region = 3 slots
};
struct Layout final {
  Region regions[kNamespaceCount]{};
};
[[nodiscard]] bool validLayout(const Layout& layout, std::size_t store_size) noexcept;

struct SchemaVersion final {
  std::uint16_t major{0U};
  std::uint16_t minor{0U};
};

enum class LoadStatus : std::uint8_t {
  kOk = 0U,
  kEmpty = 1U,           // no slot holds a complete record
  kFutureVersion = 2U,   // newest record is from a newer major: not interpreted
  kBufferTooSmall = 3U,
  kStoreError = 4U,
  kBadNamespace = 5U,
  kMigrationBackupFailed = 6U,
};
enum class SaveStatus : std::uint8_t {
  kOk = 0U,
  kFutureVersionPresent = 1U,  // refused: would overwrite newer data
  kTooLarge = 2U,
  kStoreError = 3U,            // torn or failed write; the previous record remains
  kBadNamespace = 4U,
};

struct LoadInfo final {
  SchemaVersion version{};
  std::uint32_t generation{0U};
  std::uint32_t length{0U};
  std::uint8_t slot{0xFFU};  // 0 = A, 1 = B
};

class PersistentStore final {
 public:
  PersistentStore(ByteStore& store, const Layout& layout) noexcept : store_(store), layout_(layout) {}
  [[nodiscard]] bool valid() const noexcept { return validLayout(layout_, store_.size()); }
  [[nodiscard]] LoadStatus load(Namespace ns, std::uint16_t supported_major, std::uint8_t* out,
                                std::size_t capacity, LoadInfo& info) const noexcept;
  [[nodiscard]] SaveStatus save(Namespace ns, SchemaVersion version, std::uint16_t supported_major,
                                const std::uint8_t* payload, std::size_t length) noexcept;
  // Copies the newest valid slot verbatim into the backup slot (read-back
  // verified); skips the write if the backup already holds identical bytes.
  [[nodiscard]] bool preserveNewest(Namespace ns) noexcept;
  [[nodiscard]] LoadStatus loadBackup(Namespace ns, std::uint8_t* out, std::size_t capacity,
                                      LoadInfo& info) const noexcept;

 private:
  struct SlotState final {
    bool header_ok{false};
    bool complete{false};
    SchemaVersion version{};
    std::uint32_t generation{0U};
    std::uint32_t length{0U};
    std::uint32_t payload_crc{0U};
  };
  [[nodiscard]] const Region* region(Namespace ns) const noexcept;
  [[nodiscard]] SlotState inspect(const Region& r, std::size_t slot) const noexcept;
  [[nodiscard]] int newest(const SlotState* states) const noexcept;

  ByteStore& store_;
  Layout layout_;
};

// --- control-state records --------------------------------------------------------

inline constexpr SchemaVersion kControlStateVersion{2U, 0U};
inline constexpr std::uint16_t kLegacyControlStateMajor = 1U;
inline constexpr std::size_t kControlStateEntryBytes = 12U;
inline constexpr std::size_t kLegacyRecordBytes = 8U;
inline constexpr std::size_t kMaxControlStatePayload = 4U + kControlStateEntryBytes * cv2::kMaxSnapshotEntries;

struct ControlStateImage final {
  struct Entry final {
    cv2::TargetRef target{};
    cv2::TypedValue value{};
  };
  std::uint16_t count{0U};
  Entry entries[cv2::kMaxSnapshotEntries]{};
};

// A behavioural remap the caller may admit by name (never applied by default).
struct NamedMigration final {
  const char* name{nullptr};
  // Receives the verbatim legacy payload and the default-migrated image.
  bool (*apply)(const std::uint8_t* legacy_payload, std::size_t length, ControlStateImage& image) noexcept{nullptr};
};
struct MigrationPolicy final {
  const NamedMigration* admitted{nullptr};
  std::size_t count{0U};
};

struct ControlLoadReport final {
  LoadStatus status{LoadStatus::kEmpty};
  SchemaVersion loaded{};
  bool migrated{false};
  bool original_preserved{false};
  bool migrated_saved{false};
  std::size_t dropped{0U};         // records naming no known target or an illegal value
  std::size_t named_applied{0U};
};

[[nodiscard]] std::size_t encodeControlState(const ControlStateImage& image, std::uint8_t* out,
                                             std::size_t capacity) noexcept;
// Decodes a major-2 record; entries that are unknown or illegal are dropped.
[[nodiscard]] bool decodeControlState(const std::uint8_t* in, std::size_t length, ControlStateImage& out,
                                      std::size_t& dropped) noexcept;
// Default (parity) migration of a major-1 record: v1 control ids map only to
// their declared v1 alias targets (Mood -> mood_legacy, never Liveiness).
[[nodiscard]] bool migrateLegacyControlState(const std::uint8_t* in, std::size_t length, ControlStateImage& out,
                                             std::size_t& dropped) noexcept;

// Loads (and when needed migrates and re-saves) the control state. `out` is
// written only on kOk.
[[nodiscard]] ControlLoadReport loadControlState(PersistentStore& store, const MigrationPolicy& policy,
                                                 ControlStateImage& out) noexcept;
[[nodiscard]] SaveStatus saveControlState(PersistentStore& store, const ControlStateImage& image) noexcept;

}  // namespace k1::core::control::v2
