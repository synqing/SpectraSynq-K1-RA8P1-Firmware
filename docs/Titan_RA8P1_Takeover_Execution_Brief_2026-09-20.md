# Titan Mini / RA8P1 — development review and agent execution brief

Reviewed: 19 September 2026, 22:20 UTC / 20 September 2026, 06:20 Perth.

Repository: [synqing/SpectraSynq-K1-RA8P1-Firmware](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware).

Audited commit: **`431140853dd8b58af53240ef84d5fb1a08bd8b45`**. Both remote `main` and `lane/k1-ra8p1-002` pointed to this commit, checked again at the end of inspection. No pull requests, open issues, or Actions runs were returned by GitHub. Absence of Actions is not evidence that the local tests failed.

This review inspected current repository source, history, build logic, test harnesses, and evidence documents, plus the earlier `k1-ra8p1-002_remaining_15d558a2.plan.md`. It ran focused host checks. It did not access the Mac, query Titan, inspect external raw hardware receipts, cross-build an ARM image, change firmware, flash, or modify GitHub. Hardware results below are explicitly identified as repository-recorded history, not newly witnessed measurements.

## 1. Assessment and finish line

**Titan is progressing as an evaluation platform. It is not yet a qualified K1 product runtime, and the evidence does not justify promoting it over RT1062.** The portable compute path has substantial demonstrated value: exact scalar parity, a qualified scalar configuration, live microphone-to-analysis integration, palette handling, and a hardware-driven transmitter. The remaining difficulty is closing the complete live system with trustworthy measurements.

The immediate engineering priority is the unresolved LED transmission failure. Three controlled DMA policies produced the same terminal one-word remainder. Recent audio sensitivity, dwell, and boot-mode changes may improve behaviour, but they do not resolve or qualify that transmitter. More visual tuning while transmission remains unproven would confound the diagnosis.

There is also evidence-maintenance debt. Several documents call different images “current”; the external receipt index carries an obsolete current image; the waveform scorer can accept unsupported physical claims; and the timing instrument's build configuration conflicts with its written measurement contract. These are concrete repair tasks, not reasons to restart the project.

The receiving agent's immediate finish line is:

1. Establish a consistent source/build/device/evidence checkpoint.
2. Repair capture scoring and observation identity so unsupported results cannot become physical or current-build acceptance.
3. Obtain a new, correctly correlated P601/DIN capture, isolate the first measured divergence, implement its bounded repair, and repeat the existing controls.
4. Complete honest uncapped timing analysis and qualify the selected combined live-audio/visual image within an explicit scope.
5. Leave product-output, physical latency, comparison, and hardware-dependent work with exact prerequisites and owners.

Completion means executing the software work, testing it, retaining receipts, and closing the applicable acceptance rows. A revised plan alone is not completion. Unavailable hardware may block the affected measurement; it does not block independent source, harness, documentation, or replay work.

This packet is an implementation handoff prepared at Captain's request. No implementation or live-device action was performed by this review. When assigned to execute it, follow the existing device operator protocol and any newer explicit instruction. Do not infer permission to move wiring, alter sibling repositories, or make the final architecture decision.

## 2. Authority, environment, and invariants

Read these in order before changing the platform:

| Authority | Purpose and precedence |
| --- | --- |
| Root [`AGENTS.md`](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/AGENTS.md) | Source boundaries, negative proofs, platform-memory recall, Rearm protocol, centre-origin mandate. |
| `docs/REFERENCE-MANIFEST.md` | Exact behavioural and BSP pins. Never follow a moving sibling HEAD as behavioural authority. |
| `docs/evidence/K1-RA8P1-002/STATUS.md`, newest dated entry | Latest repository-recorded programme state. Historical paragraphs below it do not override it. |
| `docs/evidence/K1-RA8P1-002/TITAN-COLOUR-INTEGRATION.md` | Guard repair, three failed policies, immutable fault witness, original control procedure. Its old resident-image wording is superseded. |
| `docs/evidence/K1-RA8P1-002/TITAN-MIC-LED2-SESSION-CANON.md` | Settled microphone, PHY/LED2 and operator facts. Its September 13 residence is historical. |
| `docs/superpowers/plans/2026-09-19-k1-ra8p1-002-remaining.md` | Updated continuation; supersedes the longer earlier plan where the work has advanced. |
| `docs/decisions/2026-09-10-audio-to-light-ceiling.md` | Current latency definition and 12 ms median ceiling; overrides stale blanket sub-8-ms wording. |
| `docs/BRIEF-2026-09-10-uncapped-ap-timing.md` | Questions Q1–Q5 and the original controlled measurement conditions. Reconcile its TCM conflict before calling that experiment complete. |
| `docs/EXECUTION-BRIEF.md` | Original mission and final comparison questions; not a direction to redo bootstrap. |

Expected Mac locations, taken from repository authority:

| Item | Location / identity |
| --- | --- |
| Firmware checkout | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware` |
| External evidence root | `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002` |
| Platform knowledge package | `/Users/spectrasynq/Workspace_Management/Software/agent-skills/packages/ra8p1-titan-engineering` |
| Behavioural reference | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware` at `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a` |
| BSP reference | `/Users/spectrasynq/Workspace_Management/Software/sdk-bsp-ra8p1-titan-mini` at `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7` |
| Supporting Lab | `/Users/spectrasynq/SpectraSynq-EdgeAI-Lab` — programmer and external campaign assets, not this firmware's edit authority |
| Physical target | Titan Mini HW V1.0; UID **`545433931bd25436593630352d068363`** |
| Application USB | `045b:5310`; an enumerated port does not establish build or UID |

The platform knowledge package was unavailable in this review environment. Repository-linked source receipts and the session canon were used, as `AGENTS.md` permits. The Mac agent must load the canonical package, run its `scripts/platform_memory.py --check`, and recall the relevant `pins`, `clocks`, `dma`, and `ws2816` topics. Do not substitute an old generated plugin cache.

Preserve these invariants:

- M85 owns AP and VP. M33 remains parked. U55 is not an implicit production dependency. D-cache remains disabled unless a separately scoped experiment is explicitly reopened; a cache-on candidate previously lost USB.
- Preserve `MEDIA_TIME_48K`, musical time, event time versus availability time, affine clock mapping, epochs, and bounded rendering semantics. Do not borrow RT1062 timing constants as Titan measurements.
- Current production-candidate AP contract is **24,000 Hz, 180 samples, 7,500 µs hop, 80 bins, crossover 40**. The 12.8 kHz/96 legacy path and 16 kHz microphone diagnostic are separate contracts.
- Logical geometry is **two channels of 160 pixels each**. Centres are 79/80. The current 128-pixel WS2812 bench centre is 63/64. Every exposed motion must move centre-to-edge or edge-to-centre; mirror symmetry alone is insufficient.
- The onboard microphones are LinkMems **LMD2718T261-OA1**, not IM69D130. P502/PDMDAT2 and P812/PDMCLK2 are shared. U14 LOW/ch2 RISE is the expected programme lane; U13 HIGH/ch0 FALL is the expected measurement lane. Acoustic identity remains unresolved.
- P601 is GTIOC6A. GPT6 generates pulses; GPT0 provides the hardware stop. **P004 has no GPT/serial-output route.** P603/GTIOC7A is a possible later route, requiring physical-work approval and peripheral-ownership checks.
- DMA duty data belongs in DMA-accessible SRAM. Do not put it in DTCM because CPU access there is fast.
- Keep shared K1 code free of FSP/RT-Thread headers. Stage changes into disposable build copies; preserve behavioural imports and identify intentional derivatives.
- Keep DualMCU, Titan BSP, their working trees, and their histories intact. DualMCU Lane F, Teensy bring-up, PCB work, and this Titan lane are separate programmes.
- Preserve existing checkout ownership. The applicable campaign canon says not to create a sibling worktree for this slice. Do not reset dirty work or invoke `git add -A` indiscriminately. Current remote branch equality does not authorise rewriting either branch.
- Run sequentially. The prior independent-review dispatch was rejected; do not relaunch agents, change delegation controls, or relabel self-review as independent G8 acceptance.
- No synthetic computer tones or white noise in the room. Silent host fixtures remain useful. Use the permitted real-audio runner and an identified recording for acoustic work; do not run a repeated same-song loop by default.

## 3. Current state: preserve the distinction between source and hardware

| Area | What the evidence supports | What remains open |
| --- | --- | --- |
| Remote source | Both branches at `4311408…`; September 19 work is published. | Mac checkout, uncommitted work, current owner and resident device identity were not observed by this review. |
| Scalar F1 parity | Repository records 14,000 hops / 7,364,000 exactly compared typed fields, including 320 pixels per hop, on identified Titan. | This is covered compute parity, not the complete treated/packed physical output chain. |
| Historical G4 scalar | Exact build `603e3f1721b36abc9d5dc6cb5cb9ee4e370c8638cf48023a2b471f0926733ebb`, HEX `619077a52fa372cea97380674be2ef39ae940a76aeb659c141b0e371ea03ad6e`: 240,000 frozen hops; zero correctness/deadline/late-start failures; reported workload max 5.598 ms. | Only that slim-status O3, DTCM, cache-off scalar image is qualified. No inheritance to live PDM, GPT, newer renderer, or NPU image. |
| Earlier unslim scalar / NPU | O2/O3 historical 6,000-hop runs missed 2,005/2,006 deadlines. Generic P4/E1 passed its bounded scheduled campaign. | Actual K1 F2 coexistence remains unqualified. Generic interleaving is not simultaneous contention or a useful deployed model. |
| LED completion guard | Nonzero DMCRA now faults; immutable v2 diagnostic exists; host mutations reject false completion. | Three on-silicon policies still failed at the terminal word. Physical cause and visible blue/white corruption are unresolved. |
| Current residence | Latest STATUS records profiler **`32dd1f5f…`**, HEX **`9f12869a…`**, written September 19. It replaced `a3f37e8a…` before wire capture. | Full hashes, programme/app identity, actual present residence, and Q1–Q5 raw result must be recovered locally. Do not expand abbreviated hashes by guessing. |
| Live boot / dwell / gain | Current source boots mode 32, active+emit, no carousel; nominal PDM sensitivity is Q8=4096 (×16); dwell uses an explicit valid flag and keeps rendering during the dwell interval. | Latest live-audio image was recorded as built but not programmed after two ROM timeouts. Later gain/dwell commits have no new target qualification in the inspected index. |
| Live microphones | 40 kHz nominal configuration, SINCRNG 10 source, measured-rate ASRC into 24 kHz/180 exist. History records roughly 41,406–41,411 Hz and 133.3 AP hops/s. | Current-image rails, capsule identity, delay/response, restart behaviour, and combined qualification. Nominal 40 kHz is not measured 40 kHz. |
| Palettes / centre motion | Current host palette suite passes, including 44 palettes, morphing, geometry/direction checks, autostart and snap-freeze negatives. | Host animation and sparse sampled dumps do not establish physical smoothness, travelling ribbons, optical quality, or absence of corrupted frames. |
| Product output | `frame_blend.*`, `product_runtime_policy.*`, TRUE16 packer and host work exist. | Blend/policy are not in `import-slices.json` or the build's local-file list and are not integrated into the default palette path. Full WS2816/multi-lane/current-limit admission remains. |
| SS-03 | Broker, ownership tools and prior proof assets are published. Stale 1.8 GB broker log was archived and hashed. | Expected-build binding, replay/reconnect acceptance and useful truthful telemetry remain. Stopping the old broker did not repair its source. |
| Decision gates | G6=`NO_QUALIFYING_CANDIDATE`, G7=`NOT_RUN_NO_CANDIDATE`; architecture ruling deferred. | Physical F3/G5, combined timing, K1-L, appropriate comparisons, bridge/power evidence, G8 independence and Captain's K1-C ruling. |

The three colour failures are valuable controls, not three cures:

| Configuration | Build | Fault frame | Terminal observation |
| --- | --- | ---: | --- |
| Fixed, PDM-first | `d9700b14…` | 3468 | 1 DMA word remaining; no false credited completion |
| Round-robin | `49c582db…` | 2063 | Same remainder |
| Fixed, LED-first | `a3f37e8a…` | 2030 | Same remainder; source advanced 3069 of 3070 DMA words |

Each retained failing payload was 384 all-zero GRB bytes: **128 × 24 = 3072 bits**, with **3070 DMA words plus two preloaded words**. This rejects the tested arbitration-policy fixes. It does not prove a particular timer, DMA, shifter, or supply root cause. Do not write a fourth arbitration-map experiment as the next action.

Recent source commits worth understanding, rather than redoing:

| Commit | Change |
| --- | --- |
| `8f976a9` | Capture contract, profiler preparation, broker archive and bounded GPT mutation runners |
| `fa83ac0` | Previously unpublished GPT/PDM/LED2/ASRC sources and host tests |
| `dcdbdac` | Live WaveformK1 autostart and additional host gates |
| `fe4c94b` | Restored nominal ×16 PDM sensitivity |
| `fae1e7b` | Renderer remains active through silence dwell; valid flag avoids treating timestamp zero as never armed |
| `0ed99e3` | Serial Studio broker and proof assets |
| `4311408` | Status-LED build receipts and flicker stills; these historical assets do not prove a newly flashed live image |

## 4. New review findings to act on

**F-01 — Physical capture scoring can produce unsupported success. Priority: before waveform acceptance.**

In `scripts/score_p601_capture.py`, `_probe()` checks field presence and the length of a hash string. It does not open/hash a raw capture. `score_capture()` trusts `source='physical'`, does not validate the supplied pulse widths, periods, reset interval, or decoded payload, and returns `pass: true` even for a short/clipped frame. Its own self-test constructs synthetic data with `source='physical'` and expects a physical stamp.

This review executed the current scorer and reproduced:

