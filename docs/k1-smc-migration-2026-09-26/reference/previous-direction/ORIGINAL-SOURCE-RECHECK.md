# Source recheck — corrections and regression targets

Prepared 25 September 2026 (Australia/Perth) using Remote Desktop Commander reads of the user's Mac during the follow-up asking whether the assessment was executable.

This is a source-review record. No new product build, host-test execution, serial interrogation, flash, electrical measurement or optical measurement was performed. The reads were not an atomic filesystem snapshot. Rehash/revalidate at execution; do not use this note as current image identity.

Root for all console paths below:
`/Users/spectrasynq/Workspace_Management/Software/K1.Sliders/`

The follow-up directly read `.git/HEAD` and `.git/refs/heads/r2-console-flash`: the branch remained `r2-console-flash` at `f143da58eb8d5ff71bef32579d77aa2add8bc65d`. Nonetheless, the source differed from the preceding inspection. That establishes why a refreshed content inventory is mandatory.

## R01 — host and device select different persistence behaviour

Sources: `src/console/ui_r2/k1ui_profile_store.h` and `.cpp`, especially constructor declarations, `boot()` and `persistActiveDraft()`; `tests/host/console/run_host_tests.sh`.

Observed implementation:

```cpp
bool ProfileStore::persistActiveDraft() noexcept {
#ifdef K1_UI_R2
  return true;
#else
  // Encodes and calls the real PersistentStore.
```

The device header declares a default constructor and pointer-backed drafts under `K1_UI_R2`; the alternate branch uses `ByteStore`/`PersistentStore`. The host runner's explicit FLAGS do not include `-DK1_UI_R2`.

Finding: a passing alternate-branch persistence test does not validate device persistence. Device saves are a no-op in the read code. The active editor/profile path must be tested under the actual device preprocessor configuration.

## R02 — allocation failure can retain a successful outcome

Source: `src/console/ui_r2/k1ui_profile_store.cpp`, `applyDraft()` and `commit()`.

Observed sequence:

```cpp
outcome.accepted = result.ok();
// ... compile error fields are copied; compile rejection returns.
#ifdef K1_UI_R2
if (!active_draft_) {
  active_draft_ = new (std::nothrow) cv2::ProfileDraft{};
  if (!active_draft_) return false;
}
if (!active_compiled_) {
  active_compiled_ = new (std::nothrow) cv2::CompiledProfile{};
  if (!active_compiled_) return false;
}
```

The caller then does:

```cpp
CommitOutcome outcome{};
applyDraft(next, outcome);
return outcome;
```

Inference directly from control flow: after successful compilation, allocation failure at either activation allocation returns false without clearing `accepted`. `commit()` ignores that boolean and can return an accepted outcome despite failed activation. This is a specific source-level defect path, not a reproduced target failure. Inject allocator failure at every allocation site and prove no accepted/partial transaction.

The getters also create fallback objects dynamically and call `abort()` on allocation failure. Inspect those error paths rather than treating a memory-saving refactor as complete when startup succeeds.

## R03 — the actual device no longer instantiates the earlier v2 client in main

Source: `src/console/main.cpp`.

The read R2 path contains:

```cpp
static void k1_ui_r2_transport_send_stub(
    const k1::console::input::GestureIntent&) {
    ++s_ui_r2_transport_stub_calls;
}
```

The scan task drains `next_intent()` into this stub. `s_profile_view.commit` is assigned `TxnStatus::Accepted` from the local outcome; the branch logs `profile committed locally`. The UI link field uses `ble_midi_connected()` and the engine epoch is set to zero.

Finding: the current source does not establish v2 carriage, application acceptance or engine-synchronised state. A locally accepted draft can be a legitimate distinct state, but cannot stand in for durable save or engine application.

## R04 — reconnect-time legacy writes remain reachable

Source: `src/console/main.cpp`, `on_subscribe`, `scan_task`, `maybe_hysteresis`, `ble_tx_task`, `setup`.

The subscription callback sets `s_send_all=true`. The transmit task also sets it on subscription transition, then emits all enabled fader values. The normal scan loop invokes legacy hysteresis/emission, and the R2 scan block additionally processes its own input runtime. The BLE transmit task is created in setup without an R2 exclusion.

Finding: enabling BLE can make the legacy path active independently of the R2 session/binding model. This is a reachable source path, not a fresh on-wire observation. Before using an R2 candidate with the real engine, prove zero stationary subscription writes and a single selected owner for each physical control event.

## R05 — profile navigation and snapshot publication need real integration

Source: `src/console/main.cpp`, the UI-intent drain and snapshot callback.

Navigation intents are drained/counted but the read code does not apply the compiled view to the input runtime. The scan task updates profile-view state that the LVGL callback copies; comments acknowledge missing coherent producer/consumer publication. A plain shared struct or unsafely reused pair of buffers does not constitute a concurrency proof.

Finding: add the real admitted binding/view path and properly synchronised snapshot ownership. Verify these at the application adapter boundary, not solely inside isolated UI widgets.

## R06 — stack/lifetime pressure can move rather than disappear

Sources: `src/console/ui_r2/k1ui_profile_store.cpp::decodeDraft`, `commit`, `applyDraft`, `undo`, `importProfile`; `src/console/main.cpp::setup` and its scan task.

The read decoder still declares `cv2::ProfileDraft draft{}` automatically. The scan task is created with a 4096 stack argument. Other profile buffers are now lazy allocations or static pointers.

Review target: inspect actual object sizes, compiler stack usage and whole call-chain depth, plus first-use heap high-water under editing/import/undo. This note does not claim a measured stack overflow; compiler optimisation and the actual call path must be checked.

## R07 — the proposed tile height was not a selected solution

Source: `src/console/display.cpp::lvgl_rounder_cb`.

The rounder aligns starting coordinates down to even values and expands ending coordinates to odd values, producing even-sized spans. Therefore the earlier 15-line arithmetic example is not automatically an appropriate display tile choice. Prove tile capacity, rounded regions, rotation, flush completion and driver constraints against the actual configuration.

## R08 — instruction files contain historical sections

Source: DualMCU `docs/agent/DUALMCU_BENCH_IDENTITY_CANON.md`.

It includes the later decision to retain both S3 images, but also older UART-only/H2 instructions that conflict with the later SPI source and receipts inspected in the previous assessment. Retain valid USB/MAC/board refusal rules; do not treat an old transport paragraph as authorisation to undo newer integration. Resolve the actual source/physical mapping before affected bench work.

The referenced top-level `PROTECTED_FILES.md` returned ENOENT in this read. That is a missing reference at that path, not proof of unrestricted write authority. Locate the current protected-file procedure.

## R09 — pinned test dependency is not the active integration checkout

Source: `tests/host/console/DUALMCU_PIN` and `run_host_tests.sh`.

The directly read pin was `97dc8cf73318dfea1329e29a879fa8b9db1a7a6b`. The prior active DualMCU inspection anchor was `21de29b`. The runner requires explicit `DUALMCU_ROOT` and checks HEAD against the pin. Never reset the active integration tree merely to make the host test pass. Use the sanctioned reference procedure or a reviewed repin; an unpinned diagnostic override is not pinned-gate acceptance.

## Source-review boundaries

The facts above refer to source read during this follow-up. They do not establish whether another agent has already repaired them, whether a new image was built, or what image is now on the console. Grok must refresh the selected files, ownership and device evidence before applying changes. No personal credentials or unrelated project contents are included in this note.
