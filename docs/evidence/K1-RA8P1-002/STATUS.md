# K1-RA8P1-002 current execution

Captain authorised sequential end-to-end implementation after delegation was
rejected. No new agent launches or guard changes were attempted. The two existing
repositories remain on `lane/k1-ra8p1-002`; no worktree was created.

## ROM-entry chat (Captain 2026-09-12, HARD)

During USER/BOOT + RESET the chat is the instrument. Do not go mute.
Do not put the only ack in a log. Speak each beat as it happens:

1. Waiting — "Hold USER and BOOT, then RESET. I am watching."
2. Bootloader seen — "I see it. Writing." (same moment, not after the write)
3. Write verified — "Write verified. Release USER and BOOT. Press RESET."
4. App back — "Board is back." plus what runs next.

A 180 s waiter with one line at the start and silence until timeout is a fail.

## What is true now

- **Titan lane, 2026-09-19.** DualMCU Lane F is a separate programme and is not
  executed from this tree. Colour-integrity image `a3f37e8a…` is **no longer
  resident**. Captain Rearm wrote uncapped profiler `32dd1f5f…` HEX `9f12869a…`
  (WRITE_VERIFIED, UID `545433931bd25436593630352d068363`) over it before
  P601/DIN capture. `WAVEFORM_CAPTURED` is still not stamped: no Mac analyser
  is attached. Predicted 3072-bit / 3070-DMA scorer
  (`scripts/score_p601_capture.py`) remains fail-closed. Ruling vs RT1062 stays
  deferred. Autostart source now boots live WaveformK1 (mode 32, emit on, no
  4 s carousel). Host `python3 -u scripts/test_palette_runtime.py` PASS
  (`PALETTE_COMPATIBILITY_PASS`, morph live-audio allowed to differ). Live-audio
  image is built, not programmed (two ROM waiters timed out). Protocol:
  [P601-DIN-CAPTURE-PROTOCOL.md](P601-DIN-CAPTURE-PROTOCOL.md).

- **Live colour control, 2026-09-16.** Fixed build `d9700b14…` failed at frame
  3468 with one remaining DMA word. Matched round-robin build `49c582db…` was
  subsequently programmed and independently identified; it failed at frame 2063
  with the same one-word terminal underrun. Neither guard fabricated completion.
  Both runs restored settings and released CDC without reset/retry. Round-robin
  is rejected as the cure. Fixed-priority LED-first build `a3f37e8a…` (LED DMAC0,
  PDM DMAC1/2) was subsequently programmed on the same UID and failed at frame
  2030 with the same one-word remainder. Its JSON channel labels were stale
  literals; linked DMAC objects prove the remap, and a source-only telemetry
  correction now cross-compiles. The runner restored settings and released CDC.
  No P601/DIN or optical cure claim. Details and raw receipts:
  [TITAN-COLOUR-INTEGRATION.md](TITAN-COLOUR-INTEGRATION.md).

- **Priority override, 2026-09-16: colour integrity.** Onwards development is
  paused for Captain's named packet. The former driver silently accepted one
  or two unfinished DMA words, so earlier zero-send-error windows do not prove
  clean transmission. The resident driver has the corrected completion guard,
  which caught the latest fault. The immutable explicit v2
  witness (v1 unchanged), safe DMA-policy staging and matched fixed/round-robin
  ARM builds are implemented and host/build verified; fixed and round-robin were
  programmed and failed as above. LED-first was also programmed and failed.
  See [TITAN-COLOUR-INTEGRATION.md](TITAN-COLOUR-INTEGRATION.md) for executed
  commands and the numbered remaining silicon/wire/visible-corruption path.
  No electrical or optical cure is claimed.

- **Pre-colour baseline, 2026-09-16.** Build `1594115d…` is no longer
  resident. Its isolated-hit TRUE_BLACK and Ride It half-volume run-06 remain
  historical: hop max 13585, lights respond then black, zero reported send
  errors, emit never froze, zero new rails. Those zero-error counts predate the
  corrected completion guard and are not transmission proof. P601/optical/capsule
  remain OPEN.