| Input | Returned result |
| --- | --- |
| `complete_synthetic(..., source='physical')`, no waveform file | `pass=true`, `WAVEFORM_CAPTURED`, `physical_claims=true` |
| Same object with first/last high widths −10 ns, reset 0 ns, and hash `'z' * 64` | Same success; `first_divergence=null` |
| 3071-bit clipped frame | `pass=true`, with `bit_count_ok=false` and a divergence label |

A real capture of a bad waveform should be retained as captured evidence. That is different from waveform qualification. The repair must separate provenance, capture validity, and signal conformance instead of erasing failed captures or using one ambiguous `pass` flag.

**F-02 — The old retained-frame capture contract is no longer executable as written.**

Latest STATUS and the repository plan explicitly record the September 19 Rearm over the faulted image. The protocol footer and scorer still insist on the historic `a3f37e8a` frame 2030. That past physical event cannot be recovered by reading its saved bytes or replaying them now. The repository records the preserve-resident hold as waived; this is not a wire-capture waiver or a cure.

Prepare a new reproduction with a new run/boot/frame identity and instrument acquisition armed before the fault. Keep the old witness immutable. Reusing the same payload or seeing frame number 2030 after another boot is not proof that the waveform and fault are the same event.

**F-03 — Timing experiment configuration contradicts its brief.**

The timing brief requires empty ITCM/DTCM. `build_scalar.py` applies `apply_tempo_dtcm_overlay()` whenever `--resident-controls` is provided; `--stage-profile` requires those controls. The overlay places two ACF arrays in `.dtcm`. The new overlay-order regression intentionally preserves both profiling and DTCM. This review reproduced that staging behaviour.

Raw timing support is already implemented; do not rewrite it just because `Distribution<8000>` remains for compact non-profile summaries. The real task is to obtain/validate the raw run and identify its configuration. A DTCM run can answer questions about that configuration; it cannot be presented as the original empty-TCM controlled experiment. Also correct the receipt schema label: `run_scalar_schedule.py` decodes raw version 2 but writes a `K1T1.v1` descriptive string.

**F-04 — SS-03 source does not enforce the plan's expected-build requirement.**

`titan_snapshot.identity_ok()` checks protocol, UID, nonempty build/source strings, and a parseable contract. It does not compare against a selected programme/application checkpoint. `titan_broker.attach()` uses that function; later polling does not establish a new build identity. This review confirmed that arbitrary `superseded-unbound-build` and `new-unverified-build` strings both return `(True, 'ok')` for the expected UID.

Recognising a device is useful. Qualification must additionally bind it to the expected candidate. Expose those as distinct states. Do not promote a newly observed build to accepted simply because the UID is familiar. Review field semantics too: `titan_snapshot.py` currently assigns `last_emit_cycles` to a field named `hop_max_us`; that is not AP-hop timing.

**F-05 — Documentation and machine-readable authority disagree.**

- `external-receipts.json` still calls `f0205ef8…` / `led-build-04` current, while STATUS records the newer profiler. It indexes profiler build receipts but not the corresponding September 19 programme/application/Q1–Q5 result.
- `combined-runtime-contract.json` still says the last GPT attempt had zero completions, despite later engine checkpoints and three measured terminal failures. Its DMA map describes the default PDM-first configuration, not every candidate.
- The onboard-microphone contract still describes inherited SINCRNG 5/candidate 10 while current source writes 10 and host checks enforce it.
- The 001↔002 evidence map is a short placeholder, not the requested crosswalk.
- The physical inventory and continuation ledger carry obsolete sub-8-ms latency wording and old “resident” claims.
- `.gitignore` now excludes stage trees, but Git still tracks **835 files / 30,504,473 bytes** under the old PCM1808 stage tree. Ignore rules do not untrack historical files. This is housekeeping, not a reason to rewrite history or delete the only evidence copy.

**F-06 — The real-music runner is an observation tool, not a complete acceptance gate.**

Source inspection of `scripts/run_mode32_real_audio.py` found that it forces version-1 mode-32 settings and brightness 255 without restoring the prior configuration; `authored_stats()` treats a frame shorter than 480 bytes as black; only positive raw rails are recorded; and the receipt is a summary without a complete qualification verdict. Its `hop_max` is sample peak amplitude, not execution time. Before using this runner for WP5 acceptance, add exact-length validation, full identity/configuration and recording hashes, required-counter validation, both rail signs, explicit settings restoration and an honest failure receipt on exceptions. Reuse the stronger ownership/restoration patterns in `run_colour_integrity.py`. Do not call missing samples silence or black.

## 5. Execution sequence

### WP0 — Reconcile the live checkpoint without disturbing it

**Inputs:** audited SHA, current checkout, source authorities above, external receipts. **Owner:** receiving CLI agent.

1. Read current `AGENTS.md` and platform memory. Inspect checkout branch, local/remote SHAs, tracked/untracked changes, and ownership. Preserve unrelated work. Do not assume the old `c2d4dca + unpublished week` description still applies.
2. Inspect current CDC ownership and any broker/programmer session before opening the port. Do not kill an unknown owner or remove a live lock. Historical D-state process reports are not current process evidence.
3. Find the full `g4-uncapped-raw-hops-build-20260919-02` receipt, related programme events, application INFO, frozen resident header/profile, and any completed raw timing run. Verify their hashes and relationships. Search those named assets first; do not broadly reconstruct earlier campaigns.
4. With CDC explicitly available, use the existing identity path to bind UID, full build ID, source pin, contract, cache/M33/U55 state, and boot/run context. USB enumeration and a successful write receipt are insufficient substitutes for app identity.
5. Preserve existing on-device diagnostics before a reset or image change. If the profiler still has useful results, extract them before restoring a live image. Record what is available versus absent.
6. Update a single current-state entry in STATUS and the corresponding current fields of the receipt index. Keep historical campaign records immutable; add a dated correction rather than rewriting their outcomes. Mark unresolved current fields unknown instead of copying the latest-looking hash.

Suggested read-only source checks on the Mac:

```sh
cd /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware
git status --short
git branch --show-current
git rev-parse HEAD
git log -8 --oneline
git diff --stat
bash scripts/check_references.sh
```

`check_references.sh` now checks pin ancestry. That does not make a moved/dirty reference checkout safe for arbitrary import. Behavioural comparisons must still read the pinned object. The ARM builder separately requires the BSP at its exact clean pin. Record ahead/dirty state without resetting either sibling.

**Exit:** source ownership established; exact device state identified or explicitly unavailable; decisive external receipts bound; a truthful current checkpoint replaces contradictory current-state claims. If hardware is unavailable, continue WP1/WP2 and all other host-only preparation.

### WP1 — Repair the capture gate and bind a new acquisition

**Targets:** `scripts/score_p601_capture.py`, `scripts/gpt_fault_witness.py`, `tests/host/test_score_p601_capture.py`, `P601-DIN-CAPTURE-PROTOCOL.md`.

