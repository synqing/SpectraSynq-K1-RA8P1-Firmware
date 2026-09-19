#include "core/pixel.h"
#include "core/visual/frame_blend.h"
#include "core/visual/product_runtime_policy.h"

#include <cassert>
#include <cstddef>
#include <cstdio>

using k1::core::Pixel8;
using k1::core::PixelSpan;
using k1::core::visual::ProductRuntimePolicy;
using k1::core::visual::ProductRuntimePolicyState;
using k1::core::visual::applyFrameBlending;
using k1::core::visual::applyProductCurrentLimit;
using k1::core::visual::applyProductEdgePolicy;
using k1::core::visual::applyProductOutputGain;

int main() {
  Pixel8 primary[160]{};
  Pixel8 secondary[160]{};
  for (auto& pixel : primary) {
    pixel = {200U, 20U, 10U};
  }
  for (auto& pixel : secondary) {
    pixel = {200U, 20U, 10U};
  }
  ProductRuntimePolicy policy{};
  policy.edge_enabled = true;
  policy.edge_mode = 2U;
  policy.edge_strength = 1.0F;
  ProductRuntimePolicyState state{};
  applyProductEdgePolicy(policy, state, PixelSpan{primary, 160U},
                         PixelSpan{secondary, 160U});
  assert(primary[79].red >= 198U && primary[79].red <= 202U);
  assert(secondary[80].red >= 198U && secondary[80].red <= 202U);
  assert(primary[0].red != 200U);
  assert(secondary[159].red != 200U);

  Pixel8 hot_a[160]{};
  Pixel8 hot_b[160]{};
  for (auto& pixel : hot_a) {
    pixel = {255U, 255U, 255U};
  }
  for (auto& pixel : hot_b) {
    pixel = {255U, 255U, 255U};
  }
  applyProductCurrentLimit(PixelSpan{hot_a, 160U}, PixelSpan{hot_b, 160U}, 1000U);
  assert(hot_a[0].red < 255U);
  assert(hot_a[0].red == hot_b[0].red);

  Pixel8 disabled[2]{{255U, 64U, 32U}, {12U, 24U, 48U}};
  Pixel8 zero_master[2]{{255U, 64U, 32U}, {12U, 24U, 48U}};
  applyProductOutputGain(PixelSpan{disabled, 2U}, false, 65535U, 255U);
  applyProductOutputGain(PixelSpan{zero_master, 2U}, true, 0U, 255U);
  for (std::size_t i = 0; i < 2U; ++i) {
    assert(disabled[i].red == 0U && disabled[i].green == 0U && disabled[i].blue == 0U);
    assert(zero_master[i].red == 0U && zero_master[i].green == 0U &&
           zero_master[i].blue == 0U);
  }

  Pixel8 unity[1]{{200U, 100U, 50U}};
  applyProductOutputGain(PixelSpan{unity, 1U}, true, 32768U, 255U);
  assert(unity[0].red >= 99U && unity[0].red <= 101U);

  Pixel8 current[2]{{10U, 20U, 30U}, {40U, 50U, 60U}};
  Pixel8 previous[2]{{0U, 0U, 0U}, {0U, 0U, 0U}};
  applyFrameBlending(PixelSpan{current, 2U}, PixelSpan{previous, 2U}, 0U, 0.008F);
  assert(previous[0].red == 10U && previous[1].blue == 60U);

  Pixel8 scaled[2]{{10U, 0U, 0U}, {10U, 0U, 0U}};
  applyProductOutputGain(PixelSpan{scaled, 2U}, true, 65535U, 255U);
  assert(scaled[0].red == 10U);

  std::puts("K1_OUTPUT_COMPOSITION=PASS");
  return 0;
}
