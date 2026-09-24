#include "palette_runtime.h"
#include "core/visual/product_palette.h"
#include "core/visual/product_effect_renderer.h"
#include <cassert>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <cstdio>
using namespace k1;
using namespace k1::core;
using namespace k1::core::visual;
using namespace k1::titan;
int main() {
  static_assert(kProductPaletteCount == 44U);
#ifndef K1_SNAP_FREEZE_ONLY
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
      if (p.green != p.red) assert(wire[3*i] != p.red);
    }
    assert(!runtime.packBenchGrb(wire, sizeof(wire)-1));
    // All native positions survive the two-DIN GRB48 mapping, without resampling.
    for(unsigned ch=0; ch<2; ++ch) for(unsigned brightness : {0U,128U,255U}) {
      auto transport=c; transport.output_channel=ch; transport.brightness=brightness;
      assert(runtime.configure(transport,0U)); assert(runtime.step(0U,nullptr));
      for(unsigned lane=0; lane<2; ++lane) {
        std::uint8_t packed[480];
        assert(runtime.packBenchGrb48Lane(packed,sizeof(packed),lane)==480);
        for(unsigned i=0; i<80; ++i) {
          const auto p=runtime.channel(ch).frame()[lane*80+i];
          const unsigned components[]{p.green,p.red,p.blue};
          for(unsigned component=0; component<3; ++component) {
            const unsigned value=(unsigned(packed[i*6+component*2])<<8)|packed[i*6+component*2+1];
            assert(value==components[component]*257U*brightness/255U);
          }
        }
        std::memset(packed,0xa5,sizeof(packed));
        assert(!runtime.packBenchGrb48Lane(packed,sizeof(packed)-1,lane));
        assert(!runtime.packBenchGrb48Lane(packed,sizeof(packed),2));
        assert(!runtime.packBenchGrb48Lane(nullptr,sizeof(packed),lane));
        for(auto byte:packed) assert(byte==0xa5);
      }
    }
    // Round 4 (ORCH) acceptance A1-A4, through PaletteRuntime::step() (not
    // the endpoint alone). A6 (test_hd_pixel16 from its runner) lives in
    // scripts/test_hd_pixel16.py, unrelated to this file.
    {
      using core::visual::wide::WideCaptureProvenanceV1;
      // A1: use_wide_native16=false gives packed bytes identical to
      // packBenchGrb48Lane, whether or not use_wide_route is also on (mode 0
      // here is outside kWideRouteAdmittedModesV1 regardless).
      for (bool route : {false, true}) {
        auto transport = c; transport.output_channel = 0U; transport.brightness = 200U;
        transport.use_wide_native16 = false; transport.use_wide_route = route;
        assert(runtime.configure(transport, 0U)); assert(runtime.step(0U, nullptr));
        for (unsigned lane = 0; lane < 2; ++lane) {
          std::uint8_t legacy[480], selected[480];
          assert(runtime.packBenchGrb48Lane(legacy, sizeof(legacy), lane) == 480);
          assert(runtime.packNative16Lane(selected, sizeof(selected), lane,
                                          &runtime.wideFrame(0U)) == 480);
          assert(!std::memcmp(legacy, selected, sizeof(legacy)));
        }
      }
      std::printf("WIDE_ROUTE_A1_PASS default_off_identical=true routes_checked=2\n");

      // A4: with use_wide_native16=false, route on and route off keep the
      // legacy Pixel8 behaviour identically (mode 0/preview is outside
      // kWideRouteAdmittedModesV1, so this is expected -- confirms A1's
      // route=false/route=true cases agree with each other, not just each
      // with the legacy pack).
      {
        auto route_off = c; route_off.output_channel = 0U;
        route_off.use_wide_native16 = false; route_off.use_wide_route = false;
        auto route_on = route_off; route_on.use_wide_route = true;
        assert(runtime.configure(route_off, 0U)); assert(runtime.step(0U, nullptr));
        Pixel8 frame_off[160];
        std::memcpy(frame_off, runtime.channel(0).frame().data(), sizeof(frame_off));
        assert(runtime.configure(route_on, 0U)); assert(runtime.step(0U, nullptr));
        assert(!std::memcmp(frame_off, runtime.channel(0).frame().data(), sizeof(frame_off)));
        std::printf("WIDE_ROUTE_A4_PASS native16_off_route_toggle_identical=true\n");
      }

      // A2/A3: route on, native16 on, mode 3 (Bloom), driven with real
      // audio through step() (not a hand-built frame).
      {
        contract::AudioFeaturesV1 features{};
        audio::TempoTrackerEvent tempo{};
        VisualWaveformHistory waveform{};
        auto live = c; live.output_channel = 0U; live.mode_a = 3U; live.mode_b = 3U;
        live.flags = 1U; live.use_wide_route = true; live.use_wide_native16 = true;
        live.brightness = 255U;
        assert(runtime.configure(live, 0U));
        // Force the chromatic injection path (byte-domain HSV, "a declared
        // narrowing adapter, scaled afterwards in float" per wide_bloom.h) --
        // the palette path instead ports paletteColour() operation for
        // operation, which reproduces legacy byte output exactly and so never
        // carries extra precision at injection.
        runtime.channel(0U).controls().palette_mode_enabled = false;
        runtime.channel(1U).controls().palette_mode_enabled = false;
        const auto a5_start = std::clock();
        for (unsigned frame = 0; frame < 90; ++frame) {
          // Continuously time-varying level (never a fixed-point repeat), so
          // Bloom's exponential retention (alpha^frames) actually produces a
          // spread of distinct floats across the strip instead of settling
          // into one repeated 8-bit-exact injected value everywhere.
          features.peak_scaled = 0.5F + 0.45F * std::sin(float(frame) * 0.37F);
          features.vu_level = 0.5F + 0.45F * std::cos(float(frame) * 0.53F);
          // Colour/injection amplitude comes from chroma_a_origin, NOT
          // peak_scaled/vu_level (those only gate musical-presence/keep_live
          // in k1MusicalPresence) -- see chromaticColour()/paletteColour()
          // in product_effect_renderer.cpp. A left-zeroed chroma vector
          // renders exactly black regardless of peak/vu, which is what an
          // earlier version of this test discovered the hard way.
          for (unsigned bin = 0; bin < contract::kChromaBinCount; ++bin) {
            features.chroma_a_origin[bin] =
                0.5F + 0.45F * std::sin(float(frame) * 0.29F + float(bin) * 0.8F);
          }
          const VisualAudioFrameView view{features, tempo, waveform, 0U};
          assert(runtime.step(std::uint64_t(frame) * 20000U, &view));
        }
        // A5: no allocation in step() (by construction -- wide_workspace_,
        // wide_a_/wide_b_, wide_route_a_/wide_route_b_ are all
        // PaletteRuntime members, not step()-local). Host cost figure
        // (labelled host, not a target timing claim): both channels,
        // use_wide_native16 on, per step() call including the legacy
        // Pixel8 render/treatment this cycle still runs too.
        const double a5_host_us_per_step =
            1e6 * double(std::clock() - a5_start) / double(CLOCKS_PER_SEC) / 90.0;
        std::printf("WIDE_ROUTE_A5_HOST_COST host_us_per_step=%.2f label=host\n", a5_host_us_per_step);
        assert(runtime.provenance(0U) == WideCaptureProvenanceV1::kWideRenderer);
        const auto pixels = runtime.channel(0).frame();
        const auto& native = runtime.wideFrame(0U);
        bool offlattice = false, samebyte_diffnative = false;
        for (unsigned i = 0; i < 160 && !offlattice; ++i) {
          const auto& n = native[i];
          if (n.red % 257U || n.green % 257U || n.blue % 257U) offlattice = true;
        }
        for (unsigned i = 0; i < 160 && !samebyte_diffnative; ++i) {
          for (unsigned j = i + 1; j < 160; ++j) {
            if (pixels[i].red == pixels[j].red && pixels[i].green == pixels[j].green &&
                pixels[i].blue == pixels[j].blue &&
                (native[i].red != native[j].red || native[i].green != native[j].green ||
                 native[i].blue != native[j].blue)) {
              samebyte_diffnative = true;
              break;
            }
          }
        }
        assert(offlattice);
        assert(samebyte_diffnative);
        // Titan's incumbent current limiting: none (see endpointConfig()'s
        // comment in palette_runtime.h). Proof: even this near-saturated
        // centre-injected render never trips the limiter.
        assert(runtime.endpointReport().limiter_scale == 1.0F);
        assert(runtime.endpointReport().channel[0].nonfinite == 0U);
        assert(runtime.endpointReport().channel[1].nonfinite == 0U);
        std::printf("WIDE_ROUTE_A2_PASS mode3_routed=true offlattice_words=true "
                    "samebyte_diffnative_pair=true limiter_inert=true\n");

        // A3 negative case: force capture to lift Pixel8 by re-rendering
        // mode 0 (preview, outside kWideRouteAdmittedModesV1) so
        // captureRenderedFrameV1 falls back to kLiftedPixel8 -- then the
        // off-lattice claim must go red (a lifted Pixel8 word is always an
        // exact multiple of 257), proving A2 is not vacuous. Revert after.
        auto lifted = live; lifted.mode_a = 0U; lifted.mode_b = 0U;
        assert(runtime.configure(lifted, 0U)); assert(runtime.step(0U, nullptr));
        assert(runtime.provenance(0U) == WideCaptureProvenanceV1::kLiftedPixel8);
        bool lifted_offlattice = false;
        for (unsigned i = 0; i < 160; ++i) {
          const auto& n = runtime.wideFrame(0U)[i];
          if (n.red % 257U || n.green % 257U || n.blue % 257U) { lifted_offlattice = true; break; }
        }
        assert(!lifted_offlattice);  // A2's own claim, correctly, does not hold here
        assert(runtime.configure(live, 0U));  // revert to the routed config
        std::printf("WIDE_ROUTE_A3_MUTATION_PASS forced_lift_stays_on_lattice=true reverted=true\n");
      }
    }
    assert(runtime.configure(c,0U));
    auto invalid = c; invalid.palette_b = 44;
    assert(!runtime.configure(invalid, 1));
    assert(runtime.config().palette_b == c.palette_b);
    invalid = c; invalid.version = 0; assert(!runtime.configure(invalid, 1));
    invalid = c; invalid.flags = 8; assert(!runtime.configure(invalid, 1));
    invalid = c; invalid.output_channel = 2; assert(!runtime.configure(invalid, 1));
    invalid = c; invalid.mode_a = 65536U; assert(!runtime.configure(invalid, 1));
    runtime.stop(); assert(!runtime.step(1000000U, nullptr));
  }
  {
    PaletteRuntime bounce_runtime;
    PaletteConfig bounce;
    bounce.palette_a = 0U;
    bounce.palette_b = 1U;
    bounce.mode_a = kDiagnosticBounceMode;
    bounce.mode_b = kDiagnosticBounceMode;
    bounce.brightness = 255U;
    bounce.flags = 1U;
#ifdef K1_PALETTE_MORPH
    bounce.version = 3U;
    bounce.transition_ms = 0U;
#endif
    assert(bounce_runtime.configure(bounce, 0U));
    for (unsigned radius : {0U, 1U, 5U, 32U, 63U}) {
      assert(bounce_runtime.step(std::uint64_t(radius) * kDiagnosticBounceFrameUs, nullptr));
      std::uint8_t wire[384];
      assert(bounce_runtime.packBenchGrb(wire, sizeof(wire)) == sizeof(wire));
      for (unsigned i = 0; i < 128U; ++i) {
        const bool on = wire[i * 3U] || wire[i * 3U + 1U] || wire[i * 3U + 2U];
        bool expect = false;
        for (unsigned tail = 0; tail < 6U && tail <= radius; ++tail) {
          if (i == 63U - radius + tail || i == 64U + radius - tail) expect = true;
        }
        assert(on == expect);
      }
    }
    assert(bounce_runtime.configure(bounce, 0U));
    assert(bounce_runtime.step(0U, nullptr));
    std::uint8_t first[384], second[384];
    assert(bounce_runtime.packBenchGrb(first, sizeof(first)) == sizeof(first));
    bounce.palette_a = 2U;
    bounce.palette_b = 3U;
    assert(bounce_runtime.configure(bounce, 0U));
    assert(bounce_runtime.step(0U, nullptr));
    assert(bounce_runtime.packBenchGrb(second, sizeof(second)) == sizeof(second));
    for (unsigned i = 0; i < 128U; ++i) {
      const bool a = first[i * 3U] || first[i * 3U + 1U] || first[i * 3U + 2U];
      const bool b = second[i * 3U] || second[i * 3U + 1U] || second[i * 3U + 2U];
      assert(a == b);
    }
  }
  PaletteConfig cycle; cycle.flags = 3U;
  assert(catalogue.configure(cycle, 1000U));
  for (unsigned i = 0; i <= 44; ++i) {
    assert(catalogue.step(1000ULL + i*4000000ULL, nullptr));
    assert(catalogue.channel(0).controls().palette_id == i%44);
    assert(catalogue.channel(1).controls().palette_id == (i+1)%44);
  }
