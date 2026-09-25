#include "k1_live_runtime.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

using k1::titan::LiveAudioRuntime;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: replay_live_k1 hops.pcm\n");
    return 2;
  }
  FILE* fp = std::fopen(argv[1], "rb");
  if (fp == nullptr) {
    return 2;
  }
  std::vector<std::int16_t> pcm;
  std::int16_t sample = 0;
  while (std::fread(&sample, 2, 1, fp) == 1) {
    pcm.push_back(sample);
  }
  std::fclose(fp);
  LiveAudioRuntime runtime;
  runtime.initialise(1000000000U);
  std::uint64_t seq = 0;
  std::uint32_t hops = 0;
  for (std::size_t offset = 0; offset + 180 <= pcm.size(); offset += 180) {
    k1_audio_hop_t hop{};
    seq += 1;
    if (k1_audio_hop_init_interval(&hop, 1, seq) != 0) {
      return 3;
    }
    std::memcpy(hop.pcm, pcm.data() + offset, 180 * 2);
    hop.supporting_dma_receipt_us = seq * 7500U;
    hop.newest_sample_capture_estimate_us = hop.supporting_dma_receipt_us;
    hop.capture_estimate_valid = 1;
    runtime.advanceHostCycles(argc > 2 ? 15000000U : 7500000U);
    if (runtime.consume(hop) != k1::titan::LiveHopStatus::Ok) {
      return 4;
    }
    hops += 1;
  }
  const auto snap = runtime.snapshot();
  std::printf("{\"hops\":%u,\"generation\":%llu,\"peak\":%.6g,\"tempo_bpm\":%.6g,\"valid\":%u}\n",
              hops,
              static_cast<unsigned long long>(snap.publication_generation),
              static_cast<double>(snap.peak_scaled),
              static_cast<double>(snap.tempo_bpm),
              unsigned(snap.valid));
  return hops == 0 ? 5 : 0;
}
