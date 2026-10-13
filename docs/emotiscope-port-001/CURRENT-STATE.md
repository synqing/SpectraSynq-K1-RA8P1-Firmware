## 13 October 2026, 16:21 AWST — five-mode candidate rejected for Tempo timing; recovery restored

The full five-mode implementation is in source (`07ce1cd`, storage repair `14d31d3`, public readback repair `91cd16d`); this supersedes earlier statements that M14–M17 have not started. Host production `69eda75` exposes all five when the installed firmware advertises them. No physical qualification or completed migration is claimed.

Candidate family-003 (`4b39a908b76f6e75817136cd7cd1496cf1b3f5ab50aac5450dcb8ee987948310`, compiled source `14d31d3`) was installed and rejected. The matched Hybrid/Bloom passage had no new audio gaps or skipped releases. Selecting Tempo/Bloom produced 413 additional sampled audio gaps and 65 additional skipped releases during its passage; in-house analysis maximum rose from 1,101,992 to 6,448,928 cycles. Preserve `_scratch/waveform-migration-001/family-physical-tempo-002/` including the failed physical recording. The proposed repair is exact arithmetic-preserving reuse in the existing autocorrelation calculation, not reduced effect scope or changed musical cadence.

Recovery image `95499e22d9bc761171535a33c51c7e06c1bd93b87795e6a9eaeffbe70823a9e4` (source `d33945a3f3df6b20064909a890046c604f373f92`, schema `ce1b968f85c4f6a4ef0f1e56db5564c31c3b9c2feab0bde2c5cc13a817940787`) was restored through E2 Lite, target power OFF / 1 MHz. UID readback MATCH: `545433931bd25436593630352d068363`. Programming interrupted normal output; the application and both-edge settings were then restored. Evidence: `_scratch/waveform-migration-001/rollback-family-003/` and `rollback-family-003-restored.json`.

**Current operating state:** bottom mode32 / palette32; top mode6 / scene3 / palette41; master255, both gains0.25, emission ON and secondary enabled. These are the latest captured operator settings, superseding the earlier palette1/topmode1 entries. SQ4 unchanged. Health confirms advancing frames (~100 Hz), no LED faults, accepted/rendered controls; the deliberately disconnected SMC-Mixer remains absent, while Surface controls work. Camera was reconnected with both plate ends in frame. Further five-effect deployment requires correcting the observed timing failure; all original M00–M24 assignments and failures remain below.

## 13 October 2026 — operator correction: lower Hybrid active

Captain rejected the unchanged show after root restored both Bloom renderers. Root has corrected the selection through one existing live control: bottom mode 32 (Waveform Hybrid K1), top mode 1 / scene 3 detail Bloom. All other authored fields and actual output settings are unchanged: master 255, gains 0.25, both outputs enabled. Firmware remains d33945a / build 95499e22… / UID 545433931bd25436593630352d068363. No reflash or SQ4 overwrite.

Fresh before/after readback: [active correction](evidence/waveform-migration-001/hybrid-009-operator-correction-active.json). Root inspected an actual short plate recording, retained with two wire/MIR samples under `_scratch/waveform-migration-001/operator-correction-physical-001/`. Hybrid remains selected. This supersedes the preceding Bloom leave-state; visual/music qualification and the other four Waveform ports remain unfinished.

## 13 October 2026 — Hybrid 009 installed; working two-board Bloom restored

Firmware source `d33945a3f3df6b20064909a890046c604f373f92`, build `95499e22d9bc761171535a33c51c7e06c1bd93b87795e6a9eaeffbe70823a9e4`, schema `ce1b968f85c4f6a4ef0f1e56db5564c31c3b9c2feab0bde2c5cc13a817940787`, UID `545433931bd25436593630352d068363`. Verified programme/readback on existing E2 Lite 1 MHz route with target power OFF. Candidate 009 reuses the existing Titan acoustic admission independently per Hybrid renderer, retaining source audio numerics. Source/FIR throughput repairs and host boundary checks are in the full migration record.

Current running composition is the restored two-board continuous Bloom: mode 1 / scene 3 / phrase 0 both, palette 1, master 255, board gains 0.25, emission ON / secondary ON, body return 0.25 unsaved trial/detail return 0.55. Full readback: [leave-state](evidence/waveform-migration-001/hybrid-009-leave-state.json). Saved SQ4 and boot defaults were not replaced. Hybrid 32 remains selectable, not physically accepted.