1. Preserve historical prediction support, but accept an explicit verified witness/run pair for a new candidate. Validate full UID/build, boot/run identity, frame ID, profile, actual pixel/byte/bit lengths, payload hash, duty hash, and two-preload accounting. Do not replace historic constants with an unvalidated “accept anything” path.
2. Define a capture manifest naming the raw instrument files, channels/probe points, acquisition/session identity, sample interval/timebase, thresholds, trigger/correlation method, selected LED part/profile, and decoder version. Resolve only declared paths and recompute SHA-256 from the actual files.
3. Decode edge data from both P601 and first-LED DIN. Check complete decoded payload, bit count, first and last pulse, all high/low/period timings, and reset low against the populated part's documented limits. Quantify instrument resolution so an uncertain measurement cannot silently pass a tight margin.
4. Check same-acquisition/same-frame correlation. All-black traffic repeats, so payload equality alone is insufficient. Use an appropriate instrument trigger, segmented frame indexing, or an explicitly reviewed low-overhead marker with verified pin ownership. Do not invent a spare pin or silently change the waveform being diagnosed.
5. Separate outcomes, for example: `capture_present`, `raw_files_verified`, `frame_correlated`, `waveform_conforms`, and `first_divergence`. Synthetic/parser tests must never create physical qualification. A valid capture of bad pulses must remain available as a failing signal result.
6. Add executable negatives for missing/tampered raw file, non-hex hash, synthetic relabelling, wrong build/session/frame, missing probe, invalid/non-finite timing, payload mismatch, wrong profile, 3071/3073 bits, clipped final pulse, and insufficient reset. Include a valid parser fixture with synthetic provenance. Do not call that fixture physical evidence.
7. Remove local-Mac witness dependency from pure parser/scorer unit tests. The current suite's `setUpClass()` requires an external receipt, preventing it from proving general behaviour in a clean checkout. Keep optional historical-receipt integration tests separate and explicit about missing prerequisites.

**Exit:** the reproduced false successes are rejected; capture provenance and signal conformance are distinct; a new acquisition can be scored without impersonating the lost event. No wire PASS is granted yet.

### WP2 — Finish available software hygiene and observation correctness

**Targets:** `tools/serial-studio/titan_snapshot.py`, `titan_broker.py`, their existing tests, affected documentation/build metadata.

- Retain completed fixes: gold-extract path-based import, scheduler's LED2 linkage, SConscript missing-K1 guard, and bounded GPT mutations. Do not spend another cycle implementing them.
- Bind broker qualification to an explicit accepted build/application checkpoint: UID, full build/source, contract and relevant configuration. A read-only observation of another build may be reported truthfully as unaccepted, but cannot inherit qualification.
- Invalidate binding and metric validity on disconnect, age expiry, restart, or identity change. Rebind before resuming current-image claims. Test build change on the same UID, stale-to-fresh recovery, rejected opcode, failed cleanup, and ownership handoff.
- Preserve one CDC owner. Broker and target runners must not compete. Keep read-only monitoring free of hidden configuration, flash, or reset commands.
- Correct telemetry units and meanings. Do not substitute renderer frame count for completed transmission or emit duration for hop execution time. Missing measurements stay unavailable until instrumentation supplies them.
- Ensure record rotation/limits and write failure behaviour prevent recurrence of the runaway log. Preserve the already archived log and its hash; do not recreate the old broker session.
- Add remaining subprocess timeouts only where necessary. `test_palette_runtime.py` bounds mutation executables but still has unbounded ordinary compile/run calls. A harness timeout fails the run; it is not a rejected firmware mutation or a PASS.
- Fix stale CLI help describing `--palette-autostart` as a carousel. Actual current behaviour is live mode 32, flags 5. Keep source, help, test expectations, and boot receipt aligned.
- Repair F-06 before live qualification: keep real-audio observation distinct from acceptance, reject truncated/missing data, and restore the actual starting settings on every path. Add negatives for a short frame, missing counters, capture exception and restoration failure.

Useful observability must answer actual debugging questions. Reuse available firmware fields and add bounded instrumentation only for a demonstrated gap:

| Question | Required trustworthy observations |
| --- | --- |
| Is live audio healthy? | Measured input rate/lock, paired-slot/AP-hop progress, queue ownership/high-water, drops/overflow/recovery, positive and negative raw rails, post-gain clips, ASRC starvation |
| Is the machine meeting cadence? | AP execution and completion/lateness, render cost/skips, queue pressure, instrumentation cost, observer on/off comparison |
| Is light being transmitted? | Submitted/DMA-completed/hardware-stopped/latched counts separately; fault code/frame; remaining words; profile; preparation cost |
| Is the visual state behaving? | Input publication/generation, musical presence, dwell age/path, injection/transport/decay evidence, frame CRC/occupancy with declared sampling |
| Can a run be trusted? | UID/build/contract, boot/connection epoch, metric age/validity, CDC owner, host drop/write errors, live versus replay provenance |

Actual Historian replay and restart recovery close SS-03 only when their current proof assets support them. Retain accepted SS-02 work. Do not reduce the dashboard to four generic values or invent firmware numbers for unavailable panels.

**Exit:** host identity/staleness/ownership negatives pass, field meanings are correct, and remaining live SS-03 steps are prepared. These software repairs do not require a speculative LED flash.

### WP3 — Complete the uncapped timing measurement in the correct configuration

**Targets:** `fixture_app.cpp`, `stage_probe.h`, `k1_double_probe.*`, `stage_profile.py`, `tempo_dtcm_overlay.py`, `build_scalar.py`, `run_scalar_schedule.py`, existing timing brief.

First reuse any already completed raw run from WP0. The current source contains a 6,000-record raw trace, per-stage cycles, double-call counters, and a known-delay mutation. The compact 8,000 µs histograms are not authoritative percentiles for this experiment.

1. Write an explicit configuration record: O3 variant, frozen header and PCM hashes, sliced/unsliced tempo, DTCM/ITCM placement, cache/FPU/vectorisation state, clock, observer mode, full build/HEX identity. Use ELF/map evidence for placement.
2. Resolve F-03 without destroying historical results. Preserve the qualified DTCM image. Add an explicit build choice if needed so the original empty-TCM measurement can actually be built; preserve the current default only as a named, identified variant. Hash and report configuration choices. Never call a DTCM result empty-TCM.
3. Retain the same frozen input bytes for profile/control comparisons. Do not regenerate both sides' goldens to hide a behavioural change. Do not re-run the entire old corpus merely to verify document edits.
4. Collect all 6,000 rows, with index, total cycles/cost, release lateness, completion/deadline result, tempo-updated flag, stage costs, and double-call/cost data. Decode after the timing window. Check sequence/completeness and reconcile totals against the target counters.
5. Answer Q1–Q5: uncapped tempo/ordinary distributions; labelled miss cross-tab; measured stage attribution; dynamic double emulation calls/cost; matched observer overhead. Report zero activity as zero only if the measurement actually covers it.
6. Execute the known 5 ms delay at hop 136 and confirm its index and measured magnitude. Preserve the mutation receipt as a negative, not qualification. Host one-hop fixture uses hop 0; do not conflate those two tests.
7. Distinguish an ordinary hop whose own work is too slow from one that starts late because the previous tempo job blocked it. The raw format already carries lateness. The existing brief's falsification wording is not permission to hide ordinary misses, nor does a propagated late start prove ordinary compute is intrinsically too expensive.
8. Report whether tempo p99 fits 22,500 µs. That answer alone does not pass a 7,500 µs release-to-completion contract: a long synchronous tempo call still blocks the next release. Any later slicing/decoupling must preserve publication age/epoch semantics and independently pass the required timing/correctness gate.

