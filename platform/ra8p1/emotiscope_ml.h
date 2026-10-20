#ifndef K1_EMOTISCOPE_ML_H
#define K1_EMOTISCOPE_ML_H
#include "emotiscope_port.h"

// Retained Emotiscope encoder/decoder-shaped network; unlabelled features only.
// One primary owner prepares a result before either board renders. No heap,
// queues, host dependency or audio-producer duplication.
class EmotiscopeMl {
 public:
  using Clock = uint32_t (*)();
  static constexpr uint32_t model_id = 0x6cf109c9u;
  static constexpr uint32_t period_us = 40000u;
  static constexpr uint32_t expiry_us = 100000u;
  // Zero publication sequence is reserved for an uninitialised publication.
  // Nonzero sequence and time ordering use unsigned modular differences.
  void prepare(const K1EmotiscopeAudioPublication& input, uint32_t now_us,
               bool enabled, Clock cycles = nullptr, Clock clock_us = nullptr);
  const K1EmotiscopeMlResult& result() const { return result_; }
  // On-demand numerical witness: exact raw pre-sqrt input belonging to the
  // currently valid result. Single owner, fixed storage; false leaves out alone.
  bool inference_input(float output[64], uint32_t now_us) const;
  void reset();
  static bool infer(const float smooth[64], float latent[8], float reconstruction[64]);
  static bool usable(const K1EmotiscopeAudioPublication& input,
                     const K1EmotiscopeMlResult& result, uint32_t now_us);
  // Bounded reconstruction weighting changes spectral geometry without adding
  // energy. Disabled, zero strength and invalid results copy raw exactly.
  static bool apply(const K1EmotiscopeAudioPublication& raw,
                    const K1EmotiscopeMlResult& result,
                    const K1EmotiscopeMlConfig& config, uint32_t now_us,
                    K1EmotiscopeAudioPublication& output);
  // Optional real palette-coordinate consumer. This is an unlabelled latent
  // difference, not recognised genre/emotion; bounded to +/-1/16 palette turn.
  static float palette_offset(const K1EmotiscopeAudioPublication& raw,
                              const K1EmotiscopeMlResult& result,
                              const K1EmotiscopeMlConfig& config, uint32_t now_us);
 private:
  void invalidate(bool rejected);
  K1EmotiscopeMlResult result_{};
  float inference_input_[64]{};
  uint32_t last_sequence_ = 0u, last_audio_hops_ = 0u, last_input_us_ = 0u;
  uint32_t last_inference_us_ = 0u;
  uint32_t last_inference_audio_hops_ = 0u;
  bool warmed_ = false, cadence_seen_ = false, chronology_seen_ = false;
};
#endif
