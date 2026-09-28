#pragma once
#include "core/visual/ribbon_material.h"

namespace k1::core::visual {
struct PairedFrame16 final { RibbonFrame16 top{}, bottom{}; };
struct PairedOrientation final { bool swap_edges=false, reverse_top=false, reverse_bottom=false; };
// Two complete physical 160-pixel edge streams, 960 bytes each. This does not
// bind the current two-80-pixel bench outputs to a 320-pixel product topology.
bool stagePaired(const PairedFrame16& frame, PairedOrientation orientation,
                 std::uint8_t* top, std::size_t top_size,
                 std::uint8_t* bottom, std::size_t bottom_size) noexcept;

struct PairedWaveSettings final {
  float wavelength_px=18.F;
  float velocity_px_s=22.F;
  float edge_offset_cycles=.375F;
  float articulation=.7F;
  std::uint16_t palette_id=33;
};
struct PairedWaveTelemetry final {
  float envelope=0, phase_cycles=0, limiter_gain=1;
  unsigned attacks=0, omitted=0;
  bool audio_available=false, frame_valid=true;
};
// A single coordinated scene clock controls both edge fields. The retained
// phase is never restarted by an onset; centre-launched articulations have
// independent lifetimes and deposited palette ownership.
class PairedWaveScene final {
 public:
  bool configure(const PairedWaveSettings&,std::uint64_t time_us) noexcept;
  void reset(std::uint64_t time_us=0) noexcept;
  void consume(const VisualAudioFrameView&,std::uint64_t time_us) noexcept;
  const PairedFrame16& render(std::uint64_t time_us) noexcept;
  const PairedWaveTelemetry& telemetry() const noexcept { return telemetry_; }
 private:
  struct Accent {
    bool active=false, top_leads=true;
    double age=0;
    float strength=0;
    RibbonRgb top{},bottom{};
  };
  bool advance(std::uint64_t) noexcept;
  PairedWaveSettings settings_{};
  PairedWaveTelemetry telemetry_{};
  PairedFrame16 frame_{};
  std::array<Accent,12> accents_{};
  std::uint64_t time_us_=0, publication_us_=0,last_attack_us_=0;
  std::uint32_t sequence_=0,onset_=0;
  double phase_=0;
  float energy_=0,envelope_=0,timbre_=0,colour_position_=.3F,previous_energy_=0;
  bool initialised_=false,have_sequence_=false,have_attack_=false;
};
} // namespace k1::core::visual