- **Historical resident, 2026-09-15.** prog-20260915-04 `55320f89…` HEX `0aeadba4…` PCM ×16, inherited SINCRNG 5. Isolated-hit and music baselines on that image remain historical.

  | Result | Status | Evidence |
  | --- | --- | --- |
  | Isolated-hit wake | ON-SILICON PASS with sampled dumps | `pdm-ap-gpt-run-20260915-05/isolated-hit.json`: quiet hop med 189 then hit hop 5901 / LED 34 (margin hop≥1877, LED≥28). Gap hold 5.075 s then black. Emit +1565. Hop rate 133.35/s. 8 Hz dumps, not full-cadence smoothness. |
  | Isolated-hit (prior) | INCONCLUSIVE | `…-03/isolated-hit.json` speaker missed room. `…-02/snap-wake.json` click < room. `…-04` dense USB froze emit (observer). |
  | Ordinary music | ON-SILICON BASELINE PASS (sampled) | `pdm-ap-gpt-run-20260915-07/music-baseline.json`. Quiet black. Sparse hop 1073 / LED 102 / 60 CRCs. Pad hop 1917–2457 always lit. Perc hop 2142 / LED 66. After drums: LSB then black at +5.45 s. Emit +4943, hop 133.32/s. 8 Hz dumps, speaker-limited, not full-cadence or optical KEEP. Run-06 kept as the short-silence miss. |
  | PDM / gain | PARTIAL | Source: SINCDEC 49 written. Resident 55320f89 does not write SINCRNG (inherited BSP 5). Working tree now assigns sincrng=10, UNFLASHED. U13/U14 acoustic identity unproved. Gain ×16 live. Loud-guard 1710 on hop 29201 (`…-03`). |
  | Sample clock | ON-SILICON PASS for 180-hop rate | `measured_hz` 41406–41411, `rate_locked` 1, hop rate 133.12–133.42/s vs 133.333. `last_hop_dt_us` ~7148 is PDM slot period, not hop period. |
  | Sparse peak/VU colour fallback | HOST PASS, unflashed | Overlay deposits when chroma_strength < 0.08. `test_mode32_wake` SPARSE_FALLBACK energy=260 max=15 (was 0). Not on resident. |
  | Harmonic frontend | HOST MEASURED | 80 bins, 110–10548 Hz. No coverage below 110 Hz. Low-bin tone peaks disagree with labels (bin 0→12). High bins match. `…-08/host-experiments.json`. |
  | Arm A current scalar tempo | HOST RETAIN | 80→80, 120→119, 160→80 half-time. Onset flags 0 on hop-impulse fixture. |
  | Arm B novelty domain | HOST REJECT extra chain | Novelty is post-AGC in source. Gain probe did not invent onsets. |
  | Arms C–E | REJECT this cycle | C poisoned by low-bin GDFT. D already has half-time. E has no labelled onsets. |
  | Combined runtime | PARTIAL from dumps | Hop 133.3/s, asrc_starved 0, emit advances at ≤8 Hz USB. Dense 20 Hz USB froze emit (observer). No new soak. |
  | P601 / optical | OPEN | No scope this cycle. |

  Next unfinished action: unflashed SINCRNG-10 apply + sparse-fallback overlay are in source only. P601 waveform and optical KEEP remain open. Resident 55320f89 unchanged. No flash this cycle.
- **Historical, 2026-09-15.** Pre-close snap-wake `…-02`: 38/40 lit, click did not beat room. 3526df5c 12/90 black-run 61 is historical.
- **40 kHz SINCRNG 10 image ready, 2026-09-14.** Only the loudest sounds reach the lights because SINCDEC 49 kept BSP SINCRNG 5. Image `b700a591…` HEX `9788ecd0…` sets SINCRNG 10 (between Table 50.7 M=62/9 and M=42/11). Host leftover-5 compile guard PASS. Freeze-fix included. Resident remains `dc07d97d…`. Not flashed. Off-table: score sat_neg after Rearm.
- **Freeze-fix programmed, 2026-09-14.** Rearm prog-07 WRITE_VERIFIED HEX `55004b35…` build `dc07d97d…` UID match. App identity `dc07d97d…`. Live-configured WaveformK1 (mode 32). GPT ran ~18210 frames then latched `K1_WS281X_FAULT_DMA` (first_fault 6, errors 1, owned 0). Software frames still advance; emit does not. LED dump stayed black for 18 s — no snap in that window. Decay not scored. Needs app RESET (not ROM) then a scored snap dump.
- **Snap freeze, 2026-09-14.** A loud finger snap did reach WaveformK1 (nonzero LED dump, max 255) then stuck: CRC `3276929227` frozen while GPT still emitted. DualMCU `production_runtime.cpp` clears the frame before `renderProductChannel`; Titan `palette_runtime.cpp` did not. `transportOutward` adds onto the destination, so an uncleared frame saturates and stays. Fix is the DualMCU caller clear, not a DualMCU pin edit. Host: `SNAP_FREEZE_MUTATION_PASS`. Image `dc07d97d…` HEX `55004b35…` now resident.
- **Sound-to-light first break: bounce mode, 2026-09-14.** Combined soak image still resident (`d245b3c4…`) until the freeze-fix flash. Autostart was mode 64 PALETTE_BOUNCE (ignores audio). Live-configured to mode 32 WaveformK1. Quiet RMS ~5 never crossed the 0.02 peak floor; the snap did. Receipt `pdm-ap-gpt-run-20260914-01/sound-to-light.json`.
- **ASRC rate, historical 2026-09-14 / superseded 2026-09-15.** Older soak: 41 405.5 Hz PDM with ASRC stuck at 40000/24000 → ~3.51% fast AP. Resident `55320f89…` now reports `measured_hz` 41406–41411, `rate_locked` 1, hop rate 133.35/s. `last_hop_dt_us` ~7148 remains the PDM slot clock, not the hop clock.
- **G4 unchanged checks do not apply to this combined firmware.** Historical PASS remains `g4-acf-dtcm-slimstatus-o3-qual-01` on HEX `619077a5…` (frozen 240 000 hops, no live PDM, no GPT DMA). Combined image owns AP from PDM and render from GPT; injecting that profiler would not be the same test. Do not overwrite the historical receipt.
- **Combined soak PASS on silicon, 2026-09-14.** Build `d245b3c4ff0fbbdd2a59373004287de8b4727b196d068a870e91ba11b50f9c61` HEX `744b85659eaf5699ab34151f7b5fb258feb2de8f14c14e7ca22182fd27ac2bf5`. 45 s: +6217 AP hops, overflow 0, GPT 12677→13827, GTIOR bit 4 set. Waveform not captured.

