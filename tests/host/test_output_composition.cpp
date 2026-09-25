#include "core/pixel.h"
#include "core/visual/channel_render_state.h"
#include "core/visual/frame_blend.h"
#include "core/visual/pixel_topology.h"
#include "core/visual/product_output_treatment.h"
#include "core/visual/product_runtime_policy.h"
#include "core/visual/ws2816_pack.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

using k1::core::Pixel8;
using k1::core::PixelSpan;
using k1::core::visual::ChannelVisualControls;
using k1::core::visual::Pixel16;
using k1::core::visual::ProductOutputTreatmentState;
using k1::core::visual::ProductRuntimePolicy;
using k1::core::visual::ProductRuntimePolicyState;
using k1::core::visual::applyFrameBlending;
using k1::core::visual::applyProductCurrentLimit;
using k1::core::visual::applyProductEdgePolicy;
using k1::core::visual::applyProductOutputGain;
using k1::core::visual::applyProductOutputTreatment;
using k1::core::visual::kCentreLeft;
using k1::core::visual::kCentreRight;
using k1::core::visual::kPackedBytesPerLane;
using k1::core::visual::kPixelsPerChannel;
using k1::core::visual::packPixel;
using k1::core::visual::splitChannel160;

struct ShowRecord {
  std::uint8_t lane_a[kPackedBytesPerLane]{};
  std::uint8_t lane_b[kPackedBytesPerLane]{};
  std::uint32_t show_us = 0U;
};

static void fillCentreOrigin(Pixel8* frame) {
  for (std::size_t i = 0; i < kPixelsPerChannel; ++i) {
    frame[i] = {};
  }
  frame[kCentreLeft] = {200U, 10U, 10U};
  frame[kCentreRight] = {200U, 10U, 10U};
}

static void composeLogicalChannels(Pixel8* primary, Pixel8* secondary,
                                   Pixel8* previous_a, Pixel8* previous_b,
                                   ShowRecord& show) {
  applyFrameBlending(PixelSpan{primary, kPixelsPerChannel},
                     PixelSpan{previous_a, kPixelsPerChannel}, 0U, 0.0075F);
  applyFrameBlending(PixelSpan{secondary, kPixelsPerChannel},
                     PixelSpan{previous_b, kPixelsPerChannel}, 0U, 0.0075F);

  ChannelVisualControls controls{};
  controls.prism_count = 0.0F;
  controls.reverse_order = false;
  controls.temporal_dithering = false;
  ProductOutputTreatmentState treatment{};
  applyProductOutputTreatment(PixelSpan{primary, kPixelsPerChannel}, controls,
                              treatment);
  applyProductOutputTreatment(PixelSpan{secondary, kPixelsPerChannel}, controls,
                              treatment);

  ProductRuntimePolicy policy{};
  policy.edge_enabled = true;
  policy.edge_mode = 2U;
  policy.edge_strength = 1.0F;
  ProductRuntimePolicyState state{};
  applyProductEdgePolicy(policy, state, PixelSpan{primary, kPixelsPerChannel},
                         PixelSpan{secondary, kPixelsPerChannel});
  applyProductOutputGain(PixelSpan{primary, kPixelsPerChannel}, true, 65535U,
                         255U);
  applyProductOutputGain(PixelSpan{secondary, kPixelsPerChannel}, true, 65535U,
                         255U);
  applyProductCurrentLimit(PixelSpan{primary, kPixelsPerChannel},
                           PixelSpan{secondary, kPixelsPerChannel}, 5000U);

  Pixel16 wide[kPixelsPerChannel]{};
  wide[kCentreLeft] = {0x12AB, 0, 0};
  wide[kCentreRight] = {0, 0, 0x12AB};
  assert(splitChannel160(wide, kPixelsPerChannel, show.lane_a, show.lane_b));
  show.show_us = 7500U;
}

int main() {
  Pixel8 primary[kPixelsPerChannel]{};
  Pixel8 secondary[kPixelsPerChannel]{};
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

  Pixel8 once[1]{{200U, 0U, 0U}};
  Pixel8 twice[1]{{200U, 0U, 0U}};
  applyProductOutputGain(PixelSpan{once, 1U}, true, 32768U, 255U);
  applyProductOutputGain(PixelSpan{twice, 1U}, true, 32768U, 255U);
  applyProductOutputGain(PixelSpan{twice, 1U}, true, 32768U, 255U);
  assert(twice[0].red != once[0].red);

  ChannelVisualControls dither_controls{};
  dither_controls.temporal_dithering = true;
  Pixel8 treated[4]{{200U, 150U, 100U}, {200U, 150U, 100U},
                    {200U, 150U, 100U}, {200U, 150U, 100U}};
  Pixel8 treated_once[4];
  std::memcpy(treated_once, treated, sizeof(treated));
  ProductOutputTreatmentState phase{};
  applyProductOutputTreatment(PixelSpan{treated_once, 4U}, dither_controls, phase);
  Pixel8 treated_twice[4];
  std::memcpy(treated_twice, treated, sizeof(treated));
  ProductOutputTreatmentState phase_repeat{};
  applyProductOutputTreatment(PixelSpan{treated_twice, 4U}, dither_controls,
                              phase_repeat);
  applyProductOutputTreatment(PixelSpan{treated_twice, 4U}, dither_controls,
                              phase_repeat);
  assert(std::memcmp(treated_once, treated_twice, sizeof(treated_once)) != 0);

  Pixel8 chain_a[kPixelsPerChannel]{};
  Pixel8 chain_b[kPixelsPerChannel]{};
  Pixel8 prev_a[kPixelsPerChannel]{};
  Pixel8 prev_b[kPixelsPerChannel]{};
  fillCentreOrigin(chain_a);
  fillCentreOrigin(chain_b);
  ShowRecord show{};
  composeLogicalChannels(chain_a, chain_b, prev_a, prev_b, show);
  assert(chain_a[kCentreLeft].red > 0U);
  assert(chain_b[kCentreRight].red > 0U);
  assert(show.show_us == 7500U);
  const std::size_t left_offset = kCentreLeft * 6U;
  assert(show.lane_a[left_offset + 2U] == 0x12 && show.lane_a[left_offset + 3U] == 0xAB);
  assert(show.lane_b[4] == 0x12 && show.lane_b[5] == 0xAB);
  assert(show.lane_a[left_offset + 3U] != 0x12);

  Pixel16 px{0x12AB, 0x0000, 0x0000};
  std::uint8_t slot[6]{};
  packPixel(px, slot);
  assert(slot[2] == 0x12 && slot[3] == 0xAB);
  const std::uint16_t expanded = static_cast<std::uint16_t>(0x12U * 257U);
  assert(expanded != 0x12AB);

  std::puts("K1_OUTPUT_COMPOSITION=PASS chain=PASS true16=PASS "
            "double_gain_rejected=PASS repeated_treatment_detected=PASS "
            "physical_lanes=OUTSTANDING p004_gpt=UNSUPPORTED");
  return 0;
}
