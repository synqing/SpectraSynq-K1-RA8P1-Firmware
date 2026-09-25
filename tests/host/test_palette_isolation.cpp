#include "palette_runtime.h"
#include "core/pixel.h"
#include "core/visual/product_palette.h"
#include "core/visual/visual_audio_frame.h"

#include <cassert>
#include <cstdio>
#include <cstring>

using namespace k1::contract;
using namespace k1::core::audio;
using namespace k1::core::visual;
using namespace k1::titan;
using k1::core::Pixel8;

struct Bundle {
  AudioFeaturesV1 audio{};
  TempoTrackerEvent tempo{};
  VisualWaveformHistory waveform{};
  VisualAudioFrameView view() const noexcept {
    return VisualAudioFrameView{audio, tempo, waveform, audio.publish_time_us};
  }
};

static void fill_musical(Bundle& b) noexcept {
  b.audio = AudioFeaturesV1{};
  b.audio.sequence = 1U;
  b.audio.peak_scaled = 0.80F;
  b.audio.vu_level = 0.80F;
  b.audio.event_flags = 0U;
  b.audio.validity_flags = kValidBaseSnapshot | kValidChroma | kValidSpectrum;
  b.audio.chroma_a_origin[0] = 1.0F;
  b.audio.spectrum[7] = 1.0F;
  b.audio.publish_time_us = 1000;
  b.waveform.peak_scaled = 0.80F;
  b.waveform.raw_maximum = 0.80F;
}

static void fill_quiet(Bundle& b) noexcept {
  b.audio = AudioFeaturesV1{};
  b.audio.sequence = 1U;
  b.audio.peak_scaled = 0.01F;
  b.audio.vu_level = 0.01F;
  b.audio.event_flags = kEventSilence;
  b.audio.publish_time_us = 1000;
}

static void copy_frame(const ChannelRenderState& ch, Pixel8 dest[160]) noexcept {
  const auto frame = ch.frame();
  for (unsigned i = 0; i < 160U; ++i) dest[i] = frame[i];
}

static unsigned differ(const Pixel8 a[160], const Pixel8 b[160]) noexcept {
  unsigned n = 0;
  for (unsigned i = 0; i < 160U; ++i) {
    if (a[i].red != b[i].red || a[i].green != b[i].green || a[i].blue != b[i].blue)
      ++n;
  }
  return n;
}

static bool all_black(const ChannelRenderState& ch) noexcept {
  const auto frame = ch.frame();
  for (unsigned i = 0; i < 160U; ++i) {
    if (frame[i].red || frame[i].green || frame[i].blue) return false;
  }
  return true;
}

static PaletteConfig make_cfg(std::uint32_t palette, std::uint32_t mode) noexcept {
  PaletteConfig c;
  c.palette_a = palette;
  c.palette_b = palette;
  c.mode_a = mode;
  c.mode_b = mode;
  c.flags = 1U;
  c.brightness = 255U;
  return c;
}

static void step_n(PaletteRuntime& runtime, Bundle& bundle, unsigned frames,
                   unsigned start = 0U) noexcept {
  for (unsigned n = 0; n < frames; ++n) {
    bundle.audio.sequence = start + n + 1U;
    const auto view = bundle.view();
    runtime.step(static_cast<std::uint64_t>(start + n) * kPalettePeriodUs, &view);
  }
}

