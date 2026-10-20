#include "k1_live_boot_identity.h"

#include <cstring>

namespace k1::titan {
namespace {

void put_u32(std::uint8_t* p, std::uint32_t v) noexcept {
  p[0] = static_cast<std::uint8_t>(v);
  p[1] = static_cast<std::uint8_t>(v >> 8U);
  p[2] = static_cast<std::uint8_t>(v >> 16U);
  p[3] = static_cast<std::uint8_t>(v >> 24U);
}

std::uint32_t get_u32(const std::uint8_t* p) noexcept {
  return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8U) |
         (static_cast<std::uint32_t>(p[2]) << 16U) | (static_cast<std::uint32_t>(p[3]) << 24U);
}

std::uint32_t slot_crc(std::uint32_t generation) noexcept {
  std::uint8_t bytes[4];
  put_u32(bytes, generation);
  return k1_crc32_ieee(bytes, 4U);
}

bool slot_ok(const std::uint8_t* journal, unsigned slot) noexcept {
  const std::uint32_t generation = get_u32(journal + 8U + slot * 8U);
  const std::uint32_t crc = get_u32(journal + 12U + slot * 8U);
  return generation != 0U && crc == slot_crc(generation);
}

#if defined(K1_RA8P1_TARGET)
std::uint8_t live_boot_journal_bytes[kLiveBootJournalBytes]
    __attribute__((section(".noinit"), used, aligned(4)));
#else
std::uint8_t live_boot_journal_bytes[kLiveBootJournalBytes];
#endif

bool ram_read(void* ctx, std::uint8_t* dest, std::size_t n) noexcept {
  if (ctx == nullptr || dest == nullptr || n != kLiveBootJournalBytes) {
    return false;
  }
  std::memcpy(dest, ctx, n);
  return true;
}

bool ram_write(void* ctx, const std::uint8_t* src, std::size_t n) noexcept {
  if (ctx == nullptr || src == nullptr || n != kLiveBootJournalBytes) {
    return false;
  }
  std::memcpy(ctx, src, n);
  return true;
}

}  // namespace

bool liveIssueBootIdentity(LiveBootStore* store, std::uint32_t* out_id) noexcept {
  if (out_id != nullptr) {
    *out_id = 0;
  }
  if (store == nullptr || store->read == nullptr || store->write == nullptr || out_id == nullptr) {
    return false;
  }
  std::uint8_t journal[kLiveBootJournalBytes]{};
  if (!store->read(store->ctx, journal, kLiveBootJournalBytes)) {
    return false;
  }
  std::uint32_t current = 0;
  unsigned active = 0;
  if (get_u32(journal) == kLiveBootJournalMagic) {
    active = get_u32(journal + 4) & 1U;
    if (slot_ok(journal, active)) {
      current = get_u32(journal + 8U + active * 8U);
    } else if (slot_ok(journal, active ^ 1U)) {
      active ^= 1U;
      current = get_u32(journal + 8U + active * 8U);
    }
  }
  const std::uint32_t next = current + 1U;
  if (next == 0U) {
    return false;
  }
  const unsigned inactive = active ^ 1U;
  std::uint8_t next_journal[kLiveBootJournalBytes]{};
  std::memcpy(next_journal, journal, kLiveBootJournalBytes);
  put_u32(next_journal, kLiveBootJournalMagic);
  put_u32(next_journal + 8U + inactive * 8U, next);
  put_u32(next_journal + 12U + inactive * 8U, slot_crc(next));
  put_u32(next_journal + 4, inactive);
  if (!store->write(store->ctx, next_journal, kLiveBootJournalBytes)) {
    return false;
  }
  std::uint8_t verify[kLiveBootJournalBytes]{};
  if (!store->read(store->ctx, verify, kLiveBootJournalBytes)) {
    return false;
  }
  if (get_u32(verify) != kLiveBootJournalMagic || (get_u32(verify + 4) & 1U) != inactive) {
    return false;
  }
  if (!slot_ok(verify, inactive) || get_u32(verify + 8U + inactive * 8U) != next) {
    return false;
  }
  *out_id = next;
  return true;
}

LiveBootStore liveRamNoinitBootStore() noexcept {
  return LiveBootStore{ram_read, ram_write, live_boot_journal_bytes};
}

}  // namespace k1::titan