#endif
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
  {
    PaletteRuntime snap;
    PaletteConfig impulse_config;
    impulse_config.mode_a = 32U;
    impulse_config.mode_b = 32U;
    impulse_config.flags = 1U;
    impulse_config.brightness = 255U;
#ifdef K1_PALETTE_MORPH
    impulse_config.version = 3U;
#endif
    assert(snap.configure(impulse_config, 0U));
    contract::AudioFeaturesV1 impulse = audio;
    impulse.event_flags = 0U;
    impulse.validity_flags =
        contract::kValidBaseSnapshot | contract::kValidChroma |
        contract::kValidSpectrum;
    impulse.peak_scaled = 0.95F;
    impulse.vu_level = 0.90F;
    impulse.chroma_a_origin[0] = 0.90F;
    impulse.chroma_a_origin[7] = 0.35F;
    impulse.chroma_strength = 0.80F;
    VisualWaveformHistory impulse_wave{};
    impulse_wave.sample_count = 96U;
    impulse_wave.raw_maximum = 30000.0F;
    impulse_wave.peak_scaled = 0.95F;
    for (std::size_t history = 0U; history < kVisualWaveformHistoryFrames;
         ++history) {
      for (std::size_t sample = 0U; sample < impulse_wave.sample_count;
           ++sample) {
        impulse_wave.frames[history][sample] =
            static_cast<std::int16_t>((sample % 12U) * 2000);
      }
    }
    const VisualAudioFrameView impulse_view{impulse, tempo, impulse_wave, 0U};
    for (unsigned frame = 0; frame < 8U; ++frame) {
      assert(snap.step(std::uint64_t(frame) * kPalettePeriodUs, &impulse_view));
    }
    // Match the stuck silicon dump: max 255, then silence. Uncleared
    // transportOutward saturates in place; DualMCU clears first.
    for (unsigned ch = 0; ch < 2U; ++ch) {
      auto& seeded = const_cast<ChannelRenderState&>(snap.channel(ch));
      for (unsigned i = 0; i < 160U; ++i) {
        seeded.frame()[i] = Pixel8{255U, 255U, 255U};
        seeded.previousFrame()[i] = Pixel8{255U, 255U, 255U};
      }
    }
    assert(snap.configure(impulse_config, 8U * kPalettePeriodUs));
    for (unsigned ch = 0; ch < 2U; ++ch) {
      auto& seeded = const_cast<ChannelRenderState&>(snap.channel(ch));
      for (unsigned i = 0; i < 160U; ++i) {
        seeded.frame()[i] = Pixel8{255U, 255U, 255U};
        seeded.previousFrame()[i] = Pixel8{255U, 255U, 255U};
      }
    }
    // Silence is dwell-aging, not the mutated live clear. Keep peak musical so
    // step() takes prepareAudio/clearFrame; uncleared add saturates at 255.
    for (unsigned frame = 8U; frame < 16U; ++frame) {
      assert(snap.step(std::uint64_t(frame) * kPalettePeriodUs, &impulse_view));
    }
    unsigned quiet_max = 0U;
    unsigned quiet_sum = 0U;
    for (unsigned i = 0; i < 160U; ++i) {
      const auto pixel = snap.channel(0).frame()[i];
      quiet_sum += unsigned(pixel.red) + unsigned(pixel.green) +
                   unsigned(pixel.blue);
      for (auto value : {pixel.red, pixel.green, pixel.blue}) {
        if (value > quiet_max) quiet_max = value;
      }
    }
    if (quiet_sum >= 160U * 3U * 255U) {
      std::fprintf(stderr, "SNAP_FREEZE quiet_sum=%u quiet_max=%u\n", quiet_sum,
                   quiet_max);
      std::_Exit(2);
    }
#ifdef K1_SNAP_FREEZE_ONLY
    std::fprintf(stderr, "SNAP_FREEZE_ESCAPED quiet_sum=%u quiet_max=%u\n",
                 quiet_sum, quiet_max);
    std::_Exit(3);
#endif
  }
