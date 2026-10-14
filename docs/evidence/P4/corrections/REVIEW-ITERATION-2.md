# P4 — ANALYSIS RATE — independent fresh-context review, iteration 2 (required corrections)

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This review was read-only. I did not touch a device, serial port, USB, broker, `launchctl` or flash. Host tests ran in place with `PYTHONDONTWRITEBYTECODE=1 -p no:cacheprovider`; the porcelain status hash was `496ba2465577` before and after. Gate re-runs and mutants ran only in `/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/review-p4c2/`. This file is the only repository file written. The agent cannot listen.

Reviewer: Reality Checker role (model opus), fresh context, 14 Oct 2026. Subject: `lane/titan-lagfree-20261014` at HEAD `e9cd6fe`. The P4 corrections are `1784b06`, `ea18803` and `19f45d2`. The image source files have not changed since `1784b06`. The subject also includes `RECEIPT-ITERATION-2.md` and its evidence. The receipt's committed copy in `docs/evidence/P4/corrections/` is byte-identical: sha `0154b8dd…`.

## Verdict: ACCEPT AS PARTIAL

**Accepted:** the engineering in all three required corrections, the F9 fix, and the correction image `f6daf7fb`.
- I reproduced every device number in the receipt from the raw files.
- The host results reproduce exactly.

**Not accepted yet:** closing P4. The receipt needs text fixes, and one process deviation has to be disclosed and settled. No more device work is needed for P4 itself.

## In plain English

- **The startup stall is gone, and the device shows it.**
  - The longest single capture poll since boot fell from about 8.66 ms to 0.81 ms. One microphone slot is 7.15 ms.
  - I checked that "since boot" really means since boot, and that the window includes both rate-lock filter designs (F-A).
- **The 24 kHz path's loss counter is now read.** It shows 0 drops since boot, and the stream epoch did not change during the capture.
- **The resampler's own cost is now reported for every hop.** The median is 286 k cycles, and no value came near the field's ceiling.
- **The new filter matches the old one to about one part in a million.**
  - The frozen P4 host gate passes unchanged. I re-ran it and got a byte-identical result.
  - I planted errors back into the code. Each one turned a test red.
- **The 28.8-hour timing wrap is fixed.** The test fails when the old arithmetic is put back.
- **Five things in the receipt still need fixing:**
  1. Several lines still say "device not run", although the same receipt later reports the device pass.
  2. During the deploy, the plan's stop rule fired and was not followed. A hub-port power cycle that no script contains was used instead, and the candidate was programmed a second time. The receipt reports the facts but never says the stop rule fired.
  3. A gap of about 4.4 minutes, when the Titan was missing from USB with its light unobserved, is not listed.
  4. Defect D1, the USB serial failure after reset, now happens every time on this cabling. The canonised fix ("one reset restores it") is now false.
  5. The Titan no longer runs `f6daf7fb`. The P6 image `b5595c35` replaced it at 13:56:51Z, and that image contains the P4 corrections.

## Required corrections from iteration 1: status

| # | Iteration-1 requirement | Status | Evidence I checked |
|---|---|---|---|
| 1 | Export the 24 kHz counters, re-run BOOM, show drops Δ 0, and report the resampler's cycles separately | **Closed** | `pdm_target.c:1025-1035` (status), `k1_live_runtime.cpp:952-968` (MIR), `k1_live_runtime.cpp:426-433` (per-hop field). The MIR before/after (`cost-p4-corr-boom-001/counters.json`) shows `analysis24.drops` 0 → 0 and `epoch` 2 → 2. `sequence` went 5,555 → 16,362, which equals the in-house generation both times. The donor/in-house generation ratio is 1.5001 and then 1.5000. I recomputed D4 from `timing.jsonl`: BOOM 28–75 s gives p50 286,032, p99 293,648, max 295,952, mean 191,371. 0 of 16,200 rows are saturated. |
| 2 | Measure or remove the rate-lock stall, and disclose "designed offline" | **Closed** (removed and measured) | `k1_poly24.c:59` now uses the offline table `k1_poly24_window.h`. The symbol `k1_poly24_window` is at `0x0208c770` (`arm-p4-corrections-001/symbols.txt:1810`). Since-boot `poll_max_cycles` is 808,555 in both snapshots, under 7,150,000. `designs` is 3, `design_max_cycles` 660,956 and `design_last_cycles` 616,271. The plan deviation is disclosed at RECEIPT-ITERATION-2.md:31. |
| 3 | Receipt text: F2, F4, F5, F6, F10 | **Closed**, with new text defects (F-C, F-F, F-H) | See "Text corrections" below. |
| 4 (opt.) | F9 wrap test | **Closed** | See F-B. |
| 4 (opt.) | F7 donor-stream check | **Not met, honestly recorded** | RECEIPT-ITERATION-2.md:70-77. The donor resampler blob is identical, which agrees with my iteration-1 finding. |

