#include "core/visual/fastled_colour_compat.h"

#include <cstdint>

namespace k1::core::visual {
namespace {

constexpr std::uint8_t kHueRed = 0U;
constexpr std::uint8_t kHueOrange = 32U;
constexpr std::uint8_t kHueYellow = 64U;
constexpr std::uint8_t kHueGreen = 96U;
constexpr std::uint8_t kHueAqua = 128U;
constexpr std::uint8_t kHueBlue = 160U;
constexpr std::uint8_t kHuePurple = 192U;
constexpr std::uint8_t kHuePink = 224U;

std::uint8_t scale8(const std::uint8_t input,
                    const std::uint8_t scale) noexcept {
  return static_cast<std::uint8_t>(
      (static_cast<std::uint16_t>(input) *
       static_cast<std::uint16_t>(scale + 1U)) >>
      8U);
}

std::uint8_t scale8Video(const std::uint8_t input,
                         const std::uint8_t scale) noexcept {
  return static_cast<std::uint8_t>(
      ((static_cast<std::uint16_t>(input) * scale) >> 8U) +
      ((input != 0U && scale != 0U) ? 1U : 0U));
}

std::uint8_t subtractSaturating(const int left, const int right) noexcept {
  return static_cast<std::uint8_t>(left > right ? left - right : 0);
}

std::uint8_t addSaturating8(const std::uint8_t left,
                            const std::uint16_t right) noexcept {
  const std::uint16_t sum = static_cast<std::uint16_t>(left) + right;
  return static_cast<std::uint8_t>(sum > 255U ? 255U : sum);
}

std::uint16_t integerSquareRoot(std::uint32_t value) noexcept {
  std::uint32_t result = 0U;
  std::uint32_t bit = 1UL << 30U;
  while (bit > value) {
    bit >>= 2U;
  }
  while (bit != 0U) {
    if (value >= result + bit) {
      value -= result + bit;
      result = (result >> 1U) + bit;
    } else {
      result >>= 1U;
    }
    bit >>= 2U;
  }
  return static_cast<std::uint16_t>(result);
}

}  // namespace

Pixel8 fastLedHsvToRgbRainbow(const Hsv8 colour) noexcept {
  const std::uint8_t hue = colour.hue;
  std::uint8_t offset8 = static_cast<std::uint8_t>(hue & 0x1FU);
  offset8 = static_cast<std::uint8_t>(offset8 << 3U);
  const std::uint8_t third = scale8(offset8, 85U);
  std::uint8_t red = 0U;
  std::uint8_t green = 0U;
  std::uint8_t blue = 0U;
  if ((hue & 0x80U) == 0U) {
    if ((hue & 0x40U) == 0U) {
      if ((hue & 0x20U) == 0U) {
        red = static_cast<std::uint8_t>(255U - third);
        green = third;
      } else {
        red = 171U;
        green = static_cast<std::uint8_t>(85U + third);
      }
    } else if ((hue & 0x20U) == 0U) {
      const std::uint8_t two_thirds = scale8(offset8, 170U);
      red = static_cast<std::uint8_t>(171U - two_thirds);
      green = static_cast<std::uint8_t>(170U + third);
    } else {
      green = static_cast<std::uint8_t>(255U - third);
      blue = third;
    }
  } else if ((hue & 0x40U) == 0U) {
    if ((hue & 0x20U) == 0U) {
      const std::uint8_t two_thirds = scale8(offset8, 170U);
      green = static_cast<std::uint8_t>(171U - two_thirds);
      blue = static_cast<std::uint8_t>(85U + two_thirds);
    } else {
      red = third;
      blue = static_cast<std::uint8_t>(255U - third);
    }
  } else if ((hue & 0x20U) == 0U) {
    red = static_cast<std::uint8_t>(85U + third);
    blue = static_cast<std::uint8_t>(171U - third);
  } else {
    red = static_cast<std::uint8_t>(170U + third);
    blue = static_cast<std::uint8_t>(85U - third);
  }

  if (colour.saturation != 255U) {
    if (colour.saturation == 0U) {
      red = 255U;
      green = 255U;
      blue = 255U;
    } else {
      std::uint8_t desaturation =
          static_cast<std::uint8_t>(255U - colour.saturation);
      desaturation = scale8Video(desaturation, desaturation);
      const std::uint8_t saturation_scale =
          static_cast<std::uint8_t>(255U - desaturation);
      red = scale8(red, saturation_scale);
      green = scale8(green, saturation_scale);
      blue = scale8(blue, saturation_scale);
      red = static_cast<std::uint8_t>(red + desaturation);
      green = static_cast<std::uint8_t>(green + desaturation);
      blue = static_cast<std::uint8_t>(blue + desaturation);
    }
  }
  if (colour.value != 255U) {
    const std::uint8_t value = scale8Video(colour.value, colour.value);
    if (value == 0U) {
      red = 0U;
      green = 0U;
      blue = 0U;
    } else {
      red = scale8(red, value);
      green = scale8(green, value);
      blue = scale8(blue, value);
    }
  }
  return {red, green, blue};
}

Hsv8 rgbToFastLedHsvApproximate(const Pixel8 colour) noexcept {
  std::uint8_t red = colour.red;
  std::uint8_t green = colour.green;
  std::uint8_t blue = colour.blue;
  std::uint8_t desaturation = red;
  if (green < desaturation) {
    desaturation = green;
  }
  if (blue < desaturation) {
    desaturation = blue;
  }
  red = static_cast<std::uint8_t>(red - desaturation);
  green = static_cast<std::uint8_t>(green - desaturation);
  blue = static_cast<std::uint8_t>(blue - desaturation);
  std::uint8_t saturation = static_cast<std::uint8_t>(255U - desaturation);
  if (saturation != 255U) {
    saturation = static_cast<std::uint8_t>(
        255U - integerSquareRoot((255U - saturation) * 256U));
  }
  if (static_cast<std::uint16_t>(red) + green + blue == 0U) {
    return {0U, 0U, static_cast<std::uint8_t>(255U - saturation)};
  }
  if (saturation < 255U) {
    if (saturation == 0U) {
      saturation = 1U;
    }
    const std::uint32_t scale_up = 65535U / saturation;
    red = static_cast<std::uint8_t>((red * scale_up) / 256U);
    green = static_cast<std::uint8_t>((green * scale_up) / 256U);
    blue = static_cast<std::uint8_t>((blue * scale_up) / 256U);
  }
  std::uint16_t total = static_cast<std::uint16_t>(red) + green + blue;
  if (total < 255U) {
    if (total == 0U) {
      total = 1U;
    }
    const std::uint32_t scale_up = 65535U / total;
    red = static_cast<std::uint8_t>((red * scale_up) / 256U);
    green = static_cast<std::uint8_t>((green * scale_up) / 256U);
    blue = static_cast<std::uint8_t>((blue * scale_up) / 256U);
  }
  const std::uint8_t value = total > 255U
                                 ? 255U
                                 : (addSaturating8(desaturation, total) == 255U
                                        ? 255U
                                        : static_cast<std::uint8_t>(
                                              integerSquareRoot(
                                                  addSaturating8(desaturation,
                                                                total) *
                                                  256U)));
  std::uint8_t highest = red;
  if (green > highest) {
    highest = green;
  }
  if (blue > highest) {
    highest = blue;
  }
  std::uint8_t hue = 0U;
  if (highest == red) {
    if (green == 0U) {
      hue = static_cast<std::uint8_t>((kHuePurple + kHuePink) / 2U);
      hue = static_cast<std::uint8_t>(
          hue + scale8(subtractSaturating(red, 128), 96U));
    } else if (static_cast<std::uint8_t>(red - green) > green) {
      hue = static_cast<std::uint8_t>(
          kHueRed + scale8(green, static_cast<std::uint8_t>((32U * 256U) / 85U)));
    } else {
      hue = static_cast<std::uint8_t>(
          kHueOrange +
          scale8(subtractSaturating(
                     static_cast<std::uint8_t>((green - 85U) + (171U - red)),
                     4),
                 static_cast<std::uint8_t>((32U * 256U) / 85U)));
    }
  } else if (highest == green) {
    if (blue == 0U) {
      const std::uint8_t red_adjust =
          scale8(subtractSaturating(171, red), 47U);
      const std::uint8_t green_adjust =
          scale8(subtractSaturating(green, 171), 96U);
      hue = static_cast<std::uint8_t>(
          kHueYellow + static_cast<std::uint8_t>(red_adjust + green_adjust) /
                           2U);
    } else if (static_cast<std::uint8_t>(green - blue) > blue) {
      hue = static_cast<std::uint8_t>(
          kHueGreen +
          scale8(blue, static_cast<std::uint8_t>((32U * 256U) / 85U)));
    } else {
      hue = static_cast<std::uint8_t>(
          kHueAqua +
          scale8(subtractSaturating(blue, 85),
                 static_cast<std::uint8_t>((8U * 256U) / 42U)));
    }
  } else if (red == 0U) {
    hue = static_cast<std::uint8_t>(
        kHueAqua + ((kHueBlue - kHueAqua) / 4U) +
        scale8(subtractSaturating(blue, 128), 48U));
  } else if (static_cast<std::uint8_t>(blue - red) > red) {
    hue = static_cast<std::uint8_t>(
        kHueBlue +
        scale8(red, static_cast<std::uint8_t>((32U * 256U) / 85U)));
  } else {
    hue = static_cast<std::uint8_t>(
        kHuePurple +
        scale8(subtractSaturating(red, 85),
               static_cast<std::uint8_t>((32U * 256U) / 85U)));
  }
  return {static_cast<std::uint8_t>(hue + 1U), saturation, value};
}

Pixel8 forceFastLedSaturation(const Pixel8 colour,
                              const std::uint8_t saturation) noexcept {
  Hsv8 hsv = rgbToFastLedHsvApproximate(colour);
  hsv.saturation = saturation;
  return fastLedHsvToRgbRainbow(hsv);
}

Pixel8 forceFastLedHue(const Pixel8 colour, const std::uint8_t hue) noexcept {
  Hsv8 hsv = rgbToFastLedHsvApproximate(colour);
  hsv.hue = hue;
  return fastLedHsvToRgbRainbow(hsv);
}

}  // namespace k1::core::visual
