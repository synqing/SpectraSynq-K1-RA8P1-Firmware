"""Optional hop-slice of tempo ACF. DualMCU imports stay intact on disk."""
import hashlib

def apply_tempo_acf_slice_overlay(root):
    header = root / 'core/audio/tempo_tracker.h'
    source = root / 'core/audio/tempo_tracker.cpp'
    before_h = header.read_text()
    after_h = before_h
    old = '#include "core/audio/tempo_acf.h"\n#include "core/audio/audio_rate_config.h"'
    new = '#include "core/audio/tempo_acf.h"\n#include "core/audio/tempo_acf_slice.h"\n#include "core/audio/audio_rate_config.h"'
    if after_h.count(old) != 1:
        raise RuntimeError('tempo overlay header include mismatch')
    after_h = after_h.replace(old, new)
    old = '    std::uint16_t frame_counter = 0;\n    TempoTrackerEvent event{};'
    new = ('    std::uint16_t frame_counter = 0;\n'
           '    TempoAcfSliceJob acf_job{};\n'
           '    TempoTrackerEvent event{};')
    if after_h.count(old) != 1:
        raise RuntimeError('tempo overlay header state mismatch')
    after_h = after_h.replace(old, new)

    before_c = source.read_text()
    after_c = before_c
    old = '#include "core/audio/tempo_tracker.h"\n'
    new = '#include "core/audio/tempo_tracker.h"\n#include "core/audio/tempo_acf_slice.h"\n'
    if after_c.count(old) != 1:
        raise RuntimeError('tempo overlay cpp include mismatch')
    after_c = after_c.replace(old, new)
    old = '''    if (++state.frame_counter < kNoveltyDecimation) {
        if (state.event.beat_tick) state.event.beat_tick = false;
        return;
    }'''
    new = '''    if (++state.frame_counter < kNoveltyDecimation) {
        if (state.event.beat_tick) state.event.beat_tick = false;
        if (state.acf_job.active) {
            const unsigned chunk =
                static_cast<unsigned>((state.acf_job.lag_count + 2) / 3);
            if (tempoAcfSlicePump(state.acf_job, chunk ? chunk : 1U) &&
                state.acf_job.lags_done) {
                tempoAcfSliceFinish(state.acf_job, state.acf);
                updateTempoBank(state, state.acf_job.delta_seconds);
                advanceFlywheel(state, state.acf_job.delta_seconds,
                                state.acf_job.sample * state.novelty_scale);
                state.event = buildOutput(state);
                state.event.updated = true;
            }
        }
        return;
    }'''
    if after_c.count(old) != 1:
        raise RuntimeError('tempo overlay early-return mismatch')
    after_c = after_c.replace(old, new)
    old = '''    computeTempoAcfAtRate(state.novelty_history,
                          state.history_index,
                          state.novelty_scale,
                          state.novelty_rate_hz,
                          state.acf);
    updateTempoBank(state, delta_seconds);
    advanceFlywheel(
        state, delta_seconds, sample * state.novelty_scale);
    state.event = buildOutput(state);
    state.event.updated = true;'''
    new = '''    if (state.acf_job.active) {
        while (state.acf_job.active && !state.acf_job.lags_done) {
            tempoAcfSlicePump(state.acf_job, 65535U);
        }
        if (state.acf_job.lags_done) {
            tempoAcfSliceFinish(state.acf_job, state.acf);
            updateTempoBank(state, state.acf_job.delta_seconds);
            advanceFlywheel(state, state.acf_job.delta_seconds,
                            state.acf_job.sample * state.novelty_scale);
            state.event = buildOutput(state);
            state.event.updated = true;
        }
    }
    tempoAcfSliceBegin(state.acf_job,
                       state.novelty_history,
                       state.history_index,
                       state.novelty_scale,
                       state.novelty_rate_hz,
                       delta_seconds,
                       sample);
    {
        const unsigned chunk =
            static_cast<unsigned>((state.acf_job.lag_count + 2) / 3);
        if (tempoAcfSlicePump(state.acf_job, chunk ? chunk : 1U) &&
            state.acf_job.lags_done) {
            tempoAcfSliceFinish(state.acf_job, state.acf);
            updateTempoBank(state, delta_seconds);
            advanceFlywheel(state, delta_seconds, sample * state.novelty_scale);
            state.event = buildOutput(state);
            state.event.updated = true;
        }
    }'''
    if after_c.count(old) != 1:
        raise RuntimeError('tempo overlay ACF call mismatch')
    after_c = after_c.replace(old, new)
    header.write_text(after_h)
    source.write_text(after_c)
    return {
        'core/audio/tempo_tracker.h': {
            'before': hashlib.sha256(before_h.encode()).hexdigest(),
            'after': hashlib.sha256(after_h.encode()).hexdigest(),
        },
        'core/audio/tempo_tracker.cpp': {
            'before': hashlib.sha256(before_c.encode()).hexdigest(),
            'after': hashlib.sha256(after_c.encode()).hexdigest(),
        },
    }
