#include "core/audio/feature_strength_shape.h"

#include <cassert>
#include <cstdio>

using k1::contract::AudioFeaturesV1;
using k1::contract::kEventSilence;
using k1::contract::kSpectrumBinCount;
using k1::core::audio::StrengthShapeAdapter;

int main() {
  StrengthShapeAdapter adapter;
  AudioFeaturesV1 audio{};
  audio.event_flags = 0;
  audio.spectrum[4] = 1.0F;
  audio.spectral_energy = 0.4F;
  audio.sequence = 1;
  auto first = adapter.observe(audio);
  assert(first.valid);
  assert(first.surprise == 0.0F);
  audio.sequence = 2;
  auto second = adapter.observe(audio);
  assert(second.valid);
  assert(second.shape_distance < 1.0e-6F);

  AudioFeaturesV1 other = audio;
  other.spectrum[4] = 0.0F;
  other.spectrum[40] = 1.0F;
  other.sequence = 3;
  auto changed = adapter.observe(other);
  assert(changed.valid);
  assert(changed.shape_distance > 0.1F);

  AudioFeaturesV1 silent{};
  silent.event_flags = kEventSilence;
  silent.sequence = 4;
  auto quiet = adapter.observe(silent);
  assert(!quiet.valid);

  adapter.reset(9);
  assert(adapter.epoch() == 9U);
  AudioFeaturesV1 prefix = audio;
  prefix.sequence = 10;
  auto a = adapter.observe(prefix);
  StrengthShapeAdapter clone;
  clone.reset(9);
  auto b = clone.observe(prefix);
  assert(a.valid && b.valid);
  assert(a.strength == b.strength);

  std::puts("K1_STRENGTH_SHAPE=PASS");
  return 0;
}
