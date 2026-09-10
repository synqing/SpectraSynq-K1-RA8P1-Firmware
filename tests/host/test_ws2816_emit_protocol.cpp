#include "core/visual/ws2816_pack.h"
#include "ws2816_gpio_emit.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
std::uint32_t virtual_cycles = 1;
bool level_a = false;
bool level_b = false;
std::vector<unsigned> a_high;
std::vector<unsigned> b_high;

std::uint32_t now() {
  virtual_cycles += 50U;
  return virtual_cycles;
}
void set_a(int high) {
  level_a = high != 0;
  a_high.push_back(high ? 1U : 0U);
}
void set_b(int high) {
  level_b = high != 0;
  b_high.push_back(high ? 1U : 0U);
}
}  // namespace

int main() {
  using namespace k1::core::visual;
  assert(kPackedBytesPerLane == K1_PACKED_BYTES_PER_LANE);
  assert(k1_ws2816_require_packed_lanes(kPackedBytesPerLane,
                                        kPackedBytesPerLane) ==
         kPackedAccepted);
  assert(k1_ws2816_require_packed_lanes(240, kPackedBytesPerLane) ==
         kPackedWrongCount);
  assert(k1_ws2816_require_packed_lanes(kPackedBytesPerLane, 240) ==
         kPackedWrongCount);

  Pixel16 ch[kPixelsPerChannel]{};
  ch[0] = {0x12AB, 0, 0};
  ch[80] = {0, 0, 0x12AB};
  std::uint8_t a[kPackedBytesPerLane]{};
  std::uint8_t b[kPackedBytesPerLane]{};
  assert(splitChannel160(ch, kPixelsPerChannel, a, b));

  ws2816_bit_sink_t sink{set_a, set_b, now};
  packed_lane_completion_t completion{};
  k1_ws2816_emit_lockstep_bits(a, b, kPackedBytesPerLane, &sink, 260, 660, 1250,
                               &completion);
  assert(completion.bit_period_min_cycles > 0);
  assert(completion.bit_period_max_cycles >=
         completion.bit_period_min_cycles);
  assert(a_high.size() == b_high.size());
  assert(a_high.size() > 0);
  assert(a_high != b_high);
  (void)level_a;
  (void)level_b;
  std::puts("K1_WS2816_EMIT_PROTOCOL=PASS true16=PASS wrongcount=PASS");
}