Latest comparison: `_scratch/waveform-migration-001/weak-009/original.mov` and `root-visual-inspection.json`; same approved audio at 0.25 afplay gain, one locked 100-second session. Both passages complete, no observed audio gaps or ASRC rejects. Root inspected actual plate stills: musical colour separation and sampled dark recovery visible; intermittent later light persists. Its acoustic cause is unproved. Host weak wake/independence passes are not broad physical qualification.

**Full migration incomplete. M13 complete playback-speed review with music is BLOCKED by the current inspection tools; M14–M17 have not started.** Do not silently advance effects or replace that operation with stills. Exact continuation and all original M00–M24 tasks remain in [the migration record](INHOUSE-WAVEFORM-MIGRATION-20261012.md). Both autonomous optical-edge outputs remain enabled. Historical entries below retain their original state and are superseded only by this dated checkpoint.

## 13 October 2026 — candidate 008 installed; quiet-input assay in progress

THE TITAN MUST CONTINUE AUTONOMOUS MUSIC. Root is the sole live owner.
This dated checkpoint supersedes the installed-state descriptions below; all
older failures and recovery records remain historical evidence.

Installed source `2f83736e78805cc7333499302835edabb17997a3`, build
`924aee6c55d2476bb2a164e3faf669d34cedbab90b87c381ef823ce164d19bc8`, UID
`545433931bd25436593630352d068363`, schema
`ce1b968f85c4f6a4ef0f1e56db5564c31c3b9c2feab0bde2c5cc13a817940787`.
Identity evidence: `_scratch/waveform-migration-001/hybrid-008-admission.json`
and `programme-hybrid-008/readback.json`. Root is actively running the bounded
`weak-008c` assay; this record does **not** declare final settings or leave-state.

The exact two-span 95-tap FIR change `451dd056` resolved the demonstrated runtime
capture pressure in the complete matched-007/008 passages: approximately 100 Hz
rendering/200 Hz audio publication, with no observed new skipped releases, audio
gaps, ASRC rejections or raw overflows. Diagnostic USB responses are still slower
in the mixed pair; throughput equivalence is not claimed. Candidate 007's quiet
consumer defect was subsequently corrected in 008: confirmed silence now stops
new Hybrid deposits while history drains. Focused host checks cover that consumer.

**Quiet recovery remains unproved.** All 81 active 008 in-house observations have
`silence=false`; post-playback RMS spikes above the existing 0.08 threshold
interrupt the five-second quiet dwell. The source of that energy is unknown.
Inactive-before silence flags are not quiet evidence. The 0.08 wake limitation
and weak-input assay remain open. Reuse of existing Bloom admission is under
local development, not installed or accepted.

Complete matched-008 recording exists with unchanged output controls, but
playback-speed audiovisual review remains BLOCKED by available tools. Video-only
recordings/stills do not qualify musical motion or preference. M13/M18 and the
migration remain incomplete; no next mode has been accepted. See the complete
[canonical migration record](INHOUSE-WAVEFORM-MIGRATION-20261012.md) and
`matched-007/runtime-evidence-analysis.json` /
`matched-008/runtime-evidence-analysis.json` under
`_scratch/waveform-migration-001/`. Candidates 002–006 and the retained
`037dd22f…` recovery image remain preserved. Root completes the active assay and
its scoped correction before recording a final operating state.

---

## 13 October 2026 — Hybrid scheduler candidate006 rejected and rolled back

Candidate `f8d2c002` / build `ce969aa81e2b2855cee970038758d87ffcd47063c3fddb01a530a695abf724ec` added a bounded yield when LED transmission ends during an audio hop. It passed production-loop scheduling checks and all 211 compiled inputs matched the commit, but target acceptance failed: two successive MIR samples show audio gaps 1 → 2, stream epoch 2 → 3 and invalid/warming donor publication. Cached PDM telemetry independently records one ASRC queue rejection, 1,030 discarded samples and a 14,395 µs maximum PDM service gap; it predates the second MIR gap. No new physical comparison or visual pass is claimed.

Root restored the known working `037dd22f…` firmware through the established E2 Lite route, then restored the complete entry composition on both boards: scene3/phrase0, master255, gains0.25/0.25, palette1, bodyreturn0.25 unsaved trial and detailreturn0.55. Saved SQ4 remains unchanged. Evidence: `_scratch/waveform-migration-001/hybrid-006-bounded/`, `programme-hybrid-006/`, `rollback-hybrid-006/`, and `rollback-hybrid-006-active.json`. The six-line scheduler correction was reverted in `6e07b19`. Current investigation is the existing raw-capture-to-ASRC drain boundary; no subsequent flash is justified until a specific mechanism is established. M13 and full migration remain incomplete.

