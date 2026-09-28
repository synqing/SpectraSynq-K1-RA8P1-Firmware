#include "k1_live_protocol.h"
#include "k1_live_runtime.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

using k1::titan::LiveAudioRuntime;
using k1::titan::LiveHopStatus;
using k1::titan::LiveSnapshotFields;
using k1::titan::kLiveSnapshotBytes;

static void fill_hop(k1_audio_hop_t* hop, std::uint64_t epoch, std::uint64_t sequence) {
  assert(k1_audio_hop_init_interval(hop, epoch, sequence) == 0);
  hop->source_rate_hz = 40000;
  hop->phase_increment_q16 = (40000u << 16) / 24000u;
  hop->supporting_dma_receipt_us = 1000u * sequence;
  hop->newest_sample_capture_estimate_us = 900u * sequence;
  hop->capture_estimate_valid = 1;
  hop->descriptor_ready_us = 1100u * sequence;
  for (unsigned i = 0; i < K1_AUDIO_HOP_SAMPLES; ++i) {
    hop->pcm[i] = static_cast<std::int16_t>(std::sin(i * 0.1) * 8000.0);
  }
}

int main() {
  LiveSnapshotFields fields{};
  fields.flags = 1;
  fields.publication_generation = 9;
  fields.hop_sequence = 3;
  fields.peak_scaled = 0.5F;
  fields.spectrum[7] = 1.25F;
  fields.chroma[2] = 0.75F;
  std::uint8_t wire[kLiveSnapshotBytes];
  assert(k1::titan::liveEncodeSnapshot(wire, sizeof(wire), fields) == kLiveSnapshotBytes);
  assert(k1::titan::liveGetU32(wire) == 0x53564C54U);
  assert(k1::titan::liveGetU16(wire + 4) == 1);
  assert(k1::titan::liveGetU64(wire + 8) == 9);
  assert(k1::titan::liveGetU64(wire + 24) == 3);
  k1::titan::LiveRenderReceipt receipt{};
  k1::titan::liveNotePatchAccepted(&receipt, 11, 4);
  k1::titan::liveNotePatchAccepted(&receipt, 12, 5);
  assert(receipt.superseded_request_id == 11);
  assert(receipt.pending_request_id == 12);
  k1::titan::liveNoteRendererConsumed(&receipt, 9, 3, 4400);
  assert(receipt.rendered_request_id == 12);
  assert(receipt.rendered_hop == 9);
  assert(receipt.rendered_publication_us == 4400);
  assert(receipt.pending_request_id == 0);
  assert(k1::titan::livePathConsumesAcceptedRevision("effect"));
  assert(k1::titan::livePathConsumesAcceptedRevision("hold"));
  assert(!k1::titan::livePathConsumesAcceptedRevision("preview"));
  assert(!k1::titan::livePathConsumesAcceptedRevision("dwell"));
  assert(!k1::titan::livePathConsumesAcceptedRevision("no_audio"));
  fields.last_rendered_request_id = receipt.rendered_request_id;
  fields.last_rendered_revision = receipt.rendered_revision;
  fields.last_rendered_frame = receipt.rendered_frame;
  fields.last_rendered_hop = receipt.rendered_hop;
  fields.superseded_request_id = receipt.superseded_request_id;
  fields.accepted_request_id = receipt.accepted_request_id;
  assert(k1::titan::liveEncodeSnapshot(wire, sizeof(wire), fields) == kLiveSnapshotBytes);
  assert(k1::titan::liveGetU32(wire + 812) == 12U);
  assert(k1::titan::liveGetU32(wire + 828) == 11U);
  assert(k1::titan::liveGetU32(wire + 832) == 12U);
  float peak = 0;
  std::memcpy(&peak, wire + 96, 4);
  assert(std::fabs(peak - 0.5F) < 1.0e-6F);
  float spec7 = 0;
  std::memcpy(&spec7, wire + 204 + 7 * 4, 4);
  assert(std::fabs(spec7 - 1.25F) < 1.0e-6F);

  k1::titan::LiveConfigBlob cfg{};
  cfg.mode_a = 32;
  cfg.last_patch_request_id = 99;
  k1::titan::liveInitFocusUnity(cfg.focus_a);
  k1::titan::liveInitFocusUnity(cfg.focus_b);
  std::uint8_t blob[k1::titan::kLiveConfigBlobBytes];
  assert(k1::titan::liveEncodeConfig(blob, sizeof(blob), cfg) ==
         k1::titan::kLiveConfigBlobBytesV1);
  k1::titan::LiveConfigBlob back{};
  assert(k1::titan::liveDecodeConfig(blob, k1::titan::kLiveConfigBlobBytesV1, &back));
  assert(back.mode_a == 32);
  assert(back.last_patch_request_id == 99);
  assert(k1::titan::liveGetU32(blob + 52) == 99U);
  assert(std::fabs(back.focus_a[0] - 1.0F) < 1.0e-6F);

  cfg.version = 2;
  cfg.visual_a.chroma = 0.42F;
  cfg.visual_a.flags = k1::titan::kVisEnabled | k1::titan::kVisMirror |
                       k1::titan::kVisChromatic;
  cfg.visual_b.vp_bloom_alpha = 0.5F;
  cfg.boot_id = 0x01000007U;
  assert(k1::titan::liveEncodeConfig(blob, sizeof(blob), cfg) ==
         k1::titan::kLiveConfigBlobBytesV2);
  assert(k1::titan::liveGetU32(blob + k1::titan::kLiveConfigBootIdOffset) == 0x01000007U);
  k1::titan::LiveConfigBlob v2{};
  assert(k1::titan::liveDecodeConfig(blob, k1::titan::kLiveConfigBlobBytesV2, &v2));
  assert(v2.version == 2);
  assert(v2.boot_id == 0x01000007U);
  assert(std::fabs(v2.visual_a.chroma - 0.42F) < 1.0e-6F);
  assert(std::fabs(v2.visual_b.vp_bloom_alpha - 0.5F) < 1.0e-6F);
  assert((v2.visual_a.flags & k1::titan::kVisMirror) != 0U);
  assert(!k1::titan::liveDecodeConfig(blob, 100, &v2));

  std::uint8_t page[64];
  const auto n = k1::titan::liveEncodeSchemaPage(page, sizeof(page), 0, 16);
  assert(n == 28);
  assert(k1::titan::liveGetU32(page + 4) == kTitanLiveSchemaLen);

  k1::titan::LiveConfigBlob patch_cfg{};
  patch_cfg.version = 2;
  patch_cfg.revision = 7;
  patch_cfg.boot_id = 1;
  patch_cfg.emit_on = 0;
  patch_cfg.flags = 1;
  patch_cfg.mode_a = 32;
  patch_cfg.visual_a.hue_position = 0.1F;
  k1::titan::LivePatchEntry one{};
  one.field_id = k1::titan::kPatchVisHuePosition;
  one.kind = k1::titan::kLivePatchKindF32;
  one.value = 0.42F;
  assert(k1::titan::liveApplyPatch(&patch_cfg, 7, 1, 0, &one, 1) == k1::titan::LivePatchStatus::Ok);
  assert(std::fabs(patch_cfg.visual_a.hue_position - 0.42F) < 1.0e-6F);
  assert(patch_cfg.emit_on == 0);
  assert(patch_cfg.flags == 1);
  assert(patch_cfg.mode_a == 32);
  assert(patch_cfg.revision == 8);
  assert(patch_cfg.boot_id == 1);
  assert(k1::titan::liveApplyPatch(&patch_cfg, 7, 1, 0, &one, 1) == k1::titan::LivePatchStatus::Stale);
  const float kept = patch_cfg.visual_a.hue_position;
  assert(k1::titan::liveApplyPatch(&patch_cfg, 8, 2, 0, &one, 1) ==
         k1::titan::LivePatchStatus::WrongBoot);
  assert(std::fabs(patch_cfg.visual_a.hue_position - kept) < 1.0e-6F);
  assert(patch_cfg.revision == 8);
  k1::titan::LiveConfigBlob zero_boot = patch_cfg;
  zero_boot.boot_id = 0;
  zero_boot.revision = 0;
  assert(k1::titan::liveApplyPatch(&zero_boot, 0, 0, 0, &one, 1) ==
         k1::titan::LivePatchStatus::WrongBoot);
  assert(zero_boot.revision == 0);
  k1::titan::LivePatchEntry bad{};
  bad.field_id = 99;
  bad.kind = k1::titan::kLivePatchKindF32;
  bad.value = 0.2F;
  assert(k1::titan::liveApplyPatch(&patch_cfg, 8, 1, 0, &bad, 1) ==
         k1::titan::LivePatchStatus::Forbidden);
  k1::titan::LivePatchEntry nan{};
  nan.field_id = k1::titan::kPatchVisHuePosition;
  nan.kind = k1::titan::kLivePatchKindF32;
  nan.value = std::numeric_limits<float>::quiet_NaN();
  assert(k1::titan::liveApplyPatch(&patch_cfg, 8, 1, 0, &nan, 1) ==
         k1::titan::LivePatchStatus::Invalid);
  std::uint8_t patch_wire[64];
  const auto patch_n =
      k1::titan::liveEncodePatch(patch_wire, sizeof(patch_wire), 8, 42, 0, 1, &one, 1);
  assert(patch_n == 24);
  k1::titan::LivePatchEntry decoded[8]{};
  std::uint32_t decoded_rev = 0;
  std::uint32_t decoded_req = 0;
  std::uint32_t decoded_boot = 0;
  std::uint16_t decoded_ch = 99;
  std::uint16_t decoded_n = 0;
  assert(k1::titan::liveDecodePatch(patch_wire, patch_n, &decoded_rev, &decoded_req, &decoded_ch,
                                   &decoded_boot, decoded, &decoded_n, 8));
  assert(decoded_rev == 8);
  assert(decoded_req == 42);
  assert(decoded_ch == 0);
  assert(decoded_boot == 1);
  assert(decoded_n == 1);
  assert(decoded[0].field_id == k1::titan::kPatchVisHuePosition);

  k1::titan::LiveConfigBlob ab{};
  ab.version = 2;
  ab.revision = 1;
  ab.boot_id = 1;
  ab.visual_a.hue_position = 0.11F;
  ab.visual_b.hue_position = 0.22F;
  k1::titan::LivePatchEntry b_hue{};
  b_hue.field_id = k1::titan::kPatchVisHuePosition;
  b_hue.kind = k1::titan::kLivePatchKindF32;
  b_hue.value = 0.37F;
  assert(k1::titan::liveApplyPatch(&ab, 1, 1, 1, &b_hue, 1) == k1::titan::LivePatchStatus::Ok);
  assert(std::fabs(ab.visual_a.hue_position - 0.11F) < 1.0e-6F);
  assert(std::fabs(ab.visual_b.hue_position - 0.37F) < 1.0e-6F);
  std::uint8_t b_wire[64];
  const auto b_n = k1::titan::liveEncodePatch(b_wire, sizeof(b_wire), 2, 99, 1, 1, &b_hue, 1);
  std::uint32_t b_rev = 0, b_req = 0, b_boot = 0;
  std::uint16_t b_ch = 0, b_count = 0;
  k1::titan::LivePatchEntry b_decoded[8]{};
  assert(k1::titan::liveDecodePatch(b_wire, b_n, &b_rev, &b_req, &b_ch, &b_boot, b_decoded, &b_count,
                                   8));
  assert(b_ch == 1);
  assert(b_req == 99);
  assert(b_boot == 1);

  LiveAudioRuntime runtime;
  runtime.initialise(1000000000U);
  runtime.advanceHostCycles(1000000U);
  k1_audio_hop_t hop{};
  fill_hop(&hop, 1, 1);
  runtime.advanceHostCycles(2000000U);
  assert(runtime.consume(hop) == LiveHopStatus::Ok);
  std::uint8_t snap[kLiveSnapshotBytes];
  assert(runtime.encodeSnapshot(snap, sizeof(snap), runtime.snapshot().publish_time_us) == kLiveSnapshotBytes);
  std::uint8_t events[4096];
  const auto ev = runtime.encodeEvents(events, sizeof(events), 0);
  assert(ev >= 64);
  assert(k1::titan::liveGetU16(events + 32) >= 1);
  std::uint8_t timing[4096];
  const auto tm = runtime.encodeTiming(timing, sizeof(timing), 0);
  assert(tm >= 64);

  std::printf("LIVE_PROTOCOL_HOST_PASS snap=%u events=%u timing=%u schema=%s\n",
              unsigned(kLiveSnapshotBytes), unsigned(ev), unsigned(tm), kTitanLiveSchemaSha256);
  return 0;
}
