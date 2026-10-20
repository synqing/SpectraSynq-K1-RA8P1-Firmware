#ifndef K1_EMOTISCOPE_EDGE_COLOUR_H
#define K1_EMOTISCOPE_EDGE_COLOUR_H

#include <cmath>
#include <cstdint>

// One config belongs to each visual context. Pair selection is snapshotted by
// the caller; this helper has no mutable state, allocation or hardware access.
struct EmotiscopeEdgeColourConfig {
  std::uint32_t enabled = 0;
  std::uint32_t mode = 0;
  std::uint32_t spatial = 0;
  float strength = 0.0f;
  float spread_degrees = 30.0f;
};

struct EmotiscopeEdgeColourRelation {
  bool participates = false;
  float offset_turns = 0.0f;
  float retention = 1.0f;
};

class EmotiscopeEdgeColour {
 public:
  enum Mode : std::uint32_t {
    OFF = 0, ANALOGOUS = 1, COMPLEMENTARY = 2,
    SPLIT_COMPLEMENTARY = 3, SATURATION_VEIL = 4, TRIADIC = 5, TETRADIC = 6
  };
  enum Spatial : std::uint32_t { UNIFORM = 0, CENTRE_MASK = 1 };
  enum Pair : std::uint32_t { INDEPENDENT = 0, ONE_SIDED = 1, SPLIT = 2, MIRROR = 3 };
  using Sampler = void (*)(std::uint32_t, float, float[3]);

  static bool valid(const EmotiscopeEdgeColourConfig& config) noexcept {
    return config.enabled <= 1u && config.mode <= TETRADIC &&
        config.spatial <= CENTRE_MASK && std::isfinite(config.strength) &&
        config.strength >= 0.0f && config.strength <= 1.0f &&
        std::isfinite(config.spread_degrees) &&
        config.spread_degrees >= 0.0f && config.spread_degrees <= 60.0f;
  }

  static std::uint32_t effective_pair(std::uint32_t mode,
                                      std::uint32_t pair) noexcept {
    return mode == COMPLEMENTARY && pair == MIRROR ? SPLIT : pair;
  }

  // Legacy k1_edgemixer.cpp:169-208, 862-904. Float control values retain
  // legacy uint8 truncation, including the veil's integer division rounding.
  static EmotiscopeEdgeColourRelation relation(
      const EmotiscopeEdgeColourConfig& config, std::uint32_t pair,
      bool primary) noexcept {
    EmotiscopeEdgeColourRelation result;
    if (!valid(config) || pair > MIRROR) return result;
    const auto spread = static_cast<std::uint32_t>(config.spread_degrees);
    float degrees = 0.0f;
    switch (config.mode) {
      case ANALOGOUS: degrees = static_cast<float>(spread); break;
      case COMPLEMENTARY: degrees = 180.0f; result.retention = 217.0f / 255.0f; break;
      case SPLIT_COMPLEMENTARY: degrees = 150.0f; result.retention = 230.0f / 255.0f; break;
      case SATURATION_VEIL:
        result.retention = static_cast<float>(255u - spread * 230u / 60u) / 255.0f;
        break;
      case TRIADIC: case TETRADIC:
        degrees = config.mode == TRIADIC ? 120.0f : 90.0f;
        result.retention = 1.0f - static_cast<float>(spread) / 200.0f;
        break;
      default: break;
    }
    pair = effective_pair(config.mode, pair);
    float factor = 1.0f;  // Independent boards each receive their full relation.
    if (pair == SPLIT) factor = primary ? -0.5f : 0.5f;
    else if (pair == MIRROR) factor = primary ? -1.0f : 1.0f;
    else if (pair == ONE_SIDED && primary) factor = 0.0f;
    result.offset_turns = factor * degrees / 360.0f;
    result.participates = config.enabled != 0u && config.mode != OFF &&
        !(pair == ONE_SIDED && primary);
    return result;
  }

  // Whole-board geometry: physical indices79/80 both yield1/159 at160 pixels.
  // Use after mapping a contribution's donor coordinate into physical space.
  static float physical_centre_weight(std::uint32_t index,
                                      std::uint32_t count = 160u) noexcept {
    if (count <= 1u || index >= count) return 0.0f;
    const float end = static_cast<float>(count - 1u);
    return std::fabs(2.0f * static_cast<float>(index) - end) / end;
  }

  // u is the authored palette coordinate AFTER caller reversal/evolution.
  // Apply before brightness and before histories/trails compose contributions.
  // Veil deliberately uses palette-route BT.601 luminance retention rather than
  // the legacy raw-RGB/OKLab branch. There is no RGB-to-palette inversion.
  // False leaves output untouched. Valid bypasses copy original byte-for-byte.
  // Original/output may alias; sampler must return finite RGB in[0,1].
  // palette_id is user selection1..44; sampler receives catalogue index0..43.
  static bool apply(const EmotiscopeEdgeColourConfig& config,
                    std::uint32_t pair, bool primary, float u,
                    float centre_distance, const float original[3],
                    Sampler sampler, std::uint32_t palette_id,
                    float output[3]) noexcept {
    if (!valid(config) || pair > MIRROR || !original || !output ||
        !std::isfinite(u) || !std::isfinite(centre_distance) ||
        centre_distance < 0.0f || centre_distance > 1.0f) return false;
    for (unsigned c = 0; c < 3; ++c)
      if (!std::isfinite(original[c]) || original[c] < 0.0f || original[c] > 1.0f)
        return false;
    const auto rel = relation(config, pair, primary);
    const float amount = config.strength *
        (config.spatial == CENTRE_MASK ? centre_distance : 1.0f);
    const float peak = maximum(original);
    if (!rel.participates || amount == 0.0f || peak <= 2.0f / 255.0f) {
      copy(original, output);
      return true;
    }
    if (!sampler || palette_id < 1u || palette_id > 44u) return false;
    // Wrap before adding so large finite coordinates cannot swallow the offset.
    const float wrapped = u - std::floor(u);
    const float shifted = wrapped + rel.offset_turns;
    float sampled[3] = {};
    sampler(palette_id - 1u, shifted - std::floor(shifted), sampled);
    for (float channel : sampled)
      if (!std::isfinite(channel) || channel < 0.0f || channel > 1.0f) return false;
    const float sample_peak = maximum(sampled);
    if (sample_peak <= 1.0e-6f) {
      copy(original, output);  // Never amplify a black/dark palette fallback.
      return true;
    }
    const float luminance = 0.299f * original[0] + 0.587f * original[1] +
                            0.114f * original[2];
    float resolved[3];
    for (unsigned c = 0; c < 3; ++c) {
      const float colour = sampled[c] * (peak / sample_peak);
      const float retained = clamp01(rel.retention * colour +
                                    (1.0f - rel.retention) * luminance);
      resolved[c] = original[c] + amount * (retained - original[c]);
    }
    copy(resolved, output);
    return true;
  }

 private:
  static float clamp01(float x) noexcept {
    return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
  }
  static float maximum(const float rgb[3]) noexcept {
    return std::fmax(rgb[0], std::fmax(rgb[1], rgb[2]));
  }
  static void copy(const float from[3], float to[3]) noexcept {
    for (unsigned c = 0; c < 3; ++c) to[c] = from[c];
  }
};

#endif