#ifndef K1_SNAP_FREEZE_ONLY
#ifdef K1_PALETTE_MORPH
  {
    PaletteRuntime hold;
    PaletteConfig cfg;
    cfg.mode_a = 32U;
    cfg.mode_b = 32U;
    cfg.flags = 1U;
    cfg.brightness = 255U;
#ifdef K1_PALETTE_MORPH
    cfg.version = 3U;
#endif
    assert(hold.configure(cfg, 0U));
    contract::AudioFeaturesV1 live = audio;
    live.event_flags = 0U;
    live.peak_scaled = 0.4F;
    live.vu_level = 0.4F;
    live.chroma_a_origin[0] = 0.90F;
    live.chroma_a_origin[7] = 0.35F;
    live.chroma_strength = 0.80F;
    VisualWaveformHistory live_wave{};
    live_wave.sample_count = 96U;
    live_wave.raw_maximum = 20000.0F;
    live_wave.peak_scaled = 0.4F;
    for (std::size_t history = 0U; history < kVisualWaveformHistoryFrames;
         ++history) {
      for (std::size_t sample = 0U; sample < live_wave.sample_count; ++sample) {
        live_wave.frames[history][sample] =
            static_cast<std::int16_t>((sample % 12U) * 2000);
      }
    }
    const VisualAudioFrameView live_view{live, tempo, live_wave, 0U};
    for (unsigned frame = 0; frame < 24U; ++frame) {
      assert(hold.step(std::uint64_t(frame) * kPalettePeriodUs, &live_view));
    }
    unsigned live_sum = 0U;
    for (unsigned i = 0; i < 160U; ++i) {
      const auto pixel = hold.channel(0).frame()[i];
      live_sum += unsigned(pixel.red) + unsigned(pixel.green) + unsigned(pixel.blue);
    }
    assert(live_sum > 0U);
    contract::AudioFeaturesV1 gap = live;
    gap.event_flags = contract::kEventSilence;
    gap.peak_scaled = 0.0F;
    gap.vu_level = 0.0F;
    VisualWaveformHistory gap_wave{};
    const VisualAudioFrameView gap_view{gap, tempo, gap_wave, 0U};
    for (unsigned frame = 24U; frame < 36U; ++frame) {
      assert(hold.step(std::uint64_t(frame) * kPalettePeriodUs, &gap_view));
    }
    unsigned gap_sum = 0U;
    for (unsigned i = 0; i < 160U; ++i) {
      const auto pixel = hold.channel(0).frame()[i];
      gap_sum += unsigned(pixel.red) + unsigned(pixel.green) + unsigned(pixel.blue);
    }
    if (gap_sum == 0U) {
      std::fprintf(stderr, "WAKE_GONE live_sum=%u gap_sum=0\n", live_sum);
      std::_Exit(2);
    }
  }
#endif
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
        contract::AudioFeaturesV1 governed_audio = audio;
        std::uint64_t last_live = 0U;
        bool last_live_valid = false;
        applyK1PresencePolicy(governed_audio, std::uint64_t(frame) * kPalettePeriodUs,
                              last_live, last_live_valid);
        const VisualAudioFrameView governed{governed_audio, tempo, wave, 0U};
        ref.prepareAudio(governed_audio);
        ref.clearFrame();
        assert(renderProductChannel(ref, governed, dt).rendered);
        applyProductOutputTreatment(ref.frame(), ref.controls(), ref.outputTreatmentState());
        assert(!std::memcmp(runtime.channel(ch).frame().data(), ref.frame().data(), 160*sizeof(Pixel8)));
        bool visible=false;
        for (unsigned pixel_index=0; pixel_index<160; ++pixel_index) {
          const auto pixel = ref.frame()[pixel_index];
          const auto opposite = ref.frame()[159U-pixel_index];
          if (std::memcmp(&pixel,&opposite,sizeof(Pixel8))) {
            std::fprintf(stderr,"CENTRE_MIRROR_FAILURE mode=%u palette=%u pixel=%u\n",mode,id,pixel_index);
            std::_Exit(2);
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
#endif
}