- **GPT engine checkpoint PASS, 2026-09-13 Rearm.** Identified Titan still runs build `a80c5d690cc473e02ba5772e357ff5269db597cbe63d4456384109cdfb19860e`, HEX `8cf8414c698112badcc14c916100048cdd1b7c45aa4dfe8c06651d6321d72c2d` until the combined image is flashed. Over 5 s: +607 DMA IRQs, +607 hardware stops, +607 latched frames. Receipts: `gpt-recovery-prog-20260913-01`, `gpt-recovery-run-20260913-01`.

- **Continuation 2026-09-13 (R0/R1 + host S/D/E).** Dirty tree on
  `lane/k1-ra8p1-002` at `c2d4dcad…` plus unpublished LED2/PDM/GPT work is
  preserved. Combined resource contract:
  `combined-runtime-contract.json`. Scalar G4 historical PASS is retained.
  Combined physical/NPU qualification remains open. Resident image is
  unknown until a new identity query. Do not flash to repeat G4.
- **G4 scalar 40-loop qualification PASS on the identified Titan.** Slim-status
  DTCM cache-off image `603e3f1721b36abc9d5dc6cb5cb9ee4e370c8638cf48023a2b471f0926733ebb`
  HEX `619077a52fa372cea97380674be2ef39ae940a76aeb659c141b0e371ea03ad6e`.
  240,000 hops: CRC 0, deadline 0, late starts 0, lateness max 2 µs, workload
  max 5.598 ms. Four late starts were host opcode-8 histogram JSON
  (`USB_STATUS_READ_COMPLETE`). DualMCU on disk unchanged. D-cache off. Frozen
  CRCs not regenerated. Not product ship (LED/ingress/NPU/MVE remain).
  Receipt: `g4-acf-dtcm-slimstatus-o3-qual-01`.
- **GPT6/DMA LED backend engine checkpoint is on silicon for the recovery
  image above.** The older linked WS2812 image `ws2812-gpt-dma-build-01`
  build `69714064…` HEX `42bd1d6d…` remains a retired failure (timeout /
  zero frames). P004 still has no GPT route. This GPT image does not inherit
  G4. `WAVEFORM_NOT_CAPTURED`. Photons not claimed.
- **F1 passes for the covered scalar slice.** The identified Titan executed all
  14,000 frozen hops and matched 7,364,000 typed fields exactly, including all
  320 pixels per hop. Epoch/time semantics and the million-beat probe passed.
- **G4/F2 is implemented and failed.** O2 and O3 scalar schedules missed 2,005
  and 2,006 of 6,000 frozen 7.5 ms deadlines. The identified NPU-alone,
  scheduled, saturation and semantic-failure cells all executed; no concurrent
  mode qualified for soak.
- **Generic P4/E1 passes its bounded build-09 scheduled-coexistence campaign.**
  The exact image was programmed with full 709,344-byte readback. DSP-alone,
  NPU-alone, scheduled concurrent and saturation preflights passed with polling
  on and off; heap remained 40,640 bytes and the final-bin mutation failed
  exactly once at bin 1,024. The historical build-07 30-minute runs remain valid
  observations, but no unexplained duration minimum is carried forward. This
  campaign interleaves DSP and synchronous NPU calls; it does not prove
  simultaneous M85/U55 shared-memory contention or actual-K1 F2.
- **G6 is `NO_QUALIFYING_CANDIDATE`; G7 is `NOT_RUN_NO_CANDIDATE`.** E2,
  semantic F3 and K1-C remain unpassed. The U55 smoke graph is load only.
- **Physical capture, LED output and Titan-S3 transport remain open.** USB
  enumeration alone is not physical-path proof.
- **LED dual-DIN wire smoke is recorded on this UID, not product PASS.**
  Wait-armed programme `led-prog-03` `PROGRAMME_VERIFY_PASS` on UID
  `545433931bd25436593630352d068363`, image
  `ff89a3820bc266caf8e3f818554ebe3d0913f821b326aeace5874b05b87fd7d4`. Opcode 11
  smoke `led-smoke-06` packed CRCs matched HOST TRUE16 (`crc_a=2700312459`,
  `crc_b=997782662`), emit 5,112.104 µs and latch 300.011 µs. The reported
  1,302–1,466 cycle loop-start periods are software measurements, not GPIO pulse
  widths. CRC proves packed-buffer **emission**, not pin reception or photons.
  `K1_RA8P1_LED_OUTPUT` = `WIRE_SMOKE_RECORDED`. G4 miss remains 2,005/6,000
  = 33.4%; the 5.412 ms IRQ-disabled span also cannot enter the realtime path.