Verified runner interface; substitute actual verified paths, never reuse receipt directories:

```sh
python3 scripts/run_scalar_schedule.py \
  --build "$TITAN_PROFILE_BUILD_DIR" \
  --resident "$TITAN_RESIDENT_FIXTURE_DIR" \
  --profile "$TITAN_FROZEN_PROFILE_FILE" \
  --output "$TITAN_NEW_PROFILE_RUN_DIR" \
  --loops 1 --mode scalar
```

Use a separate fresh directory with `--timing-mutation` for the negative. The runner may correctly return a failed scheduling result while preserving a valid measurement; analyse the raw evidence without relabelling that failure. `--qualification` is a different gate with the exact frozen loop count.

**Exit:** Q1–Q5 are answered or each has a precise evidence blocker; original and current configurations are clearly separated; no new product/combined PASS is inherited. If changing the resident image is necessary, prepare it under WP7. Do not consume an uncaptured new LED fault by switching back to the profiler.

### WP4 — Capture the LED failure, repair its measured cause, repeat controls

**Prerequisites:** WP0 identity and WP1 trustworthy capture path; instrument/connection available; selected image and live operator transition prepared. Keep WP3 results safe before changing images.

1. Select a source-bound diagnostic candidate. Prefer a retained guarded image for reproduction if its complete artefacts and recovery path verify. If current telemetry/correlation needs a change, make a narrow explicitly identified diagnostic build; do not combine a speculative driver cure with new gain/render behaviour.
2. Verify the actual strip part, supply, ground, shifter route and pin ownership. The 74HCT2G34GW is already documented as present; do not spend a new investigation assuming it is absent. Prove a suspected electrical defect by measurement.
3. Arm capture at P601 and first-LED DIN before reproducing the fault. Capture both ends of the same transmission and enough pre-trigger context to correlate the first fault witness. A fault-latched transmitter is stopped; attaching an instrument afterwards cannot retrieve the preceding waveform.
4. Retain v2 opcode-22 bytes, strict decoded witness, settings, PDM counters, raw waveforms, identity, and timeline in a fresh run directory. Empty opcode-22 remains v1/728 bytes; explicit little-endian version 2 requests v2/1420 bytes. Do not silently break existing clients.
5. Identify the first divergence using the following branches. Treat each as a hypothesis to test, not a preselected diagnosis.

| Captured observation | Bounded next implementation/investigation |
| --- | --- |
| All 3072 bits and payload are legal, reset is valid, but terminal DMCRA is 1 | Trace preload consumption, final request, hardware stop and completion accounting against the manual/captured timeline. Explain the discrepancy before changing the guard. |
| Missing/wrong final data or clipped pulse at P601 | Resolve GPT event count, buffering, ELC stop ordering or last DMA request from the measured edge/register sequence. |
| P601 conforms, DIN does not | Measure shifter output, cable, reference ground, levels and supply. A logic-analyser trace alone cannot establish analogue voltage margins or ringing. |
| Both probes lack a valid signal | Establish supply, continuity, peripheral ownership and trigger validity before editing render code. |
| Wire conforms but physical colours still fail | Check actual part/payload order, strip/control comparison and optical response. Do not declare the visible ticket closed from a bit count alone. |

6. Patch the smallest implicated surface. Keep the corrected nonzero-count guard, immutable first-fault retention, and independent DMA/waveform/latch completion semantics. Never restore the former `DMCRA <= 2` waiver or merely manufacture a completion callback.
7. Run the existing production-driver host and mutation tests plus one negative specific to the measured defect. Cross-build, inspect linked configuration/register path, programme the identified candidate, and collect a new same-frame comparison.
8. Repeat the existing **zero-output, warm low-load, warm USB-stress** controls, initially retaining the documented `--phase-s 20` conditions for comparability. Preserve first failure; restore declared settings and release CDC on every exit. Do not reset/retry inside the scorer to erase a fault.
9. Require progress of AP/PDM and both completion events, no new DMA errors, no PDM overflow/drop/recovery/packing error, rearm denial, skew drop, starvation or gain clips. Record both signs of raw rails. A transport pass does not by itself qualify acoustic clipping/headroom.
10. Close the visible-corruption ticket only with applicable optical evidence tied to this candidate plus the wire result. Agent-owned capture/scoring should do the diagnostic work; Captain's product judgement is not a substitute for timing or bit decoding.

Existing command shape:

```sh
python3 scripts/run_colour_integrity.py \
  --build "$TITAN_COLOUR_BUILD_DIR" \
  --output "$TITAN_NEW_COLOUR_RUN_DIR" \
  --phase-s 20 \
  --restore-settings "$TITAN_VERIFIED_RESTORE_SETTINGS_FILE"
```

The restore file must be the actual intended checkpoint, not a guessed default. If unavailable, the runner supports captured pre-test settings; read its current contract before use.

**Exit:** first physical divergence identified; repair and negative proof recorded; guard and waveform agree for the admitted frame/profile; all bounded controls complete; optical claim remains separate. If no instrument is available, report exactly the missing acquisition capability, with candidate, wiring/probe instructions and decoder ready. Continue independent work; do not invent wire evidence or another arbitration cure.

### WP5 — Admit the current live PDM/AP/VP image and its behaviour

**Prerequisite:** repaired transmitter admitted at the bench scope. **Targets:** `pdm_target.c`, `k1_pdm_sensitivity.h`, `k1_asrc_24k.*`, `fixture_app.cpp`, `palette_runtime.cpp`, and current target runners.