int main() {
#ifndef K1_LIVE_RUNTIME
  std::puts("PALETTE_ISOLATION_SKIP no_live_runtime");
  return 0;
#else
  Pixel8 first[160];
  Pixel8 second[160];
  Pixel8 again[160];

  // Preview: nullptr audio, identical time. Palette must paint the plate.
  {
    PaletteRuntime a;
    PaletteRuntime b;
    assert(a.configure(make_cfg(5U, 0U), 0U));
    assert(b.configure(make_cfg(33U, 0U), 0U));
    assert(a.step(0U, nullptr));
    assert(b.step(0U, nullptr));
    assert(std::strcmp(a.visualPath(), "preview") == 0);
    copy_frame(a.channel(0), first);
    copy_frame(b.channel(0), second);
    assert(differ(first, second) >= 40U);
    PaletteRuntime a2;
    assert(a2.configure(make_cfg(5U, 0U), 0U));
    assert(a2.step(0U, nullptr));
    copy_frame(a2.channel(0), again);
    assert(differ(first, again) == 0U);
    for (unsigned i = 0; i < 160U; ++i) {
      const unsigned radial = i < 80U ? 79U - i : i - 80U;
      const auto expected = sampleProductPaletteFastLed16(
          5U, static_cast<std::uint8_t>(radial * 255U / 79U));
      assert(!std::memcmp(&first[i], &expected, sizeof(Pixel8)));
    }
    std::puts("PALETTE_ISOLATION_PREVIEW_PASS");
  }

  // Effect: same musical snapshot, same now_us, two palettes.
  {
    Bundle musical;
    fill_musical(musical);
    PaletteRuntime a;
    PaletteRuntime b;
    assert(a.configure(make_cfg(5U, 32U), 0U));
    assert(b.configure(make_cfg(33U, 32U), 0U));
    step_n(a, musical, 48U);
    fill_musical(musical);
    step_n(b, musical, 48U);
    assert(std::strcmp(a.visualPath(), "effect") == 0);
    copy_frame(a.channel(0), first);
    copy_frame(b.channel(0), second);
    assert(differ(first, second) >= 8U);
    PaletteRuntime a2;
    fill_musical(musical);
    assert(a2.configure(make_cfg(5U, 32U), 0U));
    step_n(a2, musical, 48U);
    copy_frame(a2.channel(0), again);
    assert(differ(first, again) == 0U);
    std::puts("PALETTE_ISOLATION_EFFECT_PASS");
  }

  // Same runtime, palette retune, audio and time held after the cut.
  {
    Bundle musical;
    fill_musical(musical);
    PaletteRuntime runtime;
    assert(runtime.configure(make_cfg(5U, 32U), 0U));
    step_n(runtime, musical, 48U);
    copy_frame(runtime.channel(0), first);
    fill_musical(musical);
    assert(runtime.configure(make_cfg(33U, 32U), 48U * kPalettePeriodUs));
    step_n(runtime, musical, 48U, 48U);
    copy_frame(runtime.channel(0), second);
    assert(differ(first, second) >= 8U);
    std::puts("PALETTE_ISOLATION_RETUNE_PASS");
  }

  // Hold: musical then quiet inside the 5 s dwell. Still renderProductChannel.
  {
    Bundle musical;
    fill_musical(musical);
    Bundle quiet;
    fill_quiet(quiet);
    PaletteRuntime a;
    PaletteRuntime b;
    assert(a.configure(make_cfg(5U, 32U), 0U));
    assert(b.configure(make_cfg(33U, 32U), 0U));
    step_n(a, musical, 40U);
    fill_musical(musical);
    step_n(b, musical, 40U);
    const auto dead = quiet.view();
    assert(a.step(40U * kPalettePeriodUs, &dead));
    assert(b.step(40U * kPalettePeriodUs, &dead));
    assert(std::strcmp(a.visualPath(), "hold") == 0);
    copy_frame(a.channel(0), first);
    copy_frame(b.channel(0), second);
    assert(differ(first, second) >= 8U);
    std::puts("PALETTE_ISOLATION_HOLD_PASS");
  }

  // ChannelVisualControls reach the product renderer. Audio and time held.
  {
    Bundle musical;
    fill_musical(musical);
    PaletteRuntime a;
    PaletteRuntime b;
    assert(a.configure(make_cfg(5U, 32U), 0U));
    assert(b.configure(make_cfg(5U, 32U), 0U));
    auto& ca = a.channel(0).controls();
    auto& cb = b.channel(0).controls();
    ca.hue_position = 0.0F;
    cb.hue_position = 0.37F;
    ca.chroma = 0.0F;
    cb.chroma = 0.0F;
    ca.auto_colour_shift = cb.auto_colour_shift = true;
    ca.palette_mode_enabled = cb.palette_mode_enabled = true;
    step_n(a, musical, 48U);
    fill_musical(musical);
    cb.hue_position = 0.37F;
    cb.auto_colour_shift = true;
    cb.palette_mode_enabled = true;
    step_n(b, musical, 48U);
    copy_frame(a.channel(0), first);
    copy_frame(b.channel(0), second);
    assert(differ(first, second) >= 8U);
    std::puts("PALETTE_ISOLATION_VISUAL_CONTROLS_PASS");
  }

  // Spectrum-bin gains reach focused audio consumed by the renderer.
  {
    Bundle musical;
    fill_musical(musical);
    const auto view = musical.view();
    PaletteRuntime runtime;
    assert(runtime.configure(make_cfg(5U, 32U), 0U));
    auto& focus = runtime.channel(0).audioFocus();
    for (unsigned i = 0; i < 80U; ++i) focus.spectrum_bin_gain[i] = 0.0F;
    focus.spectrum_bin_gain[7] = 4.0F;
    assert(runtime.step(0U, &view));
    const auto& focused = runtime.channel(0).focusedAudio();
    assert(focused.spectrum[7] > 1.5F);
    assert(focused.spectrum[0] == 0.0F);
    assert(focused.spectrum[8] == 0.0F);
    std::puts("PALETTE_ISOLATION_SPECTRUM_FOCUS_PASS");
  }

  // Host idle-preview after dwell expiry + fade. Target transition remains untested.
  {
    Bundle musical;
    fill_musical(musical);
    Bundle quiet;
    fill_quiet(quiet);
    const auto live = musical.view();
    const auto dead = quiet.view();
    PaletteRuntime runtime;
    assert(runtime.configure(make_cfg(5U, 32U), 0U));
    assert(runtime.step(0U, &live));
    assert(std::strcmp(runtime.visualPath(), "effect") == 0);
    std::uint64_t now = kTitanSilenceDwellUs + kPalettePeriodUs;
    assert(runtime.step(now, &dead));
    bool saw_preview = std::strcmp(runtime.visualPath(), "preview") == 0;
    if (!saw_preview) {
      for (unsigned n = 0; n < 8U && !all_black(runtime.channel(0)); ++n) {
        now = (kTitanSilenceDwellUs + kPalettePeriodUs) + (n + 1U) * 2000000U;
        runtime.step(now, &dead);
      }
      now = kTitanSilenceDwellUs + 20000000U;
      runtime.step(now, &dead);
      saw_preview = std::strcmp(runtime.visualPath(), "preview") == 0;
    }
    assert(saw_preview);
    copy_frame(runtime.channel(0), first);
    unsigned matched = 0;
    const unsigned offset = static_cast<unsigned>((now / 20000U) & 255U);
    for (unsigned i = 0; i < 160U; ++i) {
      const unsigned radial = i < 80U ? 79U - i : i - 80U;
      const auto expected = sampleProductPaletteFastLed16(
          5U, static_cast<std::uint8_t>(radial * 255U / 79U + 256U - offset));
      if (!std::memcmp(&first[i], &expected, sizeof(Pixel8))) ++matched;
    }
    assert(matched >= 80U);
    std::puts("PALETTE_ISOLATION_HOST_IDLE_PREVIEW_PASS");
    std::puts("PALETTE_ISOLATION_TARGET_IDLE_PREVIEW=untested");
  }

  std::puts("PALETTE_ISOLATION_PASS");
  return 0;
#endif
}
