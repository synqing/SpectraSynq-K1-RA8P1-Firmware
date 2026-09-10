#include "palette_runtime.h"
#include "core/visual/product_palette.h"
#include "core/visual/product_effect_renderer.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace k1;
using namespace k1::core;
using namespace k1::core::visual;
using namespace k1::titan;
int main() {
  static_assert(kProductPaletteCount == 44U);
  char text[8192];
  PaletteRuntime catalogue;
  assert(catalogue.catalogueJson(text, sizeof(text)));
  for (const auto& entry : productPaletteCatalogue()) assert(std::strstr(text, entry.name));
  assert(std::strstr(text, "\"id\":43"));
  char guard[]{'x','x','x','x'};
  assert(catalogue.catalogueJson(guard, 2U) == 0U && guard[2] == 'x');
  assert(catalogue.catalogueJson(nullptr, 1U) == 0U);
  unsigned compared = 0;
  for (unsigned id = 0; id < kProductPaletteCount; ++id) {
    PaletteRuntime runtime;
    PaletteConfig c; c.palette_a = id; c.palette_b = 43U-id;
    c.flags = 1U; c.brightness = 255U;
    assert(runtime.configure(c, 0U)); assert(runtime.step(0U, nullptr));
    for (unsigned ch = 0; ch < 2; ++ch) {
      const auto frame = runtime.channel(ch).frame();
      const auto selected = ch ? 43U-id : id;
      assert(runtime.channel(ch).controls().palette_mode_enabled);
      for (unsigned i = 0; i < 160; ++i) {
        const auto expected = sampleProductPaletteFastLed16(selected, (i<80U?79U-i:i-80U)*255U/79U);
        assert(!std::memcmp(&frame[i], &expected, sizeof(Pixel8)));
        ++compared;
      }
    }
    std::uint8_t wire[384];
    assert(runtime.packBenchGrb(wire, sizeof(wire)) == sizeof(wire));
    for (unsigned i : {0U,63U,64U,127U}) {
      const unsigned index = i==0 ? 0 : i==63 ? 79 : i==64 ? 80 : 159;
      const auto p = runtime.channel(0).frame()[index];
      assert(wire[3*i] == p.green && wire[3*i+1] == p.red && wire[3*i+2] == p.blue);
    }
    assert(!runtime.packBenchGrb(wire, sizeof(wire)-1));
    auto invalid = c; invalid.palette_b = 44;
    assert(!runtime.configure(invalid, 1));
    assert(runtime.config().palette_b == c.palette_b);
    invalid = c; invalid.version = 0; assert(!runtime.configure(invalid, 1));
    invalid = c; invalid.flags = 8; assert(!runtime.configure(invalid, 1));
    invalid = c; invalid.output_channel = 2; assert(!runtime.configure(invalid, 1));
    invalid = c; invalid.mode_a = 65536U; assert(!runtime.configure(invalid, 1));
    runtime.stop(); assert(!runtime.step(1000000U, nullptr));
  }
  PaletteConfig cycle; cycle.flags = 3U;
  assert(catalogue.configure(cycle, 1000U));
  for (unsigned i = 0; i <= 44; ++i) {
    assert(catalogue.step(1000ULL + i*4000000ULL, nullptr));
    assert(catalogue.channel(0).controls().palette_id == i%44);
    assert(catalogue.channel(1).controls().palette_id == (i+1)%44);
  }
  // Existing renderer + treatment is the independent composition reference.
  contract::AudioFeaturesV1 audio;
  audio.validity_flags = contract::kValidBaseSnapshot | contract::kValidSpectrum;
  audio.event_flags = contract::kEventBeat | contract::kEventOnset;
  audio.peak_scaled = 0.7F; audio.vu_level = 0.6F; audio.novelty = 0.5F;
  audio.low_energy = audio.mid_energy = audio.high_energy = 0.5F;
  audio.beat_phase = 0.25F; audio.beat_confidence = 0.9F;
  audio.onset_strength = audio.bass_onset_strength = 0.8F;
  audio.transient_strength = audio.kick_strength = 0.7F;
  audio.nyquist_safe_bin_hi = 79U;
  for (unsigned i=0; i<80; ++i) audio.spectrum[i] = float(i+1)/80.0F;
  core::audio::TempoTrackerEvent tempo{};
  tempo.bpm = 120.0F; tempo.phase01 = 0.25F; tempo.confidence = 0.9F; tempo.locked = true;
  VisualWaveformHistory wave{};
  const VisualAudioFrameView view{audio, tempo, wave, 0U};
  constexpr unsigned modes[]{3,7,8,9,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,28,29,32};
  unsigned effect_frames = 0, visible_frames = 0;
  std::uint64_t digest = 14695981039346656037ULL;
  for (auto mode : modes) for (unsigned id=0; id<44; ++id) {
    PaletteRuntime runtime;
    PaletteConfig c; c.palette_a=id; c.palette_b=43-id; c.mode_a=c.mode_b=mode; c.flags=1;
    assert(runtime.configure(c, 0));
    ChannelRenderState ref_a{PixelChannelId::kChannelA}, ref_b{PixelChannelId::kChannelB};
    for (auto* ch : {&ref_a, &ref_b}) {
      ch->controls().palette_mode_enabled = true;
      ch->controls().photons_id = 65535U;
      ch->controls().mode_id = mode;
      ch->controls().palette_id = ch == &ref_a ? id : 43-id;
    }
    for (unsigned frame=0; frame<4; ++frame) {
      const auto now = std::uint64_t(frame)*kPalettePeriodUs;
      const float dt = frame ? float(kPalettePeriodUs)/1000000.0F : 1.0F/120.0F;
      assert(runtime.step(now, &view));
      for (unsigned ch=0; ch<2; ++ch) {
        auto& ref = ch ? ref_b : ref_a;
        ref.prepareAudio(audio);
        assert(renderProductChannel(ref, view, dt).rendered);
        applyProductOutputTreatment(ref.frame(), ref.controls(), ref.outputTreatmentState());
        assert(!std::memcmp(runtime.channel(ch).frame().data(), ref.frame().data(), 160*sizeof(Pixel8)));
        bool visible=false;
        for (unsigned pixel_index=0; pixel_index<160; ++pixel_index) {
          const auto pixel = ref.frame()[pixel_index];
          const auto opposite = ref.frame()[159U-pixel_index];
          if (std::memcmp(&pixel,&opposite,sizeof(Pixel8))) {
            std::fprintf(stderr,"CENTRE_MIRROR_FAILURE mode=%u palette=%u pixel=%u\n",mode,id,pixel_index);
            assert(false);
          }
          visible |= pixel.red || pixel.green || pixel.blue;
          for (auto value : {pixel.red,pixel.green,pixel.blue}) {
            digest ^= value; digest *= 1099511628211ULL;
          }
        }
        visible_frames += visible;
        ++effect_frames;
      }
    }
  }
  assert(visible_frames > 0U);
  std::printf("COMPATIBILITY_DIGEST=%016llx visible_frames=%u\n", (unsigned long long)digest, visible_frames);
  std::printf("PALETTE_RUNTIME_PASS palettes=44 preview_pixels=%u effect_frames=%u modes=23\n", compared, effect_frames);
}