# Titan Emotiscope port — current execution record

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.

## Waveform migration execution — 13 October 2026, 01:16 AWST

The approved complete M00–M24 plan is preserved in
[the migration record](INHOUSE-WAVEFORM-MIGRATION-20261012.md). Hybrid K1 is
implemented in source with independent native 16-bit histories and source-bound
feature conditioning; it is **not physically accepted**. Later effects have not
started because the plan requires this first physical checkpoint.

Candidates 002, 003 and 004 were rejected for real target timing/capture failures
and explicitly rolled back. Candidate 004 (`1bb0cb1d5b25…`, source `5975256`)
measured approximately 1.4 ms donor work per hop, 2.1 ms additional Hybrid
analysis, and 1.15 ms render work. ASRC buffer overruns repeatedly reset the
analyser; Hybrid produced no valid deposits. Its successful build and 100 Hz
frame counter do not establish a working show. Evidence:
`_scratch/waveform-migration-001/hybrid-004-bounded/`.

**Current installed state:** known working `037dd22f…`, both scene 3, phrase 0,
palette 1, emission and secondary ON, master 255, board gains .25/.25,
body return .25 and detail return .55. Full restored settings:
`_scratch/waveform-migration-001/rollback-hybrid-004-active.json`.
SQ4 remains untouched. Current source correction work targets measured analysis
cost; no ESP32-S3 hardware, new effect, wiring or output-limit changes.

Host corrections are committed and loaded: decoder v7 version normalisation
`727b07b`, complete mixed-to-donor composition restoration `41853d0`, and guarded
MIR diagnostic admission `0f30b2b`. The latter enabled the actual processing-cost
observation. The camera reframe includes both ends; no Hybrid matched music
comparison has passed. Complete playback-speed audiovisual inspection also
remains unperformed; extracted stills do not satisfy that requirement.

### Subsequent check — 13 October 2026, 01:26 AWST

Candidate 005, source `b3ca773`, build
`2f2e0432a88ae0ac97c81549aa5351fead041ef2a3e946a83b36ccae09488cfd`,
used the exact scalar recurrence. Actual Hybrid analysis fell to approximately
1.0 ms; the initial eight-second mixed-pair check retained valid audio, new
injections, no new frame misses and no capture gaps. The subsequent single-camera
comparison **failed**: baseline passage completed, candidate passage stopped
about six seconds into music after a MIR USB timeout. The resulting 57.27-second
clip is retained under `_scratch/waveform-migration-001/matched-005/`; it is an
incomplete comparison, not a passed musical assessment. Two ASRC rejections were
observed. Root inspected actual before/candidate stills; candidate lower end
hotspots remain to be assessed at matched cue points. No playback-speed audiovisual
acceptance is claimed.

USB responses were already much slower during the mixed pair's pre-music interval.
The broker's normal restart restored control; the candidate was nevertheless
rejected and baseline `037dd22f…` explicitly reinstalled. Full entry settings
were restored again: `_scratch/waveform-migration-001/rollback-hybrid-005-active.json`.
A source-bound transmission-to-idle yield and a separate timeout-quarantine
repair are in progress. They are not deployed or proved by this record.

## Historical migration assessment entry — 12 October 2026, late evening

Captain redirected further Bloom tuning to a source-grounded migration assessment
of the in-house Waveform family, including Waveform Tempo and Hybrid K1.
[Assessment, recovered reference and exact gaps](INHOUSE-WAVEFORM-MIGRATION-20261012.md).
Original assignments and evidence below remain historical; this does not close
the physical Bloom acceptance criteria.

Current installed image is `037dd22f0450f2286bce8f833722e5d410ea83fe658add7d29d91d2e97fa5272`,
from `_scratch/continuous-bloom-r4/arm-final`, on the authorised Titan UID.
It includes the admission/pre-conditioning noise repair and bounded trace fields.
Read-only health confirmed emission ON, roughly 100 frames/s and zero LED faults.
Master remains 255/255, both gains .25 and secondary output enabled.
Body return strength .25 is an unsaved trial; SQ4/canonical preset still retain
.55. Musical release is restored to 160 ms after rejection of the 60 ms trial.
The last physical comparison is inconclusive for quiet recovery because received
input remained high after playback. No new aesthetic acceptance is claimed.

The source remains dirty with the scoped R4 repairs; `arm-final2` completed but
was not deployed. Do not mistake it for the running image. Live readback is
`_scratch/continuous-bloom-r4/migration-assessment-entry-status.json`; complete
prior evidence remains under `_scratch/continuous-bloom-r4/`. Further tuning and
flashing stopped when Captain changed direction; autonomous output was preserved.

