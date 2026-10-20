# K1 R2 — independent cross-check evidence

**Review date:** 26 September 2026, Australia/Perth.  
**Remote inspection window:** approximately 02:37:55–02:46:57 +08:00.  
**Remote host:** Elroys-MacBook-Pro-5.local.  
**Scope:** repository/source review, selected host tests and counterexamples, linked target-image inspection. No product-source edits, commits, flashes, serial access, device resets, new physical measurements or product promotion.

This record transcribes the tool-observed results. The complete host scratch outputs remain at `/tmp/k1-r2-review-g7sevfwq` on the Mac; temporary storage is not a durable repository receipt. The included C++ probes are byte-identical to those compiled for the final regression runs. The replay wrapper was syntax-checked during packaging; its equivalent explicit compiler/run commands, not the wrapper itself, were executed during the remote review.

## E01 — identity and freshness

Roots are under `/Users/spectrasynq/Workspace_Management/Software/`.

| Root | Reported/retained anchor | Working state observed |
|---|---|---|
| `K1.Sliders` | `f143da58eb8d5ff71bef32579d77aa2add8bc65d`, `r2-console-flash` | Ten modified tracked files, added R2 policy/tests/evidence/recovery files. The candidate is not HEAD alone. |
| `SpectraSynq-K1-DualMCU-Firmware` | `21de29be3fff18244a4da7a81b21e13f35bb2694`, `int/reconcile-migration-lane` | Recovery notes and WS2812 driver modified. |
| `SpectraSynq-K1-RA8P1-Firmware` | `8427fccfb1c3b7fface2c2ba0a56a6017b785acf`, `wip/k1-ra8p1-002-mac-hold-2026-09-25` | Untracked planning/reference/worktree paths remain; “untouched by this session” is not “the repository is clean”. No Titan implementation reviewed here. |

The selected source hashes matched Grok's P0 ledger at entry and final read:

| Console path | SHA-256 |
|---|---|
| `src/console/main.cpp` | `b7ab42cbcec7e18fb517a5bafdb2847377d918bf498499b9ee57a0f91a3fc7b8` |
| `src/console/ui_r2/k1ui_profile_store.cpp` | `8f78787fa962b5aea97b8879e232d6b08bd043ea128c42a7e9e4f9387bfd4f17` |
| `src/console/mux.cpp` | `3a98f9e78e1ab10c9df0a116203473c059235018ecea6c8ad863655546b99c41` |
| `src/console/r2_control_policy.h` | `b176b4d7b06f87178b37557c557123d334f45c3d2fe512b69c4ddcdcd2d19031` |
| `.pio/build/console/firmware.bin` | `b70231814f177f6836abfcb5412b0084b28a5a31cff2fd9627908d57206f506d` |
| `.pio/build/console/firmware.elf` | `a25abe57c30cf45495101d7279e1683af44b7400b0be6fdf7e86c4a7726613cd` |

The BIN contains the ELF SHA-256 bytes. This corroborates the ELF/BIN pairing; it is not a new readback of the installed console and does not establish a complete transitive build-input manifest.

## E02 — two reported tests reproduced, but limited coverage

Attempted compiler: `/opt/homebrew/bin/g++-15`, with the runner's ASan/UBSan flags. Both selected builds failed at link with `ld: library 'asan' not found`. This is a toolchain/runtime availability failure, not a product-test result.

Fallback: `/usr/bin/clang++`, Apple clang 17.0.0 (clang-1700.6.3.2).

Common executed flags:

```text
-std=c++17 -O0 -g -Wall -Wextra -Werror
-fsanitize=address,undefined -fno-sanitize-recover=all -DK1_UI_R2
```

Includes: actual console input/UI, control bundle/compiler and transport bundle roots. Sources: the real `k1ui_profile_store.cpp`, vendored profile compiler/registry/codec, or the real physical-feedback adapter as applicable. Each test ran as a separate process.

```text
profile_clang BUILD_EXIT=0 RUN_EXIT=0
r2 commit accepted=1 error=0
PROFILE_STORE_R2_GATE=PASS

policy_clang BUILD_EXIT=0 RUN_EXIT=0
R2_PHYSICAL_POLICY_GATE=PASS
```

`test_profile_store_r2.cpp` first warms function-static staging buffers with a successful commit. Both following failure cases refuse an activation allocation on a fresh store; the purported `fail_after_two` case sets its counter to 100 before invoking the allocator. It does not exhaustively test every allocation or replacement of an existing profile.