## Implementation commits

| Repository / commit | Material result |
| --- | --- |
| RA8P1 `2d4c73e`, `11c4b6d` | Enforced explicit nonempty import slices, cumulative timing retention and disposable negative fixtures |
| RA8P1 `c9acaef`, `1123522` | Actual pinned time/AP/VP import, independent host replay, native tests and platform-math isolation |
| RA8P1 `f4aa77b`, `e02525f` | C++ M85 shell, identity-gated USB/recovery, scalar startup and full target instrumentation |
| RA8P1 `e218e7f`, `ab17385` | Lossless resident 6,000-hop replay and frozen scalar workload profile |
| RA8P1 `a79ff5e`, `1a93de6` | Observer-distortion repair and byte-exact O3 scalar profile |
| RA8P1 `bf5185e` | Real generated U55 command stream and alternating exact inputs scheduled beside actual K1 |
| RA8P1 `346646b` | Capacity-one semantic seam with loss/delay/stale/order/identity/finite/pressure/error/timeout recovery |
| RA8P1 `1a0f62b` | Generic P4 target scheduler for rFFT, bin-56 Goertzel and symmetric Hann-rFFT under NPU load |
| RA8P1 `b95e999` | P4 deadline guard, unused-flag rejection and explicit unresolved 6 ms authority |
| RA8P1 `195c935` | Generated-NPU closure, measured NPU duty, observer-free qualification and fail-closed heap-growth acceptance |
| RA8P1 `5a19e2e` | Integer-only target timing/status reports, mutation-bin repair and regressions that reject floating status formatting |
| EdgeAI `d79591d` | Completed bounded candidate admission and correct no-candidate branch |
| EdgeAI `0b575f3` | Independent host implementation/comparison of the actual generic P4 kernels |

## F1 evidence

`target-corpus-02/receipt.json` is ON-SILICON PASS on UID
`545433931bd25436593630352d068363`, build
`a07b6bba8f5479cdaa3aa75a6d65e4a25de787c7b2ab1e52590e5e938f58f0ec`,
source `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a` and the
`sr24000.hop180.bins80.xover40` contract.

The three cases are 6,000 synthetic-control hops and two 4,000-hop MUSDB
official-test extracts. Each hop compares 526 fields. The complete target result
is exact against the independent Arm-newlib-profile host reference. Stack
untouched fell from 27,712 to 24,920 bytes; heap used/maximum remained 40,560
bytes. M33 and U55 were parked and C++ constructor startup was witnessed.

The same image's USB-paced diagnostics measured tempo-frame maxima around
9.71 ms and ordinary-frame maxima around 4.40 ms. Those transfers were not the
resident deadline test and do not themselves pass or fail G4.

## G4 scalar result and corrected images

The corrected O2 resident run `scalar-schedule-01` completed all 6,000 hops with
zero correctness errors, zero render-budget misses, queue high-water one, zero
drops/coalesces and no heap growth. It failed the frozen schedule:

| Measurement | O2 ON-SILICON result |
| --- | ---: |
| Deadline misses | 2,005 / 6,000 |
| Release-guard failures | 2,008 / 6,000 |
| Total mean / max | 5,637.503 / 9,684 us |
| Tempo mean / max | 9,171.905 / 9,684 us |
| Ordinary mean / max | 3,871.627 / 4,370 us |
| Dual-channel render mean / max | 225.978 / 657 us |
| Comparator telemetry mean / max | 245.866 / 250 us |
| Lateness mean / max | 647.755 / 5,107 us |

The preserved target mutation run reports exactly one correctness failure, so
the full-output CRC comparator does go red. The dominant failure is actual
tempo/ACF compute, not render, telemetry, queue growth or heap use.

The O3 scalar preflight kept all 6,000 outputs exact but missed 2,006 deadlines.
Tempo mean/max improved to 8,903.676/9,391 us and ordinary mean/max to
4,080.121/4,553 us, which is still insufficient for the 7.5 ms contract. It was
not extended into the frozen 30-minute soak.

The identified NPU cells produced these target results:

| Mode | Correct NPU calls | NPU duty | Deadline misses | Backlog high-water | Result |
| --- | ---: | ---: | ---: | ---: | --- |
| NPU-alone, cold | 900 / 900 | 2.382% | 0 | 0 | FAIL: 264-byte first-use allocation |
| NPU-alone, warm diagnostic | 900 / 900 | 2.382% | 0 | 0 | Diagnostic PASS; no recurring heap growth |
| Scheduled K1 + NPU | 900 / 900 | 2.383% | 2,069 | 1 | FAIL |
| Saturation K1 + NPU | 18,000 / 18,000 | 47.647% | 6,000 | 1,730 | FAIL |
| Scheduled + semantic failures | 900 / 900 | 2.383% | 2,068 | 1 | FAIL |