## Criteria before results

- **[FACT] The frozen file is intact.** `scripts/freeze_tolerance.py verify` on `TOLERANCE-FROZEN-corrections.json` returned `{"ok": true, "problems": []}`. I passed it the H2 gate output, `poly24-measured-corrections.json`, the BOOM counters and the cost comparison. That checks three things:
  - the body hash matches `8780cd10…`;
  - the file mode is 0444;
  - every result file was written after the freeze.
- **[FACT] The run copy and the docs copy are byte-identical** (file sha `27bb9507…`).
- **[FACT] The order of events:**
  1. Freeze at 12:11:59Z (file mtime 20:11:59 local; STATUS.md:58).
  2. H2 gate output at 12:18:42Z.
  3. Correction image built at 12:19:52–12:19:59Z (`arm-p4-corrections-001/receipt.json` start/end).
  4. Commit `1784b06` at 12:21:06Z.
  5. First programme attempt at 13:47:23Z. Device results followed.
- So **D1–D5 were frozen about 95 minutes before the image ever ran.**
- **[INFERENCE] For H1–H5 I can show that the results were written after the freeze.** I cannot show that nobody looked earlier: no transcript holding the freeze write was found under `~/.claude/projects`.
- H1's limit is 2e-6 and the worst result is 1.01e-6. That margin does not suggest the limit was set after seeing the number, but it does not rule it out either.
- **The frozen image clause holds.** The build receipts of `f2f1983a` and `f6daf7fb` have identical flags. Their sources differ only in these files:
  - `k1_live_runtime.cpp`, `k1_poly24.{c,h}`, `k1_poly24_window.h`, `pdm_target.{c,h}`;
  - `build_scalar.py`. Its one changed line adds the window header to `ANALYSIS24_FILES`.
  - The capture buffer moved by 0x20 bytes.
- **No goalpost moved.** All D1 and D3 values come from the snapshots that D1 and D3 name. D5 uses `p4_cost_compare.py` against `f2f1983a` run 003a.

## Findings

**F-A — Information (confirms D1). "Since boot" is real, and the window covers both rate-lock designs.**
- [FACT] `capture_service` is zeroed only in `k1_pdm_target_initialise` (`pdm_target.c:825`). The in-poll restart path (`pdm_target.c:875-899`) calls `reset_logical_stream(1)` without zeroing it. That would raise the donor `stream_epoch` to 2, but the snapshots show 1.
- [FACT] `capture_a24_designs` and `capture_a24_epoch` are never reset (`pdm_target.c:101-111`).
  - `designs` = 3 equals the boot design (`pdm_target.c:347-350`), plus the 250 ms provisional design, plus the 2 s lock design.
  - The last two run inside `k1_pdm_target_poll`, via `process_pair` → `observe_rate` (`pdm_target.c:733`, `556-561`).
  - Any restart would add epochs and designs. `a24 epoch` = 2 is explained by the boot reset plus one `set_analysis24(1)` (`pdm_target.c:1012-1019`).
- [FACT] At the before-snapshot, uptime was 42.35 s (`capture_time_us`), which is well after the 2 s lock.
- [INFERENCE] Attribution: the poll max (808,555) is the lock-time design (616–661 k) plus an ordinary poll. The whole-run per-hop poll max is 170,920.
- The old 8.66 M figure is attributed to the old design by a paired A/B on identical flags: two old images agree, and the poll path changed only in the design. That is a strong **inference**, not a direct measurement, because the old image had no design counter. RECEIPT-ITERATION-2.md:101 says the counter "demonstrates the attribution". It demonstrates the new design's cost and is consistent with the attribution. Soften that wording.