1. Build one named integration candidate containing the accepted driver and the current intentional live changes. Bind all sources and configuration. Host success for a constituent module is not integration qualification.
2. Verify reset/autostart is mode 32 for both logical channels, active+emit, no four-second carousel. Record actual brightness/palettes/direction/settings; do not silently substitute historical brightness 255 for current boot defaults.
3. Read back live SINCDEC/SINCRNG, PDM channel/DMAC ownership, nominal/measured rate, ASRC lock and AP cadence. Source currently uses SINCDEC 49/SINCRNG 10 and nominal ×16 with loud guard. Those facts need matching target evidence; changing gain again is not the default next task.
4. Preserve the reserve-before-rearm/FILLING ownership repair and programme/measurement separation. Test bounded overflow/restart/epoch recovery with explicit failure status. Do not silently feed a 16 kHz diagnostic stream to a 24 kHz AP or fall back to fixture audio.
5. Acoustically identify capsules when a controlled observation is available. If unresolved, report `ACOUSTIC_IDENTITY_UNRESOLVED` while keeping valid transport facts; do not invent labels from correlated room recordings.
6. Qualify both raw rail signs and post-gain clipping under identified quiet/normal/transient input. Retain PCM, rate/filter/gain configuration, and observed conditions. Current `k1_asrc_24k.c` uses two-sample linear interpolation, and `pdm_target.c` locks an initial measured rate after about two seconds. That code alone does not prove sufficient anti-alias rejection, fidelity, or tracking of later clock drift. Characterise the whole PDM/filter/resampler response, delay and applicable drift range; add a bounded filtering or rate-tracking repair only if the measurement requires it. Historical frontend probes also reported no bins below 110 Hz, low-bin tone/label disagreement, and a 160→80 half-time tempo result. Reproduce those silently against the exact current contract before deciding whether they are mapping defects or declared algorithm behaviour; do not cure them by another gain change or a new feature chain.
7. Use retained real music or the permitted `run_mode32_real_audio.py --audio ... --build-id ...` path after its F-06 repair. The older `run_music_baseline.py` deliberately refuses synthetic room playback; do not bypass that guard. Bound playback and respect current session permission for acoustic operation.
8. Separate presence detection, colour injection, propagation and decay. Verify the `last_live_valid_` fix at zero time and wrap, threshold crossings, and uninterrupted rendering during the five-second dwell. Measure elapsed-time decay and the declared five-second quiet/black tail without repeated history reinitialisation or minimum-one residuals.
9. Score travelling structure, occupancy and quiet gaps separately from brightness and centroid. A filling half-strip can move its centroid without creating the requested travelling ribbon. Use native frames for algorithm checks and suitable full-cadence/optical evidence for physical smoothness. An 8 Hz dump cannot exclude a one-frame glitch at 120 Hz.
10. Measure AP execution and release-to-completion, render/preparation, DMA service, queue/backlog, and memory on the combined image. Use the existing 45-second combined runner as a preflight, then satisfy the named coverage/qualification contract. Do not extend soak duration without a stated remaining risk, and do not treat this preflight as historical G4's 240,000-hop qualification.
11. Repeat observer-off/on checks. Dense 20 Hz USB previously froze emission; dashboard/diagnostic traffic must have bounded cost and must not become a necessary production data path.

**Exit:** a named current live image passes its explicit combined preflight/acceptance, has truthful remaining limitations, and retains centre-origin/dwell/palette behaviour. No product four-lane or physical latency stamp is inherited.

### WP6 — Complete the product-output path after bench stability

**Targets:** `frame_blend.*`, `product_runtime_policy.*`, `product_output_treatment.*`, `ws2816_pack.h`, build/import admission, output backend.

This is onward work. Prepare it while hardware is unavailable, but do not conceal a blocked physical prerequisite.

1. Admit the existing local modules as a named reviewed slice. The reference pin is frozen; do not make a local derivative appear byte-identical by silently changing the manifest. Add the appropriate build/source bindings and linked-use evidence.
2. Reproduce the verified output order: **effects → blend → treatment → edge policy → gain → joint current limit → stage/show**. Avoid applying brightness or treatment twice. Compare both logical channels through the full output chain; F1's renderer pixels alone are insufficient.
3. Preserve TRUE16 payload capability independently of the current Pixel8 renderer. `packBenchGrb48Lane()` presently expands Pixel8 by ×257 and explicitly does not claim extra colour precision. That bench adapter cannot prove arbitrary low-byte preservation. Test an actual 16-bit pattern such as `0x12AB` through the wide submit/packer path without double packing, scaling or residual 8-bit dither.
4. Validate the populated WS2816C-1313-4P timing and GRB48 profile on the proven single output before adding lanes. Run the known-good-strip/control comparison from `FASTLED-PORT-DESIGN.md`; do not assume WS2812 qualification automatically qualifies WS2816.
5. Resolve physical output topology before multi-lane implementation. At 800 kbit/s, 160×48 bits is **9.6 ms payload**, exceeding an 8.333 ms/120 Hz frame even before reset. With the retained design, each 160-pixel edge needs parallel 80/80 halves; two edges require four concurrent lanes. An 80-pixel lane needs 4.8 ms payload, plus the applicable reset interval.
6. P004 cannot run the selected GPT backend. Prepare exact approved route options and pin/SCI/GPT/ELC/DMAC ownership; only Captain can authorise the physical DIN move. The second 160-pixel edge is also a physical dependency. Do not silently reduce pixel count, colour depth, or render rate to obtain a pass.
7. Budget DMA-accessible memory and concurrency from the chosen implementation. For the existing 32-bit duty-word design, one 80×48-bit lane needs 15,360 bytes; four need 61,440 bytes per bank, or 122,880 bytes for two banks, before other buffers. Verify map placement, available channels and worst-case service on the real build.
8. Prove both halves/edges launch and latch as required, preserve centre topology, enforce joint current limit, and expose submission/transfer/latch timestamps and underrun/overrun state. GPIO bit-banging that disables interrupts for about 5.4 ms remains diagnostic; it cannot replace a qualified concurrent live-audio backend.

**Exit:** full logical output composition is source/host admitted; physical WS2816/multi-lane status is explicitly PASS, FAIL, BLOCKED or NOT TESTED at its actual scope.

### WP7 — Apply the established live-programming protocol

This is the operator procedure for any dependent image transition, not another development phase or a blanket no-flash policy.

Before announcing ready: finish the candidate, host gates, cross-build, full source/ELF/HEX hashes, verified recovery image, programmer dry run, fresh programme directory, exact waiter command, and bound target-runner command. Check ownership before the operator transition. Do not discover these after Captain says Rearm.

```sh
python3 scripts/programme_scalar.py \
  --build "$TITAN_FINAL_BUILD_DIR" \
  --output "$TITAN_NEW_PROGRAMME_DIR" \
  --wait-seconds 180 --execute
```

On **Rearm**, start that prepared waiter first, then reply exactly **`WAITING`**. Consume `events.jsonl` or live events continuously. Promptly report ROM seen, identified/writing, WRITE_VERIFIED, and the separate application identity. Give “Release USER and BOOT, then RESET” once only if the operator has not already done it. If the app is responsive, proceed to the bound runner without repeating a completed action.

Never reuse programme/run directories. A timed-out waiter that wrote nothing is not a failed firmware execution. Record it and identify the next discriminating operator/tool observation; do not loop blind retries. Do not use an alternative programmer or reset path merely to bypass the established procedure.

## 6. Latency, comparison, and final architecture decision

The current latency authority is `docs/decisions/2026-09-10-audio-to-light-ceiling.md`:

| Measure | Binding requirement / limit |
| --- | --- |
| Traced audio-to-light | Newest-sample DMA-return estimate → transfer-complete of first frame carrying that AP generation; **p50 ≤12,000 µs** |
| Sample count and conditions | At least **1,000 joined Titan frames**, real music through microphone, observer off, full configuration recorded |
| Reported components | Analysis, publish-to-render wait, render, submission-to-transfer-complete |
| Additional delay | Frontend/decimation/ASRC delay and LED latch reported separately; excluded terms must not hide a regression |
| p99 ≤15,444 µs | Proposed companion bound; **not an accepted hard gate** without Captain's stamp |
| AP service p99 ≤8,000 µs | Existing named service bound; keep distinct from the 7,500 µs frozen resident release/deadline contract |
| Acoustic-to-photon | Separate physical/common-clock measurement; not inferred from software timestamps |

