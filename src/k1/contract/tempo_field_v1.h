#pragma once
#include <array>
#include <cstdint>

namespace k1::contract {
inline constexpr std::uint16_t kTempoFieldSchemaVersion = 1;
inline constexpr std::size_t kTempoFieldCapacity = 8;
inline constexpr std::size_t kTempoFieldFreshCapacity = 4;
enum class TempoTrackAdmission : std::uint8_t { unavailable, fresh, coasting, stale };
struct TempoFieldCandidateV1 final {
    std::uint16_t bin_id = 0; // Bank identity: BPM = 60 + bin_id.
    std::uint32_t generation = 0;
    float bpm = 0.0F;
    float salience = 0.0F;
    float point_salience = 0.0F;
    std::uint64_t epoch = 0;
    std::uint64_t support_start = 0;
    std::uint64_t support_end = 0;
    std::uint64_t last_observed_frame = 0;
    std::uint64_t reference_frame = 0;
    std::uint64_t observation_age_frames = 0;
    std::uint64_t raw_observed_frame = 0;
    std::uint64_t raw_observation_age_frames = 0;
    std::uint64_t beat_index = 0;
    std::uint64_t beat_period_q32 = 0;
    double phase01 = 0.0;
    float phase_uncertainty_cycles = 0.0F;
    std::uint32_t root_generation = 0;
    std::uint16_t root_id = 0;
    std::uint16_t ratio_numerator = 1;
    std::uint16_t ratio_denominator = 1;
    TempoTrackAdmission admission = TempoTrackAdmission::unavailable;
    bool phase_valid = false;
    bool uncertainty_valid = false; // Correlation phase is not attack-time uncertainty.
    bool raw_observation_valid = false;
    bool derived = false;
};
struct TempoCanonicalV1 final {
    float target_bpm = 0.0F;
    float running_bpm = 0.0F;
    float phase01 = 0.0F;
    float confidence = 0.0F;
    std::uint32_t reference_wall_ms = 0;
    std::uint16_t winner_id = 0;
    bool locked = false;
    bool media_projection_valid = false; // Wall-advanced incumbent stays in wall time.
};
struct TempoFieldV1 final {
    std::uint16_t schema_version = kTempoFieldSchemaVersion;
    std::uint32_t configuration_revision = 1;
    std::uint64_t epoch = 0;
    std::uint64_t generation = 0;
    std::uint64_t support_start = 0;
    std::uint64_t support_end = 0;
    std::uint64_t available_us = 0;
    float activity = 0.0F; // Mean square stored novelty, before ACF normalisation.
    TempoCanonicalV1 canonical{};
    std::array<TempoFieldCandidateV1, kTempoFieldCapacity> candidates{};
    std::uint8_t count = 0;
    std::uint8_t fresh_phase_count = 0;
    bool availability_valid = false;
    bool enabled = false;
    bool valid = false;
    bool warming = true;
    bool silence = true;
    bool activity_valid = false;
    bool phase_authority = false;
    bool coasting = false;
};
static_assert(sizeof(TempoFieldV1) < 2048, "Tempo field exceeds snapshot design budget");
} // namespace k1::contract