**F-B — Information (confirms F9 and H1/H5).**
- [FACT] The modular lag (`k1_poly24.c:145-160`) is exact for every reachable state while the true lag is below 2³¹ samples. Truncating `index << 32` reduces every term mod 2³² samples, so their difference stays exact.
- [FACT] Pre-roll clamp (`k1_poly24.c:139`): flush and reset zero `consumed` (`k1_poly24.c:118-128`), so `consumed << 32` cannot have wrapped while the clamp is active.
- [FACT] Within the pre-roll the clamp names sample 0. That makes the first post-flush estimate up to 47 samples (about 1.1 ms) late. This behaviour is unchanged from before (Info).
- **My mutants, all in scratch:**

  | Mutant | Result |
  |---|---|
  | (a) Restore the old clamp `centre >= zeros ? … : 0` | `test_h4_long_stream_chain[2199023255555]` **red**. The clamp fix is guarded by the chain case at 2⁴¹+3, although no planted fixture covers it. |
  | (b) Flip the sign of the backward recurrence | H1 **red** at all four rates (max diff 0.76–0.94) |
  | (c) Use the window of phase 0 for every phase | H1 **red** (about 9e-4). `test_poly24.py` stayed green, so H1 is the guard that catches this. |

- [FACT] **The planted fixture (`test_poly24_corrections.py:53-70`) patches the product function `k1_poly24_capture_lag_q32` itself.** It goes red because it reproduces the old `pdm_target.c` comparison exactly.
- **Limit:** the call site in `pdm_target.c:1091-1092` composes `source_end_exclusive - 1` and `capture_a24_source_base`. It is not host-buildable, so I verified it by reading only.
- [FACT] **My runs:**
  - `test_poly24_corrections.py` + `test_poly24.py`: **28 passed**.
  - The receipt's eight suites: **112 passed**.
  - The H1 probe: 1.013e-6, 5.96e-7, 8.94e-7 and 8.94e-7, with `step_equal` 1 at all four rates.
  - `poly24_design_reference.c` is a faithful copy of the design at `1784b06^`.
- [INFERENCE] Equivalence is host-demonstrated (clang and macOS libm). The device builds with `-ffp-contract=fast` and newlib `sinf`. Device coefficients were not compared, so the device result is configured, not demonstrated. The device's unchanged costs and valid contract are only indirect evidence.

**F-C — Low. H2: the result is reproduced, the disclosure is acceptable, and one figure is wrong.**
- [FACT] `scripts/p4_analysis24_gate.py` reads no `argv`. Its last commit is `21305dc`, so the gate has not changed since iteration 1. An accidental `--help` therefore ran the normal gate.
- [FACT] My scratch re-run gave rc 0 and sha `73aedda250b8…`. That is byte-identical to `analysis24-gate.json` and `analysis24-gate-run3-corrections-PASS.json`, in both the run folder and the docs copy.
- [FACT] Against run 2, 99 of 327 numeric fields changed. So the new design was really used. Gate (iv) is identical: room |Δ| is still 2.0, exactly at the limit (iteration-1 F8 still stands).
- **The figure:** RECEIPT-ITERATION-2.md:42 says "every value moved by at most about 1e-5". The maximum absolute change is **4.8e-5** (`planted/gain/…/ii/mean_magnitude/78`, about 3.66). The relative change is about 1.3e-5. Say "relative".
- The `--help` disclosure is honest and harmless. Keep it.

**F-D — Low. D4 encoding: one record per drain, no saturation in this run, and the tool cannot flag saturation.**
- [FACT] The only caller of `take_analysis24_cycles` is `k1_live_runtime.cpp:430`. It runs once per emitted timing record.
- [FACT] On `Rejected`/`Fault` returns (`k1_live_runtime.cpp:275`, `279`) the sum carries into the next record. Those cycles are moved to the next hop, not lost.
- [FACT] A not-ready pull still costs about 160–176 cycles. That explains the third of hops at the floor, and it matches "one hop in three carries no pull".
- [FACT] Saturation is at 0xFFFF × 16 = 1,048,560. 0 of 16,200 rows reached it.
- **Gap:** `scripts/titan_cost_capture.py:60-61` multiplies by 16 without counting saturated rows. A future saturated row would cap p50/p99 silently, although the max would still show it. Add a saturated-row count.