Implement/join bounded generation-tagged timestamps at capture/AP publication/VP use/submit/transfer completion. Missing joins, dropped frames and epoch changes cannot be silently discarded. Never use the time of a USB read as the event time. Preserve measurement uncertainty and negative correlation controls.

The legacy S3 comparison cited 43 joined frames and p50 11,576 µs; its receipt is still unbound in the inspected authority. Bind the actual baseline and method before using it as a decisive comparison. Do not relabel that S3 result an RT1062 result. A RA8P1-versus-RT1062 decision needs comparable RT1062 workload/configuration evidence from its own lane, without modifying or flashing that lane from this task.

Update `docs/RA8P1-vs-RT1062-RULING.md` after the required measurements exist. Answer all fourteen questions in `docs/EXECUTION-BRIEF.md` §20, including memory, capture/timestamps, render scheduling, tooling, BSP friction, power/thermal limits and the justified roles of MVE/M33/U55. Where evidence remains unavailable, say so; do not convert a provisional assessment into K1-C acceptance.

Allowed eventual recommendations remain:

- `PROMOTE_RA8P1_FOR_DUALMCU_EVAL`
- `KEEP_RT1062_PRIMARY_DUALMCU_EVAL`
- `CONTINUE_PARALLEL_EVALUATION`
- `REJECT_RA8P1_FOR_K1`

Captain owns the decision. Present evidence and a recommendation; do not self-stamp production migration. This review recommends retaining RT1062 as primary evaluation hardware while Titan closes the measured integration gaps. It does not reject Titan's compute capability or change the separate shipping-S3 architecture.

## 7. Work that stays parked, with explicit resumption conditions

| Work | Resume when | Next required action |
| --- | --- | --- |
| M33 partition | A measured M85 service/resource deficit justifies a split | Quantify ownership/IPC cost and define the bounded split before coding it |
| U55 useful feature / E2 / G7 | A qualifying model and new material utility evidence exist | Use the retained candidate admission process; smoke load is not a product model |
| Actual-K1 NPU coexistence / F2 | A named justified NPU workload and accepted scalar/combined baseline are available | Bind model inputs/outputs and resources; run its exact coexistence/failure contract |
| Helium/MVE or FPU/cache changes | Honest baseline and measured hot stage identify the benefit | One controlled derivative at a time, same behavioural comparison and recovery path |
| PCM1808 | Practical Hirose DF12 breakout/bridge route physically exists | Reconfirm interface/pins, then targeted capture admission; no impossible header plan |
| Titan–S3 radio bridge / K1-B | Named radio-only S3, firmware identity and physical transport | Bounded state/commands/health, clock/epoch mapping, stale/reset/disconnect recovery; no AP or pixel-stream workaround |
| P603/DIN-B move and second edge | Captain authorises exact physical work and connections are verified | Single-lane profile proof, ownership audit, then concurrent-lane acceptance |
| Power/thermal comparison | Identified instrument and declared operating/load conditions | Measure actual board consumption and temperature; do not infer it from MCU specifications |
| Musical feature promotion | Labelled held-out recordings and a declared usefulness question | Compare utility at matched output/conditions; preserve the current frontend until evidence warrants change |
| Full optical latency | Appropriate common-clock acquisition setup | Separately measure acoustic/optical path under the current accepted definition |
| Independent G8 review | Captain explicitly reopens the previously rejected dispatch path | Independent review of the finished evidence packet; do not substitute this self-review |

## 8. Acceptance matrix and STOP/GO rules

| Criterion | Method / conditions | PASS condition | Evidence |
| --- | --- | --- | --- |
| Source/device identity | Exact source, build receipt, verified write and separate app INFO | Agreed UID/build/source/contract/configuration; ownership clear | Current checkpoint, full hashes, raw INFO/events |
| Capture scorer | Executable adverse inputs and raw-file decoder tests | Unsupported provenance/timing/payload/session rejected; captured failure distinguished from waveform PASS | Host test logs and fixtures |
| Fault correlation | Instrument armed before reproduction; both probes from same event | Raw files verified; waveform tied to this boot/run/frame/witness | Instrument files, manifest, v2 witness, correlation explanation |
| Bench LED repair | Same-frame waveform plus three original bounded controls | Correct payload/timing/reset; DMCRA zero at accepted completion; both completion events accounted; no new transport faults | New build/programme/run receipts; decoded waveform |
| PDM guardrails | Bound candidate during controls and live integration | Required progress; no overflow/drop/recovery/packing/rearm/starvation/gain-clip regressions; both raw rail signs evaluated | Raw counters, PCM and configuration |
| Uncapped timing | Frozen 6,000-hop data and matched configuration | Complete recomputable Q1–Q5; delay negative located; placement/observer effects explicit | Raw trace, summaries, ELF/map, control/mutation receipts |
| Historical scalar preservation | Verify original receipt identity | Historical PASS remains scoped; no replacement or regenerated golden | Existing hashed qualification |
| Current live behaviour | Real-input and silent replay controls; current candidate | Correct live boot, centre motion, dwell/decay, no freeze, declared quiet/black tail | Native frames/PCM/counters and optical scope as applicable |
| Combined runtime | Existing bounded preflight plus required named coverage | Required cadence, backlog and memory bounds satisfied on the combined image; no inherited PASS | New source-bound combined receipt |
| SS-03 | Wrong-build/stale/reconnect/ownership negatives plus actual replay | Accurate identity/provenance/units; proper invalidation and handoff; replay proved | Current manifest, host tests, live/replay receipts |
| Product output | Full-chain host comparison, TRUE16 vectors, then physical lanes | Treatment/gain/limit/order correct; required WS2816 lanes and timing physically qualified | Host goldens, linked config, capture and physical setup |
| K1-L | ≥1,000 generation-joined real-music frames, observer off | p50 ≤12,000 µs under the accepted definition; all additional delays reported | Raw joins, clock basis, configuration, distributions |
| Final comparison | Comparable named platform evidence | All questions answered; limitations explicit; Captain decision recorded | Ruling and receipt crosswalk |

Use **PASS / FAIL / BLOCKED / NOT TESTED** per row. A diagnostic capture of a failing signal is useful evidence and not a waveform PASS. A skipped/missing dependency must not return a green aggregate result.

Stop only the affected action when:

- Target identity or CDC ownership is unknown/mismatched: no device configuration/programming; resume after ownership and identity are established.
- A new first-fault event exists without its required evidence: preserve it; do not reset or overwrite until capture/retention is complete or Captain explicitly changes that constraint.
- Capture provenance/correlation or applicable timing limits cannot be established: no physical qualification; fix the capture/contract.
- A required build, negative, resource, or timing gate fails: no dependent qualification; retain failure, implement the bounded measured repair, and rerun that gate.
- Pin ownership, wiring or an instrument is unavailable: no invented route or physical claim; leave a concrete ready-to-run task and continue independent work.
- A reference conflicts with newer explicit authority: reconcile the conflicting claim; do not silently change pins, fixture bytes, thresholds, or the source pin.

