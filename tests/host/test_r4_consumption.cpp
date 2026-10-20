#include "k1_live_protocol.h"
#include "palette_runtime.h"
#include "core/visual/visual_audio_frame.h"

#include <cassert>
#include <cstdio>
#include <cstring>

using namespace k1::contract;
using namespace k1::core::audio;
using namespace k1::core::visual;
using namespace k1::titan;

struct Bundle {
  AudioFeaturesV1 audio{};
  TempoTrackerEvent tempo{};
  VisualWaveformHistory waveform{};
  VisualAudioFrameView view() const noexcept {
    return VisualAudioFrameView{audio, tempo, waveform, audio.publish_time_us};
  }
};

static void fill_musical(Bundle& b, std::uint32_t sequence, std::uint64_t pub) noexcept {
  b.audio = AudioFeaturesV1{};
  b.audio.sequence = sequence;
  b.audio.peak_scaled = 0.80F;
  b.audio.vu_level = 0.80F;
  b.audio.validity_flags = kValidBaseSnapshot | kValidChroma | kValidSpectrum;
  b.audio.chroma_a_origin[0] = 1.0F;
  b.audio.spectrum[7] = 1.0F;
  b.audio.publish_time_us = pub;
  b.waveform.peak_scaled = 0.80F;
  b.waveform.raw_maximum = 0.80F;
}

static void fill_quiet(Bundle& b, std::uint32_t sequence, std::uint64_t pub) noexcept {
  b.audio = AudioFeaturesV1{};
  b.audio.sequence = sequence;
  b.audio.peak_scaled = 0.01F;
  b.audio.vu_level = 0.01F;
  b.audio.event_flags = kEventSilence;
  b.audio.publish_time_us = pub;
}

static PaletteConfig make_mode8() noexcept {
  PaletteConfig c;
  c.palette_a = 5U;
  c.palette_b = 5U;
  c.mode_a = 8U;
  c.mode_b = 8U;
  c.flags = 1U;
  c.brightness = 255U;
  return c;
}

static bool credit_if_consumed(PaletteRuntime& runtime, LiveRenderReceipt* receipt,
                               std::uint32_t hop, std::uint32_t frame) noexcept {
  if (!k1::titan::livePathConsumesAcceptedRevision(runtime.visualPath())) {
    return false;
  }
  k1::titan::liveNoteRendererConsumed(receipt, hop, frame, runtime.lastPublicationUs());
  return true;
}

int main() {
#ifndef K1_LIVE_RUNTIME
  std::puts("R4_CONSUMPTION_SKIP no_live_runtime");
  return 0;
#else
  // Preview / no-audio: produced frame must not credit the accepted revision.
  {
    PaletteRuntime runtime;
    assert(runtime.configure(make_mode8(), 0U));
    runtime.channel(0).controls().vp_waveform_shift_rate = 40.0F;
    LiveRenderReceipt receipt{};
    liveNotePatchAccepted(&receipt, 77, 1);
    assert(runtime.step(0U, nullptr));
    assert(std::strcmp(runtime.visualPath(), "preview") == 0);
    const bool credited = credit_if_consumed(runtime, &receipt, 0, 1);
    assert(!credited);
    assert(receipt.pending_request_id == 77);
    assert(receipt.rendered_request_id == 0);
    std::puts("R4_CONSUMPTION_PREVIEW_PASS");
  }

  // Active mode 8 effect uses the motion parameter and credits consumption.
  {
    PaletteRuntime runtime;
    assert(runtime.configure(make_mode8(), 0U));
    runtime.channel(0).controls().vp_waveform_shift_rate = 200.0F;
    LiveRenderReceipt receipt{};
    liveNotePatchAccepted(&receipt, 77, 1);
    Bundle musical;
    fill_musical(musical, 1U, 5000U);
    const auto effect_view = musical.view();
    assert(runtime.step(kPalettePeriodUs, &effect_view));
    assert(std::strcmp(runtime.visualPath(), "effect") == 0);
    const bool credited = credit_if_consumed(runtime, &receipt, 9, 3);
    assert(credited);
    assert(receipt.rendered_request_id == 77);
    assert(receipt.rendered_hop == 9);
    assert(receipt.rendered_frame == 3);
    assert(receipt.rendered_publication_us == runtime.lastPublicationUs());
    assert(receipt.pending_request_id == 0);
    std::puts("R4_CONSUMPTION_EFFECT_PASS");
  }

  // Hold still uses renderProductChannel, so it may credit; dwell must not.
  {
    PaletteRuntime runtime;
    assert(runtime.configure(make_mode8(), 0U));
    runtime.channel(0).controls().vp_waveform_shift_rate = 200.0F;
    Bundle musical;
    fill_musical(musical, 1U, 1000U);
    const auto first_view = musical.view();
    assert(runtime.step(kPalettePeriodUs, &first_view));
    assert(std::strcmp(runtime.visualPath(), "effect") == 0);
    Bundle quiet;
    fill_quiet(quiet, 2U, 2000U);
    const auto hold_view = quiet.view();
    assert(runtime.step(2U * kPalettePeriodUs, &hold_view));
    assert(std::strcmp(runtime.visualPath(), "hold") == 0);
    LiveRenderReceipt hold_receipt{};
    liveNotePatchAccepted(&hold_receipt, 80, 2);
    assert(credit_if_consumed(runtime, &hold_receipt, 10, 4));
    assert(hold_receipt.rendered_request_id == 80);
    fill_quiet(quiet, 3U, 3000U);
    const auto dwell_view = quiet.view();
    assert(runtime.step(kTitanSilenceDwellUs + 3U * kPalettePeriodUs, &dwell_view));
    assert(std::strcmp(runtime.visualPath(), "dwell") == 0);
    LiveRenderReceipt dwell_receipt{};
    liveNotePatchAccepted(&dwell_receipt, 81, 3);
    assert(!credit_if_consumed(runtime, &dwell_receipt, 11, 5));
    assert(dwell_receipt.pending_request_id == 81);
    assert(dwell_receipt.rendered_request_id == 0);
    std::puts("R4_CONSUMPTION_DWELL_PASS");
  }

  // Snapshot helper must not invent progress: pending stays until a consuming path.
  {
    LiveRenderReceipt receipt{};
    liveNotePatchAccepted(&receipt, 90, 4);
    assert(receipt.pending_request_id == 90);
    assert(receipt.rendered_request_id == 0);
    std::puts("R4_CONSUMPTION_PENDING_PASS");
  }

  std::puts("R4_CONSUMPTION_PASS");
  return 0;
#endif
}
