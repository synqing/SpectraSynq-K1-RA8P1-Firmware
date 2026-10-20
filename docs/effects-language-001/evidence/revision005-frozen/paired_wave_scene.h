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
  float width_px=7.F;
  float velocity_px_s=40.F;
  float edge_delay_s=.18F;
  float spread_px2_s=0.F;
  float half_life_s=2.8F;
  float gap_s=.32F;
  float return_strength=.22F;
  float articulation=.7F;
  std::uint16_t palette_id=33;
};
struct PairedWaveTelemetry final {
  float envelope=0, transport_px=0, limiter_gain=1;
  unsigned attacks=0, omitted=0;
  bool audio_available=false, frame_valid=true;
};
// One scene owns a retained stream of centre-deposited material. Lorenz state
// shapes formation and relative edge emphasis; it never teleports positions.
class PairedWaveScene final {
 public:
  bool configure(const PairedWaveSettings&,std::uint64_t time_us) noexcept;
  void reset(std::uint64_t time_us=0) noexcept;
  void consume(const VisualAudioFrameView&,std::uint64_t time_us) noexcept;
  const PairedFrame16& render(std::uint64_t time_us) noexcept;
  const PairedWaveTelemetry& telemetry() const noexcept { return telemetry_; }
 private:
  struct Edge {float radius=0, quiet=0;bool returning=false,waiting=false,done=false;};
  struct Knot {
    bool active=false, top_leads=true;
    float age=0,amplitude=0,width=0,delay=0,edge_balance=.5F;
    Edge edges[2]{};
    RibbonRgb top{},bottom{};
  };
  bool advance(std::uint64_t) noexcept;
  void deposit(float strength) noexcept;
  void step(float dt, bool fresh) noexcept;
  PairedWaveSettings settings_{};
  PairedWaveTelemetry telemetry_{};
  PairedFrame16 frame_{};
  std::array<Knot,144> knots_{};
  std::uint64_t time_us_=0, publication_us_=0,last_attack_us_=0;
  std::uint32_t sequence_=0,onset_=0;
  double phase_=0;
  float energy_=0,envelope_=0,timbre_=0,colour_position_=.3F,previous_energy_=0;
  float lorenz_x_=-8.F,lorenz_y_=-9.F,lorenz_z_=25.F;
  float formation_=0,deposit_clock_=0,impact_=0;
  bool initialised_=false,have_sequence_=false,have_attack_=false;
};
} // namespace k1::core::visual