**F-E — Information (confirms D3). `drops` is incremented on the only overflow path.**
- [FACT] The only overflow path is the `k1_poly24_push` failure at `pdm_target.c:675-685`, and it does `drops++` with an epoch bump.
- Every other way the 24 kHz path loses audio also bumps the a24 epoch:
  - a logical-stream reset (`pdm_target.c:354`), which the donor counters also catch;
  - `set_analysis24` (`pdm_target.c:1018`);
  - stale-test discards.
- So D3's "epoch unchanged" covers them.
- One gap: a pull `FAULT` (`k1_live_runtime.cpp:251` `break`) has no a24 counter. [INFERENCE] It is unreachable in practice: it needs `err_arg`, or a sequence near 2⁶⁴/180.

**F-F — Medium (process). The plan's stop rule fired during the deploy. It was not followed, and the receipt does not say so.**
- [FACT] `PLAN.md:39` says: "Image fails to identify, or no USB serial link after one reset. Recover to `a8215f75` immediately."
- [FACT] `programme-p4-corr-004.deploy.log` (13:50:50Z) shows `identity: MISMATCH no-info`, "D1 remedy: one … reset; recover the known-good image only if the node is still absent", then `STOP`.
- [FACT] RECEIPT-ITERATION-2.md:88 says two `-run -sig` resets did not clear it.
- [FACT] What happened instead:
  - an unscripted `uhubctl -n 05e3:0610 -p 4 -a cycle` at 13:51:02Z (STATUS.md:95);
  - then a **second programming** of the same candidate (`programme-p4-corr-005`, 13:51:25–13:51:58Z, about 33 s known dark);
  - which produced a **third** D1, cleared by a third hub cycle (`programme-p4-corr-005.d1-watch.log`).
- [INFERENCE] The second programming served only to record admission. The 004 run had already flashed `f6daf7fb` with rc 0, and the deploy script keeps the old admission on STOP.
- The outcome was good: the Titan visualised the candidate, and no recovery flash was needed.
- **But this is the same class of event as iteration-1 F4.** The receipt must:
  - name the clause that fired;
  - state that the plan's recovery was replaced, and why;
  - record the extra dark interval that the re-programme caused.

**F-G — Medium (hardware truth). D1 is now 100 % on this cabling, and the canonised fix is false.**
- [FACT] PL14 (`TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md:52-53`) says D1's rate is 2 of 15 resets and that "One SWD signature-and-run reset … restores it".
- [FACT] This session's record:
  - D1 at 13:49:01Z, 13:51:02Z and 13:52:06Z, plus the P6 deploy at 13:56:51Z (STATUS.md:95, 98);
  - two SWD resets did not clear it.
- That is a material change in a known hardware fault. It should be reported to the Captain as such, not filed as "this session's cabling", and PL14 and `smc-titan-mixer` LESSONS need to be updated.
- **Hub cycle risk:**
  - [FACT] The log names no device on port 4 (`connect []`). The hub is matched by VID:PID, not by location. The cycle also switches the USB3 companion hub's port 4.
  - [FACT] The Titan did **not** reboot on the 13:52:06Z cycle. At 21:52:40 local the MIR uptime was 42.35 s, so it booted at about 21:51:58, when programming ended. The time base is the DWT-extended counter (`pdm_target.c:269-283`).
  - [INFERENCE] So this port cycle re-enumerates USB but is **not** a Titan power cycle. It cannot substitute for the G10 power-on cold boot (PL14-A5). Nobody should cite it as a "verified switchable port".
  - [INFERENCE] If a later cabling does power the Titan from that port, the same command becomes a power-on with the E2 Lite attached. G10 recorded about 83 minutes of probable dark under exactly that condition.
- **Recommendation:** canonise the cycle only as a scripted, guarded step:
  1. Resolve the port by USB location ID to 045b:5310 before cycling.
  2. Allow one cycle at most, then recover the known-good image.
  3. Log uptime before and after the cycle.
  4. Amend the plan's stop clause to name it.
- Until then, record it as an operator deviation.

**F-H — Medium (text). The receipt contradicts itself, and some lines are stale.**
- [FACT] These lines still describe the device as not run:
  - RECEIPT-ITERATION-2.md:13 ("a device check that has not run yet");
  - :17 ("has not been run yet");
  - :25-26 ("**device not run**");
  - :45 ("Device (D1, D2): not run").
