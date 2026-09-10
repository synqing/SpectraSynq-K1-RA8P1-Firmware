#pragma once

// Opt-in target timing seam. The pinned product sources only see this header
// inside a disposable, hash-bound profiler build.
#include <cstdint>

enum K1StageId : unsigned {
    k1_stage_ap_total = 0,
    k1_stage_gdft_raw,
    k1_stage_gdft_postprocess,
    k1_stage_features,
    k1_stage_chord_detect,
    k1_stage_onset_beat,
    k1_stage_musical_saliency,
    k1_stage_tempo_total,
    k1_stage_tempo_history,
    k1_stage_tempo_acf_total,
    k1_stage_acf_prepare,
    k1_stage_acf_correlate,
    k1_stage_acf_comb,
    k1_stage_acf_normalise,
    k1_stage_tempo_bank,
    k1_stage_tempo_flywheel,
    k1_stage_tempo_output,
    k1_stage_clock_affine,
    k1_stage_musical_time,
    k1_stage_vp_render,
    k1_stage_telemetry,
    k1_stage_count,
};

extern "C" std::uint32_t k1_stage_probe_begin(unsigned stage) noexcept;
extern "C" void k1_stage_probe_end(unsigned stage,
                                    std::uint32_t started) noexcept;

#define K1_STAGE_BEGIN(name, stage)                                         \
    const std::uint32_t k1_stage_started_##name = k1_stage_probe_begin(stage)
#define K1_STAGE_END(name, stage)                                           \
    k1_stage_probe_end(stage, k1_stage_started_##name)

class K1StageScope final {
public:
    explicit K1StageScope(unsigned stage) noexcept
        : stage_(stage), started_(k1_stage_probe_begin(stage)) {}
    ~K1StageScope() noexcept { k1_stage_probe_end(stage_, started_); }

    K1StageScope(const K1StageScope&) = delete;
    K1StageScope& operator=(const K1StageScope&) = delete;

private:
    unsigned stage_;
    std::uint32_t started_;
};

#define K1_STAGE_SCOPE(name, stage) K1StageScope k1_stage_scope_##name(stage)
