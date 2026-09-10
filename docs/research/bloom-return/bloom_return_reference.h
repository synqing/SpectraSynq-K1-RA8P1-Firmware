#pragma once
// New K1 design reference. No Sensory Bridge code is copied here.
// Host-verified mathematical prototype; not integrated Titan firmware.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace bloom_reference {
struct Rgb { float r = 0, g = 0, b = 0; };
inline Rgb add(Rgb a, Rgb b) { return {a.r+b.r, a.g+b.g, a.b+b.b}; }
inline Rgb scale(Rgb a, float s) { return {a.r*s, a.g*s, a.b*s}; }

class BloomReturn {
 public:
  static constexpr std::int64_t kHop = 360; // MEDIA_TIME_48K: 7.5 ms
  static constexpr std::int64_t kMaxTravel = 96000; // 2 seconds one way
  static constexpr std::size_t kCapacity = 1070; // > 4*maxTravel/hop + 2
  enum class Push { accepted, gap_reset, stale, invalid };
  struct Parameters {
    std::int64_t travel_ticks = 48000;
    std::uint8_t returns = 1;
    float edge_gain = 0.45F;
    float centre_gain = 0.50F;
    float half_life_ticks = 96000.F;
  };

  // Travel time is latched for the history lifetime. Reconfiguration resets it.
  bool configure(Parameters p) {
    if (p.travel_ticks < kHop || p.travel_ticks > kMaxTravel || p.returns > 2 ||
        !unit(p.edge_gain) || !unit(p.centre_gain) ||
        !std::isfinite(p.half_life_ticks) || p.half_life_ticks <= 0) return false;
    params_ = p; count_ = 0; next_ = 0; return true;
  }
  void reset(std::uint32_t epoch) { epoch_ = epoch; count_ = 0; next_ = 0; }
  Push push(std::uint32_t epoch, std::int64_t tick, Rgb colour) {
    if (epoch != epoch_ || tick < 0 || !unit(colour.r) || !unit(colour.g) ||
        !unit(colour.b)) return Push::invalid;
    if (count_ && tick <= newest_) return Push::stale;
    const bool gap = count_ && tick-newest_ != kHop;
    if (gap) { count_ = 0; next_ = 0; }
    seeds_[next_] = colour;
    next_ = (next_+1) % kCapacity;
    if (count_ < kCapacity) ++count_;
    newest_ = tick;
    return gap ? Push::gap_reset : Push::accepted;
  }
  // Read-only rendering: no frame-count-dependent transport or attenuation.
  // Caller must choose a content timestamp for which seed input is available.
  bool render(std::int64_t tick, float radius, Rgb& result) const {
    result = {};
    if (!count_ || tick < 0 || tick > newest_ || !unit(radius)) return false;
    const double t = static_cast<double>(params_.travel_ticks);
    result = tap(tick, radius*t, 1.F);
    float weight_sum = 1.F;
    if (params_.returns >= 1) {
      result = add(result, tap(tick, (2.0-radius)*t, params_.edge_gain));
      weight_sum += params_.edge_gain;
    }
    if (params_.returns >= 2) {
      const float outbound = params_.edge_gain*params_.centre_gain;
      const float inbound = outbound*params_.edge_gain;
      result = add(result, tap(tick, (2.0+radius)*t, outbound));
      result = add(result, tap(tick, (4.0-radius)*t, inbound));
      weight_sum += outbound+inbound;
    }
    // Conservative, constant headroom: no clipping and no frame-varying gain pump.
    result = scale(result, 1.F/weight_sum);
    return true;
  }
  std::size_t sample_count() const { return count_; }
 private:
  static bool unit(float x) { return std::isfinite(x) && x >= 0 && x <= 1; }
  Rgb history(double tick) const {
    const double age = (static_cast<double>(newest_)-tick)/kHop;
    if (age < 0 || age > static_cast<double>(count_-1)) return {};
    // Causal hold: never blend a seed into times before its timestamp.
    // A smoother reconstruction must explicitly account for its added latency.
    const auto back = static_cast<std::size_t>(std::ceil(age));
    if (back >= count_) return {};
    const auto recent = (next_+kCapacity-1-back)%kCapacity;
    return seeds_[recent];
  }
  Rgb tap(std::int64_t now, double age, float gain) const {
    const float decay = static_cast<float>(std::exp2(-age/params_.half_life_ticks));
    return scale(history(static_cast<double>(now)-age), gain*decay);
  }
  Parameters params_{};
  std::array<Rgb, kCapacity> seeds_{};
  std::size_t next_ = 0, count_ = 0;
  std::int64_t newest_ = 0;
  std::uint32_t epoch_ = 0;
};
} // namespace bloom_reference
