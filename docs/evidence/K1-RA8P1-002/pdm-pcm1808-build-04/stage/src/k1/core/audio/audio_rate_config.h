#pragma once

#include <cstdint>

namespace k1::core::audio {

// 24 kHz / 180-sample production candidate. Hop duration remains 7.5 ms, so
// AP cadence is 133.333 Hz and novelty cadence is 44.444 Hz — the same as
// the retired 12.8 kHz / 96-sample shipping tuple. The 12.8 kHz rate is
// retained only as a frozen host-parity reference.
struct AudioRateConfiguration final {
    std::uint32_t sample_rate_hz;
    std::uint16_t hop_samples;
};

inline constexpr AudioRateConfiguration kLegacy12800AudioRate{12800U, 96U};
inline constexpr AudioRateConfiguration kProductionAudioRate{24000U, 180U};
inline constexpr AudioRateConfiguration kRt1062DifferentialAudioRate{
    kProductionAudioRate};

// Teensy Audio only accepts 16/32/64/128-sample blocks. 180 is not a
// multiple of 32, so the assembler carries leftover samples outside the
// circular window until the hop is published.
inline constexpr std::uint16_t kProductionCaptureBlockSamples = 32U;
inline constexpr std::uint16_t kProductionNumFreqs = 80U;
inline constexpr std::uint8_t kProductionGdftCrossoverBin = 40U;
inline constexpr std::uint8_t kProductionNoveltyDecimation = 3U;

[[nodiscard]] constexpr float audioApRateHz(
    const AudioRateConfiguration rate) noexcept {
    return static_cast<float>(rate.sample_rate_hz) /
           static_cast<float>(rate.hop_samples);
}

[[nodiscard]] constexpr float audioNoveltyRateHz(
    const AudioRateConfiguration rate) noexcept {
    return audioApRateHz(rate) /
           static_cast<float>(kProductionNoveltyDecimation);
}

inline constexpr char kProductionAudioContractToken[] =
    "K1_PRODUCTION_AUDIO_CONTRACT=sr24000.hop180.bins80.xover40";

static_assert(kProductionAudioRate.sample_rate_hz == 24000U);
static_assert(kProductionAudioRate.hop_samples == 180U);
static_assert(kLegacy12800AudioRate.sample_rate_hz == 12800U);
static_assert(kLegacy12800AudioRate.hop_samples == 96U);
static_assert(kProductionNumFreqs == 80U);
static_assert(kProductionGdftCrossoverBin == 40U);
static_assert(kProductionCaptureBlockSamples == 32U);

}  // namespace k1::core::audio
