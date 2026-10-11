// Host-only clock-provider seam; compile the actual fixture callback and
// LiveAudioRuntime. Synthetic cycle advancement is not a device timing claim.
#include "emotiscope_ml.h"
#include "fixture_app.h"
#include "k1_live_runtime.h"
#include "palette_clock.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>

static constexpr std::uint32_t clock_hz = 1000000000u;
static std::uint32_t raw_cycles = 0u, cycle_reads = 0u, duration_cycles = 0u;
static bool inference_active = false;
extern "C" std::uint32_t k1_cycle_count(void) {
  if (inference_active && ++cycle_reads == 3u) raw_cycles += duration_cycles;
  return raw_cycles;
}

static k1::titan::LiveAudioRuntime live_runtime;
static k1::titan::PaletteClock palette_clock;
static std::uint32_t palette_clock_hz = clock_hz;
// Generated from platform/ra8p1/fixture_app.cpp, not a copied clock helper.
#include "fixture_ml_clock_callback.inc"

static std::uint32_t callback_start = 0u, callback_finish = 0u, callback_reads = 0u;
static std::uint32_t measured_clock() {
  const auto value = donor_ml_clock_us();
  if (callback_reads++ == 0u) callback_start = value;
  else callback_finish = value;
  return value;
}
static void require(bool ok, const char* message) {
  if (!ok) { std::fprintf(stderr, "FAIL %s\n", message); std::exit(2); }
}
static void advance_us(std::uint64_t microseconds) {
  // Poll the actual owner clock more often than one raw 32-bit cycle wrap.
  while (microseconds) {
    const auto step = microseconds > 1000000u ? 1000000u : microseconds;
    raw_cycles += static_cast<std::uint32_t>(step * 1000u);
    (void)live_runtime.nowUs();
    microseconds -= step;
  }
}
static void run_case(const char* name, std::uint32_t initial_cycle,
                     std::uint32_t duration_us, std::uint64_t initial_live_us,
                     bool expect_inference_cycle_wrap = false) {
  raw_cycles = initial_cycle;
  inference_active = false;
  cycle_reads = callback_reads = 0u;
  palette_clock = {};
  live_runtime.initialise(clock_hz);
  advance_us(initial_live_us);
  // The real live clock starts first; the independent palette clock starts
  // 20 ms later. This controlled origin difference is not observed silicon skew.
  advance_us(20000u);
  (void)palette_clock.sample(k1_cycle_count(), palette_clock_hz);
  advance_us(10000u);
  K1EmotiscopeAudioPublication input{};
  input.valid = 1u; input.sequence = 1u; input.audio_hops = 65u;
  input.now_us = static_cast<std::uint32_t>(live_runtime.nowUs());
  for (auto& value : input.spectrum_smooth) value = 0.25f;
  EmotiscopeMl engine;
  engine.prepare(input, input.now_us, true, &k1_cycle_count, &measured_clock);
  require(engine.result().inference_count == 0u && !engine.result().valid,
          "warmup setup");
  advance_us(10000u);
  input.sequence = 2u; input.audio_hops = 66u;
  input.now_us = static_cast<std::uint32_t>(live_runtime.nowUs());
  duration_cycles = duration_us * 1000u;
  cycle_reads = callback_reads = 0u;
  const auto inference_start_cycles = raw_cycles;
  inference_active = true;
  engine.prepare(input, input.now_us, true, &k1_cycle_count, &measured_clock);
  inference_active = false;
  const auto& result = engine.result();
  std::printf("case=%s source=%u clock_start=%u clock_finish=%u modular_age=%u "
              "executions=%u rejected=%u cycles=%u valid=%u raw_start=%u raw_finish=%u\n", name,
              input.now_us, callback_start, callback_finish,
              std::uint32_t(callback_finish - input.now_us), result.inference_count,
              result.rejected_results, result.last_cycles, result.valid,
              inference_start_cycles, raw_cycles);
  require(result.inference_count == 1u, "real model execution count");
  require(result.last_cycles == duration_cycles && callback_reads == 2u,
          "measured actual callback and cycles");
  if (expect_inference_cycle_wrap)
    require(raw_cycles < inference_start_cycles, "raw cycles wrapped during actual inference");
  // A restored old callback must fail here, after actual model execution.
  require(result.valid == unsigned(duration_us <= 100000u),
          "result.valid actual shared-clock expectation");
  require(result.rejected_results == unsigned(duration_us > 100000u),
          "true late rejection");
  if (result.valid) {
    require(result.model_id == EmotiscopeMl::model_id && result.input_sequence == 2u,
            "admitted model identity");
    float retained[64]{};
    require(engine.inference_input(retained, callback_finish),
            "actual retained inference input");
  }
}

int main() {
  run_case("unequal_origin_short", 0u, 600u, 0u);
  run_case("exact_100000_boundary", 0u, 100000u, 0u);
  run_case("real_100001_late", 0u, 100001u, 0u);
  // The 40 ms setup ends at 0xffff0000; the actual measured inference crosses
  // the raw cycle wrap. Wrapping only during initialisation would miss this.
  run_case("DWT_cycle_wrap", 0xffff0000u - 40000000u, 600u, 0u, true);
  run_case("uint32_time_wrap", 0xffff0000u, 70000u, 0xffff0000u);
}
