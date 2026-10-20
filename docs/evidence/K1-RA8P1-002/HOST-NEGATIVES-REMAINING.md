# HOST-NEGATIVES-REMAINING

**Date:** 2026-09-20  
**Repo:** `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware`  
**Scope:** Host-only unittest / self-test runs. No firmware flashed. No CDC touched. No git commit.  
**Note:** Physical PASS not claimed. All results are host-compile + host-run only.

---

## File SHAs (working-tree bytes, sha256)

| File | SHA256 |
|------|--------|
| `tests/host/test_score_p601_capture.py` | `147959e8fb4c16f0db58e41bbdd07c19e8dd3435c36e551722b685f528c6ffd4` |
| `tests/host/test_gpt_fault_witness.py` | `923ba0582c08854fed992bc4777424063113ea7b473b8d5e079dc16ef026b351` |
| `tests/host/test_mode32_real_audio_observation.py` | `42c83e89850efdf23649515be22453c826cd99474af7e0525c26d1a9fe35fdee` |
| `tests/host/test_tempo_placement.py` | `dcc15ce5145b7bcdf2fb87ae959a016c0d463997bcb794304bb672f9750ecc28` |
| `tools/serial-studio/tests/test_identity_checkpoint.py` | `2a9a32d04dcb8ebe26c90f05fb941e5a6bb4c2242eb8ae997bf3159b471f6197` |
| `tools/serial-studio/tests/test_ss03_broker.py` | `ca8741a3ec8f67affd6e85f1848510618b20ab801c59cdc231c8974b3592eb82` |
| `tools/serial-studio/tests/test_ss03_decode.py` | `a59a5f5579ecbbd45e7eec52d62e5aa277be65e79e8c7f4470b4fccee979b4b5` |
| `scripts/score_p601_capture.py` | `51fcf8b9af677b5a39f385e60ab27ce9b8b120bb1d3b6460e2f8e4720eb0baf6` |
| `scripts/test_palette_runtime.py` | `d19fe1715a91c1761d5a02a9bb308b36d9c6f63af652a0a5990aa84df6aa1bb8` |

---

## GREEN — All Passed (do not restart this campaign)

### Run 1 — `python3 -m unittest tests.host.test_score_p601_capture tests.host.test_gpt_fault_witness -v`

**Command:**
```
python3 -m unittest tests.host.test_score_p601_capture tests.host.test_gpt_fault_witness -v
```

**Result:** 12 tests, 0 failures, 0 errors — **OK** (1.950s)

| Test | Result |
|------|--------|
| `test_score_p601_capture.ScoreP601Capture.test_empty_object_and_placeholder_hash_fail` | PASS |
| `test_score_p601_capture.ScoreP601Capture.test_hashed_raw_files_with_conforming_pulses_qualify` | PASS |
| `test_score_p601_capture.ScoreP601Capture.test_hashed_raw_files_with_illegal_pulses_remain_failing_evidence` | PASS |
| `test_score_p601_capture.ScoreP601Capture.test_historical_ws2812_diagnostic_is_not_the_retained_frame` | PASS |
| `test_score_p601_capture.ScoreP601Capture.test_missing_capture_fails` | PASS |
| `test_score_p601_capture.ScoreP601Capture.test_physical_json_without_raw_files_is_not_qualification` | PASS |
| `test_score_p601_capture.ScoreP601Capture.test_predicted_reference_matches_retained_zero_frame` | PASS |
| `test_score_p601_capture.ScoreP601Capture.test_self_test_bundle_passes_without_claiming_wire` | PASS |
| `test_score_p601_capture.ScoreP601Capture.test_short_and_synthetic_never_stamp_waveform_captured` | PASS |
| `test_score_p601_capture.ScoreP601Capture.test_written_capture_file_without_raw_files_is_not_waveform` | PASS |
| `test_gpt_fault_witness.GptFaultWitness.test_actual_fault_frame_and_hashes` | PASS |
| `test_gpt_fault_witness.GptFaultWitness.test_mutated_layout_publication_identity_or_payload_rejected` | PASS |

Last 10 lines of output:
```
test_written_capture_file_without_raw_files_is_not_waveform (...) ... ok
test_actual_fault_frame_and_hashes (...) ... ok
test_mutated_layout_publication_identity_or_payload_rejected (...) ... ok

----------------------------------------------------------------------
Ran 12 tests in 1.950s

OK
```