- Line 3 and lines 83-108 report the device PASS. A reader of the plain-English lead gets the opposite of the truth.
- [FACT] Line 110, "Installed now: `f6daf7fb`", was true when written (13:55Z). It is superseded: `b5595c35` was deployed at 13:56:51Z (STATUS.md:98). That image is "f6daf7fb + boot composition v9" (`admission-p6-v9-b5595c35.json`).
- [FACT] "D1" names two different things: the frozen criterion D1 (the stall) and the defect D1 (USB CDC). In line 88 the two read alike. Rename the defect, for example to "USB defect D1".

**F-I — Low-Medium. An unobserved interval is missing from the list.**
- [FACT] STATUS.md:93-95 record the sequence:
  - the E2 Lite was plugged in at 13:44:38Z, and the Titan's 045b:5310 dropped off USB;
  - four `rfp-cli -sig` attempts followed;
  - a programme attempt at 13:47:24Z was REFUSED because the Titan was not identified (`programme-p4-corr-003.deploy.log`);
  - the Titan returned only after a hub cycle at 13:49:01Z.
- **That is about 4.4 minutes with the light state unobserved.** RECEIPT-ITERATION-2.md:89-92 starts at 21:49:12 local and omits it. Its pointer to the G10 RECORD covers the earlier 11:43–13:06Z absence, not this one.
- Also split the final "about 49 s" into:
  - about 33 s known dark (21:51:25–21:51:58);
  - then about 15 s with the application running (the MIR uptime shows it) but the light unobserved.
- Also state whether the E2 Lite was unplugged afterwards. G10's proposed bench rule says it should be.

**F-J — Low. Deploy interpreter override.**
- [FACT] `TITAN_DEPLOY_PYTHON` is a supported knob (`smc-titan-mixer/scripts/deploy_titan_image.sh:25`).
- [FACT] The preflight import guard (`:27`) went red on the real broken Homebrew 3.14 `pyexpat` (`programme-p4-corr-001.stdout`). That is a guard seen red on a real fault.
- The override is acceptable. Two gaps:
  - the receipt gives `python3.13` with no resolved path;
  - the script's default is still the broken interpreter, so every future deploy will refuse until it is changed.
- Record the absolute path, and fix the default or the Homebrew `expat` link.

## Text corrections from iteration 1: re-check

| Item | Verdict | Evidence |
|---|---|---|
| F2 numbers | **Correct** | My recompute from each `timing.jsonl` (BOOM 28–75 s, `publish − capture`) matches the receipt table at :53-55 exactly. a8215f75: 5,465 / 9,043 / 9,589, hop 6,617. 5f1483e8: 6,017 / 6,027. f2f1983a: 6,017 / 5,998. The corrections image `f6daf7fb` gives 5,995.5 / 9,579 / 10,020, so it does not change the +0.53 ms finding. The withdrawal of "nothing else got slower" is honest. |
| F4 | **Closed** | The receipt names the clause that fired, its reading, and the 37 s known dark plus about 3 minutes unobserved (:60-62). (F-F is the same issue again in this iteration.) |
| F5 | **Closed** | :58 |
| F6 | **Closed** | :59. The transition-band values match iteration 1. |
| F10 | **Closed** | `poly24-measured-corrections.json`: 47 samples, 1.1348 ms. Passband 110 Hz moved +0.002 → −0.002 dB, and alias at 15 kHz moved −86.1 → −86.2. Both are within the stated 0.004 / 0.1 dB. The old file is kept unchanged as history. |
| F7 statement | **Accurate** | Recorded as not met. The frozen file is untouched. |
| Readback | **Confirmed** | `corr-readback/events.json`: 127 fields compared, `profile_differences {}`. Expected build `f6daf7fb`. |
| D5 | **Confirmed** | `cost-compare-dtcm003a-vs-corr001.json`: in-house p50 700,131 → 699,507; VP 196,054 → 195,723; emit 127,879 → 128,075; queue 355 → 355. Both runs have empty `counter_deltas`, 0 missing and 0 ring gaps. |

## What is claimed but not demonstrated

- **Documented:**
  - donor-stream identity (F7), from source blobs only;
  - the hub cycle's effect on Titan power. The receipt implies "power cycle"; the evidence says re-enumeration only (F-G).
- **Configured, not demonstrated:**
  - device-side coefficient equivalence (F-B);
  - the `pdm_target.c` call-site composition of the modular lag (read only);
  - saturation handling in the capture tool (F-D).
