#pragma once
// Donor musical-control codec for opcode 25. Legacy subcommands stay rejected.
// USB may parse this header. Only the realtime owner may call donorApply.
#include "emotiscope_port.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace k1::emotiscope {

inline constexpr std::uint32_t kDonorCfgGetState = 0x454D0101u;
inline constexpr std::uint32_t kDonorCfgPatch = 0x454D0102u;
inline constexpr std::uint32_t kDonorStateMagic = 0x454D5354u;
inline constexpr std::uint32_t kDonorStateVersion = 1u;
inline constexpr std::uint32_t kDonorEngineId = 1u;
inline constexpr std::uint32_t kDonorStateBytes = 160u;
inline constexpr std::uint32_t kDonorGetBytes = 16u;
inline constexpr std::uint32_t kDonorPatchHeaderBytes = 24u;
inline constexpr std::uint32_t kDonorEntryBytes = 8u;
inline constexpr std::uint32_t kDonorPatchMaxBytes = 160u;
inline constexpr std::uint32_t kDonorFieldCount = 17u;
inline constexpr std::uint32_t kDonorFieldMask = 0x1ffffu;
inline constexpr std::uint32_t kDonorFieldOutputPrecision = 17u;
inline constexpr std::uint32_t kDonorFlagOutputPrecision = 64u;
inline constexpr std::uint32_t kDonorDedupLive = 32u;
inline constexpr std::uint32_t kDonorDedupTomb = 32u;
inline constexpr std::uint32_t kDonorStatusOk = 0u;
inline constexpr std::uint32_t kDonorStatusInvalid = 3u;
inline constexpr std::uint32_t kDonorStatusStale = 5u;
inline constexpr std::uint32_t kDonorStatusWrongBoot = 6u;
inline constexpr std::uint32_t kDonorStatusConflict = 7u;
inline constexpr std::uint32_t kDonorStatusReconcile = 9u;
inline constexpr std::uint32_t kDonorStatusBusy = 10u;
inline constexpr std::uint32_t kDonorKindF32 = 1u;
inline constexpr std::uint32_t kDonorKindBool = 2u;
inline constexpr std::uint32_t kDonorKindI32 = 3u;
inline constexpr std::uint32_t kDonorKindU32 = 4u;

inline bool donorProfileRejectsPaletteConfigure() noexcept { return true; }
inline bool donorProfileAcceptsConfigSub(std::uint32_t sub) noexcept {
  return sub == kDonorCfgGetState || sub == kDonorCfgPatch;
}