`test_r2_physical_and_policy.cpp` checks the policy constant and copies one physical feedback value. It does not compile or drive the real `main.cpp` subscribe/scan/transmit paths. Removing a guard at a caller would not be detected by that constant check.

No full pinned console suite, INP/UI rendering suite, DualMCU IVV suite, or target behavioural test was executed in this review.

## E03 — four independent profile invariants fail

Probe source: `probes/review_assertions.cpp`. SHA-256:
`445a4d5fa4f60f7db187c826161b2b81e62e9bd698885fb96af9bc81f6834ff5`.

Build completed with exit 0 using the same Clang/sanitizer/device-macro configuration. Each scenario ran in a fresh process, against the actual product implementation, not a reimplementation of it. These are counterexamples, not complete replacement acceptance tests. In particular, future repair tests must also compare every active/backup field, error status, and all relevant revision/epoch state.

### Scenario 0 — revisions do not advance

Second edit uses the actual current base revision. The first range is 0–240, the edit changes it to 0–120, then Undo restores it.

```text
EXIT=1
SIZES draft=12004 compiled=11336 envelope=11554
SECOND scenario=0 accepted=1 error=0 revision1=1 revision2=1 active_max=120.0 backup=1
UNDO accepted=1 revision=1 max=240.0
REVIEW_ASSERT_FAIL scenario=0
```

The observed sequence is revision 1 → 1 → 1 despite different accepted content. This contradicts the header/parent requirement that Undo create a new admitted revision.

### Scenario 1 — backup allocation failure still accepts replacement

After one successful commit, allocator failure is injected before the second edit. Staging and active buffers already exist; the missing backup allocation fails.

```text
EXIT=1
SIZES draft=12004 compiled=11336 envelope=11554
SECOND scenario=1 accepted=1 error=0 revision1=1 revision2=1 active_max=120.0 backup=0
REVIEW_ASSERT_FAIL scenario=1
```

The edit is accepted, the active range changes, and the promised Undo backup is unavailable. The narrow false-success fix for first activation did not close failure atomicity for replacement.

### Scenario 2 — stale/mismatched expected revision is ignored

The active revision is 1. The next edit declares expected base revision 999.

```text
EXIT=1
SIZES draft=12004 compiled=11336 envelope=11554
SECOND scenario=2 accepted=1 error=0 revision1=1 revision2=1 active_max=120.0 backup=1
REVIEW_ASSERT_FAIL scenario=2
```

The edit is accepted. `BindingDraft::base_profile_revision` is documented as an expected-revision read set, not a decorative field.

### Scenario 3 — malformed slot is silently remapped

The UI binding slot is set to 99.

```text
EXIT=1
INVALID_SLOT accepted=1 active=1 first_slot_unused=0
REVIEW_ASSERT_FAIL invalid slot silently accepted
```

Source `k1ui_profile_store.cpp:461` converts an invalid slot to index 0 instead of rejecting it. Further tests must include page/channel mismatch, invalid page, malformed mapping/takeover fields and stale imports.

Relevant source: `k1ui_profile_store.cpp:348–403` (activation/allocation/persistence), `418–481` (commit), `483` onward (Undo); `k1ui_intents.h` (expected revision).

## E04 — compiled Import stack budget violation

Actual linked ELF inspected using:

```text
/Users/spectrasynq/.platformio/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp32s3-elf-objdump -d -C .pio/build/console/firmware.elf
```

Selected linked instructions:

```text
420316d4 <scan_task(void*)>:
420316d4: 022136 entry a1, 0x110

420364cc <k1ui::decodeDraft(unsigned char const*, unsigned int, k1::control_v2::ProfileDraft&)>:
420364cc: 5e6136 entry a1, 0x2f30

42036b48 <k1ui::ProfileStore::importProfile(unsigned char const*, unsigned int)>:
42036b48: 006136 entry a1, 48
```

`0x2f30 = 12,080` bytes for the decoder's frame. `main.cpp:439` allocates 4,096 stack bytes to `scan_task`. That task calls the UI-intent drain at line 284, which invokes Import and this decoder. Espressif's `xTaskCreatePinnedToCore` API specifies bytes, unlike vanilla FreeRTOS's convention.

This is now a compiled, reachable resource defect—not merely the earlier source-level suspicion about a large local. No target Import was deliberately triggered and no crash was observed in this review. Fix the ownership/workspace/call path before exercising the editor on hardware.

Official API reference consulted: `https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/system/freertos_additions.html`, `xTaskCreatePinnedToCore`, `ulStackDepth`.

## E05 — specific BLE identity-admission mismatch

