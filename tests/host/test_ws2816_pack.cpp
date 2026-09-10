#include "core/visual/ws2816_pack.h"

#include <cassert>
#include <cstdio>
#include <cstring>

using namespace k1::core::visual;

int main() {
  Pixel16 px{0x12AB, 0x0000, 0x0000};
  std::uint8_t slot[6]{};
  packPixel(px, slot);
  assert(slot[0] == 0x00 && slot[1] == 0x00 && slot[2] == 0x12);
  assert(slot[3] == 0xAB && slot[4] == 0x00 && slot[5] == 0x00);
  // ×257 of high byte would put 0x12 in the low slot — must fail that layout:
  assert(slot[3] != 0x12);

  Pixel16 ch[160]{};
  ch[0] = {0x12AB, 0, 0};
  ch[80] = {0, 0, 0x12AB};
  std::uint8_t a[kPackedBytesPerLane]{}, b[kPackedBytesPerLane]{};
  assert(splitChannel160(ch, 160, a, b));
  assert(a[2] == 0x12 && a[3] == 0xAB);
  assert(b[4] == 0x12 && b[5] == 0xAB);
  assert(a[480 - 1] == 0 && b[0] == 0);

  Pixel16 too_many[161]{};
  assert(!splitChannel160(too_many, 161, a, b));
  assert(!splitChannel160(ch, 80, a, b));

  std::uint8_t twenty_four[3]{0x12, 0xAB, 0x00};
  assert(std::memcmp(slot, twenty_four, 3) != 0);
  std::puts("K1_WS2816_PACK=PASS true16=PASS split=PASS");
}