inline std::uint32_t donorCrc32(const std::uint8_t* data, std::size_t size) noexcept {
  std::uint32_t crc = 0xffffffffu;
  for (std::size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
  }
  return ~crc;
}
inline void donorPutU32(std::uint8_t* p, std::uint32_t value) noexcept {
  p[0] = static_cast<std::uint8_t>(value);
  p[1] = static_cast<std::uint8_t>(value >> 8);
  p[2] = static_cast<std::uint8_t>(value >> 16);
  p[3] = static_cast<std::uint8_t>(value >> 24);
}
inline std::uint32_t donorGetU32(const std::uint8_t* p) noexcept {
  return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
         (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
inline void donorPutU16(std::uint8_t* p, std::uint16_t value) noexcept {
  p[0] = static_cast<std::uint8_t>(value);
  p[1] = static_cast<std::uint8_t>(value >> 8);
}
inline std::uint16_t donorGetU16(const std::uint8_t* p) noexcept {
  return std::uint16_t(p[0] | (p[1] << 8));
}
inline void donorPutF32(std::uint8_t* p, float value) noexcept {
  std::uint32_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  donorPutU32(p, bits);
}
inline float donorGetF32(const std::uint8_t* p) noexcept {
  const std::uint32_t bits = donorGetU32(p);
  float value = 0;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}
inline std::uint32_t donorPopcount(std::uint32_t mask) noexcept {
  std::uint32_t count = 0;
  while (mask) { count += mask & 1u; mask >>= 1; }
  return count;
}
inline std::uint32_t donorFieldKind(std::uint32_t field) noexcept {
  if (field >= 1u && field <= 8u) return kDonorKindF32;
  if (field == 9u) return kDonorKindI32;
  if ((field >= 10u && field <= 14u) || field == 16u || field == 17u) return kDonorKindBool;
  if (field == 15u) return kDonorKindU32;
  return 0u;
}

struct DonorIntent {
  std::uint32_t sub = 0, request_id = 0, boot_id = 0, expected_revision = 0;
  std::uint32_t mask = 0, payload_crc = 0;
  float brightness = 0, softness = 0, color = 0, warmth = 0;
  float color_range = 0, speed = 0, saturation = 0, background = 0;
  std::int32_t mode = 0;
  std::uint8_t mirror = 0, screensaver = 0, dither = 0, auto_color = 0, reverse = 0;
  std::uint32_t hardware_brightness = 0, emission = 0, output_precision = 0;
};

struct DonorDedupSlot {
  std::uint32_t id = 0, crc = 0;
  std::uint8_t response[kDonorStateBytes]{};
  std::uint8_t valid = 0;
};
struct DonorDedup {
  DonorDedupSlot live[kDonorDedupLive]{};
  std::uint32_t tomb[kDonorDedupTomb]{};
  std::uint8_t tomb_valid[kDonorDedupTomb]{};
  std::uint8_t live_next = 0, tomb_next = 0, live_count = 0, tomb_count = 0;
};

struct DonorObserved {
  std::uint32_t boot_id = 0, revision = 0, hardware_brightness = 0, emission = 0;
  std::uint32_t output_precision = 0;
  std::uint32_t accepted_request_id = 0, rendered_request_id = 0;
  std::uint32_t emitted_request_id = 0, rendered_frame = 0;
  std::uint8_t schema_sha[32]{};
};
struct DonorApplyResult {
  std::uint32_t status = 0, revision = 0, hardware_brightness = 0, emission = 0;
  std::uint32_t output_precision = 0;
  std::uint8_t changed = 0, mode_transition = 0, revision_changed = 0;
  std::uint8_t output_precision_changed = 0;
  std::uint8_t brightness_changed = 0, emission_changed = 0, emission_fell = 0;
  std::uint8_t mode_changed = 0;
  std::int32_t mode = 0;
  std::uint8_t response[kDonorStateBytes]{};
};

inline std::uint32_t donorApplicability(const K1EmotiscopeConfig& config) noexcept {
  // Bits 0..15 only; output_precision (field 17) is always applicable.
  std::uint32_t bits = 0xffffu;
  if (config.current_mode != 0) bits &= ~(1u << 5);
  if (!(config.color_range > 0.0f)) bits &= ~(1u << 13);
  return bits;
}

inline void donorEncodeState(std::uint8_t out[kDonorStateBytes], const K1EmotiscopeConfig& config,
                             const DonorObserved& observed, std::uint32_t request_id,
                             std::uint32_t last_status, std::uint32_t dedup_code,
                             std::uint32_t changed, std::uint32_t mode_transition) noexcept {
  std::memset(out, 0, kDonorStateBytes);
  donorPutU32(out + 0, kDonorStateMagic);
  donorPutU32(out + 4, kDonorStateVersion);
  donorPutU32(out + 8, kDonorEngineId);
  donorPutU32(out + 12, observed.boot_id);
  donorPutU32(out + 16, observed.revision);
  donorPutU32(out + 20, request_id);
  donorPutF32(out + 24, config.brightness);
  donorPutF32(out + 28, config.softness);
  donorPutF32(out + 32, config.color);
  donorPutF32(out + 36, config.warmth);
  donorPutF32(out + 40, config.color_range);
  donorPutF32(out + 44, config.speed);
  donorPutF32(out + 48, config.saturation);
  donorPutF32(out + 52, config.background);
  donorPutF32(out + 56, k1_emotiscope_effective_hue());
  donorPutU32(out + 60, static_cast<std::uint32_t>(config.current_mode));
  std::uint32_t flags = 0;
  if (config.mirror_mode) flags |= 1u;
  if (config.screensaver) flags |= 2u;
  if (config.temporal_dithering) flags |= 4u;
  if (config.auto_color_cycle) flags |= 8u;
  if (config.reverse_color_range) flags |= 16u;
  if (observed.emission) flags |= 32u;
  if (observed.output_precision) flags |= kDonorFlagOutputPrecision;
  donorPutU32(out + 64, flags);
  donorPutU32(out + 68, observed.hardware_brightness);
  donorPutU32(out + 72, donorApplicability(config));
  donorPutU32(out + 76, observed.accepted_request_id);
  donorPutU32(out + 80, observed.rendered_request_id);
  donorPutU32(out + 84, observed.emitted_request_id);
  donorPutU32(out + 88, observed.rendered_frame);
  donorPutU32(out + 92, last_status);
  donorPutU32(out + 96, dedup_code);
  donorPutU32(out + 100, changed);
  donorPutU32(out + 104, mode_transition);
  std::memcpy(out + 108, observed.schema_sha, 32);
}

inline std::uint32_t donorParse(const std::uint8_t* bytes, std::uint32_t size, DonorIntent* out) noexcept {
  if (!bytes || !out) return kDonorStatusInvalid;
  *out = DonorIntent{};
  if (size < 4u) return kDonorStatusInvalid;
  const std::uint32_t sub = donorGetU32(bytes);
  if (!donorProfileAcceptsConfigSub(sub)) return kDonorStatusInvalid;
  if (sub == kDonorCfgGetState) {
    if (size != kDonorGetBytes) return kDonorStatusInvalid;
    const std::uint32_t request = donorGetU32(bytes + 4);
    const std::uint32_t boot = donorGetU32(bytes + 8);
    if (request == 0u || donorGetU32(bytes + 12) != 0u) return kDonorStatusInvalid;
    out->sub = sub;
    out->request_id = request;
    out->boot_id = boot;
    out->payload_crc = donorCrc32(bytes, size);
    return kDonorStatusOk;
  }
  if (size < kDonorPatchHeaderBytes || size > kDonorPatchMaxBytes) return kDonorStatusInvalid;
  const std::uint32_t request = donorGetU32(bytes + 8);
  const std::uint32_t mask = donorGetU32(bytes + 16);
  const std::uint32_t count = donorGetU32(bytes + 20);
  if (request == 0u || (mask & ~kDonorFieldMask) || count > kDonorFieldCount ||
      size != kDonorPatchHeaderBytes + count * kDonorEntryBytes ||
      count != donorPopcount(mask)) return kDonorStatusInvalid;
  std::uint32_t built = 0;
  std::uint16_t previous = 0;
  for (std::uint32_t i = 0; i < count; ++i) {
    const std::uint8_t* entry = bytes + kDonorPatchHeaderBytes + i * kDonorEntryBytes;
    const std::uint16_t field = donorGetU16(entry);
    const std::uint16_t kind = donorGetU16(entry + 2);
    const std::uint32_t raw = donorGetU32(entry + 4);
    if (field < 1u || field > kDonorFieldCount || field <= previous || kind != donorFieldKind(field))
      return kDonorStatusInvalid;
    previous = field;
    built |= 1u << (field - 1u);
    if (kind == kDonorKindF32) {
      const float value = donorGetF32(entry + 4);
      if (!(value >= 0.0f && value <= 1.0f)) return kDonorStatusInvalid;
      float* dest = nullptr;
      switch (field) {
        case 1: dest = &out->brightness; break;
        case 2: dest = &out->softness; break;
        case 3: dest = &out->color; break;
        case 4: dest = &out->warmth; break;
        case 5: dest = &out->color_range; break;
        case 6: dest = &out->speed; break;
        case 7: dest = &out->saturation; break;
        default: dest = &out->background; break;
      }
      *dest = value;
    } else if (kind == kDonorKindBool) {
      if (raw > 1u) return kDonorStatusInvalid;
      std::uint8_t* dest = nullptr;
      switch (field) {
        case 10: dest = &out->mirror; break;
        case 11: dest = &out->screensaver; break;
        case 12: dest = &out->dither; break;
        case 13: dest = &out->auto_color; break;
        case 14: dest = &out->reverse; break;
        case 17: out->output_precision = raw; dest = nullptr; break;
        default: out->emission = raw; dest = nullptr; break;
      }
      if (dest) *dest = static_cast<std::uint8_t>(raw);
    } else if (kind == kDonorKindI32) {
      const std::int32_t mode = static_cast<std::int32_t>(raw);
      if (mode < 0 || mode > 6) return kDonorStatusInvalid;
      out->mode = mode;
    } else {
      if (raw > 255u) return kDonorStatusInvalid;
      out->hardware_brightness = raw;
    }
  }
  if (built != mask) return kDonorStatusInvalid;
  out->sub = sub;
  out->request_id = request;
  out->boot_id = donorGetU32(bytes + 12);
  out->expected_revision = donorGetU32(bytes + 4);
  out->mask = mask;
  out->payload_crc = donorCrc32(bytes, size);
  return kDonorStatusOk;
}

inline void donorRemember(DonorDedup* dedup, std::uint32_t id, std::uint32_t crc,
                          const std::uint8_t response[kDonorStateBytes]) noexcept {
  if (dedup->live_count == kDonorDedupLive) {
    DonorDedupSlot& evicted = dedup->live[dedup->live_next];
    if (evicted.valid) {
      dedup->tomb[dedup->tomb_next] = evicted.id;
      dedup->tomb_valid[dedup->tomb_next] = 1u;
      dedup->tomb_next = static_cast<std::uint8_t>((dedup->tomb_next + 1u) % kDonorDedupTomb);
      if (dedup->tomb_count < kDonorDedupTomb) ++dedup->tomb_count;
    }
  } else {
    ++dedup->live_count;
  }
  DonorDedupSlot& slot = dedup->live[dedup->live_next];
  slot.id = id;
  slot.crc = crc;
  slot.valid = 1u;
  std::memcpy(slot.response, response, kDonorStateBytes);
  dedup->live_next = static_cast<std::uint8_t>((dedup->live_next + 1u) % kDonorDedupLive);
}

inline void donorApply(DonorDedup* dedup, const DonorIntent& intent, DonorObserved observed,
                       DonorApplyResult* result) noexcept {
  std::memset(result, 0, sizeof(*result));
  result->revision = observed.revision;
  result->hardware_brightness = observed.hardware_brightness;
  result->emission = observed.emission;
  result->output_precision = observed.output_precision;
  auto finish = [&](std::uint32_t status, std::uint32_t dedup_code, std::uint32_t changed,
                    std::uint32_t mode_transition) {
    K1EmotiscopeConfig config{};
    k1_emotiscope_get_config(&config);
    DonorObserved encoded = observed;
    encoded.revision = result->revision;
    encoded.hardware_brightness = result->hardware_brightness;
    encoded.emission = result->emission;
    encoded.output_precision = result->output_precision;
    if (result->revision_changed) encoded.accepted_request_id = intent.request_id;
    donorEncodeState(result->response, config, encoded, intent.request_id, status, dedup_code,
                     changed, mode_transition);
    result->status = status;
    result->changed = static_cast<std::uint8_t>(changed);
    result->mode_transition = static_cast<std::uint8_t>(mode_transition);
  };
  if (intent.boot_id != observed.boot_id) { finish(kDonorStatusWrongBoot, 0, 0, 0); return; }
  for (std::uint32_t i = 0; i < kDonorDedupLive; ++i) {
    const DonorDedupSlot& slot = dedup->live[i];
    if (!slot.valid || slot.id != intent.request_id) continue;
    if (slot.crc != intent.payload_crc) { finish(kDonorStatusConflict, 0, 0, 0); return; }
    std::memcpy(result->response, slot.response, kDonorStateBytes);
    donorPutU32(result->response + 96, 1u);
    result->status = donorGetU32(result->response + 92);
    result->revision = donorGetU32(result->response + 16);
    return;
  }
  for (std::uint32_t i = 0; i < kDonorDedupTomb; ++i) {
    if (dedup->tomb_valid[i] && dedup->tomb[i] == intent.request_id) {
      finish(kDonorStatusReconcile, 0, 0, 0);
      return;
    }
  }
  if (intent.sub == kDonorCfgPatch && intent.expected_revision != observed.revision) {
    finish(kDonorStatusStale, 0, 0, 0);
    return;
  }
  std::uint32_t changed = 0, mode_transition = 0;
  if (intent.sub == kDonorCfgPatch) {
    K1EmotiscopePatch patch{};
    patch.mask = intent.mask & 0x3fffu;
    patch.brightness = intent.brightness;
    patch.softness = intent.softness;
    patch.color = intent.color;
    patch.warmth = intent.warmth;
    patch.color_range = intent.color_range;
    patch.speed = intent.speed;
    patch.saturation = intent.saturation;
    patch.background = intent.background;
    patch.current_mode = intent.mode;
    patch.mirror_mode = intent.mirror;
    patch.screensaver = intent.screensaver;
    patch.temporal_dithering = intent.dither;
    patch.auto_color_cycle = intent.auto_color;
    patch.reverse_color_range = intent.reverse;
    if (!k1_emotiscope_patch(&patch, &changed, &mode_transition)) {
      finish(kDonorStatusInvalid, 0, 0, 0);
      return;
    }
    if (intent.mask & (1u << 14)) {
      if (intent.hardware_brightness != observed.hardware_brightness) {
        result->hardware_brightness = intent.hardware_brightness;
        result->brightness_changed = 1u;
        changed = 1u;
      }
    }
    if (intent.mask & (1u << 15)) {
      if (intent.emission != observed.emission) {
        result->emission_fell = observed.emission && !intent.emission ? 1u : 0u;
        result->emission = intent.emission;
        result->emission_changed = 1u;
        changed = 1u;
      }
    }
    if (intent.mask & (1u << 16)) {
      if (intent.output_precision != observed.output_precision) {
        result->output_precision = intent.output_precision;
        result->output_precision_changed = 1u;
        changed = 1u;
      }
    }
    if (mode_transition) {
      result->mode_changed = 1u;
      K1EmotiscopeConfig config{};
      k1_emotiscope_get_config(&config);
      result->mode = config.current_mode;
    }
    if (changed) {
      result->revision = observed.revision + 1u;
      result->revision_changed = 1u;
    }
  }
  finish(kDonorStatusOk, 0, changed, mode_transition);
  donorRemember(dedup, intent.request_id, intent.payload_crc, result->response);
}

inline void donorEncodeGoldenPatch(std::uint8_t out[32]) noexcept {
  std::memset(out, 0, 32);
  donorPutU32(out + 0, kDonorCfgPatch);
  donorPutU32(out + 4, 0u);
  donorPutU32(out + 8, 1u);
  donorPutU32(out + 12, 0x11u);
  donorPutU32(out + 16, 1u);
  donorPutU32(out + 20, 1u);
  donorPutU16(out + 24, 1u);
  donorPutU16(out + 26, static_cast<std::uint16_t>(kDonorKindF32));
  donorPutU32(out + 28, 0x3f000000u);
}

}  // namespace k1::emotiscope
