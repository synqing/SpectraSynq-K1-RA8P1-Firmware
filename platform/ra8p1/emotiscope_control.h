#pragma once
// Donor musical-control codec for opcode 25. Legacy subcommands stay rejected.
// USB may parse this header. Only the realtime owner may call donorApply.
#include "emotiscope_port.h"
#ifdef K1_TITAN_TOP_OUTPUT
#include "emotiscope_secondary.h"
#endif
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace k1::emotiscope {

inline constexpr std::uint32_t kDonorUnsupportedHsvMask=(1u<<2)|(1u<<4)|(1u<<6);
inline constexpr std::uint32_t kDonorCfgGetState = 0x454D0101u;
inline constexpr std::uint32_t kDonorCfgPatch = 0x454D0102u;
inline constexpr std::uint32_t kDonorStateMagic = 0x454D5354u;
#ifdef K1_TITAN_TOP_OUTPUT
inline constexpr std::uint32_t kDonorStateVersion = 4u;
inline constexpr std::uint32_t kDonorStateBytes = 208u;
inline constexpr std::uint32_t kDonorPatchMaxBytes = 240u;
inline constexpr std::uint32_t kDonorFieldCount = 27u;
inline constexpr std::uint32_t kDonorFieldMask = 0x7ffffffu;
#else
inline constexpr std::uint32_t kDonorStateVersion = 2u;
inline constexpr std::uint32_t kDonorStateBytes = 160u;
inline constexpr std::uint32_t kDonorPatchMaxBytes = 184u;
inline constexpr std::uint32_t kDonorFieldCount = 20u;
inline constexpr std::uint32_t kDonorFieldMask = 0xfffffu;
#endif
inline constexpr std::uint32_t kDonorEngineId = 1u;
inline constexpr std::uint32_t kDonorGetBytes = 16u;
inline constexpr std::uint32_t kDonorPatchHeaderBytes = 24u;
inline constexpr std::uint32_t kDonorEntryBytes = 8u;
inline constexpr std::uint32_t kDonorFieldOutputIntensityA = 19u;
inline constexpr std::uint32_t kDonorFieldOutputIntensityB = 20u;
inline constexpr std::uint32_t kDonorStateIntensityAOffset = 144u;
inline constexpr std::uint32_t kDonorStateIntensityBOffset = 148u;
static_assert(kDonorPatchMaxBytes == kDonorPatchHeaderBytes + kDonorFieldCount * kDonorEntryBytes);
static_assert(kDonorStateIntensityBOffset + sizeof(float) <= kDonorStateBytes);
inline constexpr std::uint32_t kDonorFieldPalette = 18u;
inline constexpr std::uint32_t kDonorPaletteMax = 44u;  // 1..44 K1 palettes only
inline constexpr std::uint32_t kDonorStatePaletteOffset = 140u;
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
inline bool donorValidUnitFloat(float value) noexcept {
  std::uint32_t bits = 0u;
  std::memcpy(&bits, &value, sizeof(bits));
  // IEEE-754 F32 ordering: positive 0..1, or negative zero. Integer validation
  // also rejects negative subnormals when the processor flushes them to zero.
  return bits <= 0x3f800000u || bits == 0x80000000u;
}
inline bool donorValidResponseGain(float value) noexcept {
  std::uint32_t bits=0u;std::memcpy(&bits,&value,sizeof(bits));
  return bits<=0x40000000u || bits==0x80000000u;
}
inline void donorSceneGetConfig(std::uint32_t board,K1EmotiscopeConfig* config) noexcept {
#ifdef K1_TITAN_TOP_OUTPUT
  if(board==1u){k1_secondary_emotiscope_get_config(config);return;}
#else
  (void)board;
#endif
  k1_emotiscope_get_config(config);
}
inline std::uint32_t donorPopcount(std::uint32_t mask) noexcept {
  std::uint32_t count = 0;
  while (mask) { count += mask & 1u; mask >>= 1; }
  return count;
}
inline std::uint32_t donorFieldKind(std::uint32_t field) noexcept {
#ifdef K1_TITAN_TOP_OUTPUT
  if (field == 21u || field == 22u || field == 25u || field == 27u) return kDonorKindF32;
  if (field == 24u || field == 26u) return kDonorKindU32;
  if (field == 23u) return kDonorKindBool;
#endif
  if ((field >= 1u && field <= 8u) || field == kDonorFieldOutputIntensityA ||
      field == kDonorFieldOutputIntensityB) return kDonorKindF32;
  if (field == 9u) return kDonorKindI32;
  if ((field >= 10u && field <= 14u) || field == 16u || field == 17u) return kDonorKindBool;
  if (field == 15u || field == 18u) return kDonorKindU32;
  return 0u;
}

struct DonorIntent {
#ifdef K1_TITAN_TOP_OUTPUT
  std::uint32_t board = 0, edge_response = 0;
  float response_gain = 1.0f, edge_depth = 0.0f;
  float primary_intensity = 1.0f, secondary_intensity = 1.0f;
  std::uint32_t secondary_output_enabled = 0;
#endif
  std::uint32_t sub = 0, request_id = 0, boot_id = 0, expected_revision = 0;
  std::uint32_t mask = 0, payload_crc = 0;
  float brightness = 0, softness = 0, color = 0, warmth = 0;
  float color_range = 0, speed = 0, saturation = 0, background = 0;
  float output_intensity_a = 1.0f, output_intensity_b = 1.0f;
  std::int32_t mode = 0;
  std::uint8_t mirror = 0, screensaver = 0, dither = 0, auto_color = 0, reverse = 0;
  std::uint32_t hardware_brightness = 0, emission = 0, output_precision = 0, palette = 0;
};

struct DonorDedupSlot {
  std::uint32_t id = 0, crc = 0;
#ifdef K1_TITAN_TOP_OUTPUT
  std::uint32_t board=0;
#endif
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
#ifdef K1_TITAN_TOP_OUTPUT
  std::uint32_t board_capabilities = 7u;
  std::uint32_t board = 0, edge_response = 0;
  float response_gain = 1.0f, edge_depth = 0.0f;
  float primary_intensity = 1.0f, secondary_intensity = 1.0f;
  std::uint32_t secondary_output_enabled = 0;
#endif
  std::uint32_t boot_id = 0, revision = 0, hardware_brightness = 0, emission = 0;
  std::uint32_t output_precision = 0, palette = K1_EMOTISCOPE_DEFAULT_PALETTE;
  float output_intensity_a = 1.0f, output_intensity_b = 1.0f;
  std::uint32_t accepted_request_id = 0, rendered_request_id = 0;
  std::uint32_t emitted_request_id = 0, rendered_frame = 0;
  std::uint8_t schema_sha[32]{};
};
struct DonorApplyResult {
#ifdef K1_TITAN_TOP_OUTPUT
  std::uint32_t board = 0, edge_response = 0;
  float response_gain = 1.0f, edge_depth = 0.0f;
  float primary_intensity = 1.0f, secondary_intensity = 1.0f;
  std::uint32_t secondary_output_enabled = 0;
  // Actual consumers of this accepted request; globals can affect both outputs
  // irrespective of the packet-local scene selector.
  std::uint8_t affected_boards=0;
  std::uint8_t response_gain_changed=0, edge_response_changed=0, edge_depth_changed=0;
  std::uint8_t primary_intensity_changed = 0, secondary_intensity_changed = 0;
  std::uint8_t secondary_output_enabled_changed = 0;
#endif
  std::uint32_t status = 0, revision = 0, hardware_brightness = 0, emission = 0;
  std::uint32_t output_precision = 0, palette = K1_EMOTISCOPE_DEFAULT_PALETTE;
  float output_intensity_a = 1.0f, output_intensity_b = 1.0f;
  std::uint8_t intensity_a_changed = 0, intensity_b_changed = 0;
  std::uint8_t changed = 0, mode_transition = 0, revision_changed = 0;
  std::uint8_t output_precision_changed = 0, palette_changed = 0;
  std::uint8_t brightness_changed = 0, emission_changed = 0, emission_fell = 0;
  std::uint8_t mode_changed = 0;
  std::int32_t mode = 0;
  std::uint8_t response[kDonorStateBytes]{};
};

inline std::uint32_t donorApplicability(const K1EmotiscopeConfig& config) noexcept {
  // Preserve legacy bits 0..15; precision, palette and DIN intensities are
  // always applicable in state version 2, with no new applicability bits.
  std::uint32_t bits = 0xffffu & ~kDonorUnsupportedHsvMask;
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
  donorPutF32(out + 56,
#ifdef K1_TITAN_TOP_OUTPUT
      observed.board==1u?k1_secondary_emotiscope_effective_hue():
#endif
      k1_emotiscope_effective_hue());
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
  donorPutU32(out + kDonorStatePaletteOffset, observed.palette);
  donorPutF32(out + kDonorStateIntensityAOffset, observed.output_intensity_a);
  donorPutF32(out + kDonorStateIntensityBOffset, observed.output_intensity_b);
  // Bytes 152..159 remain reserved and zero in version 2.
#ifdef K1_TITAN_TOP_OUTPUT
  donorPutF32(out + 152, observed.primary_intensity);
  donorPutF32(out + 156, observed.secondary_intensity);
  donorPutU32(out + 160, observed.secondary_output_enabled);
  donorPutU32(out + 164, observed.board_capabilities);
  // Legacy reserved bytes168..175 remain zero. New v4 scene identity/response.
  donorPutU32(out+176,observed.board);donorPutF32(out+180,observed.response_gain);
  donorPutU32(out+184,observed.edge_response);donorPutF32(out+188,observed.edge_depth);
  // Bytes192..207 remain reserved and zero.
#endif
}

inline std::uint32_t donorParse(const std::uint8_t* bytes, std::uint32_t size, DonorIntent* out) noexcept {
  if (!bytes || !out) return kDonorStatusInvalid;
  *out = DonorIntent{};
  if (size < 4u) return kDonorStatusInvalid;
  const std::uint32_t sub = donorGetU32(bytes);
  if (!donorProfileAcceptsConfigSub(sub)) return kDonorStatusInvalid;
  if (sub == kDonorCfgGetState) {
    if (size != kDonorGetBytes
#ifdef K1_TITAN_TOP_OUTPUT
        && size != 20u
#endif
        ) return kDonorStatusInvalid;
#ifdef K1_TITAN_TOP_OUTPUT
    if(size==20u){out->board=donorGetU32(bytes+16);if(out->board>1u)return kDonorStatusInvalid;}
#endif
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
  if (request == 0u || (mask & ~kDonorFieldMask) || (mask&kDonorUnsupportedHsvMask) || count > kDonorFieldCount ||
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
      if (
#ifdef K1_TITAN_TOP_OUTPUT
          field==25u?!donorValidResponseGain(value):
#endif
          !donorValidUnitFloat(value)) return kDonorStatusInvalid;
      float* dest = nullptr;
      switch (field) {
        case 1: dest = &out->brightness; break;
        case 2: dest = &out->softness; break;
        case 3: dest = &out->color; break;
        case 4: dest = &out->warmth; break;
        case 5: dest = &out->color_range; break;
        case 6: dest = &out->speed; break;
        case 7: dest = &out->saturation; break;
        case 19: dest = &out->output_intensity_a; break;
        case 20: dest = &out->output_intensity_b; break;
#ifdef K1_TITAN_TOP_OUTPUT
        case 21: dest = &out->primary_intensity; break;
        case 22: dest = &out->secondary_intensity; break;
        case 25: dest = &out->response_gain; break;
        case 27: dest = &out->edge_depth; break;
#endif
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
#ifdef K1_TITAN_TOP_OUTPUT
        case 23: out->secondary_output_enabled = raw; dest = nullptr; break;
#endif
        default: out->emission = raw; dest = nullptr; break;
      }
      if (dest) *dest = static_cast<std::uint8_t>(raw);
    } else if (kind == kDonorKindI32) {
      const std::int32_t mode = static_cast<std::int32_t>(raw);
      if (mode < 0 || mode > 6) return kDonorStatusInvalid;
      out->mode = mode;
#ifdef K1_TITAN_TOP_OUTPUT
    } else if(field==24u||field==26u){
      if(raw>(field==24u?1u:2u))return kDonorStatusInvalid;
      if(field==24u)out->board=raw;else out->edge_response=raw;
#endif
    } else if (field == kDonorFieldPalette) {
      if (raw == 0u || raw > kDonorPaletteMax) return kDonorStatusInvalid;
      out->palette = raw;
    } else {
      if (raw > 255u) return kDonorStatusInvalid;
      out->hardware_brightness = raw;
    }
  }
  if (built != mask) return kDonorStatusInvalid;
#ifdef K1_TITAN_TOP_OUTPUT
  if(out->board==1u && (mask&((1u<<18)|(1u<<19))))return kDonorStatusInvalid;
#endif
  out->sub = sub;
  out->request_id = request;
  out->boot_id = donorGetU32(bytes + 12);
  out->expected_revision = donorGetU32(bytes + 4);
  out->mask = mask;
  out->payload_crc = donorCrc32(bytes, size);
  return kDonorStatusOk;
}

inline void donorRemember(DonorDedup* dedup, std::uint32_t id, std::uint32_t crc,
                          const std::uint8_t response[kDonorStateBytes], std::uint32_t board=0u) noexcept {
#ifndef K1_TITAN_TOP_OUTPUT
  (void)board;
#endif
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
#ifdef K1_TITAN_TOP_OUTPUT
  slot.board=board;
#endif
  slot.valid = 1u;
  std::memcpy(slot.response, response, kDonorStateBytes);
  dedup->live_next = static_cast<std::uint8_t>((dedup->live_next + 1u) % kDonorDedupLive);
}

inline void donorApply(DonorDedup* dedup, const DonorIntent& intent, DonorObserved observed,
                       DonorApplyResult* result
#ifdef K1_TITAN_TOP_OUTPUT
                       , bool (*secondary_admit)() = nullptr
#endif
                       ) noexcept {
  std::memset(result, 0, sizeof(*result));
  result->revision = observed.revision;
  result->hardware_brightness = observed.hardware_brightness;
  result->emission = observed.emission;
  result->output_precision = observed.output_precision;
  result->palette = observed.palette;
  result->output_intensity_a = observed.output_intensity_a;
  result->output_intensity_b = observed.output_intensity_b;
#ifdef K1_TITAN_TOP_OUTPUT
  result->board=intent.board<=1u?intent.board:0u;
  result->response_gain=observed.response_gain;result->edge_response=observed.edge_response;
  result->edge_depth=observed.edge_depth;
  result->primary_intensity = observed.primary_intensity;
  result->secondary_intensity = observed.secondary_intensity;
  result->secondary_output_enabled = observed.secondary_output_enabled;
#endif
  auto finish = [&](std::uint32_t status, std::uint32_t dedup_code, std::uint32_t changed,
                    std::uint32_t mode_transition) {
    K1EmotiscopeConfig config{};
    donorSceneGetConfig(
#ifdef K1_TITAN_TOP_OUTPUT
        result->board,
#else
        0u,
#endif
        &config);
    DonorObserved encoded = observed;
    encoded.revision = result->revision;
    encoded.hardware_brightness = result->hardware_brightness;
    encoded.emission = result->emission;
    encoded.output_precision = result->output_precision;
    encoded.palette = result->palette;
    encoded.output_intensity_a = result->output_intensity_a;
    encoded.output_intensity_b = result->output_intensity_b;
#ifdef K1_TITAN_TOP_OUTPUT
    encoded.board=result->board;encoded.response_gain=result->response_gain;
    encoded.edge_response=result->edge_response;encoded.edge_depth=result->edge_depth;
    encoded.primary_intensity = result->primary_intensity;
    encoded.secondary_intensity = result->secondary_intensity;
    encoded.secondary_output_enabled = result->secondary_output_enabled;
#endif
    if (result->revision_changed
#ifdef K1_TITAN_TOP_OUTPUT
        && (result->affected_boards&(1u<<result->board))
#endif
        ) encoded.accepted_request_id = intent.request_id;
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
    if (slot.crc != intent.payload_crc
#ifdef K1_TITAN_TOP_OUTPUT
        || slot.board != intent.board
#endif
        ) { finish(kDonorStatusConflict, 0, 0, 0); return; }
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
#ifdef K1_TITAN_TOP_OUTPUT
  if(intent.board>1u || (intent.board==1u && !(observed.board_capabilities&4u)) ||
      (intent.board==1u && (intent.mask&((1u<<18)|(1u<<19)))) ||
      ((intent.mask&(1u<<24))&&!donorValidResponseGain(intent.response_gain)) ||
      ((intent.mask&(1u<<25))&&intent.edge_response>2u) ||
      ((intent.mask&(1u<<26))&&!donorValidUnitFloat(intent.edge_depth))){
    finish(kDonorStatusInvalid,0,0,0);return;
  }
#endif
  if(intent.sub==kDonorCfgPatch && (intent.mask&kDonorUnsupportedHsvMask)){
    finish(kDonorStatusInvalid,0,0,0);return;
  }
  // The owner application boundary also validates direct intents atomically,
  // before any donor scene patch can mutate state.
  if (intent.sub == kDonorCfgPatch &&
      (((intent.mask & (1u << 17)) &&
        (intent.palette == 0u || intent.palette > kDonorPaletteMax)) ||
       ((intent.mask & (1u << 18)) && !donorValidUnitFloat(intent.output_intensity_a)) ||
       ((intent.mask & (1u << 19)) && !donorValidUnitFloat(intent.output_intensity_b)))) {
    finish(kDonorStatusInvalid, 0, 0, 0);
    return;
  }
  std::uint32_t changed = 0, mode_transition = 0;
#ifdef K1_TITAN_TOP_OUTPUT
  // Validate every masked legacy field before the admission callback can claim
  // top resources. The donor patch below performs its own validation again.
  if (intent.sub == kDonorCfgPatch) {
    const float units[] = {intent.brightness, intent.softness, intent.color,
        intent.warmth, intent.color_range, intent.speed, intent.saturation,
        intent.background};
    const std::uint8_t flags[] = {intent.mirror, intent.screensaver, intent.dither,
        intent.auto_color, intent.reverse};
    bool valid = !(intent.mask & ~kDonorFieldMask);
    for (unsigned i = 0; i < 8u; ++i)
      if ((intent.mask & (1u << i)) && !donorValidUnitFloat(units[i])) valid = false;
    for (unsigned i = 0; i < 5u; ++i)
      if ((intent.mask & (1u << (9u + i))) && flags[i] > 1u) valid = false;
    if (((intent.mask & (1u << 8)) && (intent.mode < 0 || intent.mode > 6)) ||
        ((intent.mask & (1u << 14)) && intent.hardware_brightness > 255u) ||
        ((intent.mask & (1u << 15)) && intent.emission > 1u) ||
        ((intent.mask & (1u << 16)) && intent.output_precision > 1u) ||
        ((intent.mask & (1u << 17)) && intent.palette > kDonorPaletteMax)) valid = false;
    if (!valid) { finish(kDonorStatusInvalid, 0, 0, 0); return; }
  }
  if (intent.sub == kDonorCfgPatch &&
      (((intent.mask & (1u << 20)) && !donorValidUnitFloat(intent.primary_intensity)) ||
       ((intent.mask & (1u << 21)) && !donorValidUnitFloat(intent.secondary_intensity)) ||
       ((intent.mask & (1u << 22)) && intent.secondary_output_enabled > 1u) ||
       ((intent.mask & (1u << 20)) && !(observed.board_capabilities & 1u)) ||
       ((intent.mask & (1u << 21)) && !(observed.board_capabilities & 2u)) ||
       ((intent.mask & (1u << 22)) && intent.secondary_output_enabled &&
        !(observed.board_capabilities & 2u)))) {
    finish(kDonorStatusInvalid, 0, 0, 0);
    return;
  }
  if (intent.sub == kDonorCfgPatch && (intent.mask & (1u << 22)) &&
      intent.secondary_output_enabled && !observed.secondary_output_enabled &&
      secondary_admit && !secondary_admit()) {
    observed.board_capabilities &= ~2u;
    finish(kDonorStatusInvalid, 0, 0, 0);
    return;
  }
#endif
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
    const bool scene_applied=
#ifdef K1_TITAN_TOP_OUTPUT
        intent.board==1u?k1_secondary_emotiscope_patch(&patch,&changed,&mode_transition):
#endif
        k1_emotiscope_patch(&patch,&changed,&mode_transition);
    if (!scene_applied) {
      finish(kDonorStatusInvalid, 0, 0, 0);
      return;
    }
#ifdef K1_TITAN_TOP_OUTPUT
    const bool scene_changed = changed != 0u;
#endif
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
    if (intent.mask & (1u << 17)) {
      if (intent.palette != observed.palette) {
        result->palette = intent.palette;
        result->palette_changed = 1u;
        changed = 1u;
      }
    }
    if ((intent.mask & (1u << 18)) && intent.output_intensity_a != observed.output_intensity_a) {
      result->output_intensity_a = intent.output_intensity_a;
      result->intensity_a_changed = 1u;
      changed = 1u;
    }
    if ((intent.mask & (1u << 19)) && intent.output_intensity_b != observed.output_intensity_b) {
      result->output_intensity_b = intent.output_intensity_b;
      result->intensity_b_changed = 1u;
      changed = 1u;
    }
#ifdef K1_TITAN_TOP_OUTPUT
    if((intent.mask&(1u<<24))&&intent.response_gain!=observed.response_gain){result->response_gain=intent.response_gain;result->response_gain_changed=1u;changed=1u;}
    if((intent.mask&(1u<<25))&&intent.edge_response!=observed.edge_response){result->edge_response=intent.edge_response;result->edge_response_changed=1u;changed=1u;}
    if((intent.mask&(1u<<26))&&intent.edge_depth!=observed.edge_depth){result->edge_depth=intent.edge_depth;result->edge_depth_changed=1u;changed=1u;}
    if ((intent.mask & (1u << 20)) && intent.primary_intensity != observed.primary_intensity) {
      result->primary_intensity = intent.primary_intensity;
      result->primary_intensity_changed = 1u;
      changed = 1u;
    }
    if ((intent.mask & (1u << 21)) && intent.secondary_intensity != observed.secondary_intensity) {
      result->secondary_intensity = intent.secondary_intensity;
      result->secondary_intensity_changed = 1u;
      changed = 1u;
    }
    if ((intent.mask & (1u << 22)) && intent.secondary_output_enabled != observed.secondary_output_enabled) {
      result->secondary_output_enabled = intent.secondary_output_enabled;
      result->secondary_output_enabled_changed = 1u;
      changed = 1u;
    }
#endif
    if (mode_transition) {
      result->mode_changed = 1u;
      K1EmotiscopeConfig config{};
      donorSceneGetConfig(
#ifdef K1_TITAN_TOP_OUTPUT
          intent.board,
#else
          0u,
#endif
          &config);
      result->mode = config.current_mode;
    }
#ifdef K1_TITAN_TOP_OUTPUT
    if(scene_changed || result->palette_changed || result->response_gain_changed ||
        result->edge_response_changed || result->edge_depth_changed)
      result->affected_boards|=static_cast<std::uint8_t>(1u<<intent.board);
    if(result->brightness_changed||result->emission_changed||result->output_precision_changed)
      result->affected_boards|=3u;
    if(result->intensity_a_changed||result->intensity_b_changed||result->primary_intensity_changed)
      result->affected_boards|=1u;
    if(result->secondary_intensity_changed||result->secondary_output_enabled_changed)
      result->affected_boards|=2u;
#endif
    if (changed) {
      result->revision = observed.revision + 1u;
      result->revision_changed = 1u;
    }
  }
  finish(kDonorStatusOk, 0, changed, mode_transition);
  donorRemember(dedup, intent.request_id, intent.payload_crc, result->response
#ifdef K1_TITAN_TOP_OUTPUT
      , intent.board
#endif
      );
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
