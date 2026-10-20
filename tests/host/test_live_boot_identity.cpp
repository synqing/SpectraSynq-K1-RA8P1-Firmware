#include "k1_crc32.h"
#include "k1_live_boot_identity.h"
#include "k1_live_protocol.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

void put_u32(std::uint8_t* p, std::uint32_t v) {
  p[0] = static_cast<std::uint8_t>(v);
  p[1] = static_cast<std::uint8_t>(v >> 8U);
  p[2] = static_cast<std::uint8_t>(v >> 16U);
  p[3] = static_cast<std::uint8_t>(v >> 24U);
}

std::uint32_t r4_uid_cyccnt_id(std::uint32_t uid_mix, std::uint32_t cyccnt) {
  const std::uint32_t id = (uid_mix ^ cyccnt) & 0x00FFFFFFu;
  return id == 0U ? 1U : id;
}

struct MemoryStore {
  std::uint8_t bytes[k1::titan::kLiveBootJournalBytes]{};
  bool read_ok = true;
  bool write_ok = true;
  int writes = 0;
  int fail_on_write = -1;
  int reads = 0;
  int fail_on_read = -1;
};

bool mem_read(void* ctx, std::uint8_t* dest, std::size_t n) {
  auto* store = static_cast<MemoryStore*>(ctx);
  store->reads += 1;
  if (!store->read_ok || (store->fail_on_read >= 0 && store->reads == store->fail_on_read)) {
    return false;
  }
  if (dest == nullptr || n != k1::titan::kLiveBootJournalBytes) {
    return false;
  }
  std::memcpy(dest, store->bytes, n);
  return true;
}

bool mem_write(void* ctx, const std::uint8_t* src, std::size_t n) {
  auto* store = static_cast<MemoryStore*>(ctx);
  store->writes += 1;
  if (!store->write_ok || (store->fail_on_write >= 0 && store->writes == store->fail_on_write)) {
    return false;
  }
  if (src == nullptr || n != k1::titan::kLiveBootJournalBytes) {
    return false;
  }
  std::memcpy(store->bytes, src, n);
  return true;
}

k1::titan::LiveBootStore bind(MemoryStore* store) {
  return k1::titan::LiveBootStore{mem_read, mem_write, store};
}

void seed_generation(MemoryStore* store, std::uint32_t generation) {
  std::memset(store->bytes, 0, sizeof(store->bytes));
  put_u32(store->bytes, k1::titan::kLiveBootJournalMagic);
  put_u32(store->bytes + 4, 0U);
  put_u32(store->bytes + 8, generation);
  std::uint8_t gen_bytes[4];
  put_u32(gen_bytes, generation);
  put_u32(store->bytes + 12, k1_crc32_ieee(gen_bytes, 4U));
}

}  // namespace

