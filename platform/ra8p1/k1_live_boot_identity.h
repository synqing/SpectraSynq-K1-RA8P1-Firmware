#pragma once

#include <cstddef>
#include <cstdint>

#include "k1_crc32.h"

namespace k1::titan {

inline constexpr std::uint32_t kLiveBootJournalMagic = 0x4B314247u; /* K1BG */
inline constexpr std::size_t kLiveBootJournalBytes = 24U;

struct LiveBootStore final {
  bool (*read)(void* ctx, std::uint8_t* dest, std::size_t n) = nullptr;
  bool (*write)(void* ctx, const std::uint8_t* src, std::size_t n) = nullptr;
  void* ctx = nullptr;
};

/* Monotonic boot generation. Survives warm/software/debug reset while the
   backing store retains the journal. A wiped store starts at 1. Generation
   wrap to 0 fails closed. No timer or UID mix. */
bool liveIssueBootIdentity(LiveBootStore* store, std::uint32_t* out_id) noexcept;
LiveBootStore liveRamNoinitBootStore() noexcept;

}  // namespace k1::titan
