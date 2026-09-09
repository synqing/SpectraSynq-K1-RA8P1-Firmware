#pragma once

#include "core/audio/gdft_goertzel.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace k1::core::audio {

inline constexpr std::size_t kGdftAgcBandCount = 4;
inline constexpr std::size_t kGdftSpectralHistoryLength = 5;

struct GdftPostprocessConfig final {
    // Mood low-pass sample_rate argument. Frozen typical legacy SYSTEM_FPS,
    // not AP hop rate (133.333 Hz) and not novelty rate (44.444 Hz).
    // K1-DM-112: do not replace with 133.333.
    std::uint32_t frame_rate_hz = 100;
    // AGC α = dt/(τ+dt) clock. 10 ms is the 100 Hz reference the alphas were
    // tuned at. Independent of frame_rate_hz. Not the 7.5 ms hop unless a
    // later admitted upgrade passes measured hop duration.
    float frame_dt_seconds = 0.010F;
    float mood = 0.5F;
    bool bloom_mode = false;
    bool static_noise_subtraction_enabled = false;
    bool noise_calibration_complete = true;
    bool loud_guard_enabled = true;
    std::uint8_t loud_guard_mode = 2;
    float loud_gdft_trim = 1.0F;
};

struct GdftAgcBandState final {
    std::int32_t envelope_q16 = 0;
    std::int32_t noise_floor_q16 = 65;
    std::int32_t gain_q16 = 65536;
    bool gated = true;
};

struct GdftPostprocessState final {
    std::array<float, kGdftBinCount> magnitude_average{};
    std::array<float, kGdftBinCount> magnitude_last{};
    std::array<std::int32_t, kGdftBinCount> static_noise_q16{};
    std::array<GdftAgcBandState, kGdftAgcBandCount> agc_bands{};
    std::array<std::array<std::int32_t, kGdftBinCount>,
               kGdftSpectralHistoryLength>
        spectral_history_q16{};
    std::uint8_t spectral_history_index = 0;
};

struct GdftPostprocessFrame final {
    std::array<float, kGdftBinCount> magnitude_final{};
    std::array<std::int32_t, kGdftBinCount> spectrum_q16{};
    std::array<float, kGdftBinCount> spectrum{};
    std::array<float, kGdftAgcBandCount> band_gain{};
    std::array<float, kGdftAgcBandCount> band_energy{};
    std::array<float, kGdftAgcBandCount> band_noise_floor{};
    std::int32_t novelty_q16 = 0;
    float novelty = 0.0F;
};

// Production GDFT shell after the direct recurrence: magnitude EMA, optional
// static-noise subtraction, mood low-pass, four independent Q15.16 AGC bands,
// loud-room guard, and five-frame positive spectral novelty. All state is
// caller-owned; no board headers, hidden statics, or allocation are used.
void processGdftPostprocess(const GdftRawFrame& raw,
                            const GdftConfiguration& configuration,
                            const GdftPostprocessConfig& config,
                            GdftPostprocessState& state,
                            GdftPostprocessFrame& output) noexcept;

}  // namespace k1::core::audio
