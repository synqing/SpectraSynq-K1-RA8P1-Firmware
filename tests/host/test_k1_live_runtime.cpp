#include "k1_live_runtime.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

using k1::titan::LiveAudioRuntime;
using k1::titan::LiveHopStatus;

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
  LiveAudioRuntime runtime;
  runtime.initialise(1000000000U);
  runtime.advanceHostCycles(1000000U);
  assert(runtime.latest() == nullptr);
  assert(runtime.warming());
  assert(runtime.serviceOneHop() == LiveHopStatus::NotReady);

  k1_audio_hop_t hop1{};
  fill_hop(&hop1, 7, 1);
  runtime.advanceHostCycles(2000000U);
  assert(runtime.consume(hop1) == LiveHopStatus::Ok);
  const auto* first = runtime.latest();
  assert(first != nullptr && first->valid);
  assert(first->features.sequence == 1U);
  assert(k1_audio_hop_media_end_48k(&hop1) == 360);
  const std::uint64_t first_publish = first->features.publish_time_us;
  const std::uint64_t first_capture = first->features.capture_time_us;
  assert(first_publish != first_capture);
  assert(first_publish != 0);
  assert(first_publish >= first_capture);
  assert(runtime.publicationGeneration() == 1);
  assert(runtime.hopsConsumed() == 1);
  assert(runtime.warming());
  assert(runtime.snapshot().physical_timestamp_valid == 0);

  assert(runtime.consume(hop1) == LiveHopStatus::Rejected);
  assert(runtime.hopsRejected() == 1);
  assert(runtime.hopsConsumed() == 1);

  k1_audio_hop_t hop2{};
  fill_hop(&hop2, 7, 2);
  runtime.advanceHostCycles(3000000U);
  assert(runtime.consume(hop2) == LiveHopStatus::Ok);
  const auto* second = runtime.latest();
  assert(second != nullptr);
  assert(second->features.sequence == 2U);
  assert(second->features.publish_time_us > first_publish);
  assert(runtime.publicationFresh(second->features.publish_time_us));
  assert(!runtime.publicationFresh(second->features.publish_time_us + 30001U));

  k1_audio_hop_t skip{};
  fill_hop(&skip, 7, 4);
  assert(runtime.consume(skip) == LiveHopStatus::Rejected);

  runtime.invalidate(9);
  assert(runtime.latest() == nullptr);
  assert(runtime.lastInvalidateReason() == 9U);
  assert(runtime.warming());

  k1_audio_hop_t next_epoch{};
  fill_hop(&next_epoch, 8, 1);
  runtime.advanceHostCycles(4000000U);
  assert(runtime.consume(next_epoch) == LiveHopStatus::Ok);
  assert(runtime.warmupHops() == 1);
  assert(runtime.latest() != nullptr);
  assert(runtime.snapshot().physical_timestamp_valid == 0);

  char json[1024];
  const auto n = runtime.mirJson(json, sizeof(json));
  assert(n > 0);
  assert(std::strstr(json, "\"recording\":\"scheduled_phase_8\"") != nullptr);
  assert(std::strstr(json, "\"physical_timestamp_valid\":false") != nullptr);

  std::printf("LIVE_RUNTIME_HOST_PASS hops=%u rejected=%u publish_ne_capture=true phase8=scheduled\n",
              runtime.hopsConsumed(), runtime.hopsRejected());
  return 0;
}
