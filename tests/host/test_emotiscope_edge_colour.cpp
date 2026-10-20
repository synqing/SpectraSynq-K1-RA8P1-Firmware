#include "emotiscope_edge_colour.h"
#include <cassert>
#include <cstring>
#include <limits>
#include <cstdio>
#include <initializer_list>

using Edge = EmotiscopeEdgeColour;
static unsigned samples = 0;
static float sampled_u = 0.0f;
static std::uint32_t sampled_id = 999u;
static void palette(std::uint32_t id, float u, float rgb[3]) {
  ++samples;
  sampled_u = u;
  sampled_id = id;
  // Two authored multicolour paths, with no hue reconstruction.
  rgb[0] = id == 0u ? 0.2f + 0.6f * u : 0.7f - 0.4f * u;
  rgb[1] = id == 0u ? 0.8f - 0.5f * u : 0.1f + 0.7f * u;
  rgb[2] = id == 0u ? 0.3f : 0.6f;
}
static void black(std::uint32_t, float, float rgb[3]) {
  rgb[0] = rgb[1] = rgb[2] = 0.0f;
}
static void invalid_sample(std::uint32_t, float, float rgb[3]) {
  rgb[0] = std::numeric_limits<float>::quiet_NaN();
  rgb[1] = rgb[2] = 0.5f;
}
static void near(float a, float b, float tolerance = 2.0e-6f) {
  assert(std::fabs(a - b) <= tolerance);
}
static bool same(const float a[3], const float b[3]) {
  return std::memcmp(a, b, 3u * sizeof(float)) == 0;
}
int main() {
  EmotiscopeEdgeColourConfig config;
  assert(Edge::valid(config));
  assert(!config.enabled && config.mode == Edge::OFF && !config.spatial);
  near(config.strength, 0.0f); near(config.spread_degrees, 30.0f);
  config.enabled = 1; config.strength = 0.65f;
  const float original[3] = {0.8f, 0.2f, 0.35f};
  float output[3];
  for (unsigned mode = 0; mode <= 6; ++mode) {
    config.mode = mode;
    for (unsigned spread : {0u, 17u, 60u}) {
      config.spread_degrees = static_cast<float>(spread);
      const float degrees[] = {0, static_cast<float>(spread), 180, 150, 0, 120, 90};
      const float retention[] = {1, 1, 217.0f/255, 230.0f/255,
          static_cast<float>(255u-spread*230u/60u)/255,
          1.0f-spread/200.0f, 1.0f-spread/200.0f};
      for (unsigned pair = 0; pair <= 3; ++pair) {
        const unsigned effective = mode == 2 && pair == 3 ? 2 : pair;
        for (bool primary : {false, true}) {
          const auto relation = Edge::relation(config, pair, primary);
          const float factors[4][2] = {{1,1}, {1,0}, {.5f,-.5f}, {1,-1}};
          near(relation.offset_turns, factors[effective][primary] * degrees[mode]/360);
          near(relation.retention, retention[mode]);
          assert(relation.participates == (mode != 0 && !(effective == 1 && primary)));
          for (unsigned spatial = 0; spatial <= 1; ++spatial) {
            config.spatial = spatial;
            for (float strength : {0.0f, 0.65f, 1.0f}) {
              config.strength = strength;
              for (unsigned id : {1u, 2u}) {
                assert(Edge::apply(config,pair,primary,.137f,.37f,original,palette,id,output));
                for (float c : output) assert(std::isfinite(c) && c >= 0 && c <= 1);
                if (!relation.participates || strength == 0) assert(same(original,output));
                else {
                  float shifted = .137f + relation.offset_turns;
                  shifted -= std::floor(shifted);
                  assert(sampled_id == id - 1u);
                  float rgb[3]; palette(id - 1u, shifted, rgb);
                  const float peak = std::fmax(rgb[0], std::fmax(rgb[1],rgb[2]));
                  const float luma = .299f*.8f + .587f*.2f + .114f*.35f;
                  for (unsigned c = 0; c < 3; ++c) {
                    const float target = retention[mode]*rgb[c]*.8f/peak + (1-retention[mode])*luma;
                    const float amount = strength*(spatial ? .37f : 1);
                    near(output[c], (1-amount)*original[c] + amount*target);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  for (unsigned i=0;i<160;++i) {
    near(Edge::physical_centre_weight(i), Edge::physical_centre_weight(159-i));
  }
  near(Edge::physical_centre_weight(79),1.0f/159);
  near(Edge::physical_centre_weight(80),1.0f/159);
  near(Edge::physical_centre_weight(0),1); near(Edge::physical_centre_weight(159),1);
  near(Edge::physical_centre_weight(160),0);
  config = {1, Edge::SATURATION_VEIL, Edge::UNIFORM, 1, 17.9f};
  near(Edge::relation(config,Edge::SPLIT,true).retention,190.0f/255);
  assert(Edge::apply(config,Edge::ONE_SIDED,true,.2f,1,original,palette,1,output));
  assert(same(original,output));
  const float zero[3]={0,0,0}, tiny[3]={1e-9f,2e-9f,1e-9f};
  const unsigned before = samples;
  assert(Edge::apply(config,0,false,.2f,1,zero,palette,1,output) && same(zero,output));
  assert(Edge::apply(config,0,false,.2f,1,tiny,palette,1,output) && same(tiny,output));
  assert(samples == before);
  assert(Edge::apply(config,0,false,.2f,1,original,black,1,output) && same(original,output));
  config.mode = Edge::TRIADIC;
  assert(Edge::apply(config,0,false,.2f,1,original,palette,44,output));
  assert(sampled_id == 43u);
  assert(Edge::apply(config,0,false,-2.2f,1,original,palette,1,output));
  near(sampled_u,.8f+1.0f/3-1);
  float baseline[3]; std::memcpy(baseline,output,sizeof(output));
  float alias[3]; std::memcpy(alias,original,sizeof(alias));
  assert(Edge::apply(config,0,false,-2.2f,1,alias,palette,1,alias));
  assert(same(alias,baseline));
  const auto saved = config;
  config.strength = std::numeric_limits<float>::quiet_NaN();
  assert(!Edge::valid(config));
  assert(!Edge::apply(config,0,false,0,1,original,palette,1,output));
  assert(same(output,baseline));
  config=saved;
  assert(!Edge::apply(config,0,false,0,1,original,invalid_sample,1,output));
  assert(same(output,baseline));
  assert(!Edge::apply(config,4,false,0,1,original,palette,1,output));
  assert(!Edge::apply(config,0,false,0,-1,original,palette,1,output));
  assert(!Edge::apply(config,0,false,0,1,original,palette,0,output));
  assert(!Edge::apply(config,0,false,0,1,original,nullptr,1,output));
  for (unsigned field=0;field<3;++field) {
    config=saved;
    if (field == 0) config.enabled = 2;
    if (field == 1) config.mode = 7;
    if (field == 2) config.spatial = 2;
    assert(!Edge::valid(config));
  }
  config=saved; config.spread_degrees=60.1f; assert(!Edge::valid(config));
  config=saved; config.strength=1.01f; assert(!Edge::valid(config));
  // Immutable per-board config and repeatable results after another board runs.
  config=saved;
  assert(Edge::apply(config,0,false,.137f,1,original,palette,1,baseline));
  auto other=config; other.mode=Edge::TETRADIC; other.strength=.2f;
  assert(Edge::apply(other,0,true,.6f,1,original,palette,2,output));
  assert(Edge::apply(config,0,false,.137f,1,original,palette,1,output));
  assert(same(output,baseline));
  std::puts("Edge colour source equations, palette routing, roles and finite guards passed");
}
