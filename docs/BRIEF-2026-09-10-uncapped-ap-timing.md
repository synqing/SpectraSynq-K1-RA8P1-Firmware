# Brief — uncapped AP timing re-measure (K1-RA8P1-002)

**Repository:** `SpectraSynq-K1-RA8P1-Firmware`, branch `lane/k1-ra8p1-002`
**Authority:** `AGENTS.md`, `docs/evidence/K1-RA8P1-002/STATUS.md`
**Hardware:** the identified Titan, UID `545433931bd25436593630352d068363`
**Abbreviations:** AP = audio processing. VP = visual processing. FPU = floating-point
unit. MVE = M-Profile Vector Extension (Helium). TCM = tightly-coupled memory.
NPU = neural processing unit (Ethos-U55). µs = microseconds.

## Why this exists

G4/F2 failed: the O2 and O3 scalar schedules missed 2,005 and 2,006 of 6,000 frozen
7,500 µs deadlines. But the reported tempo p50, p95 and p99 all read exactly 8,000 µs,
because the distribution saturates — `platform/ra8p1/fixture_app.cpp:88` declares
`Distribution<8000> total, tempo, ordinary;`. Only the means and maxima are trustworthy
(tempo mean 9,172 µs, ordinary mean 3,872 µs).

There is an untested hypothesis that the failure is **structural, not compute**: the
tempo update runs at `kNoveltyDecimation = 3` (`src/k1/core/audio/tempo_tracker.cpp:18`,
`kProductionNoveltyDecimation` in `audio_rate_config.h:27`), so it fires on one hop in
three and its honest budget is 22,500 µs, not 7,500 µs. Ordinary hops sit at 52% of
budget. If tempo p99 fits inside 22,500 µs, decoupling the tempo publish clears F2 with
no change to cache, TCM or FPU configuration.

That hypothesis cannot be tested against a saturated distribution. This brief measures.
**It does not fix anything.**

## Answer these five questions

**Q1 — True hop distribution.** Replace the saturating distribution with a record that
cannot saturate. Emit every hop's total cost as a raw value. Report min / p50 / p95 /
p99 / max separately for tempo hops and for ordinary hops. State the unit explicitly.

**Q2 — The miss set, labelled. This is the falsification test.** For each of the 6,000
hops emit: hop index, tempo-hop true/false, cost, deadline-missed true/false. Report the
cross-tabulation — tempo missed, tempo met, ordinary missed, ordinary met. If a material
number of *ordinary* hops miss, the structural hypothesis is dead. Say so plainly and do
not soften it.

**Q3 — Per-stage breakdown.** Time each stage separately inside the hop: `gdft_goertzel`,
`gdft_postprocess`, `onset_beat`, `musical_saliency`, `tempo_acf`, `tempo_tracker`,
`chord_detect`, `clock_affine`, `musical_time`/`media_time`, and the VP/pixel stage.
Report per-stage min / p50 / p95 / p99 / max, split by tempo vs ordinary hop. This is
what locates the ~5,300 µs tempo delta. **Do not assume it is `tempo_acf`** — measure it.

**Q4 — Double-precision emulation cost.** K1 sources are compiled `-mfpu=fpv5-sp-d16`
(single-precision FPU), so every `double` operation is software-emulated through
`__aeabi_d*` calls. Count those calls and measure their aggregate cost per hop, split
tempo vs ordinary. `AffineClockEstimator::refit` is the known concentration. If this is a
material share of the tempo delta then the fix is a type or compiler-flag change, not
scheduling — say so.

**Q5 — Observer overhead.** Report what the new instrumentation itself costs, measured
the way the existing observer-distortion repair did (commits `a79ff5e`, `1a93de6`). If
instrumented ordinary-hop cost has moved materially from the 3,872 µs baseline, the Q3
numbers are contaminated: say so and reduce granularity until they are not.

## Constraints

- Same frozen 6,000-hop input, same O3 profile, same identified Titan. **Share the
  fixture bytes; do not regenerate them on both sides.**
- Change nothing else. D-cache stays disabled, ITCM/DTCM stay empty, FPU flags unchanged,
  no MVE, no vectorisation. One variable at a time.
- Emit raw per-hop values to a receipt. Every summary figure must be recomputable from
  the raw data. Do not report a statistic the raw data cannot reproduce.
- Per `AGENTS.md` rule 12, the new instrumentation needs one negative proof that it can
  fail: inject a known delay on a known hop index and show it appears at the right index
  and the right magnitude.
- Receipts go to `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002`
  with sha256 hashes recorded in `external-receipts.json`, per existing lane convention.

## Out of scope — do not do these

Enabling D-cache. Populating TCM. Changing FPU flags. MVE/Helium. Restructuring the
publish or the schedule. Touching the DualMCU or Titan BSP repositories. Proposing an
architecture. Each of those is a separate experiment with its own single variable. If you
spot what looks like an easy win, write it down and leave it alone.

## Deliverable

Update `docs/evidence/K1-RA8P1-002/STATUS.md` with the measured result and a one-line
verdict on the structural hypothesis: **does tempo p99 fit inside 22,500 µs — yes or no?**
Plain English, no jargon lead. If the measurement contradicts the hypothesis, that is a
useful result — report it as clearly as a confirmation.