## Previous physical delivery — 12 October 2026, evening

Installed candidate `4c94653ddc3d...` / firmware source `7ae880e` is running.
Captain cleared the competing-control conflict. A clean body-spacing comparison
retained width 0.08/0.08 and trail 40 ms on body only; detail remains 0.20/0.14,
trail 120 ms. Complete two-board SQ4 save/edit/recall passes. Master 255/255,
both gains 0.25, secondary and emission ON; broker/mixer healthy, no LED faults.

The final same-session physical baseline/candidate recording includes current
render timing (maximum sampled 1.768 ms against the 10 ms period, no added
publication misses or skipped releases). A second causal probe was also inspected.
Overall acceptance remains incomplete: faint new material after digital silence
coincides with nonzero microphone input; an actually below-floor physical quiet
interval is not qualified. Aesthetic preference is PENDING Captain's judgement.

[Final clips, identity/settings, fixed criteria and exact remaining closure](CONTINUOUS-BLOOM-PHYSICAL-20261012.md).

## Retained deployed correction: continuous body/detail Bloom — 12 October 2026

Resident build `4c94653ddc3d9d2a6a56500e1422d953a48c0be50b0486d7deaec55f6e3a43e4`,
source `7ae880e`, same UID and schema. Body injection now receives one selected
attack/release stage, removing the duplicated smoothing that softened short hits.
The detail production output remains byte-identical under the retained input.
No brightness, palette, routing or transport changes.

SQ4 is running on both boards: continuous unlinked Bloom, body/detail strength 1,
ML influence OFF in this comparison, master 100/255, board gains 0.25, secondary
enabled, emission ON. Preset and physical controls were explicitly restored and
read back after programming. The approved Sonic Samba passage ran on-device.
Final identity and fault readback are retained; **physical quality is UNVERIFIED**.

[Current correction, evidence and remaining physical comparison](CONTINUOUS-BLOOM-R2.md).
The EiP product camera remains absent. Normal music output and completed source
repairs do not depend on camera availability. The existing physical comparison
criterion is still open; no replacement acceptance claim was made.

## Historical image: authored two-board scenes — 11 October 2026 (superseded)

Resident build `47bc25b5f69c9e27c90dfe089343412569825beef07bed1a4f94e47718ef28d4`,
compiled from `eda9bf4` (all207 receipt inputs match). Host Surface source `0aa05f4`.
Schema `6d68932e22c0796bcac290a233df9e235410b7af724ac71112525a86017b561b`, v6,
capabilities127; target UID `545433931bd25436593630352d068363`. Programme used the
existing sealed E2Lite path, probe powerOFF,1MHz, bounded code area, with identity
MATCH after reset. Final programming ran21:42:35–21:42:59AWST; identity readback21:43:09.
Earlier integration image was programmed21:32:29–21:32:54; both interruptions were
followed by autonomous output. No physical continuity claim during reset.

Current confirmed composition: Travelling Dual Ribbons on both boards, no pending
scene, hardware brightness100, primary/secondary gains0.25, secondary enabled,
emissionON, reference precision0. Around100frames/s and zero reported LED faults.
Existing Surface is running new code; fresh-process recall confirmed complete Scene1.
Scene1=Travelling Dual Ribbons, Scene2=Vertical Call and Answer,
Scene3=Tension/Release Bloom, saved in the existing host preset store. These are host
presets, not device nonvolatile scene storage. Draft comparison is clean after readback.

All three compositions exercised on device using the existing authorised24s Sonic
Samba source window (owned playback bounded and ended). Ribbons and call/answer
reported real travel/quiet/return/recovery phases and changing gesture IDs. Bloom
reported phrase states Establish,Develop,Tension,Release,Recover. Save, locked palette,
scoped variation, whole-draft undo/redo and recall were exercised through authenticated
Surface APIs. Fresh service restart retained all three slots and restored Scene1.

Host checks:210SMC tests,287broker tests, original live-protocol/controls/output gates,
new ASan/UBSan motion/pair/timing/actual donor RGB tests and ARM build. A mixed legacy
background quiet regression failed before correction and passed afterwards. Per-frame
2ms target render ceiling has NOT been measured;100fps telemetry does not prove it.
Actual donor RGB rasters were opened and inspected; return strength was repaired from
a visibly weak initial implementation without raising physical output gains.