- **Demonstrated (device):** D1, D3, D4 and D5 values; D2 reporting; readback 127/0; 99.8 frames/s; 0 LED faults; emission ON at 13:52:13Z.
- **Not demonstrated:** the light during 13:44:38–13:49:01Z and during the D1 intervals. No camera or LED-buffer dump covers them.
- **Qualified:** nothing new. P4 is not merged.

## Required corrections (text and canon only; no device work)

1. **RECEIPT-ITERATION-2.md, stale lines.**
   - Mark :13, :17, :25-27 and :45 as superseded by the device section, keeping the history; or update them.
   - Annotate :110 with `b5595c35` (13:56:51Z), which contains the P4 corrections.
2. **Disclose the stop-rule deviation (F-F).**
   - Say the plan's stop clause and the script's remedy fired at 13:50:50Z.
   - Say what replaced them (the hub-port cycle) and why.
   - Disclose the second programming, with its added dark interval and the third D1.
3. **Complete the unobserved intervals (F-I).** Add 13:44:38–13:49:01Z, split the final 49 s, and state the E2 Lite status after the deploy.
4. **Update the D1 canon and raise the fault (F-G).**
   - Update PL14 and the `smc-titan-mixer` LESSONS: rate 3/3 on this cabling, and two SWD resets no longer clear it.
   - Settle the hub-port cycle one way or the other:
     - as a guarded script step, with location-ID check, one cycle, then recovery, and uptime logged; or
     - as forbidden.
   - Record that it did not reboot the Titan and is not a G10 power-on.
   - Rename the defect so it does not collide with frozen D1.
5. **Wording.**
   - :101 "demonstrates the attribution" → "consistent with; paired A/B inference".
   - :42 "at most about 1e-5" → "max absolute 4.8e-5, relative about 1.3e-5".
6. **Optional, low:**
   - a saturated-row count in `titan_cost_capture.py` (F-D);
   - the absolute interpreter path and a fixed deploy default (F-J).

## P4 ship path

1. **Already on silicon and in source:**
   - The Titan runs `b5595c35`, the P6 v9 evaluation image, since 13:56:51Z. It is `f6daf7fb` + v9 boot composition, so it carries every P4 correction.
   - `f6daf7fb` itself ran 13:52–13:56Z with readback 127/0 and D1–D5 PASS.
   - Recovery images: `f6daf7fb`, `f2f1983a` and `a8215f75`.
   - Source: `1784b06`, `ea18803` and `19f45d2` on `lane/titan-lagfree-20261014`. Not merged.
2. **Remaining steps:**
   - (a) the five text and canon corrections above;
   - (b) a short re-review of those edits only;
   - (c) integrate the lane into the Kx-Titan product line, and record the reviewed commit in the SpectraSynq-PRISM umbrella pointer;
   - (d) choose the shipping image: if the Captain rejects v9, re-deploy `f6daf7fb` (or a successor containing `1784b06`) through the identity-checked route, using the canonised D1 handling.
3. **Who acts:**
   - (a) and the preparation of (c) and (d): the Titan lane owner (agent);
   - (b): a fresh reviewer;
   - Captain: the P6 visual verdict; approval to merge and publish under the no-push boundary; the bench decision on the D1 hardware fault and E2 Lite cabling.
4. **The stamp that means shipped:**
   - a re-review verdict of ACCEPT on P4;
   - the umbrella records a reviewed Kx-Titan commit that contains `1784b06`;
   - the Titan reports that commit's build hash, with readback 127 fields 0 differences, after the identity-checked deploy.

## Re-review of corrections (same reviewer, read-only, 14 Oct 2026)

Scope: only the edits made for required corrections 1–5. I checked:
- `RECEIPT-ITERATION-2.md`. The run copy and the committed docs copy (`348e953`) are byte-identical (sha `c8e69de7…`).
- Kx-Titan `f6aeacf`, which adds PL14-A11.
- agent-skills `3264307`, which changes the `smc-titan-mixer` SKILL.md row.

Nothing else was re-checked, and no device or USB was touched.

### Final verdict: ACCEPT

All five required corrections are applied in substance. P4's review gate is passed. Closing P4 still follows the ship path below.