---

### Run 2 — `python3 scripts/score_p601_capture.py --self-test`

**Command:**
```
python3 scripts/score_p601_capture.py --self-test
```

**Result:** PASS (exit 0, 477ms)

Last 30 lines of output:
```json
{
  "pass": true,
  "K1_P601_SELFTEST": "PASS",
  "waveform": "NOT_CAPTURED",
  "physical_claims": false,
  "predicted_bits": 3072,
  "predicted_dma_words": 3070,
  "packed_grb_sha256": "a1a4f5721c1c4610af7f71078f3a68c330536d679803b0e0507ee8dc10c5dfca",
  "proofs": [
    "MISSING_FAIL",
    "HISTORICAL_FAIL",
    "SHORT_SYNTHETIC_NOT_CAPTURED",
    "COMPLETE_SYNTHETIC_NOT_CAPTURED",
    "OVERCLAIM_FAIL",
    "PHYSICAL_WITHOUT_FILES_REJECTED"
  ]
}
```

---

### Run 3 — serial-studio tests (test_identity_checkpoint, test_ss03_broker, test_ss03_decode)

**Command (dotted path via unittest — returned exit 5, NO TESTS RAN):**
```
python3 -m unittest tools.serial-studio.tests.test_identity_checkpoint \
  tools.serial-studio.tests.test_ss03_broker \
  tools.serial-studio.tests.test_ss03_decode -v
```
> **Root cause:** Tests are pytest-style function tests (no class inheritance from `unittest.TestCase`).  
> No `__init__.py` in `tools/serial-studio/tests/`. Dotted-path import fails silently in unittest runner.

**Fallback command (used; PASS):**
```
cd tools/serial-studio && PYTHONPATH=. python3 -m pytest \
  tests/test_identity_checkpoint.py tests/test_ss03_broker.py \
  tests/test_ss03_decode.py -v
```

**Result:** 13 passed in 1.28s (exit 0)

| Test | Result |
|------|--------|
| `test_identity_checkpoint.py::test_unbound_builds_are_identified_not_accepted` | PASS |
| `test_identity_checkpoint.py::test_checkpoint_match_and_mismatch` | PASS |
| `test_identity_checkpoint.py::test_bind_keeps_unaccepted_observation_truthful` | PASS |
| `test_ss03_broker.py::test_payload_never_writes_serial` | PASS |
| `test_ss03_broker.py::test_replay_only_origin` | PASS |
| `test_ss03_broker.py::test_stale_device_age_advances_without_new_sample` | PASS |
| `test_ss03_broker.py::test_display_sequence_does_not_clear_freeze` | PASS |
| `test_ss03_decode.py::test_schema2_unchanged` | PASS |
| `test_ss03_decode.py::test_independent_unavailable` | PASS |
| `test_ss03_decode.py::test_replay_overrides_live_state` | PASS |
| `test_ss03_decode.py::test_identity_same_frame` | PASS |
| `test_ss03_decode.py::test_last_emit_cycles_is_not_hop_compute` | PASS |
| `test_ss03_decode.py::test_csv_rejects_comma` | PASS |

Last 10 lines of output:
```
tests/test_ss03_decode.py::test_identity_same_frame PASSED               [ 84%]
tests/test_ss03_decode.py::test_last_emit_cycles_is_not_hop_compute PASSED [ 92%]
tests/test_ss03_decode.py::test_csv_rejects_comma PASSED                 [100%]

============================== 13 passed in 1.28s ==============================
```

---

### Run 4 — `python3 -m unittest tests.host.test_mode32_real_audio_observation -v`

**Command:**
```
python3 -m unittest tests.host.test_mode32_real_audio_observation -v
```

**Result:** 2 tests, 0 failures — **OK** (0.000s)

| Test | Result |
|------|--------|
| `Mode32Observation.test_exact_length_black_is_zero` | PASS |
| `Mode32Observation.test_short_frame_is_not_black` | PASS |

Last 10 lines of output:
```
test_exact_length_black_is_zero (...) ... ok
test_short_frame_is_not_black (...) ... ok

----------------------------------------------------------------------
Ran 2 tests in 0.000s

OK
```

---

### Run 5 — `python3 -u scripts/test_palette_runtime.py`

**Command:**
```
python3 -u scripts/test_palette_runtime.py
```

**Result:** PASS (exit 0, elapsed ~371s — host compile+run script, clang++ only, no ARM toolchain)