Optical acceptance remains BLOCKED by camera evidence: supplied EiP camera connected
but delivered nearly black video (first decoded frame max3/255, mean0.0126). This cannot
prove light movement through the plate. No alternate camera or guessed view was used.
Required remaining path:1.obtain usable product imagery through the existing camera;
2.compare all three scenes including handoff/quiet/return on the plate;3.measure target
render cost and admit the result through the applicable assignment governance.

Evidence: [implementation](COMPOSED-SCENES.md),
[final readback](evidence/composed-scenes-20261011/final-surface-readback.json),
[ARM receipt](evidence/composed-scenes-20261011/arm-receipt.json),
[programme receipt](evidence/composed-scenes-20261011/programme-receipt.json).
Raw scene observations and black camera originals: `_scratch/composed-scenes-live`.
Native workflow verification rejected the donor project for the PRISM launch session;
workflow_gate reports no bound ledger and explicitly no completion certificate.
No task registry or acceptance record was invented. No push performed.

## Historical image: palette is the only colour source — 1 October 2026 (superseded)


Resident since 17:09 AWST: build `becb0186f0a66fbb04b0e0f4a79c0480b8b963db0fe3f518a91af9bb2dd2b5c7`, HEX `8ecc9fba931ee2178cccf643efef8d132e144e764494614eeaf999f50e53c045`, ELF `97923cbb8ff0b7ea80548e775b68765475da87b583d5b08a2683016e62a2b6e3`, schema unchanged `e40f6598…`, same UID, pins, DMA and 80+80 split. Build `evidence/arm-palette-003`, programme `evidence/programme-palette-003`, admission `evidence/smc-palette-003/accepted-candidate.json` (broker LaunchAgent points here).

Captain: "Palette colours NEED to be the primary colour producer." Audit of every control against palettes 1–44 found four leaks in the previous image: Colour offset the palette (seam where the palette end meets its start, and bass no longer at the palette start); Background added native Emotiscope HSV colours, unsquared since the gamma skip, so a 0.25 setting laid a ~6% rainbow wash over the palette; the screensaver drew fixed red/orange/green/blue dots; per-channel peak soft-clip shifted bright hues; Hype's second dot read Colour range.

Now, palettes 1–44: palette position = strip position (bass = palette start), Reverse flips it, auto colour scrolls it (K1 auto shift); Colour, Colour range and Saturation inert. Background = the same palette at (0.25·b)², the donor's light level at the same setting. Screensaver dots at palette positions 1/8, 3/8, 5/8, 7/8. Peak soft-clip scales whole pixels. Hype's colour-range term pinned to half the palette during the mode draw only (same render thread; restored before background). Warmth (incandescent filter, default 0) still tints palettes when raised; brightness, softness, speed, mirror, dither, output and precision do not change colour. Palette 0 unchanged.

Checks: `k1palette` 4,032 pixels plus background, peak, screensaver and Hype checks against the independent K1 oracle; 8 planted palette faults each rejected; palette 0 still byte-identical to the reference; `check_instrument.sh` PASS; receipt 194/194 match HEAD `93e4385`. Board: identity MATCH, 100 frames/s, 0 LED faults, palette 31 accepted and rendered. No LED-frame capture. Show stopped about 28 s (17:08:47–17:09:16).

## Previous image: palettes drawn the way K1 drew them — 1 October 2026 — superseded above

Resident since 16:48 AWST: build `05d016a7f1232ab389379d9c1bd9a1344e9ee5e292a569b8fca9affebe3b260c`, HEX `349916e714cea1806fd78d3a9b8a87f6b35214f80b239d944027622c50f816ab`, ELF `dadafbee814ea8b544326ab6c4985333162a4e89b5713870551f602459a475dc`, schema unchanged `e40f6598…`, same UID, pins, DMA (LED 0/3, mic 1/2) and 80+80 split. Build `evidence/arm-palette-002`, programme `evidence/programme-palette-002`, admission `evidence/smc-palette-002/accepted-candidate.json` (broker LaunchAgent points here).

Why: the palette data was byte-identical to K1 (44/44 stop tables), but the Titan drew it differently: the donor's final x² gamma (K1 had `ENABLE_OUTPUT_GAMMA 0` since 20 May, "crushes midtones"), only a Colour-range slice of the palette, and a saturation grey blend. Modelled on the Captain's settings this halved the visible colours (9.9 → 4.8 per strip) and made half-loud passages 3.2× darker. Comparison page: https://claude.ai/artifact/X31AHA1nkSFMeUxt6qDaVG.

