#include "k1_live_protocol.h"
#include "k1_live_runtime.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

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
  float peak = 0;
  std::memcpy(&peak, wire + 96, 4);
  assert(std::fabs(peak - 0.5F) < 1.0e-6F);
  float spec7 = 0;
  std::memcpy(&spec7, wire + 204 + 7 * 4, 4);
  assert(std::fabs(spec7 - 1.25F) < 1.0e-6F);

  k1::titan::LiveConfigBlob cfg{};
  cfg.mode_a = 32;
  k1::titan::liveInitFocusUnity(cfg.focus_a);
  k1::titan::liveInitFocusUnity(cfg.focus_b);
  std::uint8_t blob[k1::titan::kLiveConfigBlobBytes];
  assert(k1::titan::liveEncodeConfig(blob, sizeof(blob), cfg) ==
         k1::titan::kLiveConfigBlobBytesV1);
  k1::titan::LiveConfigBlob back{};
  assert(k1::titan::liveDecodeConfig(blob, k1::titan::kLiveConfigBlobBytesV1, &back));
  assert(back.mode_a == 32);
  assert(std::fabs(back.focus_a[0] - 1.0F) < 1.0e-6F);

  cfg.version = 2;
  cfg.visual_a.chroma = 0.42F;
  cfg.visual_a.flags = k1::titan::kVisEnabled | k1::titan::kVisMirror |
                       k1::titan::kVisChromatic;
  cfg.visual_b.vp_bloom_alpha = 0.5F;
  assert(k1::titan::liveEncodeConfig(blob, sizeof(blob), cfg) ==
         k1::titan::kLiveConfigBlobBytesV2);
  k1::titan::LiveConfigBlob v2{};
  assert(k1::titan::liveDecodeConfig(blob, k1::titan::kLiveConfigBlobBytesV2, &v2));
  assert(v2.version == 2);
  assert(std::fabs(v2.visual_a.chroma - 0.42F) < 1.0e-6F);
  assert(std::fabs(v2.visual_b.vp_bloom_alpha - 0.5F) < 1.0e-6F);
  assert((v2.visual_a.flags & k1::titan::kVisMirror) != 0U);
  assert(!k1::titan::liveDecodeConfig(blob, 100, &v2));

  std::uint8_t page[64];
  const auto n = k1::titan::liveEncodeSchemaPage(page, sizeof(page), 0, 16);
  assert(n == 28);
  assert(k1::titan::liveGetU32(page + 4) == kTitanLiveSchemaLen);

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