Last 30 lines of output:
```
AUTOSTART WS2816 PALETTE_PROTOCOL_PASS palettes=44 channels=2 native_output_after_disconnect=true
clang++: warning: treating 'c' input as 'c++' when in C++ mode, this behavior is deprecated [-Wdeprecated]
clang++: warning: treating 'c' input as 'c++' when in C++ mode, this behavior is deprecated [-Wdeprecated]
clang++: warning: treating 'c' input as 'c++' when in C++ mode, this behavior is deprecated [-Wdeprecated]
AUTOSTART_MUTATION_PASS bounce_boot_rejected=true
MORPH ACTIVE_EFFECT_MORPH_PASS modes_at_start=23 modes_changed_at_midpoint=10
PALETTE_TRANSITION_PASS exact_endpoints=11264 interrupted_pairs=44 contributors=44 bytes_per_channel=392 host_us_per_160_worst=505.15
MORPH CENTRE_EFFECTS_PASS combinations=352 mirror_and_direction_samples=337920 visible_frames=4224 physical_128_mirror=true peak_out=20 peak_in=59 clock_wrap_and_fraction=true
BACKEND PALETTE_BACKEND configured=unknown emit_on=unknown emit_off=disabled physical_admission=unproven
BACKEND PALETTE_BACKEND configured=gpio_diagnostic emit_on=gpio_diagnostic emit_off=disabled physical_admission=unproven
BACKEND PALETTE_BACKEND configured=ws2816_gpio emit_on=ws2816_gpio emit_off=disabled physical_admission=unproven
BACKEND PALETTE_BACKEND configured=gpt_dma emit_on=gpt_dma emit_off=disabled physical_admission=unproven
BACKEND_EXCLUSIVE_PASS gpt_dma_vs_ws2816=true
PALETTE_COMPATIBILITY_PASS independent_executables=2 overlay_mutation_rejected=true live_audio_differs=true
```

> **Note:** `physical_admission=unproven` on all backends is correct and expected. This is a host-compile artefact; it does not claim wire-level admission.

---

### Run 6 — `python3 -m unittest tests.host.test_tempo_placement -v`

*Matched criteria: tests matching `*tempo*placement*`*

**Command:**
```
python3 -m unittest tests.host.test_tempo_placement -v
```

**Result:** 5 tests, 0 failures — **OK** (0.392s)

| Test | Result |
|------|--------|
| `TempoPlacementTests.test_build_scalar_requires_named_placement` | PASS |
| `TempoPlacementTests.test_elf_dtcm_requires_dtcm_addresses` | PASS |
| `TempoPlacementTests.test_elf_empty_tcm_rejects_tcm_symbols` | PASS |
| `TempoPlacementTests.test_receipt_rejects_mismatched_overlay` | PASS |
| `TempoPlacementTests.test_staged_empty_tcm_does_not_rewrite` | PASS |

---

## RED / ERROR — Remaining negatives to execute

None of the six commanded test suites failed. **Zero failing tests.** There are no RED entries from the runs above.

### Negatives found in source but already covered

The following negative/mutation patterns were grepped and confirmed covered by the passing suites above:

| Pattern | Location | Coverage |
|---------|----------|----------|
| `missing-counter` (missing capture fails) | `test_score_p601_capture.py:test_missing_capture_fails` | **COVERED — PASS** |
| `capture-exception` (empty/placeholder hash fail) | `test_score_p601_capture.py:test_empty_object_and_placeholder_hash_fail` | **COVERED — PASS** |
| `restore-failure` (injected restore error) | `test_colour_integrity.py` (assertIn `'injected restore failure'`) | **NOT RUN — see §NOT RUN** |
| `age invalidation` (stale device age) | `test_ss03_broker.py::test_stale_device_age_advances_without_new_sample` | **COVERED — PASS** |
| `last_emit_cycles as hop_max_us` | `test_ss03_decode.py::test_last_emit_cycles_is_not_hop_compute` | **COVERED — PASS** |
| `synthetic-physical tamper` | `test_score_p601_capture.py:test_short_and_synthetic_never_stamp_waveform_captured` | **COVERED — PASS** |
| `synthetic-physical tamper` (application hash) | `test_pdm_capture.py` and `test_audio_npu_pair.py` (bind_adapter tamper) | **NOT RUN — see §NOT RUN** |
| `identity_ok` separation | `test_ss03_decode.py::test_identity_same_frame`, `test_identity_checkpoint.py` | **COVERED — PASS** |
| `overlay_mutation_rejected` | `scripts/test_palette_runtime.py` `PALETTE_COMPATIBILITY_PASS` | **COVERED — PASS** |
| `mutated layout rejected` | `test_gpt_fault_witness.py::test_mutated_layout_publication_identity_or_payload_rejected` | **COVERED — PASS** |