Now, for palettes 1–44 only: palette position = Colour + strip position (reverse mirrors it), level = 0.65v²+0.35v (K1 SQUARE_ITER 1), no saturation blend, final gamma skipped. Palette 0 is the donor unchanged (precision-0 output still byte-identical to the reference). The leds.h background keeps the donor hue slice (background defaults to 0). Host: Colour range and Saturation show "No effect with a palette on".

Checks: `k1palette` host case, 2,016 pixels against an independent K1 oracle, worst error 1.2e-7; planted faults (gamma kept, colour range used, linear level) each rejected; `scripts/check_instrument.sh` PASS; receipt sources 194/194 match HEAD `07f5bc2`. On the board: identity MATCH, 100 frames/s, 0 LED faults; palette 31 PATCH accepted and rendered. No LED-frame capture was taken (the wire snapshot needs the show stopped), so on-board colour is not separately measured. The show stopped about 25 s during programming (16:47:37–16:48:05).

## Previous image: K1 palettes restored — 30 September 2026 — superseded above

Resident: build `5cbd964d1494bc9480333206b367006326e8e3fad44df6a86f81a0dd3985cbef`, HEX `3e89dd282aff0d0c2b464299f756043175beb3a76ef7907bb5a87bf5154a2813`, ELF `5004027784351cffcaefcf2b64b4b5fbb84c57c8803badd142bf10e893f833ed`, schema `e40f6598e57c9493de6f3e6d19935d404d9f5cdbb35c064770fe365ad476a0bb`, same UID, pins, DMA and 80+80 split. Build `evidence/arm-palette-001`, programme `evidence/programme-palette-001`, admission `evidence/smc-palette-001/accepted-candidate.json` (sha `48bd7894…`, used by the broker LaunchAgent).

Why: the Emotiscope port had dropped the K1 44-palette catalogue, so every mode could only use the donor's plain HSV hues. The donor `hsv()` call is now routed through `sampleProductPaletteHd` when a palette is selected. New PATCH field 18 `palette` (0–44, GET state offset 140): 0 = native Emotiscope colour (byte-identical to before), 1–44 = the K1 product palettes. Colour range spreads how much of the palette shows; saturation still blends toward grey. Boot palette 0. Round trip 0→1→0 accepted and rendered on the board. Host: Knob 2 sweeps all 44, Up/Down step one, the surface page shows the palette name. Output check `palette25-changes-wire`: 140,890 of 960,000 bytes differ from palette 0.

The show stopped for about 25 s during programming and resumed on application start.

## Previous image, supervision and A/B — 30 September 2026 (Dependable Instrument v1) — superseded above

Resident since 06:57 AWST: build `c23e1b61f1de37fdc5179548ef2aa350921be23bb9728ab05d3644f6e08acb74`, HEX `5337d52473a599eb42faa3acbf237dcdce1b1756b8d814c1d572a5fecdb10448`, ELF `89582d0013d8da2ec064cf8bbf61aac1b47fce7f02782c565213ca1411bfaf20`, schema `407d24207fa2a7f14c9ccf34ed601bc94bab1e873fc47be3e83850c1c1924688`, UID `545433931bd25436593630352d068363`. Boots Spectrum, 100/255, emission on, `output_precision` 0. Same P601/GPT6/DMA0, P604/GPT8/DMA3, mic DMA1/2 and 80+80 split. Build `evidence/arm-precision-001`, programme `evidence/programme-precision-001`, admission `evidence/smc-precision-001/accepted-candidate.json` (sha `3d494442…`).

`output_precision` (PATCH field 17, GET flag bit 6): 0 = reference RGB8 dithered ×257 (unchanged; byte-identical to `titan-emotiscope-ref-r1` over the 1000-frame fixture), 1 = the same post-gamma floats rounded to 16 bits. Owner-applied at a frame boundary; never in scenes/defaults; reboot returns 0. Round trip 0→1→0 verified on the board (`evidence/instrument-v1-precision-01`). No A/B verdict yet.

Broker and mixer controller run under launchd (`com.spectrasynq.titan-broker`, `com.spectrasynq.titan-mixer`); commands are on the operator card. The broker exits when Titan's USB node vanishes so launchd re-attaches it. Crash (SIGKILL) and bootout/bootstrap tests passed with frames advancing throughout (`evidence/instrument-v1-launchd-02`). One honest check: `scripts/check_instrument.sh`.

