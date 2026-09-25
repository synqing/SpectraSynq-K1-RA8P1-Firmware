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

  /* X7: full TRUE16 GRB48 byte order is G_hi G_lo R_hi R_lo B_hi B_lo. */
  Pixel16 full{0x12AB, 0x34CD, 0x56EF};
  std::uint8_t grb48[6]{};
  packPixel(full, grb48);
  const std::uint8_t expect_grb48[6] = {0x34, 0xCD, 0x12, 0xAB, 0x56, 0xEF};
  assert(std::memcmp(grb48, expect_grb48, 6) == 0);

  /* RGB8×257 is explicitly not TRUE16 (high byte replicated into both slots). */
  const std::uint8_t rgb8_x257[6] = {
      static_cast<std::uint8_t>((0x34u * 257u) >> 8),
      static_cast<std::uint8_t>((0x34u * 257u) & 0xffu),
      static_cast<std::uint8_t>((0x12u * 257u) >> 8),
      static_cast<std::uint8_t>((0x12u * 257u) & 0xffu),
      static_cast<std::uint8_t>((0x56u * 257u) >> 8),
      static_cast<std::uint8_t>((0x56u * 257u) & 0xffu)};
  assert(std::memcmp(grb48, rgb8_x257, 6) != 0);
  assert(rgb8_x257[0] == 0x34 && rgb8_x257[1] == 0x34);
  assert(rgb8_x257[2] == 0x12 && rgb8_x257[3] == 0x12);
  assert(rgb8_x257[4] == 0x56 && rgb8_x257[5] == 0x56);

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
  std::puts("K1_WS2816_PACK=PASS true16=PASS split=PASS "
            "grb48=34CD12AB56EF rgb8x257_distinct=PASS");
}