All K1 outputs remained exact and every NPU output matched. No scheduled or
saturation mode qualified for soak. The fresh O3 and NPU boots each exposed the
same one-time 264-byte increase before later runs stabilised; this is consistent
with first-use status/formatting infrastructure rather than recurring workload
allocation. The runners now prime one idle status response before the resource
baseline while preserving zero-growth acceptance for the measured schedule.

Five source-bound images were produced; build 08 is the preserved intermediate
P4-only formatter repair and build 09 includes the same correction for the K1
resident schedule reporter:

| Image | Build ID | text / data / BSS | HEX SHA-256 | State |
| --- | --- | --- | --- | --- |
| O3 scalar | `52c1cf2c…de361a` | 425,852 / 18,104 / 351,812 | `06d12585…a80a9` | Flashed, verified, preflight failed |
| K1 + identified U55 + failure seam | `690f3e20…8664fc` | 636,676 / 18,128 / 607,992 | `0417f481…74eac` | Flashed, verified, all bounded cells executed |
| Generic P4 + K1 + identified U55, build 07 | `4e72847c…bc6565` | 666,500 / 42,408 / 657,172 | `e604a4dc…c148c` | Flashed and verified; full P4 campaign executed |
| Generic P4 reporter repair, build 08 | `a6c93438…63eeee` | 666,852 / 42,408 / 657,172 | `be08cd34…5a2b78` | Two 120-second ROM-entry windows expired without a device or write; superseded before flashing |
| Generic P4 integer-only reporters, build 09 | `c4ceebe7…0e87bb` | 666,932 / 42,408 / 657,172 | `82e72f58…ede320` | Flashed with full 709,344-byte readback; bounded source-bound campaign PASS |

All use Arm GNU 13.3.1, scalar M85 FP, `-ffp-contract=off`, no fast-math,
disabled vectorisers and no MVE. M33 remains parked. The NPU images bind the
same external generated graph sources and two alternating INT8 inputs; raw PMU
cycle, NPU_ACTIVE and MAC_ACTIVE counters are recorded without a factor-of-two
conversion.

The semantic seam never feeds smoke output to lighting. Its target campaign
accepted 309 controlled updates, fired each required rejection/error cell once,
used exactly one capacity-one pressure replacement, recorded nine fallback hops
and nine recoveries, and finished valid. The enclosing coexistence cell still
failed on 2,068 deadlines, so semantic recovery correctness does not pass F2.

## Generic P4/E1 boundary

The profile `e1-p4-workload-profile.json` was frozen before target capture. It
binds 16 kHz, 2,048 samples, seed 0, bin 56 (437.5 Hz), all 1,025 output bins,
the actual C kernels, packed fixture, compiler/BSP and identified NPU inputs.
It uses a 128 ms non-overlap release/deadline, 50 ms NPU cadence, a 2 ms NPU
admission guard and capacity-one scheduling. Concurrent and saturation
qualification each require 14,063 releases (1,800.064 seconds). The comparator
uses a predeclared two-ULP FFT limit and 1e-6 Goertzel absolute limit; a final-bin
mutation must fail exactly one release.

Build 07's corrected final-bin mutation reports exactly one failed release at
bin 1,024. DSP-alone, NPU-alone, concurrent and saturation preflights pass with
polling on and off except the first cold DSP cell's 264-byte heap increase. All
numerics, deadlines, guard checks and queue bounds pass in that cold cell.

The cold increase was traced to the first nonzero floating status response,
not the static DSP workspace: the linked call path is newlib
`_svfprintf_r -> _dtoa_r -> _malloc_r`, while the following runs remained at
40,904 bytes. Commit `5a19e2e` replaces P4 and resident-schedule timing fields
with bounded integer fixed-point formatting. Host tests intercept `snprintf`
and reject any floating conversion. Build 09 binds that change, and its cold
target run retains the 40,640-byte heap baseline.

Build 07 completed both frozen qualifications:

| Mode | DSP releases | NPU calls / duty | DSP mean / max | Misses / guard | Queue | Resources | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Concurrent + semantic failures | 14,063 | 36,001 / 2.380% | 52,946.231 / 52,950 us | 0 / 0 | high-water 1 | heap 40,904 -> 40,904 | PASS |
| Saturation | 14,063 | 829,759 / 54.864% | 52,946.323 / 52,950 us | 0 / 0 | high-water 1 | heap 40,904 -> 40,904 | PASS |

The concurrent qualification also finished semantic-valid with one injected
loss and nine recoveries. Both runs used the identified U55 and recorded nonzero
raw cycle, NPU-active and MAC-active counters. They remain historical build-07
observations. Build 09 subsequently passed the complete affected short matrix:

| Build-09 bounded cell | Observer | Result |
| --- | --- | --- |
| DSP alone | on / off | PASS / PASS |
| NPU alone | on / off | PASS / PASS |
| Scheduled concurrent | on / off | PASS / PASS |
| Saturation | on / off | PASS / PASS |
| Final-bin mutation | on | PASS: one detected failure at bin 1,024 |

Every non-mutation cell reported zero numeric/NPU/deadline/guard failures,
queue high-water at most one, and heap 40,640 -> 40,640. Saturation recorded
59.7449% measured harness duty. Generic E1 is therefore accepted for this
bounded scheduled-coexistence profile. There is no new generic soak requirement.

## Semantic selection

The completed scope is the existing ShareStudent checkpoint with the existing
one-second non-overlapping frontend/evaluation schedule. Exact weights loaded;
four-source dependence was checked on 48 cached official-test windows. The 1 Hz
schedule fails both admitted transport envelopes, and exact zero PCM emits
nonzero shares `[0.131588, 0.329106, 0.228604, 0.310702]`.

That is a completed necessary-condition failure for this bounded tuple, not a
missing prerequisite. `NO_QUALIFYING_CANDIDATE` and
`NOT_RUN_NO_CANDIDATE` therefore apply. No student I/O freeze, export, Titan
semantic deployment, new ontology or commercial clearance is claimed.

## Physical inventory and board state

The most recent identified board state is `led-build-04`,
`f0205ef836e6be37f0a700d7da9c8c8a86b4243a0986666d4435085a87da56be`, from
`led-prog-03/receipt.json` on canonical UID
`545433931bd25436593630352d068363`. `led-smoke-06` then rechecked that UID,
build ID and source pin before opcode 11. Re-enumeration and runtime identity
remain required before every later board operation; port names are not identity.

No qualified PDM capture, optical/GPIO timing instrument, full product-output
backend, bridge framing, clock mapping, power or thermal instrument has been
established. A single 80/80 split stick has a bounded packed-buffer emission
smoke only. Generic E1 is no longer a prerequisite blocker, but G5/K1-B remain
open on their actual physical dependencies.

## G8 review and decision packet

The independent-review dispatch was rejected by the delegation guard. Per the
Captain's instruction it was not retried and the guard was not changed. The
following is an orchestrator self-red-team, not independent acceptance:

| False-PASS risk challenged | Current disposition |
| --- | --- |
| Stale board or source identity | Target runners require the exact UID, build, source pin, contract, M33 state, C++ startup and expected U55 state |
| Partial-output comparison | F1 checks all 526 typed fields per hop; P4 checks all 1,025 bins of both FFT outputs plus Goertzel; end-field mutations go red |
| Mismatched NPU graph/public inputs | Profiles and runners bind all nine generated source/header files, source ONNX, same-compilation TFLite, bundle/run identity and two packed inputs |
| Dead-code workload | Build receipts require linked call symbols. Generic P4 qualifications executed 28,126 DSP releases and 865,760 identified-U55 calls with exact outputs, raw PMU activity and measured wall duty |
| Counter wrap or clock fiction | 32-bit wrap extension is host-tested; target runners require DWT/tick agreement within 2%, and both 1,800.064-second P4 qualifications passed that check |
| Hidden fast-math or MVE | Compiler flags disable contraction, fast-math and vectorisers; ELF attributes and disassembly reject MVE |
| Missed heavy frames | The 6,000-hop scalar corpus includes every heavy tempo update. O2 reports 2,005 deadline misses rather than averaging them away |
| Queue or observer false pass | Capacity, backlog, drops and coalesces are checked; P4 polling-on/off preflights are separate and qualification is frozen polling-off |
| Heap activity hidden by free-space reserve | Heap-pool, live-use and high-water growth fail. Build 09's cold target cell proved the integer-only reporter retains the 40,640-byte baseline |
| Event/availability or failure recovery confusion | Event and availability timestamps are distinct; wrong identity, non-finite, future, stale, ordering, pressure, error, timeout and recovery cells pass on HOST and remain required on target |
| Future-context or candidate-generated semantic goldens | No semantic candidate qualified. G7 is not run; the smoke graph cannot pass E2 or feed lighting |
| Physical claims inferred from HOST/USB | Capture, LED, S3 transport, power and thermal cells remain explicitly open |

The decision comparison is therefore:

| Factor | RA8P1 evidence | Production comparison / decision effect |
| --- | --- | --- |
| Correctness and deadlines | F1 exact on 14,000 hops; O2/O3 miss 2,005/2,006 deadlines; generic P4 concurrent and saturation each pass 14,063 releases with zero misses | No fresh comparable S3 campaign exists, so K1-A stays separate and open. Eventual generic E1 success cannot repair the actual-K1 F2 failure or justify migration |
| Memory | F1 retains 24,920 stack bytes. NPU runs retain at least 25,144 bytes after all cells; first-use allocations stabilise and do not recur | No fresh cross-platform memory receipt; measured schedules enforce reserve and zero post-prime heap growth |
| Transfer and physical latency | Identified USB fixture transport works; the actual Titan-S3/capture/output path is unproved | K1-B and physical F3 stay open; host traffic is not substituted |
| Tooling and recovery | The proven route programmed and verified O3, NPU and P4 images; failed ROM-entry attempts made no write | Recovery is source-bound but physical ROM entry remains intermittent |
| Power and thermal | No identified measurement exists | No production power/thermal conclusion is allowed |
| Implementation complexity | Actual K1 M85, optional U55 load, failure seam and separate P4 workload now build reproducibly | A production move adds unqualified M85/U55/S3 integration and recovery surfaces while the current platform remains the mature choice |