Fault observed 06:17–06:54: the USB branch shared by Titan, keypad, AV adapter, ESP32 and MD202 dropped twice; Titan lost power once (rebooted 06:18) and its USB then stayed absent until an SWD identity read restarted the application. Cause not established (bench cabling or hub). Evidence `evidence/instrument-v1-launchd-01`, `evidence/instrument-v1-titan-usb-01`.

## Previous controls image and host — superseded 30 September 2026

Updated 29 September 2026 from the running broker/controller, not from Git HEAD.

Resident build `393527e4ff81eb5c4fe54e8be3bf874bc37dce9fd89a2d9fec3d4b3e7ac8c4cb`; HEX SHA-256 `73ad9be5401110a06f1e7f728bf592c34c4e6170d147b1c4faaaf7831cf30071`; ELF SHA-256 `ed997e83fee4da3cde1f23fe6acdc49e41e184cd01e71f68e274d9ba3790849f`; schema `5f3c9fa627067bf3357bdb142b4d4efcb8d16211164431864826af9102286f60`. UID `545433931bd25436593630352d068363`. Spectrum, output100/255 and emissionON were verified at the baseline/restoration. Subsequent live Knob1 input set output to255/255; that operator choice was preserved. Normal default100 is unchanged. The earlier port004 image below is preserved history, not current firmware.

Current control interface: typed Emotiscope GET/PATCH under opcode25. Legacy opcode16 mutation and the old performance config are rejected. **Do not use the historical eight-word opcode16 instructions below on this image.** Continuous160/80+80 mapping, P601/P604, DMA allocation and the donor AP/VP are unchanged.

The controller is running on the existing admitted SMC-Mixer Bluetooth endpoint. Dashboard8776, status8767, existing sole broker socket `/tmp/titan-emotiscope-control/titan-control.sock`. No second flash occurred during Captain's host repair. A real synthetic softness change and exact restoration reached accepted/rendered/completed target state. This is not a human gesture or optical result.

155 complete-collection host tests passed in the documented existing-dependency environment; 39 focused checks passed in the production virtualenv. Full physical control qualification is still open. Fader1/Fader4 remain gated. The first 20-second host window had no input; subsequent real mixer CC traffic and advancing accepted/rendered/completed state are retained in the repair evidence. Capture-finalisation hardening remains outstanding. Details, failing historical logs and exact runtime invocation: `evidence/captain-host-repair-20260929-01/CLOSEOUT.md`. Operator card: SMC worktree `host/titan_control/EMOTISCOPE-OPERATOR-20260929.md`.

## Scope and authority

The user's 29 September 2026 pivot parks the bespoke Titan AP/VP and ports the known Emotiscope.HIL audio and visual engines. The original effects worktree, rejected appearances, original contracts, Aurora A image and unprogrammed Aurora B image remain preserved. This execution does not close the old AP/VP/CTL/IVV contracts or claim operator appearance acceptance.

Implementation worktree: /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001
Branch: feat/titan-emotiscope-port-001.
Parent commit: 4ced4d1d0001ba22788759eb3318a73179f94d81.

## Historical initial port image — superseded above

Final programmed build: 72773a7461590edbdb53080670f6c61608fb973c889f7bdcc1b9a65da4b8319b.
HEX SHA256: 28acf53a54259ac6f8e8d0a2ef3e6fda7aa5d513859310f757de3ba2edd815eb.
ELF SHA256: d1d446c6f4db432a0b1e4cd53f7df8fb9b437e2057ec663d2ed9be91b4693fb7.
Donor: Emotiscope.HIL/v1.1_build at 7f86939bd7c16c4a6dd5789813e9d551246ed134.
Schema: c7b67f1eecbe836721c97331eef895813857191e273999c96b1f3fbf41bce39a.

Boot is autonomous emission ON, donor Spectrum mode 1, hardware brightness 100/255. The native donor default colour is retained. The donor's startup fade is retained; it reaches full master gain after approximately eleven seconds at the explicit 100 Hz port cadence.

All seven active donor modes are present: Analog, Spectrum, Octave, Metronome, Spectronome, Hype and Bloom. Network, ESP32 RMT, touch hardware and NVS services are replaced or excluded. This is a complete donor musical AP/VP port, not an ESP32 peripheral emulation.

## Active path

Titan PDM capture and original per-block microphone loud guard -> 95-tap antialias filter -> measured-source-rate conversion directly to 12800 Hz, 64 samples -> original donor Goertzel, chroma, VU, tempo -> complete donor effect and colour/output processing -> donor gamma and temporal RGB8 dithering -> nearest original pixel mapping from 128 to 160 -> RGB8 expansion for GRB48 -> exactly one hardware brightness gain.