---

## NOT RUN — with reason

| Suite / File | Reason |
|---|---|
| `tests.host.test_colour_integrity` | Not in the commanded run list. Contains injected `restore_failure` negative. Run separately with `python3 -m unittest tests.host.test_colour_integrity -v` if restore-failure coverage is required. |
| `tests.host.test_pdm_capture` | Not in the commanded run list. Contains `tampered["application"] = "ab" * 32` synthetic-physical tamper negative. Likely requires a real or mock PDM fixture. |
| `tests.host.test_audio_npu_pair` | Not in the commanded run list. Contains same `bind_adapter` tamper pattern as test_pdm_capture. May need NPU fixture data. |
| All other `tests/host/test_*.py` (≥15 files) | Outside commanded scope: `test_arm_numeric_probe`, `test_compare_stage_profiles`, `test_dmac_priority_stage`, `test_dual_pdm_target`, `test_fixture_wire`, `test_freeze_s3_comparator`, `test_gold_extract`, `test_no_synthetic_room_audio`, `test_pcm1808_core`, `test_pcm1808_target`, `test_platform_math_probe`, `test_quiet_gap_runner`, `test_quiet_gap_score`, `test_resident_fixture`, `test_scalar_codegen`, `test_sconscript_k1_guard`, `test_stage_profile`, `test_target_resources`, `test_target_trace_audit`, `test_verify_imports` |
| All other `tools/serial-studio/tests/test_*.py` (≥6 files) | Outside commanded scope: `test_checkpoint_match`, `test_source_ownership`, `test_ss03_lock`, `test_ss03_transport`, `test_titan_parser`, `test_watchdog_stale` |
| `scripts/test_*.py` other than `test_palette_runtime.py` | Outside commanded scope or require target hardware |

---

## skipTest recorded

One conditional skip was observed in the passing suite:

- `tests/host/test_score_p601_capture.py:54` — `self.skipTest('optional Mac historical receipt not present')`  
  This test (`test_historical_ws2812_diagnostic_is_not_the_retained_frame`) completed as **OK** in this run, meaning the Mac historical receipt **was present** and the skip branch was not taken.

---

## Summary counts

| Run | Tests | Passed | Failed | Errors | Skipped |
|-----|-------|--------|--------|--------|---------|
| 1: test_score_p601_capture + test_gpt_fault_witness | 12 | 12 | 0 | 0 | 0 |
| 2: score_p601_capture --self-test | 1 (self-test) | 1 | 0 | 0 | — |
| 3: test_identity_checkpoint + test_ss03_broker + test_ss03_decode | 13 | 13 | 0 | 0 | 0 |
| 4: test_mode32_real_audio_observation | 2 | 2 | 0 | 0 | 0 |
| 5: test_palette_runtime.py (script) | ~14 sub-checks | 14 | 0 | 0 | — |
| 6: test_tempo_placement | 5 | 5 | 0 | 0 | 0 |
| **TOTAL** | **47** | **47** | **0** | **0** | **0** |

## Orchestrator re-run (lead, 2026-09-20 afternoon)

Decision-critical suites re-run by the persistent lead, not taken from the subagent alone:

- `python3 -m unittest tests.host.test_score_p601_capture tests.host.test_gpt_fault_witness tests.host.test_tempo_placement tests.host.test_mode32_real_audio_observation -v` → 19 OK
- `python3 scripts/score_p601_capture.py --self-test` → PASS, waveform=NOT_CAPTURED
- `cd tools/serial-studio && PYTHONPATH=. python3 -m pytest tests/test_identity_checkpoint.py tests/test_ss03_broker.py tests/test_ss03_decode.py -q` → 13 passed
- Additional remaining negative: `python3 -m unittest tests.host.test_colour_integrity -v` → 8 OK (includes restore-error CDC release)

`test_pdm_capture` and `test_audio_npu_pair` remain NOT RUN.

Palette `scripts/test_palette_runtime.py` (~6 min host compile) was not re-run this afternoon.