Source reviewed:

- Console `src/console/midi.cpp`, complete file. `ble_midi_begin()` creates only the standard MIDI service (`createService` at line 125), MIDI characteristic and CCCD. It advertises `K1-CONSOLE`.
- Completed search of `src/console` for `createService|9f3e5c1|K1C24|IdentityService` found only that service creation.
- S3 `firmware/s3_ble/ble_remoted_central.cpp:184` onward: discover identity service and validate before subscribing. Failure returns to the disconnect path.
- S3 `firmware/s3_ble/ble_identity.h:113–132`: a peer named K1-CONSOLE without the identity characteristic is refused.
- `firmware/s3_ble/k1_console_contract.generated.h`: generated from the peer registry; product 0xC024, magic K1C24, 87-byte identity including registry/layout integrity checks.

Executed header-level receiver probe, exit 0:

```text
IDENTITY scan_may_connect=1 missing_identity_authorised=0 generated_fixture_authorised=1 corrupted_fixture_parsed=0
```

Source SHA-256: `9920423b73c9d5c5ceac75b7923dcffed28be82d1f1ba88a23e2a92ea914e0bd`.

Meaning: scan admission and post-connect identity admission are different. When this source recognises the peer as K1-CONSOLE, missing identity predicts rejection before subscription. This is a concrete interoperability defect in the source pair. It does NOT prove the unique cause of an earlier disconnect without binding the resident radio image and observing the failure stage/peer name. Encryption or discovery faults could additionally exist.

The generated fixture is used only to test the receiver. Do not paste its bytes into an incompatible console or bypass the guard. Its layout describes a specific legacy mapping; actual R2 identity/capability and payload semantics must agree truthfully on both sides.

## E06 — memory gain is real; attribution and completeness need correction

Observed linker map:

```text
.dram0.data            19364
.dram0.bss            128496
.bss.work_mem_int$0    49152  .../libd64/lvgl/misc/lv_mem.c.o
.bss._ZL12s_ui_runtime 16000  .../src/console/main.cpp.o
.bss._ZL5s_inp         29728  .../src/console/main.cpp.o
```

The 49,152-byte pool is LVGL, not Bluedroid. The real v2 client is absent; lazy profile allocations are not represented by their eventual payload sizes in this static census.

Using the observed host type sizes, the retained payload storage after successful first use of Commit, a second Commit, Undo, Import and Export can include five 12,004-byte drafts, two 11,336-byte compiled profiles, and the 11,554-byte export buffer: **94,246 bytes**, excluding allocator overhead, fallback getters, decoder stack and the missing client. This is an allocation-lifetime calculation from the code—not measured target heap usage and not proof all allocations use internal RAM. Allocation placement must be established against the actual SDK configuration.

## E07 — remaining application seams

`main.cpp:166–185` reads live mutable input/profile/local data from the LVGL task while scan updates them. It also marks a fallback physical value valid without checking whether a valid scan has occurred, and stamps UI refresh time rather than preserving input sample age. The physical helper test does not cover that callback.

`main.cpp:188` and the transmit task guard explicit subscribe-send-all, but `maybe_hysteresis` at line 227 and its unconditional scan call still feed legacy MIDI. Stimulus and pad paths also need the selected-owner rule. A zero-valued static baseline can qualify an initial stationary nonzero position through the legacy hysteresis logic. This is source reachability, not a newly captured over-air write.

`main.cpp:302` passes only mux sweep duration to `finish_scan`, excluding substantial work performed by the task. It is not evidence that whole scan execution meets the 5 ms schedule.

`persistActiveDraft()` now returns false, but line 401 discards its return and the UI still maps local compilation to generic Accepted. An intentionally RAM-only draft is permissible only with explicit unsaved/unapplied status; a durable commit requires the actual persistence outcome.

## E08 — evidence boundaries and corrections to the report

Grok's boot note corroborates its reported first ADC sweep and BLE advertisement. That note is not independent raw target data and does not establish per-fader travel, calibration, successful subscription, returned engine state or a visible R2 legend.

An LED output-buffer dump cannot close physical emission or optical fidelity. The report's later “scored dump” shortcut does not supersede its own P4 electrical/optical distinction or the handoff. The earlier short-strip problem being a wrong-image issue is reported, but not independently established in this audit; retain candidate/timeline evidence rather than treating a causal statement as measurement.

The optional preflight script belonged to the supplied handoff package; it was not promised as a tracked Sliders file. The pinned host reference has an existing sanctioned fetch helper; do not reset the live integration checkout or silently weaken the pin gate.