One continuous 160-pixel scene is split at logical 79/80. DIN-A carries 0–79 on P601/GPT6/DMAC0; DIN-B carries 80–159 on P604/GPT8/DMAC3. The microphone retains DMA1/2. No second strip or independent second-edge scene is claimed. This is RGB8 donor output carried by a 16-bit wire format, not authored TRUE16.

Compiled call-site checks confirm that LiveAudioRuntime calls donor AP and PaletteRuntime calls donor VP; those owners do not call legacy AP or product rendering. Existing compatibility source is still staged where required by the fixture framework.

## Declared adaptations

- Native 128-pixel donor geometry remains inside the engine. Spatial expansion occurs only after its final RGB8 output.
- Explicit 100 Hz rendering is a port choice, not a recovered historical S3 frame rate.
- Original tempo full-window index -1 was repaired in two expressions. The pristine original and restored-defect mutation both fail AddressSanitizer.
- Novelty deadline comparison is wrap-safe. Extended milliseconds preserve time across microsecond rollover; backward execution timestamps are rejected. AP and VP share the same monotonic target clock.
- A capture discontinuity clears only the PCM capture history, invalidates fresh audio and rewarms 64 chunks/320 ms. Visual, colour, phase, configuration and dither histories remain. This preserves prior material; it does not reconstruct missing audio.
- ARM platform shims preserve scalar DSP operations and define M_PI. No invented pitch estimator or K1 AP feature substitution drives the donor.
- Donor chroma is preserved in its actual D#/Eb-origin ordering. Unsupported legacy beat, onset, saliency and tempo-field claims stay unset.

## Verification

Source-bound host evidence:
- out/emotiscope-ap-parity-002/result.json: 34000 AP hops, two 75-second trance fixtures plus synthetic input; 193505382 scalar AP comparisons agree exactly.
- out/emotiscope-visual-verification-003/result.json: all active modes, independent Spectrum output-stage and dither oracle, gap preservation and clock rollover. Five deliberately broken variants rejected.
- out/emotiscope-output-001/result.json: all 960000 wire bytes match an independent donor/output calculation over 1000 frames. Asymmetric markers cover all source pixels, endpoints, centre pair and 80+80 split. Wrong split, duplicate gain and second gamma variants rejected.
- evidence/live-bridge-host-002/receipt.json: latest wrapper, contract checks, 64-hop warmup, 5000 us deadline, 240 media ticks/hop and recovery. Three broken bridge variants rejected.
- evidence/capture-adapter-host-01.json: direct 12.8 kHz adapter, sample accounting, sanitizer and filter checks.
- evidence/legacy-asrc-byte-parity-01.json: original 24 kHz profile preserved over 2368000 samples.

ARM 001 retains its missing-M_PI compile failure. ARM 002 is superseded compile evidence with legacy schema. ARM 003 was programmed and passed initial live and Sonic observations. ARM 004 changes only the two diagnostic descriptions for capture rate/profile and physical backend pins. Its active donor and schema are identical to 003.

Exact-image build: evidence/arm-port-004/receipt.json.
Exact-image programme: evidence/programme-port-004/receipt.json.
Target observations: evidence/target-sonic-004/result.json and evidence/target-carte-004/result.json.
The final measured aggregate is in evidence/target-summary-004.json.

Programming used E2 Lite 5AS079228B, SWD 1 MHz, probe power OFF, -noquery -run, and the existing permitted range ending before configuration memory. The application returned as UID 545433931bd25436593630352d068363 and the exact build above.

## Historical pre-controls interface — not valid for the current image

For this profile opcode 16 version 1 selects donor mode 0–6 using the existing queued owner mechanism. Send eight little-endian uint32 words: version=1, palette_a=0, palette_b=0, mode_a=mode, mode_b=mode, flags=5, brightness=100, output_channel=0. Preserve emission. Explicit operator dimming remains possible; the normal boot default stays at least 100.

Legacy opcode 25 performance/config controls are deliberately rejected because they describe the parked engine. The new schema states this. A K1 slider/control integration is not claimed. Use one CDC owner and the existing lease; release the serial handle and its flock when finished.

The implementation and physical deployment phase is complete when the exact image is identified and the bounded tests recorded. Visual acceptance remains the operator's decision on the actual plate. No software counter is optical proof. Long-duration unattended continuity, absolute acoustic timing and full calibration are not established by these short observations.

The old source and rollback identities are under evidence/parked-20260929. Restore them only for a concrete recovery need or a subsequent operator instruction; do not overwrite or reinterpret their historical receipts.
