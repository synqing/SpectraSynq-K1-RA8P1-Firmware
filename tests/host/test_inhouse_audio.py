#!/usr/bin/env python3
"""Source-extracted in-house audio oracle and bounded host regressions. No device I/O."""
import argparse
import hashlib
import json
from pathlib import Path
import resource
import subprocess

ROOT = Path(__file__).resolve().parents[2]
REFERENCE = Path('/Users/spectrasynq/SpectraSynq_K1_Firmware/SPECTRASYNQ_K1_FIRMWARE')
PINS = {
    'audio/k1_gdft_core.cpp': '4ef8a4465027807b4615d43c87e77adc474bf347ebb3f58cc776652dfbc469a1',
    'audio/k1_gdft_lane4_exact.h': '01c7e5519f9338e38bb58e910007d31b0cbd895bb27d72ea7d0c81ff561742fc',
    'audio/k1_agc_dt_clock.h': 'f9d51c9f0cdca9675e242b61d45bb03727c011c4c12ba57e105df2a37cd971f9',
    'system/utilities.h': 'e40b3290e096afbef27db51996ec9dbe5aa06b87234521f5c177dcb0fcbc345a',
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def oracle(reference, output):
    for rel, expected in PINS.items():
        if sha(reference / rel) != expected:
            raise RuntimeError('Reference identity changed: ' + rel)
    source = (reference / 'audio/k1_gdft_core.cpp').read_text()
    constants = source.split('  // Time constants: 100 Hz reference', 1)[1]
    constants = constants[constants.index('#if K1_AGC_DT_CLOCK_V1'):]
    constants = constants.split('#ifdef K1_AGC_PERBAND_V1', 1)[0]
    branch = source.split('  // 1. Broadband signal level (average magnitude across bins)', 1)[1]
    branch = branch.split('  // Telemetry mirror — broadband gain/energy across all band slots', 1)[0]
    utilities = (reference / 'system/utilities.h').read_text()
    low_pass = utilities[utilities.index('inline float low_pass_filter('):]
    low_pass = low_pass[:low_pass.index('\n}') + 2]
    (output / 'k1_gdft_lane4_exact.h').write_bytes((reference / 'audio/k1_gdft_lane4_exact.h').read_bytes())
    (output / 'k1_agc_dt_clock.h').write_bytes((reference / 'audio/k1_agc_dt_clock.h').read_bytes())
    header = '''#pragma once
#include "k1_gdft_lane4_exact.h"
#include "core/audio/gdft_goertzel.h"
#include <array>
std::array<float,80> source_broadband(const k1::core::audio::GdftRawFrame&,
    const k1::core::audio::GdftConfiguration&, bool);
'''
    (output / 'inhouse_source_oracle.h').write_text(header)
    wrapper = '''#include "inhouse_source_oracle.h"
#include "FixedPoints.h"
#include "FixedPointsCommon.h"
#include <cmath>
#define PI 3.14159265358979323846
static int64_t source_now_us = 0;
static int64_t esp_timer_get_time() { return source_now_us; }
#include "k1_agc_dt_clock.h"
'''
    wrapper += low_pass + '\n'
    wrapper += '''std::array<float,80> source_broadband(const k1::core::audio::GdftRawFrame& input,
    const k1::core::audio::GdftConfiguration& frequencies, bool first) {
  source_now_us += first ? 10000 : 7500;
  constexpr unsigned NUM_FREQS = 80, NUM_AGC_BANDS = 4;
  static SQ15x16 agc_envelope = SQ15x16(0.0);
  static SQ15x16 agc_noise_floor = SQ15x16(0.001);
  static bool agc_gated = true;
  const bool VP_FIX_AGC_SOFT_KNEE = true;
  SQ15x16 agc_active_floor_debug[NUM_AGC_BANDS]{};
  float magnitudes_final[NUM_FREQS]{};
  static float magnitudes_average[NUM_FREQS]{}, magnitudes_last[NUM_FREQS]{};
  SQ15x16 spectral_tilt_lut[NUM_FREQS]{}, spectrogram[NUM_FREQS]{};
  for (unsigned i=0;i<NUM_FREQS;++i) {
    magnitudes_average[i]=input.bins[i].normalised_magnitude*0.3f + magnitudes_average[i]*0.7f;
    magnitudes_final[i]=low_pass_filter(magnitudes_average[i], magnitudes_last[i],100,
        float(1.0 + 10.0 * double(0.05f)));
    magnitudes_last[i]=magnitudes_final[i];
    float hz=frequencies[i].target_frequency_hz;
    spectral_tilt_lut[i]=SQ15x16(hz<200.0f?1.30f:hz>3000.0f?0.85f:1.0f);
  }
'''
    wrapper += constants + branch
    wrapper += '''
  std::array<float,80> out{};
  for (unsigned i=0;i<NUM_FREQS;++i) out[i]=float(spectrogram[i]);
  return out;
}
'''
    (output / 'inhouse_source_oracle.cpp').write_text(wrapper)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--reference', default=REFERENCE, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    oracle(args.reference, output)
    platform = ROOT / 'platform/ra8p1'
    source_path = platform / 'inhouse_audio.cpp'
    source = source_path.read_text()
    paths = [source_path, platform / 'inhouse_audio.h',
             ROOT / 'src/k1/core/visual/inhouse_waveform.h',
             ROOT / 'src/k1/core/audio/gdft_goertzel.cpp',
             ROOT / 'tests/host/test_inhouse_audio.cpp',
             ROOT / 'tests/host/test_inhouse_lane_overflow.cpp', Path(__file__)]
    receipt = {'scope': 'host arithmetic/chronology only; not latency, calibration or optical qualification',
               'source_sha256': {str(p.relative_to(ROOT)): sha(p) for p in paths},
               'reference_sha256': PINS, 'cases': []}
    variants = [('normal', None, None),
                ('wrong_rate', 'makeGdftConfiguration(core::audio::kLegacy12800AudioRate)',
                 'makeGdftConfiguration(core::audio::kProductionAudioRate)'),
                ('missing_peak_summary', 'out.observed_peak_scaled = std::max(out.observed_peak_scaled, item.peak);',
                 'out.observed_peak_scaled = 0.0F;'),
                ('stale_admitted', 'now_us - out.publish_us > stale_us', 'false')]
    variants += [('shared_mood', 'auto& state = conditioning_[board];', 'auto& state = conditioning_[0];'),
                 ('changed_source_lpf_clock', 'static_cast<double>(config_.reference_presentation_hz)',
                  '(static_cast<double>(kInhouseSampleRate) / kInhouseHopSamples)')]
    okay = True
    for name, old, new in variants:
        variant = output / (name + '.cpp')
        if old is not None:
            assert source.count(old) == 1
            variant.write_text(source.replace(old, new))
        else:
            variant.write_text(source)
        binary = output / name
        # The unchanged Arduino FixedPoints oracle uses in-class constants of
        # its incomplete own type, accepted by the source's GCC toolchain.
        command = ['/opt/homebrew/bin/g++-16', '-std=c++17', '-O1', '-g',
                   '-ffp-contract=off', '-fno-omit-frame-pointer',
                   '-fsanitize=undefined', '-fno-sanitize-recover=all',
                   '-I' + str(ROOT / 'src/k1'), '-I' + str(ROOT / 'src'),
                   '-I' + str(platform), '-I' + str(output),
                   '-I' + str(ROOT / 'tests/reference'),
                   '-I' + str(ROOT / 'tests/reference/vendor/FixedPoints/src'),
                   str(variant), str(ROOT / 'src/k1/core/audio/gdft_goertzel.cpp'),
                   str(output / 'inhouse_source_oracle.cpp'),
                   str(ROOT / 'tests/host/test_inhouse_audio.cpp'), '-o', str(binary)]
        build = subprocess.run(command, capture_output=True, text=True)
        (output / (name + '-build.log')).write_text(build.stdout + build.stderr)
        case = {'name': name, 'command': command, 'build_exit': build.returncode}
        matched = False
        if build.returncode == 0:
            run = subprocess.run([str(binary)], capture_output=True, text=True)
            (output / (name + '-run.log')).write_text(run.stdout + run.stderr)
            case.update(run_exit=run.returncode, binary_sha256=sha(binary))
            matched = run.returncode == 0 if old is None else run.returncode != 0 and 'Assertion failed' in run.stderr
        case['expected_exit_observed'] = matched
        receipt['cases'].append(case)
        okay &= matched
    normal_command = receipt['cases'][0]['command']
    prefix = normal_command[:normal_command.index(str(output / 'normal.cpp'))]
    boundary_command = prefix + [str(ROOT / 'tests/host/test_inhouse_lane_overflow.cpp'),
                                 str(ROOT / 'src/k1/core/audio/gdft_goertzel.cpp'),
                                 '-o', str(output / 'overflow-boundaries')]
    build = subprocess.run(boundary_command, capture_output=True, text=True)
    (output / 'overflow-build.log').write_text(build.stdout + build.stderr)
    boundary = {'name': 'overflow_boundaries', 'command': boundary_command, 'build_exit': build.returncode}
    matched = False
    if build.returncode == 0:
        run = subprocess.run([str(output / 'overflow-boundaries')], capture_output=True, text=True)
        (output / 'overflow-run.log').write_text(run.stdout + run.stderr)
        boundary.update(run_exit=run.returncode)
        matched = run.returncode == 0
    boundary['expected_exit_observed'] = matched
    receipt['cases'].append(boundary)
    okay &= matched
    receipt['source_unchanged'] = receipt['source_sha256'] == {str(p.relative_to(ROOT)): sha(p) for p in paths}
    okay &= receipt['source_unchanged']
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'expected_exits_observed': okay, 'receipt': str(output / 'receipt.json'),
                      'cases': [{k: c.get(k) for k in ('name', 'build_exit', 'run_exit', 'expected_exit_observed')}
                                for c in receipt['cases']]}, indent=2))
    return 0 if okay else 1


if __name__ == '__main__':
    raise SystemExit(main())
