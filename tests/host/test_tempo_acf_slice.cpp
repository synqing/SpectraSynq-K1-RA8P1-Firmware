#include "core/audio/tempo_acf.h"
#include "core/audio/tempo_acf_slice.h"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace k1::core::audio;

static void fill_ring(TempoNoveltyRing& ring, unsigned seed) {
  for (std::uint16_t i = 0; i < kTempoAcfHistoryLength; ++i) {
    const float phase = static_cast<float>((i * 13U + seed) % 97U) / 96.0F;
    ring[i] = 0.2F + 0.6F * phase * phase;
  }
}

int main() {
  TempoNoveltyRing ring{};
  fill_ring(ring, 7U);
  const std::uint16_t oldest = 40;
  const float scale = 0.85F;
  const float rate = 44.4444427F;
  TempoAcfOutput direct{};
  computeTempoAcfAtRate(ring, oldest, scale, rate, direct);
  TempoAcfSliceJob job{};
  tempoAcfSliceBegin(job, ring, oldest, scale, rate, 0.0225F, 0.4F);
  unsigned pumps = 0;
  while (!job.lags_done) {
    const unsigned chunk =
        static_cast<unsigned>((job.lag_count + 2) / 3);
    const bool done = tempoAcfSlicePump(job, chunk ? chunk : 1U);
    ++pumps;
    assert(pumps <= 3U);
    if (done) break;
  }
  assert(job.lags_done);
  TempoAcfOutput sliced{};
  tempoAcfSliceFinish(job, sliced);
  assert(direct.valid == sliced.valid);
  for (std::uint16_t bin = 0; bin < kTempoAcfBinCount; ++bin) {
    assert(direct.comb_salience[bin] == sliced.comb_salience[bin]);
    assert(direct.point_salience[bin] == sliced.point_salience[bin]);
  }
  TempoAcfSliceJob broken{};
  tempoAcfSliceBegin(broken, ring, oldest, scale, rate, 0.0225F, 0.4F);
  assert(broken.lag_count > 1);
  assert(!tempoAcfSlicePump(broken, 1U));
  TempoAcfOutput incomplete{};
  tempoAcfSliceFinish(broken, incomplete);
  bool differed = false;
  for (std::uint16_t bin = 0; bin < kTempoAcfBinCount; ++bin) {
    if (incomplete.comb_salience[bin] != direct.comb_salience[bin]) {
      differed = true;
      break;
    }
  }
  assert(differed);
  std::printf("TEMPO_ACF_SLICE_PASS pumps=%u lags=%d\n", pumps, job.lag_count);
  return 0;
}