| # | Correction | Status | Evidence |
|---|---|---|---|
| 1 | Stale lines; installed image | **Closed** | Superseded markers at receipt :13, :17, :25, :26 and :45. Line :3 and line :110 now name `b5595c35` (13:56:51Z) as installed now, and `f6daf7fb` as installed at capture time. |
| 2 | Stop-rule deviation | **Closed** | Amendments :116-125. They name the fired clause, the script's remedy and its STOP. They state what replaced them and why, and give the cost: a second programming of about 33 s known dark and a third occurrence. They say plainly that it "remains a deviation". |
| 3 | Unobserved intervals; E2 Lite status | **Closed** | The table at :129-138 covers 13:44:38–13:52:13Z plus the P6 deploy. It separates known dark from unobserved, and the 49 s is split. :140 says the E2 Lite is still attached and that the Captain has been asked. |
| 4 | D1 canon, fault report, hub cycle, rename | **Closed** | PL14-A11 corrects the false "one reset restores it", records the 4 of 4 rate, and says the bench fault was reported to the Captain. The SKILL row gives a guarded procedure: check the locationID, one cycle, log uptime, then recover. It says "not a power-on and never a G10 cold-boot substitute". The defect is renamed USB-CDC-NOATTACH (:114). |
| 5 | Wording | **Closed** | :42 now reads "4.8e-5 absolute (about 1.3e-5 relative)". :101 now reads "consistent with … paired A/B inference, not a direct measurement". |
| 6 (optional) | Saturated-row count; deploy default interpreter | **Not done. Does not block.** | 0 saturated rows in this run, and the max field shows saturation anyway. The absolute override path is now recorded in the SKILL row (`/opt/homebrew/opt/python@3.13/bin/python3.13`). Fix it before the next deploy: the script's default still refuses until then. |

### Non-blocking nits (low; fix whenever the receipt is next touched)

- **Stale wording.** Receipt :88 and :91 still say "D1" and "power cycle of the Titan's hub port". The old interval list at :89-92 is not marked as superseded by the table at :129-138.
- **Mislabelled claim.** At :127 the parenthesis "(the Titan is not powered from that port)" sits under [FACT]. Only the uptime observation is a fact; the power topology is an [INFERENCE] from it. PL14-A11 and the SKILL row repeat the same parenthesis.
- **The automatic watcher is not explicitly guarded.** The SKILL row's watcher "cycles 8 s after `programme rc=0`". It should be bound to the same guarded steps 1–4 (locationID check, ONE cycle). Otherwise the guard is missing from that route.
- **Plan stop clause not amended.** The hub cycle is still not built into `deploy_titan_image.sh`, and the written stop clause in the lane's plan is not amended. The next plan that reuses "after one reset → recover" will hit the same deviation. Carry this into the next lane plan rather than reopening P4.

### Ship path, unchanged except for step 2

1. **Already on the device:** `b5595c35`, which includes all P4 corrections. Source: `1784b06`, `ea18803`, `19f45d2` and `348e953` on `lane/titan-lagfree-20261014`. Not merged.
2. **Remaining steps:**
   - integrate the lane into Kx-Titan, and record the reviewed commit in the SpectraSynq-PRISM umbrella pointer;
   - if the Captain rejects v9, re-deploy a build containing `1784b06` through the identity-checked route, using the guarded USB-CDC-NOATTACH procedure;
   - optional item 6.
3. **Who acts:**
   - the lane owner prepares the integration;
   - the Captain approves merge and publication under the no-push boundary, gives the P6 visual verdict, and decides on the E2 Lite cabling and the USB-CDC-NOATTACH bench fault.
4. **Stamp:**
   - this ACCEPT;
   - the umbrella records a reviewed Kx-Titan commit that contains `1784b06`;
   - the Titan reports that build's hash with readback 127/0 after the identity-checked deploy.

## Changelog

| Date | Author | Change |
|---|---|---|
| 2026-10-14 | agent:claude (Reality Checker, opus; fresh-context reviewer, not lane owner) | Created: review of P4 required corrections iteration 2 (RECEIPT-ITERATION-2.md, commits 1784b06/ea18803/19f45d2). |
| 2026-10-14 | agent:claude (same reviewer) | Re-review of corrections 1–5 (receipt 348e953, Kx-Titan f6aeacf, agent-skills 3264307): final verdict ACCEPT; item 6 non-blocking; four low nits. |
