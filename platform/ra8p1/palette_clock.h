#pragma once
#include <cstdint>
namespace k1::titan {
// Observe the existing free-running DWT; never reset or reconfigure it. Poll
// more often than one 32-bit wrap (4.295 s at 1 GHz). The normal palette loop
// polls every few milliseconds. Debugger halts are outside this clock contract.
class PaletteClock final {
 public:
  std::uint64_t sample(std::uint32_t cycles, std::uint32_t hz) noexcept {
    if (seen_ && hz) {
      const std::uint64_t numerator =
          std::uint64_t(std::uint32_t(cycles-last_))*1000000U + remainder_;
      us_ += numerator / hz;
      remainder_ = numerator % hz;
    }
    last_ = cycles; seen_ = true;
    return us_;
  }
 private:
  std::uint64_t us_=0U, remainder_=0U;
  std::uint32_t last_=0U;
  bool seen_=false;
};
} // namespace k1::titan