int main() {
  assert(r4_uid_cyccnt_id(0U, 0x12345678U) == r4_uid_cyccnt_id(0U, 0x13345678U));
  assert(r4_uid_cyccnt_id(0U, 0x12345678U) == (0x12345678U & 0x00FFFFFFu));

  MemoryStore retained{};
  auto store = bind(&retained);
  std::uint32_t first = 0;
  std::uint32_t second = 0;
  std::uint32_t third = 0;
  assert(k1::titan::liveIssueBootIdentity(&store, &first));
  assert(k1::titan::liveIssueBootIdentity(&store, &second));
  assert(k1::titan::liveIssueBootIdentity(&store, &third));
  assert(first == 1U);
  assert(second == 2U);
  assert(third == 3U);
  assert(first != second && second != third);

  MemoryStore alias{};
  seed_generation(&alias, 0x00FFFFFFU);
  auto alias_store = bind(&alias);
  std::uint32_t crossed = 0;
  assert(k1::titan::liveIssueBootIdentity(&alias_store, &crossed));
  assert(crossed == 0x01000000U);
  assert((crossed & 0x00FFFFFFu) == 0U);

  MemoryStore wrap{};
  seed_generation(&wrap, 0xFFFFFFFFU);
  auto wrap_store = bind(&wrap);
  std::uint32_t wrapped = 99;
  assert(!k1::titan::liveIssueBootIdentity(&wrap_store, &wrapped));
  assert(wrapped == 0U);

  MemoryStore fail_write{};
  fail_write.write_ok = false;
  auto fail_store = bind(&fail_write);
  std::uint32_t failed = 7;
  assert(!k1::titan::liveIssueBootIdentity(&fail_store, &failed));
  assert(failed == 0U);

  MemoryStore fail_read{};
  fail_read.read_ok = false;
  auto fail_read_store = bind(&fail_read);
  std::uint32_t unread = 7;
  assert(!k1::titan::liveIssueBootIdentity(&fail_read_store, &unread));
  assert(unread == 0U);

  MemoryStore verify_fail{};
  verify_fail.fail_on_read = 2;
  auto verify_store = bind(&verify_fail);
  std::uint32_t unverified = 7;
  assert(!k1::titan::liveIssueBootIdentity(&verify_store, &unverified));
  assert(unverified == 0U);

  auto ram = k1::titan::liveRamNoinitBootStore();
  std::memset(ram.ctx, 0, k1::titan::kLiveBootJournalBytes);
  std::uint32_t ram_a = 0;
  std::uint32_t ram_b = 0;
  assert(k1::titan::liveIssueBootIdentity(&ram, &ram_a));
  assert(k1::titan::liveIssueBootIdentity(&ram, &ram_b));
  assert(ram_a == 1U);
  assert(ram_b == 2U);

  k1::titan::LiveConfigBlob cfg{};
  cfg.version = 2;
  cfg.revision = 0;
  cfg.boot_id = 2;
  cfg.visual_a.vp_waveform_shift_rate = 120.0F;
  k1::titan::LivePatchEntry shift{};
  shift.field_id = k1::titan::kPatchVisShiftRate;
  shift.kind = k1::titan::kLivePatchKindF32;
  shift.value = 240.0F;
  assert(k1::titan::liveApplyPatch(&cfg, 0, 1, 0, &shift, 1) ==
         k1::titan::LivePatchStatus::WrongBoot);
  assert(cfg.revision == 0);
  assert(cfg.visual_a.vp_waveform_shift_rate == 120.0F);
  assert(k1::titan::liveApplyPatch(&cfg, 0, 2, 0, &shift, 1) == k1::titan::LivePatchStatus::Ok);
  assert(cfg.revision == 1);
  assert(cfg.visual_a.vp_waveform_shift_rate == 240.0F);
  const float kept = cfg.visual_a.vp_waveform_shift_rate;
  assert(k1::titan::liveApplyPatch(&cfg, 0, 2, 0, &shift, 1) == k1::titan::LivePatchStatus::Stale);
  assert(cfg.revision == 1);
  assert(cfg.visual_a.vp_waveform_shift_rate == kept);
  shift.value = 40.0F;
  assert(k1::titan::liveApplyPatch(&cfg, 1, 2, 0, &shift, 1) == k1::titan::LivePatchStatus::Ok);
  assert(cfg.visual_a.vp_waveform_shift_rate == 40.0F);

  k1::titan::LiveConfigBlob denied{};
  denied.version = 2;
  denied.revision = 0;
  denied.boot_id = 0;
  denied.visual_a.vp_waveform_shift_rate = 120.0F;
  shift.value = 240.0F;
  assert(k1::titan::liveApplyPatch(&denied, 0, 0, 0, &shift, 1) ==
         k1::titan::LivePatchStatus::WrongBoot);
  assert(denied.visual_a.vp_waveform_shift_rate == 120.0F);

  std::uint8_t blob[k1::titan::kLiveConfigBlobBytesV2];
  cfg.boot_id = 0x01000000U;
  assert(k1::titan::liveEncodeConfig(blob, sizeof(blob), cfg) == k1::titan::kLiveConfigBlobBytesV2);
  assert(k1::titan::liveGetU32(blob + k1::titan::kLiveConfigBootIdOffset) == 0x01000000U);
  k1::titan::LiveConfigBlob decoded{};
  assert(k1::titan::liveDecodeConfig(blob, k1::titan::kLiveConfigBlobBytesV2, &decoded));
  assert(decoded.boot_id == 0x01000000U);
  k1::titan::LiveConfigBlob legacy{};
  assert(k1::titan::liveDecodeConfig(blob, k1::titan::kLiveConfigBlobBytesV2Legacy, &legacy));
  assert(legacy.boot_id == 0U);

  k1::titan::LiveRenderReceipt receipt{};
  receipt.rendered_request_id = 12;
  receipt.emitted_request_id = 0;
  assert(!k1::titan::liveNotePairCompleted(&receipt, 7, 8, 11));
  assert(receipt.emitted_request_id == 0);
  assert(k1::titan::liveNotePairCompleted(&receipt, 7, 7, 11));
  assert(receipt.emitted_request_id == 11);
  assert(receipt.rendered_request_id == 12);
  k1::titan::liveNotePhysicalEmit(&receipt);
  assert(receipt.emitted_request_id == 12);
  receipt.emitted_request_id = 0;
  assert(!k1::titan::liveNotePairCompleted(&receipt, 7, 7, 0));
  assert(receipt.emitted_request_id == 0);

  std::printf("LIVE_BOOT_IDENTITY_HOST_PASS first=%u second=%u crossed=%u\n", first, second,
              crossed);
  return 0;
}