**Recommendation:** retain the existing production platform and keep RA8P1 as a
bench candidate. Reconsider an AP/VP migration only after O3 scalar and the
identified NPU campaigns pass their frozen preflights/qualifications, the
build-09 cold fix and source-bound P4 rerun close E1, a fresh comparable
production-S3 campaign closes K1-A, the
identified Titan-S3/capture/output path closes K1-B/F3, and power/thermal evidence
is measured. The no-candidate outcome keeps E2, semantic F3 and K1-C unpassed.

## Gate matrix

| Gate | State | Remaining proof |
| --- | --- | --- |
| G0 | PASS | Preserve source/import authorities |
| G1 | PASS, HOST + identified target | Preserve timing/identity regression |
| G2 | PASS for frozen HOST AP/VP slice | Preserve profile and scope |
| G3 / F1 | PASS for covered modules | Physical capture/output explicitly separate |
| G4 scalar | PASS on exact slim-status O3 DTCM image `603e3f17…` | Combined/NPU/physical images do not inherit this stamp |
| G4 / F2 NPU coexistence | Historical O2/O3-without-slim and identified NPU cells FAILED | Q2 derived profile still required; generic E1 already PASS |
| G5 / physical F3 | OPEN / dependencies identified | Identified capture/output/bridge hardware and measured paths |
| G6 | `NO_QUALIFYING_CANDIDATE` | New material evidence would be required to reopen |
| G7 | `NOT_RUN_NO_CANDIDATE` | Correct terminal state for this candidate scope |
| G8 | PARTIAL | Physical cells, fresh S3 comparison and independent review unavailable |
| E1 | PASS for bounded generic scheduled coexistence | Build 09 short matrix is source-bound; simultaneous contention is explicitly unproved |
| E2 / semantic F3 | UNPASSED | No qualifying selected/deployed candidate |
| K1-A | OPEN, independent of F2 | E1 plus fresh comparable production-S3 campaign |
| K1-B | OPEN | E1 plus identified real Titan-S3 transport |
| K1-C | UNPASSED | E1/E2/K1-A/K1-B, then Captain decision |

Current recommendation remains: retain the existing production platform and
treat RA8P1 as a bench candidate. F1 and bounded generic E1 are real; the
actual-K1 O2/O3 deadline failures, missing physical evidence and no semantic
candidate do not support production migration.

## Exact continuation

External root:
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002`.
`external-receipts.json` binds decisive receipts. Failed receipts are preserved.

Build 09 is programmed and its bounded source-bound campaign is complete. Do
not repeat generic P4 or extend it by duration. The next implementation task remains
stage-level profiling of the actual K1 tempo-heavy hop, followed by the smallest
exact-behaviour change that reduces its measured cost. Rebind the board by UID
before target execution.

Agent observation 2026-09-12: GPT/DMA is no longer host-only. `build_scalar.py`
copies `ws281x_gpt_dma_hw.c` and stages DMAC2. The WS2812 autonomous image is
built; flashing is waiting on identified ROM entry. FastLED GPIO smoke remains
as a diagnostic path and cannot close GPT acceptance. GitHub has no open issues
on this repository.

Agent observation 2026-09-11: `raw-stage-o3-profile-03` is a passing PRE-SILICON
image; `raw-stage-o3-profile-prog-01` failed with zero RA USB Boot devices while
application CDC `045b:5310` `/dev/cu.usbmodem00000000000011` was present. Live
INFO matched UID `545433931bd25436593630352d068363` and build `70c2323a…`
(showcase restore). That image is not the resident G4 profiler, so the 22,500 µs
hypothesis is still unmeasured. Static nm/objdump of the profiler ELF counted
118 `__aeabi_d*` call sites (43 in `AffineClockEstimator::refit`, 40 in fixture
serialise, 15 in `bindTempoPhase`). Runtime wrap counters are now in the
profiler path (raw-trace version 2).

Host regression:

```sh
python3 -m unittest discover -s tests/host -v
python3 scripts/verify_imports.py --slice timing --enforce
python3 scripts/verify_imports.py --slice product --enforce
python3 scripts/test_ws2816_pack.py
python3 scripts/test_ws2816_emit_protocol.py
python3 scripts/test_gold_extract.py
python3 scripts/compare_led_backend.py
python3 scripts/test_ws281x_gpt_dma.py
python3 scripts/test_rate_adapt.py
python3 scripts/test_double_probe.py
python3 scripts/test_fixture_protocol.py
python3 scripts/test_schedule_protocol.py
python3 scripts/test_semantic_sidecar.py
python3 scripts/test_p4_runtime.py \
  --kernels /Users/spectrasynq/SpectraSynq-EdgeAI-Lab/deployment/ra8p1/titan_p4 \
  --fixture /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/p4-fixture-01/p4_fixture.h
