#!/usr/bin/env python3
"""Instrument disposable copies of the pinned K1 timing sources."""
from __future__ import annotations

import hashlib
from pathlib import Path


def _replace_once(path: Path, before: str, after: str) -> None:
    text = path.read_text()
    count = text.count(before)
    if count != 1:
        raise RuntimeError(
            f"stage-profile anchor count for {path}: expected 1, got {count}"
        )
    path.write_text(text.replace(before, after))


def instrument_stage_sources(k1_root: Path) -> dict[str, str]:
    """Apply the declared probe boundaries and return transformed hashes."""
    audio = k1_root / "core/audio/audio_pipeline.cpp"
    tracker = k1_root / "core/audio/tempo_tracker.cpp"
    acf = k1_root / "core/audio/tempo_acf.cpp"

    _replace_once(
        audio,
        '#include "core/audio/chord_detect.h"\n',
        '#include "core/audio/chord_detect.h"\n#include "stage_probe.h"\n',
    )
    _replace_once(
        audio,
        "    GdftRawFrame raw{};\n"
        "    analyseGdftRaw(*input.samples, gdft_configuration_, raw);\n",
        "    GdftRawFrame raw{};\n"
        "    K1_STAGE_BEGIN(gdft_raw, k1_stage_gdft_raw);\n"
        "    analyseGdftRaw(*input.samples, gdft_configuration_, raw);\n"
        "    K1_STAGE_END(gdft_raw, k1_stage_gdft_raw);\n",
    )
    _replace_once(
        audio,
        "    GdftPostprocessFrame gdft{};\n"
        "    processGdftPostprocess(\n"
        "        raw, gdft_configuration_, input.gdft, gdft_state_, gdft);\n",
        "    GdftPostprocessFrame gdft{};\n"
        "    K1_STAGE_BEGIN(gdft_postprocess, k1_stage_gdft_postprocess);\n"
        "    processGdftPostprocess(\n"
        "        raw, gdft_configuration_, input.gdft, gdft_state_, gdft);\n"
        "    K1_STAGE_END(gdft_postprocess, k1_stage_gdft_postprocess);\n",
    )
    _replace_once(
        audio,
        "    auto& features = output.features;\n",
        "    K1_STAGE_BEGIN(features, k1_stage_features);\n"
        "    auto& features = output.features;\n",
    )
    _replace_once(
        audio,
        "    features.chord_fifth_strength = chord.fifth_strength;\n\n"
        "    output.onset = onset_.update(features);\n"
        "    publishOnset(output.onset, features);\n"
        "    output.saliency = saliency_.update(features);\n",
        "    features.chord_fifth_strength = chord.fifth_strength;\n"
        "    K1_STAGE_END(features, k1_stage_features);\n\n"
        "    K1_STAGE_BEGIN(onset_saliency, k1_stage_onset_saliency);\n"
        "    output.onset = onset_.update(features);\n"
        "    publishOnset(output.onset, features);\n"
        "    output.saliency = saliency_.update(features);\n"
        "    K1_STAGE_END(onset_saliency, k1_stage_onset_saliency);\n",
    )
    _replace_once(
        audio,
        "    updateTempoTracker(tempo_state_,\n"
        "                       TempoTrackerInput{input.frame_ms,\n"
        "                                         features.novelty,\n"
        "                                         input.silence});\n"
        "    output.tempo = readTempoTracker(tempo_state_);\n\n"
        "    if (input.media_time_valid) {\n",
        "    K1_STAGE_BEGIN(tempo_total, k1_stage_tempo_total);\n"
        "    updateTempoTracker(tempo_state_,\n"
        "                       TempoTrackerInput{input.frame_ms,\n"
        "                                         features.novelty,\n"
        "                                         input.silence});\n"
        "    output.tempo = readTempoTracker(tempo_state_);\n"
        "    K1_STAGE_END(tempo_total, k1_stage_tempo_total);\n\n"
        "    K1_STAGE_BEGIN(musical_time, k1_stage_musical_time);\n"
        "    if (input.media_time_valid) {\n",
    )
    _replace_once(
        audio,
        "    }\n\n"
        "    output.gdft_overflow_count = raw.q0_overflow_count;\n",
        "    }\n"
        "    K1_STAGE_END(musical_time, k1_stage_musical_time);\n\n"
        "    output.gdft_overflow_count = raw.q0_overflow_count;\n",
    )

    _replace_once(
        tracker,
        '#include "core/audio/tempo_tracker.h"\n',
        '#include "core/audio/tempo_tracker.h"\n#include "stage_probe.h"\n',
    )
    _replace_once(
        tracker,
        "    state.frame_counter = 0;\n"
        "    const std::uint32_t elapsed =\n",
        "    state.frame_counter = 0;\n"
        "    K1_STAGE_BEGIN(tempo_history, k1_stage_tempo_history);\n"
        "    const std::uint32_t elapsed =\n",
    )
    _replace_once(
        tracker,
        "    checkSilence(state);\n"
        "    computeTempoAcfAtRate(state.novelty_history,\n",
        "    checkSilence(state);\n"
        "    K1_STAGE_END(tempo_history, k1_stage_tempo_history);\n"
        "    K1_STAGE_BEGIN(tempo_acf_total, k1_stage_tempo_acf_total);\n"
        "    computeTempoAcfAtRate(state.novelty_history,\n",
    )
    _replace_once(
        tracker,
        "                          state.novelty_rate_hz,\n"
        "                          state.acf);\n"
        "    updateTempoBank(state, delta_seconds);\n"
        "    advanceFlywheel(\n"
        "        state, delta_seconds, sample * state.novelty_scale);\n"
        "    state.event = buildOutput(state);\n",
        "                          state.novelty_rate_hz,\n"
        "                          state.acf);\n"
        "    K1_STAGE_END(tempo_acf_total, k1_stage_tempo_acf_total);\n"
        "    K1_STAGE_BEGIN(tempo_bank, k1_stage_tempo_bank);\n"
        "    updateTempoBank(state, delta_seconds);\n"
        "    K1_STAGE_END(tempo_bank, k1_stage_tempo_bank);\n"
        "    K1_STAGE_BEGIN(tempo_flywheel, k1_stage_tempo_flywheel);\n"
        "    advanceFlywheel(\n"
        "        state, delta_seconds, sample * state.novelty_scale);\n"
        "    K1_STAGE_END(tempo_flywheel, k1_stage_tempo_flywheel);\n"
        "    K1_STAGE_BEGIN(tempo_output, k1_stage_tempo_output);\n"
        "    state.event = buildOutput(state);\n"
        "    K1_STAGE_END(tempo_output, k1_stage_tempo_output);\n",
    )

    _replace_once(
        acf,
        '#include "core/audio/tempo_acf.h"\n',
        '#include "core/audio/tempo_acf.h"\n#include "stage_probe.h"\n',
    )
    _replace_once(
        acf,
        "    const float kNoveltyRateHz = novelty_rate_hz;\n"
        "    std::array<float, kTempoAcfHistoryLength> work{};\n",
        "    const float kNoveltyRateHz = novelty_rate_hz;\n"
        "    K1_STAGE_BEGIN(acf_prepare, k1_stage_acf_prepare);\n"
        "    std::array<float, kTempoAcfHistoryLength> work{};\n",
    )
    _replace_once(
        acf,
        "    for (float& value : work) value -= mean;\n"
        "    const int minimum_lag = static_cast<int>(\n",
        "    for (float& value : work) value -= mean;\n"
        "    K1_STAGE_END(acf_prepare, k1_stage_acf_prepare);\n"
        "    const int minimum_lag = static_cast<int>(\n",
    )
    _replace_once(
        acf,
        "    std::array<float, kAcfTableLength> acf{};\n"
        "    for (int row = 0; row < lag_count; ++row) {\n",
        "    std::array<float, kAcfTableLength> acf{};\n"
        "    K1_STAGE_BEGIN(acf_correlate, k1_stage_acf_correlate);\n"
        "    for (int row = 0; row < lag_count; ++row) {\n",
    )
    _replace_once(
        acf,
        "        acf[static_cast<std::size_t>(row)] = sum;\n"
        "    }\n\n"
        "    float maximum_comb = 1.0e-12F;\n",
        "        acf[static_cast<std::size_t>(row)] = sum;\n"
        "    }\n"
        "    K1_STAGE_END(acf_correlate, k1_stage_acf_correlate);\n\n"
        "    K1_STAGE_BEGIN(acf_comb, k1_stage_acf_comb);\n"
        "    float maximum_comb = 1.0e-12F;\n",
    )
    _replace_once(
        acf,
        "        }\n"
        "    }\n\n"
        "    const float inverse_comb = 1.0F / maximum_comb;\n",
        "        }\n"
        "    }\n"
        "    K1_STAGE_END(acf_comb, k1_stage_acf_comb);\n\n"
        "    K1_STAGE_BEGIN(acf_normalise, k1_stage_acf_normalise);\n"
        "    const float inverse_comb = 1.0F / maximum_comb;\n",
    )
    _replace_once(
        acf,
        "    output.valid = maximum_comb > 1.0e-6F;\n",
        "    output.valid = maximum_comb > 1.0e-6F;\n"
        "    K1_STAGE_END(acf_normalise, k1_stage_acf_normalise);\n",
    )

    return {
        str(path.relative_to(k1_root)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in (audio, tracker, acf)
    }