Evidence-based GO is automatic when its prerequisites pass within the assigned scope. New wiring, the established operator transition, architecture ruling and the explicit independent-review restriction retain their actual owner boundaries. Do not add fresh approvals for routine host fixes or repeat permissions already granted in the executing session.

## 9. Documentation, delivery, and next-owner handoff

Complete the existing 001↔002 crosswalk rather than creating empty 001 documents. Map the thirteen original deliverables to current source-bound receipts and their limitations: environment, boot, shared time, scalar parity/performance, audio-time boundary, capture, live AP, musical render, VP parity, comparison, mutations, and final validation.

Use STATUS as the concise current index; the detailed campaign documents remain evidence. Update `external-receipts.json` with hashes of the actual new files and consistent current/last-observed state. Preserve failed and superseded receipts. Do not create duplicate ledgers merely to make the handoff appear complete.

Keep commits narrow and follow the executing session's standing source-publication authority. Separate harness/source repairs from evidence updates when useful. Inspect staged paths; exclude large logs and new vendor stage trees. Existing tracked stage cleanup is optional housekeeping only after a durable archival copy and references are verified; no force-push/history rewrite is part of this task.

Final handback must state:

1. Exact commit/diff and files changed, candidate configuration, full build/HEX IDs, physical UID and last-observed resident state.
2. PASS/FAIL/BLOCKED/NOT TESTED for each applicable acceptance row, with exact receipt paths and hashes.
3. Which finding was repaired, the mechanism, the negative proof, and what remained unmeasured.
4. CDC owner/release state, fault state, settings left on the device, and any pending prepared waiter/run command.
5. For each remaining task: owner role, exact next action, input artefacts, prerequisite, and prohibited dependent claim. If no agent/person is assigned, write **unassigned**.

The next agent should begin with WP0, then finish the immediately executable WP1/WP2 work while recovering the timing results. Once acquisition is available, execute WP4 through repair and validation. Do not end after restating this sequence.

## 10. Review verification record and reproduction

Focused checks executed in a Linux host environment against source retrieved at the audited commit:

| Check | Observed result |
| --- | --- |
| `python3 scripts/test_ws281x_gpt_dma_hw.py` | PASS: production-driver completion guard, v1/v2 layouts, immutable fault frame |
| `python3 scripts/test_ws281x_gpt_dma_mutations.py` | PASS: all five driver mutations rejected by runtime assertions |
| `python3 scripts/test_schedule_protocol.py` | PASS: scalar, NPU and profile host configurations |
| `python3 -u scripts/test_palette_runtime.py` | PASS: current runtime/protocol, autostart, snap-freeze negative, morph transitions and centre direction/geometry checks |
| `test_gpt_fault_witness.py` via unittest | 2 tests PASS |
| `test_sconscript_k1_guard.py` via unittest | 2 tests PASS |
| `test_stage_profile.py` via unittest | 4 tests PASS; this also confirms the implemented profile-then-DTCM combination |
| `python3 scripts/test_pdm_sensitivity.py` | PASS: nominal Q8=4096 and loud-guard host behaviour |
| `python3 scripts/test_pdm_sincrng.py` | PASS: SINCDEC 49/SINCRNG 10; leftover-5 mutation rejected |
| Capture-scorer challenge | Unsupported physical success reproduced as F-01 |
| Broker identity challenge | Both unbound build strings accepted, as F-04 |

The initial 197-file audit snapshot was verified against GitHub's Git blob hashes; further targeted dependencies were retrieved at the same commit. The host environment lacked pytest, so targeted unittest cases and standalone C/C++ wrappers were used. This was not a full repository suite or ARM/hardware qualification. A partial-snapshot missing dependency was retrieved before the affected timing test passed; it was not reported as a repository defect.

Minimal F-01 reproduction on the unmodified audited checkout; **this creates synthetic objects and no physical evidence**:

```python
import sys
sys.path.insert(0, 'scripts')
import score_p601_capture as s

predicted = {
    'build_id': s.RETAINED_BUILD,
    'uid': s.RETAINED_UID,
    'packed_grb_sha256':
        'a1a4f5721c1c4610af7f71078f3a68c330536d679803b0e0507ee8dc10c5dfca',
    'frame_id': 2030,
    'first_high_ns': 250,
    'last_high_ns': 250,
    'period_ns': 1250,
}
capture = s.complete_synthetic(predicted, source='physical')
for probe in ('p601', 'din'):
    capture[probe].update(first_bit_high_ns=-10,
                          last_bit_high_ns=-10,
                          reset_low_ns=0,
                          capture_sha256='z' * 64)
print(s.score_capture(capture, predicted))
# At 4311408: pass=True, waveform='WAVEFORM_CAPTURED',
# physical_claims=True, first_divergence=None.
# The repaired gate must reject this as physical qualification.
```

Primary source anchors, all pinned to the reviewed commit:

- [Current programme status](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/docs/evidence/K1-RA8P1-002/STATUS.md)
- [Colour controls and fault evidence](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/docs/evidence/K1-RA8P1-002/TITAN-COLOUR-INTEGRATION.md)
- [Capture scorer](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/scripts/score_p601_capture.py#L133)
- [Capture protocol and superseded resident](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/docs/evidence/K1-RA8P1-002/P601-DIN-CAPTURE-PROTOCOL.md)
- [Build configuration and overlay order](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/scripts/build_scalar.py#L514)
- [Uncapped measurement contract](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/docs/BRIEF-2026-09-10-uncapped-ap-timing.md)
- [Raw trace and scheduling runner](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/scripts/run_scalar_schedule.py)
- [Broker identity and metric semantics](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/tools/serial-studio/titan_snapshot.py)
- [Current palette runtime](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/platform/ra8p1/palette_runtime.cpp)
- [Real-audio observation runner requiring stricter acceptance](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/scripts/run_mode32_real_audio.py)
- [Current measured-rate interpolation](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/platform/ra8p1/k1_asrc_24k.c)
- [Production GPT/DMA driver](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/platform/ra8p1/ws281x_gpt_dma_hw.c)
- [Accepted audio-to-light definition](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/docs/decisions/2026-09-10-audio-to-light-ceiling.md)
- [Product output composition gaps](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/docs/evidence/K1-RA8P1-002/OUTPUT-PATH-COMPOSITION.md)
- [Transmitter design and pin/profile authorities](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/docs/evidence/K1-RA8P1-002/FASTLED-PORT-DESIGN.md)
- [External receipt index requiring reconciliation](https://github.com/synqing/SpectraSynq-K1-RA8P1-Firmware/blob/431140853dd8b58af53240ef84d5fb1a08bd8b45/docs/evidence/K1-RA8P1-002/external-receipts.json)