```

DualMCU remains at `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`; its `_to_delete/`
is untouched. BSP remains clean at `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7`.
The Lab's unrelated untracked strategic plan, brief, review and `test-results/`
remain unstaged. No retired cadence run, audible loop, worktree or production-S3
mutation occurred.

---
**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-16 | agent | Ban computer tones and white noise in the room. Scripts refuse. Gate test added. |
| 2026-09-16 | agent | Motion dump: occupancy spreads/fills, not a travelling dot. Speaker still 0 rails. |
| 2026-09-16 | agent | Scorer uses live_age. Headroom tones no new rails. Mode 32 dump is stationary. |
| 2026-09-16 | agent | Rearm. Isolated-hit run-20260916-01 TRUE_BLACK. Run-32 verdict kept. |
| 2026-09-15 | agent | Rearm prog-17 USB-C. Run-28: quiet chroma ~0.1 retriggers DualMCU. |
| 2026-09-15 | agent | Path tap in status JSON. Hop replay cannot cause run-26 rises. Unflashed. |
| 2026-09-15 | agent | Rearm prog-13 WRITE_VERIFIED 107f1a00 dwell-fade image. Mode 32. |
| 2026-09-15 | agent | Dwell fade: Q8.8 elapsed-time, no min-1. Host isolation PASS. Unflashed. |
| 2026-09-15 | agent | Quiet-gap run-25: leftover picture, not fresh sound. True black still open. |
| 2026-09-15 | agent | Rearm prog-12 WRITE_VERIFIED d13cd520. App back. Opcode-1 identity match. |
| 2026-09-15 | agent | Flashed d13cd520. Wake PASS emit-continuous. Music no 23 s stall. |
| 2026-09-15 | agent | U13/U14 finger-cover INCONCLUSIVE (r=0.98). Receipts run-13..15. |
| 2026-09-15 | agent | Resident c65c6fca SINCRNG10+gain 2.4. Wake PASS run-10. fcc6f3d5 clip fail kept. |
| 2026-09-15 | agent | Music baseline sampled PASS on 55320f89 (run-07). Run-06 kept. Donors next, no flash. |
| 2026-09-15 | agent | No reflash. Music baseline NEEDS_WORK on 55320f89 (run-06): pad/perc lit; 5 s post-music still LSB. |
| 2026-09-15 | agent | Isolated-hit wake ON-SILICON PASS on 55320f89 (run-05). Clock 133.35 hops/s. Music baseline next. |
| 2026-09-15 | agent | Rearm prog-04 WRITE_VERIFIED 55320f89. 8 s dump 38/40 lit, no 61-black run. Isolated click not closed. |
| 2026-09-15 | agent | Wake diagnosis of 3526df5c: DualMCU still ran between hits; host repair freezes unmirrored history. Unflashed. |
| 2026-09-14 | agent | PCM SENSITIVITY+loud-guard, 5s dwell, hold-in-place overlay. Image 3526df5c unflashed. |
| 2026-09-14 | agent | Folded K1 effect-decomposition + waveform rationale + autonomous-loop into VP-EFFECTS-CONTRACT. |
| 2026-09-14 | agent | K1-canon: DualMCU VP not authority. Unflashed 40a7769d visual SENSITIVITY 16x. |
| 2026-09-14 | agent | Resident 77f77ecf leftover<=2 DMA PASS 200s; mode 32 snap-fade PASS dim; SINCRNG 10 not flashed. |
| 2026-09-14 | agent | SINCRNG 10 image b700a591 built for 40 kHz; host mutation PASS; not flashed. |
| 2026-09-14 | agent | 40 kHz SINCDEC 49 leaves BSP SINCRNG 5; 16 k vs 40 k peak ratio 38.2 vs 39.06. |
| 2026-09-14 | agent | Snap freeze: missing DualMCU caller clearFrame. Host mutation PASS. Image dc07d97d built, not flashed. |
| 2026-09-13 | agent | Combined 40 kHz PDM→24 kHz/180+GPT image built and dry-run ready; not flashed. Clipping investigated (sparse negative rails), no gain cut. |
| 2026-09-13 | agent | GPT recovery image programmed; engine checkpoint PASS (607 DMA/stop/frames, USB alive). Waveform/optical/G4 open. |
| 2026-09-12 | agent | ROM-entry chat is live: speak waiting, bootloader seen, write verified, board back. Silence during USER/BOOT is a fail. |
| 2026-09-12 | agent | Onboard RGB status LED (P108/P109/P110 active-low). Legend: yellow boot, blue ready, magenta test, three red fail, three green pass. Opcode 20. |
