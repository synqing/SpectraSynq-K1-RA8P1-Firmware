// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
//
// Canonical little-endian codecs for the K1 control contract v2 objects.
// Layouts and validation rules are documented in control_v2_objects.h.
#include "contract/control_v2/control_v2_objects.h"

#include <array>
#include <cstring>

namespace k1::control_v2 {
namespace {

// ---------------------------------------------------------------------------
// Primitive helpers
// ---------------------------------------------------------------------------

std::uint32_t floatBits(const float value) noexcept {
  std::uint32_t bits = 0U;
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}

float bitsFloat(const std::uint32_t bits) noexcept {
  float value = 0.0F;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

// Bit-level finiteness: independent of any fast-math optimisation.
bool finiteBits(const std::uint32_t bits) noexcept {
  return (bits & 0x7F800000U) != 0x7F800000U;
}

bool finite(const float value) noexcept { return finiteBits(floatBits(value)); }

// Canonical-float check on bits: finite and not negative zero.
CodecStatus validFloat(const float value) noexcept {
  const std::uint32_t bits = floatBits(value);
  if (!finiteBits(bits)) {
    return CodecStatus::kNonFinite;
  }
  return bits == 0x80000000U ? CodecStatus::kNonCanonical : CodecStatus::kOk;
}

// Ordered comparison of finite floats (callers check finiteness first).
bool floatLess(const float a, const float b) noexcept { return a < b; }

class Writer final {
 public:
  explicit Writer(std::uint8_t* out) noexcept : out_(out) {}
  void u8(const std::uint8_t v) noexcept { out_[pos_++] = v; }
  void u16(const std::uint16_t v) noexcept {
    u8(static_cast<std::uint8_t>(v & 0xFFU));
    u8(static_cast<std::uint8_t>((v >> 8U) & 0xFFU));
  }
  void u32(const std::uint32_t v) noexcept {
    for (unsigned shift = 0U; shift < 32U; shift += 8U) {
      u8(static_cast<std::uint8_t>((v >> shift) & 0xFFU));
    }
  }
  void u64(const std::uint64_t v) noexcept {
    for (unsigned shift = 0U; shift < 64U; shift += 8U) {
      u8(static_cast<std::uint8_t>((v >> shift) & 0xFFU));
    }
  }
  void f32(const float v) noexcept { u32(floatBits(v)); }
  void zeros(const std::size_t count) noexcept {
    for (std::size_t i = 0U; i < count; ++i) {
      u8(0U);
    }
  }
  void bytes(const void* data, const std::size_t count) noexcept {
    std::memcpy(out_ + pos_, data, count);
    pos_ += count;
  }
  void typed(const TypedValue& v) noexcept {
    u8(static_cast<std::uint8_t>(v.kind));
    zeros(3U);
    u32(v.bits);
  }
  void target(const TargetRef& t) noexcept {
    u16(t.semantic_id);
    u8(static_cast<std::uint8_t>(t.channel));
    u8(0U);
  }
  void header(const ObjectKind kind, const std::size_t length) noexcept {
    u8(static_cast<std::uint8_t>(kind));
    u8(kObjectLayoutVersion);
    u16(static_cast<std::uint16_t>(length));
  }
  [[nodiscard]] std::size_t position() const noexcept { return pos_; }
  [[nodiscard]] std::uint8_t* data() const noexcept { return out_; }

 private:
  std::uint8_t* out_;
  std::size_t pos_{0U};
};

class Reader final {
 public:
  Reader(const std::uint8_t* in, const std::size_t size) noexcept
      : in_(in), size_(size) {}
  std::uint8_t u8() noexcept { return in_[pos_++]; }
  std::uint16_t u16() noexcept {
    const std::uint16_t lo = u8();
    const std::uint16_t hi = u8();
    return static_cast<std::uint16_t>(lo | static_cast<std::uint16_t>(hi << 8U));
  }
  std::uint32_t u32() noexcept {
    std::uint32_t v = 0U;
    for (unsigned shift = 0U; shift < 32U; shift += 8U) {
      v |= static_cast<std::uint32_t>(u8()) << shift;
    }
    return v;
  }
  std::uint64_t u64() noexcept {
    std::uint64_t v = 0U;
    for (unsigned shift = 0U; shift < 64U; shift += 8U) {
      v |= static_cast<std::uint64_t>(u8()) << shift;
    }
    return v;
  }
  // Returns false (and leaves the float unchanged) if the bits are non-finite.
  bool f32(float& out) noexcept {
    const std::uint32_t bits = u32();
    if (!finiteBits(bits)) {
      return false;
    }
    out = bitsFloat(bits);
    return true;
  }
  bool zeros(const std::size_t count) noexcept {
    bool ok = true;
    for (std::size_t i = 0U; i < count; ++i) {
      ok = (u8() == 0U) && ok;
    }
    return ok;
  }
  void bytes(void* dest, const std::size_t count) noexcept {
    std::memcpy(dest, in_ + pos_, count);
    pos_ += count;
  }
  [[nodiscard]] std::size_t position() const noexcept { return pos_; }
  [[nodiscard]] std::size_t size() const noexcept { return size_; }

 private:
  const std::uint8_t* in_;
  std::size_t size_;
  std::size_t pos_{0U};
};

// Reads a TypedValue: returns kOk or the structural failure.
CodecStatus readTyped(Reader& r, TypedValue& out) noexcept {
  const std::uint8_t kind = r.u8();
  if (!r.zeros(3U)) {
    (void)r.u32();
    return CodecStatus::kReservedNonZero;
  }
  const std::uint32_t bits = r.u32();
  if (kind >= kValueKindCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  out.kind = static_cast<ValueKind>(kind);
  out.bits = bits;
  return CodecStatus::kOk;
}

CodecStatus readTarget(Reader& r, TargetRef& out) noexcept {
  out.semantic_id = r.u16();
  const std::uint8_t channel = r.u8();
  const std::uint8_t reserved = r.u8();
  if (reserved != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  if (channel >= kChannelEnumCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  out.channel = static_cast<Channel>(channel);
  return CodecStatus::kOk;
}

// Validates the common header against the expected kind and actual size.
CodecStatus readHeader(Reader& r, const ObjectKind kind,
                       const std::size_t actual_size) noexcept {
  if (actual_size < 4U) {
    return CodecStatus::kTruncated;
  }
  const std::uint8_t k = r.u8();
  const std::uint8_t version = r.u8();
  const std::uint16_t length = r.u16();
  if (k != static_cast<std::uint8_t>(kind)) {
    return CodecStatus::kWrongKind;
  }
  if (version != kObjectLayoutVersion) {
    return CodecStatus::kWrongVersion;
  }
  if (length > actual_size) {
    return CodecStatus::kTruncated;
  }
  if (length != actual_size) {
    return CodecStatus::kLengthMismatch;
  }
  return CodecStatus::kOk;
}

CodecStatus validTypedValue(const TypedValue& v) noexcept {
  if (static_cast<std::uint8_t>(v.kind) >= kValueKindCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  switch (v.kind) {
    case ValueKind::kNone:
      return v.bits == 0U ? CodecStatus::kOk : CodecStatus::kInvalidValue;
    case ValueKind::kReal:
      if (!finiteBits(v.bits)) {
        return CodecStatus::kNonFinite;
      }
      return v.bits == 0x80000000U ? CodecStatus::kNonCanonical : CodecStatus::kOk;
    case ValueKind::kInteger:
      return CodecStatus::kOk;
    case ValueKind::kBoolean:
      return v.bits <= 1U ? CodecStatus::kOk : CodecStatus::kInvalidValue;
    case ValueKind::kEnum:
      return v.bits <= 0xFFFFU ? CodecStatus::kOk
                               : CodecStatus::kIntegerOutOfRange;
  }
  return CodecStatus::kEnumOutOfRange;
}

CodecStatus validTarget(const TargetRef& t) noexcept {
  if (!isParameterId(t.semantic_id)) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if (static_cast<std::uint8_t>(t.channel) >= kChannelEnumCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  return CodecStatus::kOk;
}

bool allZero(const std::uint8_t* data, const std::size_t size) noexcept {
  for (std::size_t i = 0U; i < size; ++i) {
    if (data[i] != 0U) {
      return false;
    }
  }
  return true;
}

#define K1_CV2_TRY(expr)                   \
  do {                                     \
    const CodecStatus status_ = (expr);    \
    if (status_ != CodecStatus::kOk) {     \
      return status_;                      \
    }                                      \
  } while (false)

// ---------------------------------------------------------------------------
// CRC-32 (IEEE 802.3, reflected, init/final 0xFFFFFFFF)
// ---------------------------------------------------------------------------

constexpr std::array<std::uint32_t, 256> makeCrcTable() noexcept {
  std::array<std::uint32_t, 256> table{};
  for (std::uint32_t n = 0U; n < 256U; ++n) {
    std::uint32_t c = n;
    for (int k = 0; k < 8; ++k) {
      c = (c & 1U) != 0U ? (0xEDB88320U ^ (c >> 1U)) : (c >> 1U);
    }
    table[n] = c;
  }
  return table;
}
constexpr std::array<std::uint32_t, 256> kCrcTable = makeCrcTable();

}  // namespace

std::uint32_t crc32IeeeUpdate(std::uint32_t crc, const std::uint8_t* data,
                              const std::size_t size) noexcept {
  crc = ~crc;
  for (std::size_t i = 0U; i < size; ++i) {
    crc = kCrcTable[(crc ^ data[i]) & 0xFFU] ^ (crc >> 8U);
  }
  return ~crc;
}

std::uint32_t crc32Ieee(const std::uint8_t* data, const std::size_t size) noexcept {
  return crc32IeeeUpdate(0U, data, size);
}

// ---------------------------------------------------------------------------
// SHA-256 (FIPS 180-4)
// ---------------------------------------------------------------------------

namespace {

constexpr std::uint32_t kSha256K[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU,
    0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U,
    0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U,
    0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U,
    0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
    0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
    0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U,
    0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U, 0x1e376c08U,
    0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU,
    0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

std::uint32_t rotr(const std::uint32_t v, const unsigned n) noexcept {
  return (v >> n) | (v << (32U - n));
}

void sha256Block(std::uint32_t h[8], const std::uint8_t block[64]) noexcept {
  std::uint32_t w[64];
  for (unsigned i = 0U; i < 16U; ++i) {
    w[i] = (static_cast<std::uint32_t>(block[i * 4U]) << 24U) |
           (static_cast<std::uint32_t>(block[i * 4U + 1U]) << 16U) |
           (static_cast<std::uint32_t>(block[i * 4U + 2U]) << 8U) |
           static_cast<std::uint32_t>(block[i * 4U + 3U]);
  }
  for (unsigned i = 16U; i < 64U; ++i) {
    const std::uint32_t s0 = rotr(w[i - 15U], 7U) ^ rotr(w[i - 15U], 18U) ^ (w[i - 15U] >> 3U);
    const std::uint32_t s1 = rotr(w[i - 2U], 17U) ^ rotr(w[i - 2U], 19U) ^ (w[i - 2U] >> 10U);
    w[i] = w[i - 16U] + s0 + w[i - 7U] + s1;
  }
  std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5],
                g = h[6], hh = h[7];
  for (unsigned i = 0U; i < 64U; ++i) {
    const std::uint32_t s1 = rotr(e, 6U) ^ rotr(e, 11U) ^ rotr(e, 25U);
    const std::uint32_t ch = (e & f) ^ ((~e) & g);
    const std::uint32_t t1 = hh + s1 + ch + kSha256K[i] + w[i];
    const std::uint32_t s0 = rotr(a, 2U) ^ rotr(a, 13U) ^ rotr(a, 22U);
    const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
    const std::uint32_t t2 = s0 + maj;
    hh = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  h[0] += a;
  h[1] += b;
  h[2] += c;
  h[3] += d;
  h[4] += e;
  h[5] += f;
  h[6] += g;
  h[7] += hh;
}

}  // namespace

void sha256(const std::uint8_t* data, const std::size_t size,
            std::uint8_t out[kDigestBytes]) noexcept {
  std::uint32_t h[8] = {0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
                        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
  std::size_t offset = 0U;
  while (size - offset >= 64U) {
    sha256Block(h, data + offset);
    offset += 64U;
  }
  std::uint8_t tail[128] = {};
  const std::size_t remaining = size - offset;
  if (remaining > 0U) {
    std::memcpy(tail, data + offset, remaining);
  }
  tail[remaining] = 0x80U;
  const std::size_t tail_blocks = remaining + 1U + 8U > 64U ? 2U : 1U;
  const std::uint64_t bit_length = static_cast<std::uint64_t>(size) * 8U;
  for (unsigned i = 0U; i < 8U; ++i) {
    tail[tail_blocks * 64U - 1U - i] =
        static_cast<std::uint8_t>((bit_length >> (8U * i)) & 0xFFU);
  }
  for (std::size_t block = 0U; block < tail_blocks; ++block) {
    sha256Block(h, tail + block * 64U);
  }
  for (unsigned i = 0U; i < 8U; ++i) {
    out[i * 4U] = static_cast<std::uint8_t>(h[i] >> 24U);
    out[i * 4U + 1U] = static_cast<std::uint8_t>(h[i] >> 16U);
    out[i * 4U + 2U] = static_cast<std::uint8_t>(h[i] >> 8U);
    out[i * 4U + 3U] = static_cast<std::uint8_t>(h[i]);
  }
}

// ---------------------------------------------------------------------------
// Strings
// ---------------------------------------------------------------------------

bool validKeyField(const char* field, const std::size_t width) noexcept {
  std::size_t length = 0U;
  while (length < width && field[length] != '\0') {
    const char c = field[length];
    const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
    if (!ok) {
      return false;
    }
    ++length;
  }
  if (length == 0U || length >= width) {
    return false;  // empty, or no terminating NUL inside the field
  }
  for (std::size_t i = length; i < width; ++i) {
    if (field[i] != '\0') {
      return false;
    }
  }
  return true;
}

// Labels are bounded UTF-8: well formed (no overlong forms, no surrogates,
// nothing above U+10FFFF), no C0/C1 control characters or DEL, NUL padded
// with at least one terminating NUL, so the byte length is <= width - 1.
bool validLabelField(const char* field, const std::size_t width,
                     const bool allow_empty) noexcept {
  const auto* bytes = reinterpret_cast<const unsigned char*>(field);
  std::size_t i = 0U;
  while (i < width && bytes[i] != 0U) {
    const unsigned lead = bytes[i];
    std::size_t extra = 0U;
    unsigned lo = 0x80U;
    unsigned hi = 0xBFU;
    if (lead >= 0x20U && lead <= 0x7EU) {
      ++i;
      continue;
    }
    if (lead >= 0xC2U && lead <= 0xDFU) {
      extra = 1U;
      if (lead == 0xC2U) {
        lo = 0xA0U;  // U+0080..U+009F are C1 control characters
      }
    } else if (lead >= 0xE0U && lead <= 0xEFU) {
      extra = 2U;
      if (lead == 0xE0U) {
        lo = 0xA0U;  // overlong
      } else if (lead == 0xEDU) {
        hi = 0x9FU;  // UTF-16 surrogates
      }
    } else if (lead >= 0xF0U && lead <= 0xF4U) {
      extra = 3U;
      if (lead == 0xF0U) {
        lo = 0x90U;  // overlong
      } else if (lead == 0xF4U) {
        hi = 0x8FU;  // above U+10FFFF
      }
    } else {
      return false;  // C0 control, DEL, stray continuation or invalid lead
    }
    if (i + extra >= width) {
      return false;  // sequence would consume the terminating NUL
    }
    for (std::size_t k = 1U; k <= extra; ++k) {
      const unsigned c = bytes[i + k];
      const unsigned min = k == 1U ? lo : 0x80U;
      const unsigned max = k == 1U ? hi : 0xBFU;
      if (c < min || c > max) {
        return false;
      }
    }
    i += extra + 1U;
  }
  if (i >= width || (i == 0U && !allow_empty)) {
    return false;
  }
  for (std::size_t k = i; k < width; ++k) {
    if (bytes[k] != 0U) {
      return false;
    }
  }
  return true;
}

bool copyLabel(char* field, const std::size_t width, const char* source) noexcept {
  std::size_t length = 0U;
  while (source[length] != '\0') {
    ++length;
    if (length >= width) {
      return false;
    }
  }
  std::memset(field, 0, width);
  std::memcpy(field, source, length);
  return true;
}

// ---------------------------------------------------------------------------
// Status names
// ---------------------------------------------------------------------------

const char* codecStatusName(const CodecStatus status) noexcept {
  switch (status) {
    case CodecStatus::kOk: return "ok";
    case CodecStatus::kNullBuffer: return "null-buffer";
    case CodecStatus::kBufferTooSmall: return "buffer-too-small";
    case CodecStatus::kTruncated: return "truncated";
    case CodecStatus::kLengthMismatch: return "length-mismatch";
    case CodecStatus::kWrongKind: return "wrong-kind";
    case CodecStatus::kWrongVersion: return "wrong-version";
    case CodecStatus::kReservedNonZero: return "reserved-non-zero";
    case CodecStatus::kEnumOutOfRange: return "enum-out-of-range";
    case CodecStatus::kCountExceedsCapacity: return "count-exceeds-capacity";
    case CodecStatus::kCountMismatch: return "count-mismatch";
    case CodecStatus::kNonFinite: return "non-finite";
    case CodecStatus::kNonCanonical: return "non-canonical";
    case CodecStatus::kIntegerOutOfRange: return "integer-out-of-range";
    case CodecStatus::kInvalidValue: return "invalid-value";
    case CodecStatus::kInvalidString: return "invalid-string";
    case CodecStatus::kCrcMismatch: return "crc-mismatch";
    case CodecStatus::kOrdering: return "ordering";
    case CodecStatus::kDuplicate: return "duplicate";
    case CodecStatus::kInconsistent: return "inconsistent";
  }
  return "unknown";
}

// ===========================================================================
// ParameterDescriptor
// ===========================================================================

CodecStatus validate(const ParameterDescriptor& d) noexcept {
  if (!isParameterId(d.semantic_id) || d.semantic_version == 0U) {
    return CodecStatus::kIntegerOutOfRange;
  }
  const ValueKind vk = d.value_kind;
  if (vk == ValueKind::kNone ||
      static_cast<std::uint8_t>(vk) >= kValueKindCount ||
      static_cast<std::uint8_t>(d.scope) > 1U ||
      static_cast<std::uint8_t>(d.unit) >= kUnitCount ||
      static_cast<std::uint8_t>(d.formatter) >= kFormatterCount ||
      static_cast<std::uint8_t>(d.response_owner) >= kResponseOwnerCount ||
      static_cast<std::uint8_t>(d.numeric_encoding) >= kNumericEncodingCount ||
      static_cast<std::uint8_t>(d.lifecycle) >= kLifecycleCount ||
      static_cast<std::uint8_t>(d.migration_policy) >= kMigrationPolicyCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  if ((d.flags & static_cast<std::uint16_t>(~kParamFlagsDefined)) != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  if ((d.feature_id != 0U && !isFeatureId(d.feature_id)) ||
      (d.enable_semantic_id != 0U && !isParameterId(d.enable_semantic_id)) ||
      (d.dependency_semantic_id != 0U && !isParameterId(d.dependency_semantic_id)) ||
      (d.context_id != 0U && !isContextId(d.context_id))) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if (d.enable_semantic_id == d.semantic_id ||
      d.dependency_semantic_id == d.semantic_id) {
    return CodecStatus::kInconsistent;
  }
  for (const std::uint8_t alias : d.v1_alias) {
    if (alias != kNoV1Alias && alias >= kV1ControlCount) {
      return CodecStatus::kIntegerOutOfRange;
    }
  }
  const bool legacy = (d.flags & kParamFlagLegacyAlias) != 0U;
  if (d.scope == Scope::kGlobal) {
    if (d.v1_alias[1] != kNoV1Alias ||
        legacy != (d.v1_alias[0] != kNoV1Alias)) {
      return CodecStatus::kInconsistent;
    }
  } else {
    const bool both = d.v1_alias[0] != kNoV1Alias && d.v1_alias[1] != kNoV1Alias;
    const bool neither = d.v1_alias[0] == kNoV1Alias && d.v1_alias[1] == kNoV1Alias;
    if ((legacy && !both) || (!legacy && !neither) ||
        (both && d.v1_alias[0] == d.v1_alias[1])) {
      return CodecStatus::kInconsistent;
    }
  }
  K1_CV2_TRY(validTypedValue(d.legal_min));
  K1_CV2_TRY(validTypedValue(d.legal_max));
  K1_CV2_TRY(validTypedValue(d.default_value));
  K1_CV2_TRY(validTypedValue(d.neutral));
  K1_CV2_TRY(validTypedValue(d.canonical_step));
  if (d.legal_min.kind != vk || d.legal_max.kind != vk ||
      d.default_value.kind != vk ||
      (d.neutral.kind != ValueKind::kNone && d.neutral.kind != vk)) {
    return CodecStatus::kInconsistent;
  }
  const bool numeric = vk == ValueKind::kReal || vk == ValueKind::kInteger;
  if (numeric ? d.canonical_step.kind != vk
              : d.canonical_step.kind != ValueKind::kNone) {
    return CodecStatus::kInconsistent;
  }
  if (vk != ValueKind::kEnum && d.enum_count != 0U) {
    return CodecStatus::kInconsistent;
  }
  switch (vk) {
    case ValueKind::kReal: {
      const float lo = d.legal_min.asReal();
      const float hi = d.legal_max.asReal();
      const float def = d.default_value.asReal();
      const float step = d.canonical_step.asReal();
      if (!floatLess(lo, hi) || def < lo || def > hi || step < 0.0F ||
          step > (hi - lo)) {
        return CodecStatus::kInvalidValue;
      }
      if (d.neutral.kind == ValueKind::kReal &&
          (d.neutral.asReal() < lo || d.neutral.asReal() > hi)) {
        return CodecStatus::kInvalidValue;
      }
      break;
    }
    case ValueKind::kInteger: {
      const std::int64_t lo = d.legal_min.asInteger();
      const std::int64_t hi = d.legal_max.asInteger();
      const std::int64_t def = d.default_value.asInteger();
      const std::int64_t step = d.canonical_step.asInteger();
      if (lo >= hi || def < lo || def > hi || step < 1 || step > hi - lo) {
        return CodecStatus::kInvalidValue;
      }
      if (d.neutral.kind == ValueKind::kInteger &&
          (d.neutral.asInteger() < lo || d.neutral.asInteger() > hi)) {
        return CodecStatus::kInvalidValue;
      }
      break;
    }
    case ValueKind::kBoolean:
      if (d.legal_min.bits != 0U || d.legal_max.bits != 1U) {
        return CodecStatus::kInvalidValue;
      }
      break;
    case ValueKind::kEnum:
      if (d.enum_count == 0U || d.legal_min.bits != 0U ||
          d.legal_max.bits != static_cast<std::uint32_t>(d.enum_count - 1U) ||
          d.default_value.bits > d.legal_max.bits ||
          (d.neutral.kind == ValueKind::kEnum &&
           d.neutral.bits > d.legal_max.bits)) {
        return CodecStatus::kInvalidValue;
      }
      break;
    case ValueKind::kNone:
      return CodecStatus::kEnumOutOfRange;
  }
  if ((d.flags & kParamFlagEnable) != 0U &&
      (vk != ValueKind::kBoolean || d.feature_id == 0U)) {
    return CodecStatus::kInconsistent;
  }
  if ((d.flags & kParamFlagLogDomainLegal) != 0U &&
      (vk != ValueKind::kReal || !(d.legal_min.asReal() > 0.0F))) {
    return CodecStatus::kInconsistent;
  }
  if ((d.flags & kParamFlagRelativeAdmitted) != 0U && !numeric) {
    return CodecStatus::kInconsistent;
  }
  if ((d.flags & kParamFlagSparseEnum) != 0U && vk != ValueKind::kEnum) {
    return CodecStatus::kInconsistent;
  }
  if (d.response_owner != ResponseOwner::kImmediate && vk != ValueKind::kReal) {
    return CodecStatus::kInconsistent;
  }
  switch (d.numeric_encoding) {
    case NumericEncoding::kFloat32:
      if (vk != ValueKind::kReal) {
        return CodecStatus::kInconsistent;
      }
      break;
    case NumericEncoding::kInt32:
      if (vk != ValueKind::kInteger) {
        return CodecStatus::kInconsistent;
      }
      break;
    case NumericEncoding::kUInt16Q0_16:
      if (vk != ValueKind::kInteger || d.legal_min.asInteger() < 0 ||
          d.legal_max.asInteger() > 65535) {
        return CodecStatus::kInconsistent;
      }
      break;
    case NumericEncoding::kCc14:
      if (vk != ValueKind::kInteger || d.legal_min.asInteger() < 0 ||
          d.legal_max.asInteger() > 16383) {
        return CodecStatus::kInconsistent;
      }
      break;
    case NumericEncoding::kBoolean:
      if (vk != ValueKind::kBoolean) {
        return CodecStatus::kInconsistent;
      }
      break;
    case NumericEncoding::kEnumU16:
      if (vk != ValueKind::kEnum) {
        return CodecStatus::kInconsistent;
      }
      break;
  }
  K1_CV2_TRY(validFloat(d.encoding_error_bound));
  K1_CV2_TRY(validFloat(d.settle_epsilon));
  if (d.encoding_error_bound < 0.0F || d.settle_epsilon < 0.0F) {
    return CodecStatus::kInvalidValue;
  }
  if (!validKeyField(d.key, kKeyBytes) ||
      !validLabelField(d.label, kDescriptorLabelBytes, false) ||
      !validLabelField(d.short_label, kShortLabelBytes, false)) {
    return CodecStatus::kInvalidString;
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const ParameterDescriptor&) noexcept {
  return kParameterDescriptorBytes;
}

EncodeResult encode(const ParameterDescriptor& d, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(d);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  if (capacity < kParameterDescriptorBytes) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kParameterDescriptor, kParameterDescriptorBytes);
  w.u16(d.semantic_id);
  w.u16(d.semantic_version);
  w.u8(static_cast<std::uint8_t>(d.value_kind));
  w.u8(static_cast<std::uint8_t>(d.scope));
  w.u8(static_cast<std::uint8_t>(d.unit));
  w.u8(static_cast<std::uint8_t>(d.formatter));
  w.u8(static_cast<std::uint8_t>(d.response_owner));
  w.u8(static_cast<std::uint8_t>(d.numeric_encoding));
  w.u8(static_cast<std::uint8_t>(d.lifecycle));
  w.u8(static_cast<std::uint8_t>(d.migration_policy));
  w.u16(d.flags);
  w.u16(d.feature_id);
  w.u16(d.enable_semantic_id);
  w.u16(d.dependency_semantic_id);
  w.u16(d.context_id);
  w.u8(d.v1_alias[0]);
  w.u8(d.v1_alias[1]);
  w.typed(d.legal_min);
  w.typed(d.legal_max);
  w.typed(d.default_value);
  w.typed(d.neutral);
  w.typed(d.canonical_step);
  w.f32(d.encoding_error_bound);
  w.f32(d.settle_epsilon);
  w.u16(d.enum_count);
  w.u16(0U);
  w.bytes(d.key, kKeyBytes);
  w.bytes(d.label, kDescriptorLabelBytes);
  w.bytes(d.short_label, kShortLabelBytes);
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeParameter(const std::uint8_t* in, const std::size_t size,
                            ParameterDescriptor& d) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kParameterDescriptor, size));
  if (size != kParameterDescriptorBytes) {
    return size < kParameterDescriptorBytes ? CodecStatus::kTruncated
                                            : CodecStatus::kLengthMismatch;
  }
  d.semantic_id = r.u16();
  d.semantic_version = r.u16();
  const std::uint8_t e[8] = {r.u8(), r.u8(), r.u8(), r.u8(),
                             r.u8(), r.u8(), r.u8(), r.u8()};
  if (e[0] >= kValueKindCount || e[1] > 1U || e[2] >= kUnitCount ||
      e[3] >= kFormatterCount || e[4] >= kResponseOwnerCount ||
      e[5] >= kNumericEncodingCount || e[6] >= kLifecycleCount ||
      e[7] >= kMigrationPolicyCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  d.value_kind = static_cast<ValueKind>(e[0]);
  d.scope = static_cast<Scope>(e[1]);
  d.unit = static_cast<Unit>(e[2]);
  d.formatter = static_cast<Formatter>(e[3]);
  d.response_owner = static_cast<ResponseOwner>(e[4]);
  d.numeric_encoding = static_cast<NumericEncoding>(e[5]);
  d.lifecycle = static_cast<Lifecycle>(e[6]);
  d.migration_policy = static_cast<MigrationPolicy>(e[7]);
  d.flags = r.u16();
  d.feature_id = r.u16();
  d.enable_semantic_id = r.u16();
  d.dependency_semantic_id = r.u16();
  d.context_id = r.u16();
  d.v1_alias[0] = r.u8();
  d.v1_alias[1] = r.u8();
  K1_CV2_TRY(readTyped(r, d.legal_min));
  K1_CV2_TRY(readTyped(r, d.legal_max));
  K1_CV2_TRY(readTyped(r, d.default_value));
  K1_CV2_TRY(readTyped(r, d.neutral));
  K1_CV2_TRY(readTyped(r, d.canonical_step));
  if (!r.f32(d.encoding_error_bound) || !r.f32(d.settle_epsilon)) {
    return CodecStatus::kNonFinite;
  }
  d.enum_count = r.u16();
  if (!r.zeros(2U)) {
    return CodecStatus::kReservedNonZero;
  }
  r.bytes(d.key, kKeyBytes);
  r.bytes(d.label, kDescriptorLabelBytes);
  r.bytes(d.short_label, kShortLabelBytes);
  return validate(d);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   ParameterDescriptor& out) noexcept {
  const CodecStatus status = decodeParameter(in, size, out);
  if (status != CodecStatus::kOk) {
    out = ParameterDescriptor{};
  }
  return status;
}

// ===========================================================================
// FeatureDescriptor
// ===========================================================================

namespace {
template <std::size_t N>
CodecStatus validIdList(const std::uint16_t (&ids)[N], const std::uint8_t count,
                        bool (*valid_id)(std::uint16_t), const std::uint16_t self,
                        const std::uint8_t minimum_count) noexcept {
  if (count > N) {
    return CodecStatus::kCountExceedsCapacity;
  }
  if (count < minimum_count) {
    return CodecStatus::kCountMismatch;
  }
  for (std::size_t i = 0U; i < N; ++i) {
    if (i < count) {
      if (!valid_id(ids[i]) || ids[i] == self) {
        return ids[i] == 0U ? CodecStatus::kCountMismatch
                            : CodecStatus::kIntegerOutOfRange;
      }
      for (std::size_t j = 0U; j < i; ++j) {
        if (ids[j] == ids[i]) {
          return CodecStatus::kDuplicate;
        }
      }
    } else if (ids[i] != 0U) {
      return CodecStatus::kCountMismatch;
    }
  }
  return CodecStatus::kOk;
}
bool parameterIdPredicate(const std::uint16_t id) { return isParameterId(id); }
bool featureIdPredicate(const std::uint16_t id) { return isFeatureId(id); }
}  // namespace

CodecStatus validate(const FeatureDescriptor& f) noexcept {
  if (!isFeatureId(f.feature_id) || f.feature_version == 0U ||
      !isParameterId(f.enable_semantic_id)) {
    return CodecStatus::kIntegerOutOfRange;
  }
  const auto owner = static_cast<std::uint8_t>(f.owner);
  if (owner < kFeatureOwnerMin || owner > kFeatureOwnerMax ||
      static_cast<std::uint8_t>(f.bypass_policy) >= kBypassPolicyCount ||
      static_cast<std::uint8_t>(f.reenable_policy) >= kReenablePolicyCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  if ((f.reset_causes & static_cast<std::uint16_t>(~kResetCausesDefined)) != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  if (f.max_instances_per_channel == 0U) {
    return CodecStatus::kIntegerOutOfRange;
  }
  K1_CV2_TRY(validIdList(f.amount_semantic_ids, f.amount_count,
                         parameterIdPredicate, f.enable_semantic_id, 0U));
  K1_CV2_TRY(validIdList(f.dependency_feature_ids, f.dependency_count,
                         featureIdPredicate, f.feature_id, 0U));
  if (!validKeyField(f.key, kKeyBytes) ||
      !validLabelField(f.label, kDescriptorLabelBytes, false)) {
    return CodecStatus::kInvalidString;
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const FeatureDescriptor&) noexcept {
  return kFeatureDescriptorBytes;
}

EncodeResult encode(const FeatureDescriptor& f, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(f);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  if (capacity < kFeatureDescriptorBytes) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kFeatureDescriptor, kFeatureDescriptorBytes);
  w.u16(f.feature_id);
  w.u16(f.feature_version);
  w.u16(f.enable_semantic_id);
  w.u8(static_cast<std::uint8_t>(f.owner));
  w.u8(f.stage_order);
  w.u8(static_cast<std::uint8_t>(f.bypass_policy));
  w.u8(static_cast<std::uint8_t>(f.reenable_policy));
  w.u16(f.reset_causes);
  w.u32(f.state_bytes_per_channel);
  w.u8(f.max_instances_per_channel);
  w.u8(f.amount_count);
  w.u8(f.dependency_count);
  w.u8(0U);
  for (const std::uint16_t id : f.amount_semantic_ids) {
    w.u16(id);
  }
  for (const std::uint16_t id : f.dependency_feature_ids) {
    w.u16(id);
  }
  w.bytes(f.key, kKeyBytes);
  w.bytes(f.label, kDescriptorLabelBytes);
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeFeature(const std::uint8_t* in, const std::size_t size,
                          FeatureDescriptor& f) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kFeatureDescriptor, size));
  if (size != kFeatureDescriptorBytes) {
    return size < kFeatureDescriptorBytes ? CodecStatus::kTruncated
                                          : CodecStatus::kLengthMismatch;
  }
  f.feature_id = r.u16();
  f.feature_version = r.u16();
  f.enable_semantic_id = r.u16();
  const std::uint8_t owner = r.u8();
  f.stage_order = r.u8();
  const std::uint8_t bypass = r.u8();
  const std::uint8_t reenable = r.u8();
  if (owner < kFeatureOwnerMin || owner > kFeatureOwnerMax ||
      bypass >= kBypassPolicyCount || reenable >= kReenablePolicyCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  f.owner = static_cast<FeatureOwner>(owner);
  f.bypass_policy = static_cast<BypassPolicy>(bypass);
  f.reenable_policy = static_cast<ReenablePolicy>(reenable);
  f.reset_causes = r.u16();
  f.state_bytes_per_channel = r.u32();
  f.max_instances_per_channel = r.u8();
  f.amount_count = r.u8();
  f.dependency_count = r.u8();
  if (!r.zeros(1U)) {
    return CodecStatus::kReservedNonZero;
  }
  for (std::uint16_t& id : f.amount_semantic_ids) {
    id = r.u16();
  }
  for (std::uint16_t& id : f.dependency_feature_ids) {
    id = r.u16();
  }
  r.bytes(f.key, kKeyBytes);
  r.bytes(f.label, kDescriptorLabelBytes);
  return validate(f);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   FeatureDescriptor& out) noexcept {
  const CodecStatus status = decodeFeature(in, size, out);
  if (status != CodecStatus::kOk) {
    out = FeatureDescriptor{};
  }
  return status;
}

// ===========================================================================
// VisualContextDescriptor
// ===========================================================================

CodecStatus validate(const VisualContextDescriptor& c) noexcept {
  if (!isContextId(c.context_id) || c.context_version == 0U) {
    return CodecStatus::kIntegerOutOfRange;
  }
  const auto vis = static_cast<std::uint8_t>(c.visualiser);
  if (vis < kVisualiserMin || vis > kVisualiserMax ||
      static_cast<std::uint8_t>(c.scope) > 1U ||
      static_cast<std::uint8_t>(c.primary_unit) >= kUnitCount ||
      static_cast<std::uint8_t>(c.preview_basis) > 1U) {
    return CodecStatus::kEnumOutOfRange;
  }
  K1_CV2_TRY(validIdList(c.parameter_ids, c.parameter_count,
                         parameterIdPredicate, 0U, 1U));
  K1_CV2_TRY(validIdList(c.dependency_ids, c.dependency_count,
                         parameterIdPredicate, 0U, 0U));
  if (!validKeyField(c.key, kKeyBytes) ||
      !validLabelField(c.label, kDescriptorLabelBytes, false)) {
    return CodecStatus::kInvalidString;
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const VisualContextDescriptor&) noexcept {
  return kVisualContextDescriptorBytes;
}

EncodeResult encode(const VisualContextDescriptor& c, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(c);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  if (capacity < kVisualContextDescriptorBytes) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kVisualContextDescriptor, kVisualContextDescriptorBytes);
  w.u16(c.context_id);
  w.u16(c.context_version);
  w.u8(static_cast<std::uint8_t>(c.visualiser));
  w.u8(static_cast<std::uint8_t>(c.scope));
  w.u8(static_cast<std::uint8_t>(c.primary_unit));
  w.u8(static_cast<std::uint8_t>(c.preview_basis));
  w.u8(c.parameter_count);
  w.u8(c.dependency_count);
  w.u16(0U);
  for (const std::uint16_t id : c.parameter_ids) {
    w.u16(id);
  }
  for (const std::uint16_t id : c.dependency_ids) {
    w.u16(id);
  }
  w.bytes(c.key, kKeyBytes);
  w.bytes(c.label, kDescriptorLabelBytes);
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeContext(const std::uint8_t* in, const std::size_t size,
                          VisualContextDescriptor& c) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kVisualContextDescriptor, size));
  if (size != kVisualContextDescriptorBytes) {
    return size < kVisualContextDescriptorBytes ? CodecStatus::kTruncated
                                                : CodecStatus::kLengthMismatch;
  }
  c.context_id = r.u16();
  c.context_version = r.u16();
  const std::uint8_t vis = r.u8();
  const std::uint8_t scope = r.u8();
  const std::uint8_t unit = r.u8();
  const std::uint8_t basis = r.u8();
  if (vis < kVisualiserMin || vis > kVisualiserMax || scope > 1U ||
      unit >= kUnitCount || basis > 1U) {
    return CodecStatus::kEnumOutOfRange;
  }
  c.visualiser = static_cast<Visualiser>(vis);
  c.scope = static_cast<Scope>(scope);
  c.primary_unit = static_cast<Unit>(unit);
  c.preview_basis = static_cast<PreviewBasis>(basis);
  c.parameter_count = r.u8();
  c.dependency_count = r.u8();
  if (!r.zeros(2U)) {
    return CodecStatus::kReservedNonZero;
  }
  for (std::uint16_t& id : c.parameter_ids) {
    id = r.u16();
  }
  for (std::uint16_t& id : c.dependency_ids) {
    id = r.u16();
  }
  r.bytes(c.key, kKeyBytes);
  r.bytes(c.label, kDescriptorLabelBytes);
  return validate(c);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   VisualContextDescriptor& out) noexcept {
  const CodecStatus status = decodeContext(in, size, out);
  if (status != CodecStatus::kOk) {
    out = VisualContextDescriptor{};
  }
  return status;
}

// ===========================================================================
// ControlCapabilities
// ===========================================================================

CodecStatus validate(const ControlCapabilities& c) noexcept {
  const auto platform = static_cast<std::uint8_t>(c.platform);
  if (platform < kPlatformMin || platform > kPlatformMax ||
      static_cast<std::uint8_t>(c.capacity_status) >= kCapacityStatusCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  if (c.product_id == 0U || c.layout_version_min != kObjectLayoutVersion ||
      c.layout_version_max < c.layout_version_min ||
      c.registry_major != kRegistryMajor || c.channel_count == 0U ||
      c.channel_count > kChannelCount ||
      c.max_transaction_ops > kMaxTransactionOps ||
      c.max_targets > kMaxSnapshotEntries || c.max_pages > kMaxProfilePages ||
      c.max_snapshot_entries > kMaxSnapshotEntries ||
      c.max_label_bytes > kDescriptorLabelBytes) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if (c.max_bindings != c.max_pages * kSlotsPerPage ||
      (c.max_pages > 0U && c.max_profile_bytes < compiledProfileBytes(c.max_pages))) {
    return CodecStatus::kInconsistent;
  }
  if ((c.numeric_encoding_mask & static_cast<std::uint8_t>(~kEncodingMaskDefined)) != 0U ||
      (c.input_modality_mask & static_cast<std::uint8_t>(~kModalityMaskDefined)) != 0U ||
      (c.telemetry_mask & static_cast<std::uint8_t>(~kTelemetryMaskDefined)) != 0U ||
      (c.service_mask & static_cast<std::uint8_t>(~kServiceMaskDefined)) != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  if (c.parameter_count > kMaxCapabilityParameters ||
      c.mode_count > kMaxCapabilityModes) {
    return CodecStatus::kCountExceedsCapacity;
  }
  for (std::size_t i = 0U; i < c.parameter_count; ++i) {
    const CapabilityParameterEntry& e = c.parameters[i];
    if (!isParameterId(e.semantic_id)) {
      return CodecStatus::kIntegerOutOfRange;
    }
    if (static_cast<std::uint8_t>(e.support) >= kSupportCount ||
        static_cast<std::uint8_t>(e.inactive_reason) >= kInactiveReasonCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    if ((e.support == Support::kImplemented) !=
        (e.inactive_reason == InactiveReason::kNone)) {
      return CodecStatus::kInconsistent;
    }
    if (i > 0U && c.parameters[i - 1U].semantic_id >= e.semantic_id) {
      return c.parameters[i - 1U].semantic_id == e.semantic_id
                 ? CodecStatus::kDuplicate
                 : CodecStatus::kOrdering;
    }
  }
  for (std::size_t i = 0U; i < c.mode_count; ++i) {
    const CapabilityModeEntry& e = c.modes[i];
    if (e.mode_id > 0xFFU) {
      return CodecStatus::kIntegerOutOfRange;
    }
    if (static_cast<std::uint8_t>(e.support) >= kSupportCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    if ((e.support == Support::kImplemented) != (e.liveiness_adapter_version >= 1U)) {
      return CodecStatus::kInconsistent;
    }
    if (i > 0U && c.modes[i - 1U].mode_id >= e.mode_id) {
      return c.modes[i - 1U].mode_id == e.mode_id ? CodecStatus::kDuplicate
                                                  : CodecStatus::kOrdering;
    }
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const ControlCapabilities& c) noexcept {
  return kControlCapabilitiesFixedBytes +
         kCapabilityEntryBytes * (static_cast<std::size_t>(c.parameter_count) +
                                  static_cast<std::size_t>(c.mode_count));
}

EncodeResult encode(const ControlCapabilities& c, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(c);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  const std::size_t size = encodedSize(c);
  if (capacity < size) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kControlCapabilities, size);
  w.u32(c.product_id);
  w.u8(static_cast<std::uint8_t>(c.platform));
  w.u8(static_cast<std::uint8_t>(c.capacity_status));
  w.u8(c.layout_version_min);
  w.u8(c.layout_version_max);
  w.u16(c.registry_major);
  w.u16(c.registry_minor);
  w.bytes(c.registry_sha256, kDigestBytes);
  w.u32(c.engine_identity);
  w.u8(c.channel_count);
  w.u8(c.max_transaction_ops);
  w.u8(c.max_concurrent_leases);
  w.u8(c.max_writers);
  w.u16(c.max_targets);
  w.u16(c.max_pages);
  w.u16(c.max_bindings);
  w.u16(c.max_snapshot_entries);
  w.u16(c.max_label_bytes);
  w.u16(c.duplicate_window);
  w.u32(c.max_profile_bytes);
  w.u32(c.persistence_slot_bytes);
  w.u32(c.lease_timeout_us);
  w.u32(c.response_policy_mask);
  w.u8(c.numeric_encoding_mask);
  w.u8(c.input_modality_mask);
  w.u8(c.telemetry_mask);
  w.u8(c.service_mask);
  w.u16(c.parameter_count);
  w.u16(c.mode_count);
  for (std::size_t i = 0U; i < c.parameter_count; ++i) {
    w.u16(c.parameters[i].semantic_id);
    w.u8(static_cast<std::uint8_t>(c.parameters[i].support));
    w.u8(static_cast<std::uint8_t>(c.parameters[i].inactive_reason));
  }
  for (std::size_t i = 0U; i < c.mode_count; ++i) {
    w.u16(c.modes[i].mode_id);
    w.u8(c.modes[i].liveiness_adapter_version);
    w.u8(static_cast<std::uint8_t>(c.modes[i].support));
  }
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeCapabilities(const std::uint8_t* in, const std::size_t size,
                               ControlCapabilities& c) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kControlCapabilities, size));
  if (size < kControlCapabilitiesFixedBytes) {
    return CodecStatus::kTruncated;
  }
  c.product_id = r.u32();
  const std::uint8_t platform = r.u8();
  const std::uint8_t cap_status = r.u8();
  if (platform < kPlatformMin || platform > kPlatformMax ||
      cap_status >= kCapacityStatusCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  c.platform = static_cast<Platform>(platform);
  c.capacity_status = static_cast<CapacityStatus>(cap_status);
  c.layout_version_min = r.u8();
  c.layout_version_max = r.u8();
  c.registry_major = r.u16();
  c.registry_minor = r.u16();
  r.bytes(c.registry_sha256, kDigestBytes);
  c.engine_identity = r.u32();
  c.channel_count = r.u8();
  c.max_transaction_ops = r.u8();
  c.max_concurrent_leases = r.u8();
  c.max_writers = r.u8();
  c.max_targets = r.u16();
  c.max_pages = r.u16();
  c.max_bindings = r.u16();
  c.max_snapshot_entries = r.u16();
  c.max_label_bytes = r.u16();
  c.duplicate_window = r.u16();
  c.max_profile_bytes = r.u32();
  c.persistence_slot_bytes = r.u32();
  c.lease_timeout_us = r.u32();
  c.response_policy_mask = r.u32();
  c.numeric_encoding_mask = r.u8();
  c.input_modality_mask = r.u8();
  c.telemetry_mask = r.u8();
  c.service_mask = r.u8();
  c.parameter_count = r.u16();
  c.mode_count = r.u16();
  if (c.parameter_count > kMaxCapabilityParameters ||
      c.mode_count > kMaxCapabilityModes) {
    return CodecStatus::kCountExceedsCapacity;
  }
  // The declared counts must describe exactly the bytes that follow.
  if (size != encodedSize(c)) {
    return CodecStatus::kCountMismatch;
  }
  for (std::size_t i = 0U; i < c.parameter_count; ++i) {
    c.parameters[i].semantic_id = r.u16();
    const std::uint8_t support = r.u8();
    const std::uint8_t reason = r.u8();
    if (support >= kSupportCount || reason >= kInactiveReasonCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    c.parameters[i].support = static_cast<Support>(support);
    c.parameters[i].inactive_reason = static_cast<InactiveReason>(reason);
  }
  for (std::size_t i = 0U; i < c.mode_count; ++i) {
    c.modes[i].mode_id = r.u16();
    c.modes[i].liveiness_adapter_version = r.u8();
    const std::uint8_t support = r.u8();
    if (support >= kSupportCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    c.modes[i].support = static_cast<Support>(support);
  }
  return validate(c);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   ControlCapabilities& out) noexcept {
  const CodecStatus status = decodeCapabilities(in, size, out);
  if (status != CodecStatus::kOk) {
    out = ControlCapabilities{};
  }
  return status;
}

// ===========================================================================
// CompiledProfile
// ===========================================================================

namespace {

// True for finite floats that hold an exact int32 value. The range guard
// keeps the integer conversion defined for arbitrary decoded input.
bool isIntegral(const float value) noexcept {
  return finite(value) && value >= -2147483648.0F && value <= 2147483520.0F &&
         static_cast<float>(static_cast<std::int64_t>(value)) == value;
}

CodecStatus validateBinding(const CompiledBinding& b, const CompiledPage& page,
                            const std::size_t expected_page_index,
                            const std::uint8_t expected_slot) noexcept {
  if (b.page_index != expected_page_index || b.slot != expected_slot) {
    return CodecStatus::kOrdering;
  }
  if ((b.flags & static_cast<std::uint8_t>(~kBindingFlagsDefined)) != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  if (static_cast<std::uint8_t>(b.mapping_kind) >= kMappingKindCount ||
      static_cast<std::uint8_t>(b.channel) >= kChannelEnumCount ||
      static_cast<std::uint8_t>(b.takeover) >= kTakeoverModeCount ||
      b.response_policy_id >= kMaxResponsePolicies ||
      static_cast<std::uint8_t>(b.value_kind) >= kValueKindCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  const float floats[] = {b.range_min, b.range_max, b.exponent, b.hysteresis,
                          b.quant_step, b.pickup_tolerance, b.rel_gain,
                          b.rel_accel_exponent, b.rel_reference_velocity,
                          b.rel_max_multiplier, b.rel_max_rate};
  for (const float f : floats) {
    K1_CV2_TRY(validFloat(f));
  }
  if (b.channel != page.channel) {
    return CodecStatus::kInconsistent;
  }
  if ((b.flags & kBindingFlagUnused) != 0U) {
    bool all_zero = b.flags == kBindingFlagUnused && b.semantic_id == 0U &&
                    b.mapping_kind == MappingKind::kLinear &&
                    b.takeover == TakeoverMode::kPickup &&
                    b.response_policy_id == 0U &&
                    b.value_kind == ValueKind::kNone &&
                    b.target_index == 0xFFFFU && b.input_steps == 0U;
    for (const float f : floats) {
      all_zero = all_zero && floatBits(f) == 0U;
    }
    if (!all_zero) {
      return CodecStatus::kInconsistent;
    }
    return validLabelField(b.label, kBindingLabelBytes, true) &&
                   b.label[0] == '\0'
               ? CodecStatus::kOk
               : CodecStatus::kInvalidString;
  }
  if (!isParameterId(b.semantic_id) || b.target_index == 0xFFFFU) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if (b.channel == Channel::kGlobal ||
      (b.value_kind != ValueKind::kReal && b.value_kind != ValueKind::kInteger)) {
    return CodecStatus::kInconsistent;
  }
  if (!floatLess(b.range_min, b.range_max)) {
    return CodecStatus::kInvalidValue;
  }
  if (b.mapping_kind == MappingKind::kLog && !(b.range_min > 0.0F)) {
    return CodecStatus::kInvalidValue;
  }
  if (b.mapping_kind == MappingKind::kExponent
          ? (b.exponent < kExponentMin || b.exponent > kExponentMax)
          : b.exponent != 1.0F) {
    return CodecStatus::kInvalidValue;
  }
  if (b.input_steps < kInputStepsMin || b.input_steps > kInputStepsMax) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if (b.hysteresis < 0.0F || b.hysteresis > kHysteresisMax ||
      b.pickup_tolerance < 0.0F || b.pickup_tolerance > kPickupToleranceMax) {
    return CodecStatus::kInvalidValue;
  }
  const float span = b.range_max - b.range_min;
  if (b.quant_step < 0.0F || b.quant_step > span) {
    return CodecStatus::kInvalidValue;
  }
  if (b.value_kind == ValueKind::kInteger &&
      (!isIntegral(b.range_min) || !isIntegral(b.range_max) ||
       !isIntegral(b.quant_step) || b.quant_step < 1.0F)) {
    return CodecStatus::kInvalidValue;
  }
  const bool relative = (b.flags & kBindingFlagRelative) != 0U;
  if (relative) {
    if (b.takeover != TakeoverMode::kJump || floatBits(b.pickup_tolerance) != 0U ||
        !(b.rel_gain > 0.0F) ||
        b.rel_gain > kRelativeGainMax || b.rel_accel_exponent < 0.0F ||
        b.rel_accel_exponent > kRelativeAccelExponentMax ||
        !(b.rel_reference_velocity > 0.0F) ||
        b.rel_reference_velocity > kRelativeReferenceVelocityMax ||
        b.rel_max_multiplier < 1.0F ||
        b.rel_max_multiplier > kRelativeMaxMultiplierMax ||
        !(b.rel_max_rate > 0.0F) || b.rel_max_rate > kSlewRateMax) {
      return CodecStatus::kInvalidValue;
    }
  } else if (floatBits(b.rel_gain) != 0U || floatBits(b.rel_accel_exponent) != 0U ||
             floatBits(b.rel_reference_velocity) != 0U ||
             floatBits(b.rel_max_multiplier) != 0U ||
             floatBits(b.rel_max_rate) != 0U) {
    // Absolute mappings reject acceleration and relative gain entirely.
    return CodecStatus::kInconsistent;
  }
  return validLabelField(b.label, kBindingLabelBytes, true)
             ? CodecStatus::kOk
             : CodecStatus::kInvalidString;
}

}  // namespace

CodecStatus validate(const CompiledProfile& p) noexcept {
  if (p.profile_schema_major != kProfileSchemaMajor || p.compiler_version == 0U ||
      p.profile_id == 0U || p.profile_revision == 0U) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if (p.channel_count != kChannelCount) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if (p.page_count == 0U || p.page_count > kMaxProfilePages) {
    return p.page_count == 0U ? CodecStatus::kCountMismatch
                              : CodecStatus::kCountExceedsCapacity;
  }
  if (p.binding_count != p.page_count * kSlotsPerPage) {
    return CodecStatus::kCountMismatch;
  }
  if (!validLabelField(p.profile_label, kProfileLabelBytes, false)) {
    return CodecStatus::kInvalidString;
  }
  std::uint8_t perform[kChannelCount] = {0U, 0U};
  std::uint8_t colour[kChannelCount] = {0U, 0U};
  std::uint8_t per_channel[kChannelCount] = {0U, 0U};
  for (std::size_t i = 0U; i < p.page_count; ++i) {
    const CompiledPage& page = p.pages[i];
    if (static_cast<std::uint8_t>(page.channel) >= kChannelCount ||
        static_cast<std::uint8_t>(page.kind) >= kPageKindCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    if (page.page_id == 0U || page.slot_count != kSlotsPerPage) {
      return CodecStatus::kIntegerOutOfRange;
    }
    if (!validLabelField(page.label, kPageLabelBytes, false)) {
      return CodecStatus::kInvalidString;
    }
    if (i > 0U) {
      const CompiledPage& prev = p.pages[i - 1U];
      const bool ascending =
          static_cast<std::uint8_t>(prev.channel) < static_cast<std::uint8_t>(page.channel) ||
          (prev.channel == page.channel && prev.page_id < page.page_id);
      if (!ascending) {
        return prev.channel == page.channel && prev.page_id == page.page_id
                   ? CodecStatus::kDuplicate
                   : CodecStatus::kOrdering;
      }
    }
    const auto ch = static_cast<std::size_t>(page.channel);
    ++per_channel[ch];
    if (page.kind == PageKind::kPerform && ++perform[ch] > 1U) {
      return CodecStatus::kDuplicate;
    }
    if (page.kind == PageKind::kColour && ++colour[ch] > 1U) {
      return CodecStatus::kDuplicate;
    }
  }
  if (per_channel[0] == 0U || per_channel[1] == 0U) {
    return CodecStatus::kInconsistent;
  }
  for (std::size_t i = 0U; i < p.binding_count; ++i) {
    const std::size_t page_index = i / kSlotsPerPage;
    const auto slot = static_cast<std::uint8_t>(i % kSlotsPerPage + 1U);
    K1_CV2_TRY(validateBinding(p.bindings[i], p.pages[page_index], page_index, slot));
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const CompiledProfile& p) noexcept {
  return compiledProfileBytes(p.page_count);
}

EncodeResult encode(const CompiledProfile& p, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(p);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  const std::size_t size = encodedSize(p);
  if (capacity < size) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kCompiledProfile, size);
  w.u16(p.profile_schema_major);
  w.u16(p.compiler_version);
  w.u32(p.profile_id);
  w.u32(p.profile_revision);
  w.bytes(p.registry_sha256, kDigestBytes);
  w.u8(p.channel_count);
  w.u8(p.page_count);
  w.u16(p.binding_count);
  w.bytes(p.profile_label, kProfileLabelBytes);
  for (std::size_t i = 0U; i < p.page_count; ++i) {
    const CompiledPage& page = p.pages[i];
    w.u8(page.page_id);
    w.u8(static_cast<std::uint8_t>(page.channel));
    w.u8(static_cast<std::uint8_t>(page.kind));
    w.u8(page.slot_count);
    w.bytes(page.label, kPageLabelBytes);
  }
  for (std::size_t i = 0U; i < p.binding_count; ++i) {
    const CompiledBinding& b = p.bindings[i];
    w.u8(b.page_index);
    w.u8(b.slot);
    w.u8(b.flags);
    w.u8(static_cast<std::uint8_t>(b.mapping_kind));
    w.u16(b.semantic_id);
    w.u8(static_cast<std::uint8_t>(b.channel));
    w.u8(static_cast<std::uint8_t>(b.takeover));
    w.u8(b.response_policy_id);
    w.u8(static_cast<std::uint8_t>(b.value_kind));
    w.u16(b.target_index);
    w.f32(b.range_min);
    w.f32(b.range_max);
    w.f32(b.exponent);
    w.u32(b.input_steps);
    w.f32(b.hysteresis);
    w.f32(b.quant_step);
    w.f32(b.pickup_tolerance);
    w.f32(b.rel_gain);
    w.f32(b.rel_accel_exponent);
    w.f32(b.rel_reference_velocity);
    w.f32(b.rel_max_multiplier);
    w.f32(b.rel_max_rate);
    w.bytes(b.label, kBindingLabelBytes);
  }
  w.u32(crc32Ieee(out, w.position()));
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeProfile(const std::uint8_t* in, const std::size_t size,
                          CompiledProfile& p) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kCompiledProfile, size));
  if (size < kCompiledProfileFixedBytes + kCompiledProfileTrailerBytes) {
    return CodecStatus::kTruncated;
  }
  p.profile_schema_major = r.u16();
  p.compiler_version = r.u16();
  p.profile_id = r.u32();
  p.profile_revision = r.u32();
  r.bytes(p.registry_sha256, kDigestBytes);
  p.channel_count = r.u8();
  p.page_count = r.u8();
  p.binding_count = r.u16();
  r.bytes(p.profile_label, kProfileLabelBytes);
  if (p.page_count > kMaxProfilePages ||
      p.binding_count > kMaxProfileBindings) {
    return CodecStatus::kCountExceedsCapacity;
  }
  if (p.binding_count != p.page_count * kSlotsPerPage) {
    return CodecStatus::kCountMismatch;
  }
  if (size != compiledProfileBytes(p.page_count)) {
    return CodecStatus::kCountMismatch;
  }
  const std::uint32_t crc_expected =
      static_cast<std::uint32_t>(in[size - 4U]) |
      (static_cast<std::uint32_t>(in[size - 3U]) << 8U) |
      (static_cast<std::uint32_t>(in[size - 2U]) << 16U) |
      (static_cast<std::uint32_t>(in[size - 1U]) << 24U);
  if (crc32Ieee(in, size - 4U) != crc_expected) {
    return CodecStatus::kCrcMismatch;
  }
  for (std::size_t i = 0U; i < p.page_count; ++i) {
    CompiledPage& page = p.pages[i];
    page.page_id = r.u8();
    const std::uint8_t channel = r.u8();
    const std::uint8_t kind = r.u8();
    page.slot_count = r.u8();
    if (channel >= kChannelEnumCount || kind >= kPageKindCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    page.channel = static_cast<Channel>(channel);
    page.kind = static_cast<PageKind>(kind);
    r.bytes(page.label, kPageLabelBytes);
  }
  for (std::size_t i = 0U; i < p.binding_count; ++i) {
    CompiledBinding& b = p.bindings[i];
    b.page_index = r.u8();
    b.slot = r.u8();
    b.flags = r.u8();
    const std::uint8_t mapping = r.u8();
    b.semantic_id = r.u16();
    const std::uint8_t channel = r.u8();
    const std::uint8_t takeover = r.u8();
    b.response_policy_id = r.u8();
    const std::uint8_t value_kind = r.u8();
    if (mapping >= kMappingKindCount || channel >= kChannelEnumCount ||
        takeover >= kTakeoverModeCount || b.response_policy_id >= kMaxResponsePolicies ||
        value_kind >= kValueKindCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    b.mapping_kind = static_cast<MappingKind>(mapping);
    b.channel = static_cast<Channel>(channel);
    b.takeover = static_cast<TakeoverMode>(takeover);
    b.value_kind = static_cast<ValueKind>(value_kind);
    b.target_index = r.u16();
    bool ok = r.f32(b.range_min);
    ok = r.f32(b.range_max) && ok;
    ok = r.f32(b.exponent) && ok;
    b.input_steps = r.u32();
    ok = r.f32(b.hysteresis) && ok;
    ok = r.f32(b.quant_step) && ok;
    ok = r.f32(b.pickup_tolerance) && ok;
    ok = r.f32(b.rel_gain) && ok;
    ok = r.f32(b.rel_accel_exponent) && ok;
    ok = r.f32(b.rel_reference_velocity) && ok;
    ok = r.f32(b.rel_max_multiplier) && ok;
    ok = r.f32(b.rel_max_rate) && ok;
    if (!ok) {
      return CodecStatus::kNonFinite;
    }
    r.bytes(b.label, kBindingLabelBytes);
  }
  return validate(p);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   CompiledProfile& out) noexcept {
  const CodecStatus status = decodeProfile(in, size, out);
  if (status != CodecStatus::kOk) {
    out = CompiledProfile{};
  }
  return status;
}

// ===========================================================================
// GestureIntent
// ===========================================================================

CodecStatus validate(const GestureIntent& g) noexcept {
  if (static_cast<std::uint8_t>(g.intent_kind) > 1U) {
    return CodecStatus::kEnumOutOfRange;
  }
  if ((g.flags & static_cast<std::uint8_t>(~kIntentFlagsDefined)) != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  if (g.writer_id == 0U || g.session_epoch == 0U || g.engine_epoch == 0U ||
      g.gesture_id == 0U || g.sequence == 0U || g.profile_revision == 0U ||
      g.page_id == 0U || g.slot == 0U ||
      g.slot > kSlotsPerPage || g.observed_time_us > kMaxTimeUs ||
      g.response_policy_id >= kMaxResponsePolicies) {
    return CodecStatus::kIntegerOutOfRange;
  }
  K1_CV2_TRY(validTarget(g.target));
  if (g.target.channel == Channel::kGlobal) {
    return CodecStatus::kInconsistent;
  }
  K1_CV2_TRY(validTypedValue(g.value));
  if (g.value.kind != ValueKind::kReal && g.value.kind != ValueKind::kInteger) {
    return CodecStatus::kInconsistent;
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const GestureIntent&) noexcept { return kGestureIntentBytes; }

EncodeResult encode(const GestureIntent& g, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(g);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  if (capacity < kGestureIntentBytes) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kGestureIntent, kGestureIntentBytes);
  w.u16(g.writer_id);
  w.u8(static_cast<std::uint8_t>(g.intent_kind));
  w.u8(g.flags);
  w.u32(g.session_epoch);
  w.u32(g.engine_epoch);
  w.u32(g.gesture_id);
  w.u32(g.sequence);
  w.u32(g.profile_revision);
  w.u32(g.binding_revision);
  w.target(g.target);
  w.u8(g.page_id);
  w.u8(g.slot);
  w.u8(g.response_policy_id);
  w.u8(0U);
  w.typed(g.value);
  w.u64(g.observed_time_us);
  w.u32(g.expected_target_revision);
  w.u16(g.validity_ms);
  w.u16(0U);
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeIntent(const std::uint8_t* in, const std::size_t size,
                         GestureIntent& g) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kGestureIntent, size));
  if (size != kGestureIntentBytes) {
    return size < kGestureIntentBytes ? CodecStatus::kTruncated
                                      : CodecStatus::kLengthMismatch;
  }
  g.writer_id = r.u16();
  const std::uint8_t kind = r.u8();
  if (kind > 1U) {
    return CodecStatus::kEnumOutOfRange;
  }
  g.intent_kind = static_cast<IntentKind>(kind);
  g.flags = r.u8();
  g.session_epoch = r.u32();
  g.engine_epoch = r.u32();
  g.gesture_id = r.u32();
  g.sequence = r.u32();
  g.profile_revision = r.u32();
  g.binding_revision = r.u32();
  K1_CV2_TRY(readTarget(r, g.target));
  g.page_id = r.u8();
  g.slot = r.u8();
  g.response_policy_id = r.u8();
  if (!r.zeros(1U)) {
    return CodecStatus::kReservedNonZero;
  }
  K1_CV2_TRY(readTyped(r, g.value));
  g.observed_time_us = r.u64();
  g.expected_target_revision = r.u32();
  g.validity_ms = r.u16();
  if (!r.zeros(2U)) {
    return CodecStatus::kReservedNonZero;
  }
  return validate(g);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   GestureIntent& out) noexcept {
  const CodecStatus status = decodeIntent(in, size, out);
  if (status != CodecStatus::kOk) {
    out = GestureIntent{};
  }
  return status;
}

// ===========================================================================
// ControlTransaction
// ===========================================================================

CodecStatus validate(const ControlTransaction& t) noexcept {
  if (static_cast<std::uint8_t>(t.source) >= kTransactionSourceCount ||
      static_cast<std::uint8_t>(t.atomicity) >= kAtomicityCount ||
      t.apply_timing != ApplyTiming::kNextSafeBoundary) {
    return CodecStatus::kEnumOutOfRange;
  }
  if ((t.flags & static_cast<std::uint8_t>(~kTxnFlagsDefined)) != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  if ((t.flags & kTxnFlagOverrideLeases) != 0U &&
      t.source != TransactionSource::kPresetRecall &&
      t.source != TransactionSource::kBlackout &&
      t.source != TransactionSource::kExternalSync) {
    return CodecStatus::kInconsistent;
  }
  if (t.writer_id == 0U || t.session_epoch == 0U || t.transaction_id == 0U ||
      t.engine_epoch == 0U || t.deadline_us > kMaxTimeUs) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if (t.op_count == 0U) {
    return CodecStatus::kCountMismatch;
  }
  if (t.op_count > kMaxTransactionOps) {
    return CodecStatus::kCountExceedsCapacity;
  }
  bool saw_primary = false;
  bool saw_secondary = false;
  bool saw_global = false;
  for (std::size_t i = 0U; i < t.op_count; ++i) {
    const ControlOp& op = t.ops[i];
    K1_CV2_TRY(validTarget(op.target));
    if (static_cast<std::uint8_t>(op.op_kind) >= kOpKindCount ||
        op.response_policy_id >= kMaxResponsePolicies) {
      return CodecStatus::kEnumOutOfRange;
    }
    if ((op.op_flags & static_cast<std::uint8_t>(~kOpFlagsDefined)) != 0U) {
      return CodecStatus::kReservedNonZero;
    }
    if ((op.op_flags & kOpFlagTerminal) != 0U && op.gesture_id == 0U) {
      return CodecStatus::kInconsistent;
    }
    K1_CV2_TRY(validTypedValue(op.value));
    if (op.value.kind == ValueKind::kNone ||
        (op.op_kind == OpKind::kApplyDelta && op.value.kind != ValueKind::kReal &&
         op.value.kind != ValueKind::kInteger)) {
      return CodecStatus::kInconsistent;
    }
    for (std::size_t j = 0U; j < i; ++j) {
      if (t.ops[j].target == op.target) {
        return CodecStatus::kDuplicate;
      }
    }
    saw_primary = saw_primary || op.target.channel == Channel::kPrimary;
    saw_secondary = saw_secondary || op.target.channel == Channel::kSecondary;
    saw_global = saw_global || op.target.channel == Channel::kGlobal;
  }
  switch (t.atomicity) {
    case Atomicity::kSingleTarget:
      if (t.op_count != 1U) {
        return CodecStatus::kInconsistent;
      }
      break;
    case Atomicity::kChannel:
      if (saw_global || (saw_primary && saw_secondary)) {
        return CodecStatus::kInconsistent;
      }
      break;
    case Atomicity::kLinkedChannels:
      if (saw_global || !saw_primary || !saw_secondary) {
        return CodecStatus::kInconsistent;
      }
      break;
    case Atomicity::kGlobal:
      if (saw_primary || saw_secondary) {
        return CodecStatus::kInconsistent;
      }
      break;
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const ControlTransaction& t) noexcept {
  return kControlTransactionFixedBytes +
         kControlOpBytes * static_cast<std::size_t>(t.op_count);
}

EncodeResult encode(const ControlTransaction& t, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(t);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  const std::size_t size = encodedSize(t);
  if (capacity < size) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kControlTransaction, size);
  w.u16(t.writer_id);
  w.u8(static_cast<std::uint8_t>(t.source));
  w.u8(t.flags);
  w.u32(t.session_epoch);
  w.u32(t.transaction_id);
  w.u32(t.engine_epoch);
  w.u32(t.profile_revision);
  w.u64(t.deadline_us);
  w.u8(static_cast<std::uint8_t>(t.atomicity));
  w.u8(static_cast<std::uint8_t>(t.apply_timing));
  w.u8(t.op_count);
  w.u8(0U);
  for (std::size_t i = 0U; i < t.op_count; ++i) {
    const ControlOp& op = t.ops[i];
    w.target(op.target);
    w.u8(static_cast<std::uint8_t>(op.op_kind));
    w.u8(op.op_flags);
    w.u8(op.response_policy_id);
    w.u8(0U);
    w.u32(op.expected_target_revision);
    w.u32(op.gesture_id);
    w.typed(op.value);
  }
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeTransaction(const std::uint8_t* in, const std::size_t size,
                              ControlTransaction& t) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kControlTransaction, size));
  if (size < kControlTransactionFixedBytes) {
    return CodecStatus::kTruncated;
  }
  t.writer_id = r.u16();
  const std::uint8_t source = r.u8();
  if (source >= kTransactionSourceCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  t.source = static_cast<TransactionSource>(source);
  t.flags = r.u8();
  t.session_epoch = r.u32();
  t.transaction_id = r.u32();
  t.engine_epoch = r.u32();
  t.profile_revision = r.u32();
  t.deadline_us = r.u64();
  const std::uint8_t atomicity = r.u8();
  const std::uint8_t timing = r.u8();
  if (atomicity >= kAtomicityCount ||
      timing != static_cast<std::uint8_t>(ApplyTiming::kNextSafeBoundary)) {
    return CodecStatus::kEnumOutOfRange;
  }
  t.atomicity = static_cast<Atomicity>(atomicity);
  t.apply_timing = ApplyTiming::kNextSafeBoundary;
  t.op_count = r.u8();
  if (!r.zeros(1U)) {
    return CodecStatus::kReservedNonZero;
  }
  if (t.op_count > kMaxTransactionOps) {
    return CodecStatus::kCountExceedsCapacity;
  }
  if (size != encodedSize(t)) {
    return CodecStatus::kCountMismatch;
  }
  for (std::size_t i = 0U; i < t.op_count; ++i) {
    ControlOp& op = t.ops[i];
    K1_CV2_TRY(readTarget(r, op.target));
    const std::uint8_t kind = r.u8();
    if (kind >= kOpKindCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    op.op_kind = static_cast<OpKind>(kind);
    op.op_flags = r.u8();
    op.response_policy_id = r.u8();
    if (!r.zeros(1U)) {
      return CodecStatus::kReservedNonZero;
    }
    op.expected_target_revision = r.u32();
    op.gesture_id = r.u32();
    K1_CV2_TRY(readTyped(r, op.value));
  }
  return validate(t);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   ControlTransaction& out) noexcept {
  const CodecStatus status = decodeTransaction(in, size, out);
  if (status != CodecStatus::kOk) {
    out = ControlTransaction{};
  }
  return status;
}

// ===========================================================================
// ApplyReceipt
// ===========================================================================

namespace {
CodecStatus validStateFlags(const std::uint8_t flags,
                            const InactiveReason reason) noexcept {
  if ((flags & static_cast<std::uint8_t>(~kStateFlagsDefined)) != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  if (static_cast<std::uint8_t>(reason) >= kInactiveReasonCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  if (((flags & kStateFlagActive) != 0U) != (reason == InactiveReason::kNone)) {
    return CodecStatus::kInconsistent;
  }
  return CodecStatus::kOk;
}
}  // namespace

CodecStatus validate(const ApplyReceipt& a) noexcept {
  if (static_cast<std::uint8_t>(a.outcome) >= kOutcomeCount ||
      static_cast<std::uint8_t>(a.reason) >= kReasonCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  if (a.writer_id == 0U || a.session_epoch == 0U || a.transaction_id == 0U ||
      a.engine_epoch == 0U || a.engine_time_us > kMaxTimeUs) {
    return CodecStatus::kIntegerOutOfRange;
  }
  const bool accepted = a.outcome == Outcome::kAccepted;
  if (accepted != (a.reason == Reason::kNone)) {
    return CodecStatus::kInconsistent;
  }
  if (a.op_count > kMaxTransactionOps) {
    return CodecStatus::kCountExceedsCapacity;
  }
  if (accepted && (a.op_count == 0U || a.failing_op_index != 0xFFU)) {
    return CodecStatus::kInconsistent;
  }
  if (a.failing_op_index != 0xFFU && a.failing_op_index >= a.op_count) {
    return CodecStatus::kIntegerOutOfRange;
  }
  for (std::size_t i = 0U; i < a.op_count; ++i) {
    const ReceiptOpResult& res = a.results[i];
    K1_CV2_TRY(validTarget(res.target));
    if (static_cast<std::uint8_t>(res.outcome) >= kOutcomeCount ||
        static_cast<std::uint8_t>(res.reason) >= kReasonCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    if ((res.outcome == Outcome::kAccepted) != (res.reason == Reason::kNone) ||
        (accepted && res.outcome != Outcome::kAccepted)) {
      return CodecStatus::kInconsistent;
    }
    K1_CV2_TRY(validStateFlags(res.flags, res.inactive_reason));
    if (res.target_revision > a.commit_revision) {
      return CodecStatus::kInconsistent;
    }
    K1_CV2_TRY(validTypedValue(res.accepted_base));
    if (res.outcome == Outcome::kAccepted &&
        res.accepted_base.kind == ValueKind::kNone) {
      return CodecStatus::kInconsistent;
    }
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const ApplyReceipt& a) noexcept {
  return kApplyReceiptFixedBytes +
         kReceiptOpBytes * static_cast<std::size_t>(a.op_count);
}

EncodeResult encode(const ApplyReceipt& a, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(a);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  const std::size_t size = encodedSize(a);
  if (capacity < size) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kApplyReceipt, size);
  w.u16(a.writer_id);
  w.u8(static_cast<std::uint8_t>(a.outcome));
  w.u8(static_cast<std::uint8_t>(a.reason));
  w.u32(a.session_epoch);
  w.u32(a.transaction_id);
  w.u32(a.engine_epoch);
  w.u32(a.commit_revision);
  w.u64(a.engine_time_us);
  w.u8(a.failing_op_index);
  w.u8(a.op_count);
  w.u16(0U);
  for (std::size_t i = 0U; i < a.op_count; ++i) {
    const ReceiptOpResult& res = a.results[i];
    w.target(res.target);
    w.u8(static_cast<std::uint8_t>(res.outcome));
    w.u8(static_cast<std::uint8_t>(res.reason));
    w.u8(static_cast<std::uint8_t>(res.inactive_reason));
    w.u8(res.flags);
    w.u32(res.target_revision);
    w.typed(res.accepted_base);
  }
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeReceipt(const std::uint8_t* in, const std::size_t size,
                          ApplyReceipt& a) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kApplyReceipt, size));
  if (size < kApplyReceiptFixedBytes) {
    return CodecStatus::kTruncated;
  }
  a.writer_id = r.u16();
  const std::uint8_t outcome = r.u8();
  const std::uint8_t reason = r.u8();
  if (outcome >= kOutcomeCount || reason >= kReasonCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  a.outcome = static_cast<Outcome>(outcome);
  a.reason = static_cast<Reason>(reason);
  a.session_epoch = r.u32();
  a.transaction_id = r.u32();
  a.engine_epoch = r.u32();
  a.commit_revision = r.u32();
  a.engine_time_us = r.u64();
  a.failing_op_index = r.u8();
  a.op_count = r.u8();
  if (!r.zeros(2U)) {
    return CodecStatus::kReservedNonZero;
  }
  if (a.op_count > kMaxTransactionOps) {
    return CodecStatus::kCountExceedsCapacity;
  }
  if (size != encodedSize(a)) {
    return CodecStatus::kCountMismatch;
  }
  for (std::size_t i = 0U; i < a.op_count; ++i) {
    ReceiptOpResult& res = a.results[i];
    K1_CV2_TRY(readTarget(r, res.target));
    const std::uint8_t o = r.u8();
    const std::uint8_t rs = r.u8();
    const std::uint8_t inactive = r.u8();
    if (o >= kOutcomeCount || rs >= kReasonCount || inactive >= kInactiveReasonCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    res.outcome = static_cast<Outcome>(o);
    res.reason = static_cast<Reason>(rs);
    res.inactive_reason = static_cast<InactiveReason>(inactive);
    res.flags = r.u8();
    res.target_revision = r.u32();
    K1_CV2_TRY(readTyped(r, res.accepted_base));
  }
  return validate(a);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   ApplyReceipt& out) noexcept {
  const CodecStatus status = decodeReceipt(in, size, out);
  if (status != CodecStatus::kOk) {
    out = ApplyReceipt{};
  }
  return status;
}

// ===========================================================================
// StateSnapshot
// ===========================================================================

CodecStatus validate(const StateSnapshot& s) noexcept {
  if (s.engine_epoch == 0U || s.engine_time_us > kMaxTimeUs) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if ((s.profile_revision == 0U) != allZero(s.profile_sha256, kDigestBytes)) {
    return CodecStatus::kInconsistent;
  }
  if (s.entry_count > kMaxSnapshotEntries) {
    return CodecStatus::kCountExceedsCapacity;
  }
  for (std::size_t i = 0U; i < s.entry_count; ++i) {
    const SnapshotEntry& e = s.entries[i];
    K1_CV2_TRY(validTarget(e.target));
    if (i > 0U && !targetLess(s.entries[i - 1U].target, e.target)) {
      return s.entries[i - 1U].target == e.target ? CodecStatus::kDuplicate
                                                  : CodecStatus::kOrdering;
    }
    if (e.target_revision > s.snapshot_revision) {
      return CodecStatus::kInconsistent;
    }
    K1_CV2_TRY(validTypedValue(e.base));
    if (e.base.kind == ValueKind::kNone) {
      return CodecStatus::kInvalidValue;
    }
    K1_CV2_TRY(validStateFlags(e.flags, e.inactive_reason));
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const StateSnapshot& s) noexcept {
  return kStateSnapshotFixedBytes +
         kSnapshotEntryBytes * static_cast<std::size_t>(s.entry_count) + 4U;
}

EncodeResult encode(const StateSnapshot& s, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(s);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  const std::size_t size = encodedSize(s);
  if (capacity < size) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kStateSnapshot, size);
  w.u32(s.engine_epoch);
  w.u32(s.snapshot_revision);
  w.u32(s.profile_revision);
  w.bytes(s.registry_sha256, kDigestBytes);
  w.bytes(s.profile_sha256, kDigestBytes);
  w.u64(s.engine_time_us);
  w.u16(s.entry_count);
  w.u16(0U);
  for (std::size_t i = 0U; i < s.entry_count; ++i) {
    const SnapshotEntry& e = s.entries[i];
    w.target(e.target);
    w.u32(e.target_revision);
    w.typed(e.base);
    w.u8(e.flags);
    w.u8(static_cast<std::uint8_t>(e.inactive_reason));
    w.u16(0U);
  }
  w.u32(crc32Ieee(out, w.position()));
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeSnapshot(const std::uint8_t* in, const std::size_t size,
                           StateSnapshot& s) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kStateSnapshot, size));
  if (size < kStateSnapshotFixedBytes + 4U) {
    return CodecStatus::kTruncated;
  }
  s.engine_epoch = r.u32();
  s.snapshot_revision = r.u32();
  s.profile_revision = r.u32();
  r.bytes(s.registry_sha256, kDigestBytes);
  r.bytes(s.profile_sha256, kDigestBytes);
  s.engine_time_us = r.u64();
  s.entry_count = r.u16();
  if (!r.zeros(2U)) {
    return CodecStatus::kReservedNonZero;
  }
  if (s.entry_count > kMaxSnapshotEntries) {
    return CodecStatus::kCountExceedsCapacity;
  }
  if (size != encodedSize(s)) {
    return CodecStatus::kCountMismatch;
  }
  const std::uint32_t crc_expected =
      static_cast<std::uint32_t>(in[size - 4U]) |
      (static_cast<std::uint32_t>(in[size - 3U]) << 8U) |
      (static_cast<std::uint32_t>(in[size - 2U]) << 16U) |
      (static_cast<std::uint32_t>(in[size - 1U]) << 24U);
  if (crc32Ieee(in, size - 4U) != crc_expected) {
    return CodecStatus::kCrcMismatch;
  }
  for (std::size_t i = 0U; i < s.entry_count; ++i) {
    SnapshotEntry& e = s.entries[i];
    K1_CV2_TRY(readTarget(r, e.target));
    e.target_revision = r.u32();
    K1_CV2_TRY(readTyped(r, e.base));
    e.flags = r.u8();
    const std::uint8_t inactive = r.u8();
    if (inactive >= kInactiveReasonCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    e.inactive_reason = static_cast<InactiveReason>(inactive);
    if (!r.zeros(2U)) {
      return CodecStatus::kReservedNonZero;
    }
  }
  return validate(s);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   StateSnapshot& out) noexcept {
  const CodecStatus status = decodeSnapshot(in, size, out);
  if (status != CodecStatus::kOk) {
    out = StateSnapshot{};
  }
  return status;
}

// ===========================================================================
// StateDeltaV2
// ===========================================================================

namespace {
CodecStatus validEntries(const SnapshotEntry* entries, const std::size_t count,
                         const std::uint32_t max_revision) noexcept {
  for (std::size_t i = 0U; i < count; ++i) {
    const SnapshotEntry& e = entries[i];
    K1_CV2_TRY(validTarget(e.target));
    if (i > 0U && !targetLess(entries[i - 1U].target, e.target)) {
      return entries[i - 1U].target == e.target ? CodecStatus::kDuplicate : CodecStatus::kOrdering;
    }
    if (e.target_revision > max_revision) {
      return CodecStatus::kInconsistent;
    }
    K1_CV2_TRY(validTypedValue(e.base));
    if (e.base.kind == ValueKind::kNone) {
      return CodecStatus::kInvalidValue;
    }
    K1_CV2_TRY(validStateFlags(e.flags, e.inactive_reason));
  }
  return CodecStatus::kOk;
}

bool sameEntry(const SnapshotEntry& a, const SnapshotEntry& b) noexcept {
  return a.target == b.target && a.target_revision == b.target_revision && a.base == b.base &&
         a.flags == b.flags && a.inactive_reason == b.inactive_reason;
}
}  // namespace

CodecStatus validate(const StateDelta& d) noexcept {
  if (d.engine_epoch == 0U || d.engine_time_us > kMaxTimeUs) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if (d.commit_revision <= d.base_revision) {
    return CodecStatus::kInconsistent;
  }
  if (d.entry_count > kMaxDeltaEntries) {
    return CodecStatus::kCountExceedsCapacity;
  }
  return validEntries(d.entries, d.entry_count, d.commit_revision);
}

std::size_t encodedSize(const StateDelta& d) noexcept {
  return kStateDeltaFixedBytes + kSnapshotEntryBytes * static_cast<std::size_t>(d.entry_count) + 4U;
}

EncodeResult encode(const StateDelta& d, std::uint8_t* out, const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(d);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  const std::size_t size = encodedSize(d);
  if (capacity < size) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kStateDelta, size);
  w.u32(d.engine_epoch);
  w.u32(d.base_revision);
  w.u32(d.commit_revision);
  w.u32(d.profile_revision);
  w.u64(d.engine_time_us);
  w.u16(d.entry_count);
  w.u16(0U);
  w.u32(0U);
  for (std::size_t i = 0U; i < d.entry_count; ++i) {
    const SnapshotEntry& e = d.entries[i];
    w.target(e.target);
    w.u32(e.target_revision);
    w.typed(e.base);
    w.u8(e.flags);
    w.u8(static_cast<std::uint8_t>(e.inactive_reason));
    w.u16(0U);
  }
  w.u32(crc32Ieee(out, w.position()));
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeDelta(const std::uint8_t* in, const std::size_t size, StateDelta& d) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kStateDelta, size));
  if (size < kStateDeltaFixedBytes + 4U) {
    return CodecStatus::kTruncated;
  }
  d.engine_epoch = r.u32();
  d.base_revision = r.u32();
  d.commit_revision = r.u32();
  d.profile_revision = r.u32();
  d.engine_time_us = r.u64();
  d.entry_count = r.u16();
  if (!r.zeros(6U)) {
    return CodecStatus::kReservedNonZero;
  }
  if (d.entry_count > kMaxDeltaEntries) {
    return CodecStatus::kCountExceedsCapacity;
  }
  if (size != encodedSize(d)) {
    return CodecStatus::kCountMismatch;
  }
  const std::uint32_t crc_expected =
      static_cast<std::uint32_t>(in[size - 4U]) |
      (static_cast<std::uint32_t>(in[size - 3U]) << 8U) |
      (static_cast<std::uint32_t>(in[size - 2U]) << 16U) |
      (static_cast<std::uint32_t>(in[size - 1U]) << 24U);
  if (crc32Ieee(in, size - 4U) != crc_expected) {
    return CodecStatus::kCrcMismatch;
  }
  for (std::size_t i = 0U; i < d.entry_count; ++i) {
    SnapshotEntry& e = d.entries[i];
    K1_CV2_TRY(readTarget(r, e.target));
    e.target_revision = r.u32();
    K1_CV2_TRY(readTyped(r, e.base));
    e.flags = r.u8();
    const std::uint8_t inactive = r.u8();
    if (inactive >= kInactiveReasonCount) {
      return CodecStatus::kEnumOutOfRange;
    }
    e.inactive_reason = static_cast<InactiveReason>(inactive);
    if (!r.zeros(2U)) {
      return CodecStatus::kReservedNonZero;
    }
  }
  return validate(d);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size, StateDelta& out) noexcept {
  const CodecStatus status = decodeDelta(in, size, out);
  if (status != CodecStatus::kOk) {
    out = StateDelta{};
  }
  return status;
}

DeltaBuild buildStateDelta(const StateSnapshot& from, const StateSnapshot& to, StateDelta& out) noexcept {
  if (validate(from) != CodecStatus::kOk || validate(to) != CodecStatus::kOk ||
      from.entry_count != to.entry_count) {
    return DeltaBuild::kInvalid;
  }
  for (std::size_t i = 0U; i < to.entry_count; ++i) {
    if (from.entries[i].target != to.entries[i].target) {
      return DeltaBuild::kInvalid;
    }
  }
  if (from.engine_epoch != to.engine_epoch || from.profile_revision != to.profile_revision ||
      std::memcmp(from.profile_sha256, to.profile_sha256, kDigestBytes) != 0 ||
      std::memcmp(from.registry_sha256, to.registry_sha256, kDigestBytes) != 0) {
    return DeltaBuild::kNeedSnapshot;
  }
  if (to.snapshot_revision <= from.snapshot_revision) {
    return DeltaBuild::kInvalid;
  }
  std::size_t changed = 0U;
  for (std::size_t i = 0U; i < to.entry_count; ++i) {
    changed += sameEntry(from.entries[i], to.entries[i]) ? 0U : 1U;
  }
  if (changed > kMaxDeltaEntries) {
    return DeltaBuild::kNeedSnapshot;
  }
  StateDelta d{};
  d.engine_epoch = to.engine_epoch;
  d.base_revision = from.snapshot_revision;
  d.commit_revision = to.snapshot_revision;
  d.profile_revision = to.profile_revision;
  d.engine_time_us = to.engine_time_us;
  for (std::size_t i = 0U; i < to.entry_count; ++i) {
    if (!sameEntry(from.entries[i], to.entries[i])) {
      d.entries[d.entry_count++] = to.entries[i];
    }
  }
  out = d;
  return DeltaBuild::kBuilt;
}

DeltaApply applyStateDelta(StateSnapshot& receiver, const StateDelta& delta) noexcept {
  if (validate(delta) != CodecStatus::kOk) {
    return DeltaApply::kInvalid;
  }
  if (validate(receiver) != CodecStatus::kOk || receiver.engine_epoch != delta.engine_epoch ||
      receiver.snapshot_revision != delta.base_revision ||
      receiver.profile_revision != delta.profile_revision) {
    return DeltaApply::kNeedSnapshot;
  }
  // Locate every entry first (both lists ascend), then commit.
  std::uint8_t slot[kMaxDeltaEntries]{};
  std::size_t j = 0U;
  for (std::size_t i = 0U; i < delta.entry_count; ++i) {
    while (j < receiver.entry_count && targetLess(receiver.entries[j].target, delta.entries[i].target)) {
      ++j;
    }
    if (j == receiver.entry_count || receiver.entries[j].target != delta.entries[i].target) {
      return DeltaApply::kNeedSnapshot;
    }
    slot[i] = static_cast<std::uint8_t>(j);
  }
  for (std::size_t i = 0U; i < delta.entry_count; ++i) {
    receiver.entries[slot[i]] = delta.entries[i];
  }
  receiver.snapshot_revision = delta.commit_revision;
  receiver.engine_time_us = delta.engine_time_us;
  return DeltaApply::kApplied;
}

// ===========================================================================
// EffectiveObservation
// ===========================================================================

CodecStatus validate(const EffectiveObservation& o) noexcept {
  K1_CV2_TRY(validTarget(o.target));
  if (o.engine_epoch == 0U || o.observed_time_us > kMaxTimeUs) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if (o.time_domain != TimeDomain::kEngineMonotonicUs ||
      static_cast<std::uint8_t>(o.availability) >= kAvailabilityCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  if ((o.modulation_sources & static_cast<std::uint16_t>(~kModulationDefined)) != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  K1_CV2_TRY(validTypedValue(o.effective));
  K1_CV2_TRY(validTypedValue(o.material));
  const bool present = o.availability == Availability::kAvailable ||
                       o.availability == Availability::kStale;
  if (present ? o.effective.kind == ValueKind::kNone
              : (o.effective.kind != ValueKind::kNone ||
                 o.material.kind != ValueKind::kNone ||
                 o.modulation_sources != 0U)) {
    return CodecStatus::kInconsistent;
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const EffectiveObservation&) noexcept {
  return kEffectiveObservationBytes;
}

EncodeResult encode(const EffectiveObservation& o, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(o);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  if (capacity < kEffectiveObservationBytes) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kEffectiveObservation, kEffectiveObservationBytes);
  w.target(o.target);
  w.u32(o.engine_epoch);
  w.u32(o.base_revision);
  w.typed(o.effective);
  w.u64(o.observed_time_us);
  w.u32(o.valid_for_us);
  w.u8(static_cast<std::uint8_t>(o.time_domain));
  w.u8(static_cast<std::uint8_t>(o.availability));
  w.u16(o.modulation_sources);
  w.typed(o.material);
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeObservation(const std::uint8_t* in, const std::size_t size,
                              EffectiveObservation& o) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kEffectiveObservation, size));
  if (size != kEffectiveObservationBytes) {
    return size < kEffectiveObservationBytes ? CodecStatus::kTruncated
                                             : CodecStatus::kLengthMismatch;
  }
  K1_CV2_TRY(readTarget(r, o.target));
  o.engine_epoch = r.u32();
  o.base_revision = r.u32();
  K1_CV2_TRY(readTyped(r, o.effective));
  o.observed_time_us = r.u64();
  o.valid_for_us = r.u32();
  const std::uint8_t domain = r.u8();
  const std::uint8_t availability = r.u8();
  if (domain != static_cast<std::uint8_t>(TimeDomain::kEngineMonotonicUs) ||
      availability >= kAvailabilityCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  o.time_domain = TimeDomain::kEngineMonotonicUs;
  o.availability = static_cast<Availability>(availability);
  o.modulation_sources = r.u16();
  K1_CV2_TRY(readTyped(r, o.material));
  return validate(o);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   EffectiveObservation& out) noexcept {
  const CodecStatus status = decodeObservation(in, size, out);
  if (status != CodecStatus::kOk) {
    out = EffectiveObservation{};
  }
  return status;
}


// ===========================================================================
// ProfileIdentity
// ===========================================================================

CodecStatus validate(const ProfileIdentity& p) noexcept {
  if (p.profile_id == 0U || p.profile_revision == 0U) {
    return CodecStatus::kIntegerOutOfRange;
  }
  if (allZero(p.profile_sha256, kDigestBytes)) {
    return CodecStatus::kInvalidValue;
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const ProfileIdentity&) noexcept { return kProfileIdentityBytes; }

EncodeResult encode(const ProfileIdentity& p, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(p);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  if (capacity < kProfileIdentityBytes) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.header(ObjectKind::kProfileIdentity, kProfileIdentityBytes);
  w.u32(p.profile_id);
  w.u32(p.profile_revision);
  w.bytes(p.profile_sha256, kDigestBytes);
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeIdentity(const std::uint8_t* in, const std::size_t size,
                           ProfileIdentity& p) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  Reader r(in, size);
  K1_CV2_TRY(readHeader(r, ObjectKind::kProfileIdentity, size));
  if (size != kProfileIdentityBytes) {
    return size < kProfileIdentityBytes ? CodecStatus::kTruncated
                                        : CodecStatus::kLengthMismatch;
  }
  p.profile_id = r.u32();
  p.profile_revision = r.u32();
  r.bytes(p.profile_sha256, kDigestBytes);
  return validate(p);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   ProfileIdentity& out) noexcept {
  const CodecStatus status = decodeIdentity(in, size, out);
  if (status != CodecStatus::kOk) {
    out = ProfileIdentity{};
  }
  return status;
}

// ===========================================================================
// Target keys and compact lane forms
// ===========================================================================

CodecStatus decodeTargetKey(const std::uint16_t key, TargetRef& out) noexcept {
  if ((key & 0xC000U) != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  const auto channel = static_cast<std::uint8_t>((key >> 12U) & 0x03U);
  if (channel >= kChannelEnumCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  const TargetRef candidate{static_cast<std::uint16_t>(key & 0x0FFFU),
                            static_cast<Channel>(channel)};
  const CodecStatus status = validTarget(candidate);
  if (status == CodecStatus::kOk) {
    out = candidate;
  }
  return status;
}

namespace {
CodecStatus validTargetKey(const std::uint16_t key) noexcept {
  TargetRef ignored{};
  return decodeTargetKey(key, ignored);
}
}  // namespace

CodecStatus validate(const ContinuousSampleV2& c) noexcept {
  if (static_cast<std::uint8_t>(c.intent_kind) > 1U) {
    return CodecStatus::kEnumOutOfRange;
  }
  if (c.flags != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  if (c.page_id == 0U || c.slot == 0U || c.slot > kSlotsPerPage ||
      c.response_policy_id >= kMaxResponsePolicies || c.profile_revision == 0U) {
    return CodecStatus::kIntegerOutOfRange;
  }
  K1_CV2_TRY(validTypedValue(c.value));
  if (c.value.kind != ValueKind::kReal && c.value.kind != ValueKind::kInteger) {
    return CodecStatus::kInconsistent;
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const ContinuousSampleV2&) noexcept { return kContinuousSampleBytes; }

EncodeResult encode(const ContinuousSampleV2& c, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(c);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  if (capacity < kContinuousSampleBytes) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.u8(static_cast<std::uint8_t>(CompactTag::kContinuousSample));
  w.u8(static_cast<std::uint8_t>(c.intent_kind));
  w.u8(c.flags);
  w.u8(c.page_id);
  w.u8(c.slot);
  w.u8(c.response_policy_id);
  w.u32(c.profile_revision);
  w.typed(c.value);
  w.u32(c.expected_target_revision);
  w.u32(c.observed_time_us_lo32);
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeSample(const std::uint8_t* in, const std::size_t size,
                         ContinuousSampleV2& c) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  if (size < 1U) {
    return CodecStatus::kTruncated;
  }
  if (in[0] != static_cast<std::uint8_t>(CompactTag::kContinuousSample)) {
    return CodecStatus::kWrongKind;
  }
  if (size != kContinuousSampleBytes) {
    return size < kContinuousSampleBytes ? CodecStatus::kTruncated
                                         : CodecStatus::kLengthMismatch;
  }
  Reader r(in, size);
  (void)r.u8();
  const std::uint8_t kind = r.u8();
  if (kind > 1U) {
    return CodecStatus::kEnumOutOfRange;
  }
  c.intent_kind = static_cast<IntentKind>(kind);
  c.flags = r.u8();
  c.page_id = r.u8();
  c.slot = r.u8();
  c.response_policy_id = r.u8();
  c.profile_revision = r.u32();
  K1_CV2_TRY(readTyped(r, c.value));
  c.expected_target_revision = r.u32();
  c.observed_time_us_lo32 = r.u32();
  return validate(c);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   ContinuousSampleV2& out) noexcept {
  const CodecStatus status = decodeSample(in, size, out);
  if (status != CodecStatus::kOk) {
    out = ContinuousSampleV2{};
  }
  return status;
}

CodecStatus validate(const ReceiptSummaryV2& a) noexcept {
  if (static_cast<std::uint8_t>(a.outcome) >= kOutcomeCount ||
      static_cast<std::uint8_t>(a.reason) >= kReasonCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  if (a.transaction_id == 0U || a.engine_epoch == 0U || a.op_count == 0U ||
      a.op_count > kMaxTransactionOps) {
    return a.op_count > kMaxTransactionOps ? CodecStatus::kCountExceedsCapacity
                                           : CodecStatus::kIntegerOutOfRange;
  }
  const bool accepted = a.outcome == Outcome::kAccepted;
  if (accepted != (a.reason == Reason::kNone) ||
      (accepted && a.failing_op_index != 0xFFU) ||
      a.target_revision > a.commit_revision) {
    return CodecStatus::kInconsistent;
  }
  if (a.failing_op_index != 0xFFU && a.failing_op_index >= a.op_count) {
    return CodecStatus::kIntegerOutOfRange;
  }
  return validTargetKey(a.target_key);
}

std::size_t encodedSize(const ReceiptSummaryV2&) noexcept { return kReceiptSummaryBytes; }

EncodeResult encode(const ReceiptSummaryV2& a, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(a);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  if (capacity < kReceiptSummaryBytes) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.u8(static_cast<std::uint8_t>(CompactTag::kReceiptSummary));
  w.u8(static_cast<std::uint8_t>(a.outcome));
  w.u8(static_cast<std::uint8_t>(a.reason));
  w.u8(a.failing_op_index);
  w.u32(a.transaction_id);
  w.u16(a.target_key);
  w.u8(a.op_count);
  w.u8(0U);
  w.u32(a.engine_epoch);
  w.u32(a.commit_revision);
  w.u32(a.target_revision);
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeSummary(const std::uint8_t* in, const std::size_t size,
                          ReceiptSummaryV2& a) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  if (size < 1U) {
    return CodecStatus::kTruncated;
  }
  if (in[0] != static_cast<std::uint8_t>(CompactTag::kReceiptSummary)) {
    return CodecStatus::kWrongKind;
  }
  if (size != kReceiptSummaryBytes) {
    return size < kReceiptSummaryBytes ? CodecStatus::kTruncated
                                       : CodecStatus::kLengthMismatch;
  }
  Reader r(in, size);
  (void)r.u8();
  const std::uint8_t outcome = r.u8();
  const std::uint8_t reason = r.u8();
  if (outcome >= kOutcomeCount || reason >= kReasonCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  a.outcome = static_cast<Outcome>(outcome);
  a.reason = static_cast<Reason>(reason);
  a.failing_op_index = r.u8();
  a.transaction_id = r.u32();
  a.target_key = r.u16();
  a.op_count = r.u8();
  if (!r.zeros(1U)) {
    return CodecStatus::kReservedNonZero;
  }
  a.engine_epoch = r.u32();
  a.commit_revision = r.u32();
  a.target_revision = r.u32();
  return validate(a);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   ReceiptSummaryV2& out) noexcept {
  const CodecStatus status = decodeSummary(in, size, out);
  if (status != CodecStatus::kOk) {
    out = ReceiptSummaryV2{};
  }
  return status;
}

CodecStatus validate(const EffectiveObservationCompactV2& o) noexcept {
  if (static_cast<std::uint8_t>(o.availability) >= kAvailabilityCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  if ((o.modulation_sources & static_cast<std::uint16_t>(~kModulationDefined)) != 0U) {
    return CodecStatus::kReservedNonZero;
  }
  if (o.engine_epoch == 0U) {
    return CodecStatus::kIntegerOutOfRange;
  }
  K1_CV2_TRY(validTargetKey(o.target_key));
  K1_CV2_TRY(validTypedValue(o.effective));
  const bool present = o.availability == Availability::kAvailable ||
                       o.availability == Availability::kStale;
  if (present ? o.effective.kind == ValueKind::kNone
              : (o.effective.kind != ValueKind::kNone || o.modulation_sources != 0U)) {
    return CodecStatus::kInconsistent;
  }
  return CodecStatus::kOk;
}

std::size_t encodedSize(const EffectiveObservationCompactV2&) noexcept {
  return kEffectiveObservationCompactBytes;
}

EncodeResult encode(const EffectiveObservationCompactV2& o, std::uint8_t* out,
                    const std::size_t capacity) noexcept {
  if (out == nullptr) {
    return {CodecStatus::kNullBuffer, 0U};
  }
  const CodecStatus status = validate(o);
  if (status != CodecStatus::kOk) {
    return {status, 0U};
  }
  if (capacity < kEffectiveObservationCompactBytes) {
    return {CodecStatus::kBufferTooSmall, 0U};
  }
  Writer w(out);
  w.u8(static_cast<std::uint8_t>(CompactTag::kEffectiveObservationCompact));
  w.u8(static_cast<std::uint8_t>(o.availability));
  w.u16(o.target_key);
  w.typed(o.effective);
  w.u32(o.base_revision);
  w.u32(o.engine_epoch);
  w.u32(o.observed_time_us_lo32);
  w.u32(o.valid_for_us);
  w.u16(o.modulation_sources);
  w.u16(0U);
  return {CodecStatus::kOk, w.position()};
}

namespace {
CodecStatus decodeCompactObservation(const std::uint8_t* in, const std::size_t size,
                                     EffectiveObservationCompactV2& o) noexcept {
  if (in == nullptr) {
    return CodecStatus::kNullBuffer;
  }
  if (size < 1U) {
    return CodecStatus::kTruncated;
  }
  if (in[0] != static_cast<std::uint8_t>(CompactTag::kEffectiveObservationCompact)) {
    return CodecStatus::kWrongKind;
  }
  if (size != kEffectiveObservationCompactBytes) {
    return size < kEffectiveObservationCompactBytes ? CodecStatus::kTruncated
                                                    : CodecStatus::kLengthMismatch;
  }
  Reader r(in, size);
  (void)r.u8();
  const std::uint8_t availability = r.u8();
  if (availability >= kAvailabilityCount) {
    return CodecStatus::kEnumOutOfRange;
  }
  o.availability = static_cast<Availability>(availability);
  o.target_key = r.u16();
  K1_CV2_TRY(readTyped(r, o.effective));
  o.base_revision = r.u32();
  o.engine_epoch = r.u32();
  o.observed_time_us_lo32 = r.u32();
  o.valid_for_us = r.u32();
  o.modulation_sources = r.u16();
  if (!r.zeros(2U)) {
    return CodecStatus::kReservedNonZero;
  }
  return validate(o);
}
}  // namespace

CodecStatus decode(const std::uint8_t* in, const std::size_t size,
                   EffectiveObservationCompactV2& out) noexcept {
  const CodecStatus status = decodeCompactObservation(in, size, out);
  if (status != CodecStatus::kOk) {
    out = EffectiveObservationCompactV2{};
  }
  return status;
}

// ---------------------------------------------------------------------------
// Projections
// ---------------------------------------------------------------------------

ContinuousSampleV2 toContinuousSample(const GestureIntent& g) noexcept {
  ContinuousSampleV2 c{};
  c.intent_kind = g.intent_kind;
  c.flags = 0U;
  c.page_id = g.page_id;
  c.slot = g.slot;
  c.response_policy_id = g.response_policy_id;
  c.profile_revision = g.profile_revision;
  c.value = g.value;
  c.expected_target_revision = g.expected_target_revision;
  c.observed_time_us_lo32 = static_cast<std::uint32_t>(g.observed_time_us & 0xFFFFFFFFULL);
  return c;
}

CodecStatus toGestureIntent(const ContinuousSampleV2& c,
                            const ContinuousLaneContext& lane,
                            GestureIntent& out) noexcept {
  K1_CV2_TRY(validate(c));
  GestureIntent g{};
  TargetRef target{};
  K1_CV2_TRY(decodeTargetKey(lane.target_key, target));
  g.writer_id = lane.writer_id;
  g.intent_kind = c.intent_kind;
  g.flags = lane.terminal ? kIntentFlagTerminal : 0U;
  g.session_epoch = lane.session_epoch;
  g.engine_epoch = lane.engine_epoch;
  g.gesture_id = lane.gesture_id;
  g.sequence = lane.sequence;
  g.profile_revision = c.profile_revision;
  g.binding_revision = lane.binding_revision;
  g.target = target;
  g.page_id = c.page_id;
  g.slot = c.slot;
  g.response_policy_id = c.response_policy_id;
  g.value = c.value;
  g.observed_time_us = c.observed_time_us_lo32;
  g.expected_target_revision = c.expected_target_revision;
  g.validity_ms = lane.lifetime_ms;
  K1_CV2_TRY(validate(g));
  out = g;
  return CodecStatus::kOk;
}

ReceiptSummaryV2 toReceiptSummary(const ApplyReceipt& a) noexcept {
  ReceiptSummaryV2 s{};
  s.outcome = a.outcome;
  s.reason = a.reason;
  s.failing_op_index = a.failing_op_index;
  s.transaction_id = a.transaction_id;
  s.op_count = a.op_count;
  s.engine_epoch = a.engine_epoch;
  s.commit_revision = a.commit_revision;
  if (a.op_count > 0U) {
    s.target_key = targetKey(a.results[0].target);
    s.target_revision = a.results[0].target_revision;
  }
  return s;
}

EffectiveObservationCompactV2 toCompactObservation(const EffectiveObservation& o) noexcept {
  EffectiveObservationCompactV2 c{};
  c.availability = o.availability;
  c.target_key = targetKey(o.target);
  c.effective = o.effective;
  c.base_revision = o.base_revision;
  c.engine_epoch = o.engine_epoch;
  c.observed_time_us_lo32 = static_cast<std::uint32_t>(o.observed_time_us & 0xFFFFFFFFULL);
  c.valid_for_us = o.valid_for_us;
  c.modulation_sources = o.modulation_sources;
  return c;
}

#undef K1_CV2_TRY

}  // namespace k1::control_v2
