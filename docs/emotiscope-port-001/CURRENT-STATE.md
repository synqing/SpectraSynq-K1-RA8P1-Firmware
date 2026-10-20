## 14 October 2026, 08:05 UTC — UNHOBBLED product image INSTALLED (lag-free lane P2)

**Latest verified restart — supersedes the P1 entry below for the installed image.** Installed build
`a8215f75df60855a8505aa460e652e8d205ad8d3933fcc4b6a2885e758d3ac74` from lane commit `89bb01b` with build
defaults: `titan-product` profile (`-march=armv8.1-m.main+mve.fp+fp.dp -mfpu=auto -mfloat-abi=hard
-ffp-contract=fast -fno-fast-math -O3 -funroll-loops`, vectorisers on), D-cache ON (FSP write-through + DMA
maintenance; firmware metric `dcache_enabled: true`), boot v8 unchanged (readback 127 fields, 0 diff).
Admission `_scratch/waveform-migration-001/admission-p2-final-a8215f75.json`. Measured on BOOM 0–75 s, every hop:
capture poll 2.23 M → 0.162 M cycles p50 (max 4.48 M → 0.171 M), analysis 4.5×, render 4.8×, pack 6× faster,
zero faults/losses/skips. `build_scalar.py` now defaults to this profile; `--profile resident-scalar` reproduces
historical images. Known: an ITCM code-placement experiment image did not boot (≈4–5 min dark, recovered);
the host per-pixel tolerance gate failed and was shown non-discriminating. Receipt:
`_scratch/waveform-migration-001/runs/P2-20261014T071112Z/permitted/RECEIPT.md`.

## 14 October 2026, 06:42 UTC — boot composition v8 INSTALLED (lag-free lane P1)

**Latest verified restart — supersedes the entry below.** Captain's 14 Oct ruling (Kx-Titan
`docs/migration/TITAN-CANON-2026-10-13.md` §5) approved boot composition v8; no M23 selection is pending.
Installed build `54d9a439e7c4ca598d2bf289f9552764e5b1cc7d87c47e7a0316466d98772cd5` (commit 09ae387 + embedded
profile `presets/UNSELECTED-current-hybrid-tempo-crimson-v8-20261014.json`, sha256 `a166a3fb…`; the file
name is historical) on UID 545433931bd25436593630352d068363 via the established deploy route; broker admission
`_scratch/waveform-migration-001/admission-boot-v8-54d9a439.json`. After a debugger reset (E2 Lite
signature + run, no write) the Titan restarts by itself into lower Hybrid K1 32/p32 + upper Tempo 18/Crimson
Gold 39, both outputs ON, master 255, gains 0.25 — read back with zero differences and revision 0 after boots 4,
5 and 7 (boot 6: light only, see defect), and observed lit on both boards with music while no broker/host
existed. Host-disconnect (≈19 s, during music) passed. P1 status PARTIAL: power-on (unplug) cold start is
untested and needs one physical unplug/replug (or moving the Titan's USB to a verified switchable port).
New open defect: after 2 of 15 E2 Lite resets the USB CDC serial endpoint did not enumerate (show unaffected;
one more signature-and-run recovered it). Receipt: `_scratch/waveform-migration-001/runs/P1-20261014T061642Z/permitted/RECEIPT.md`.
Lane branch `lane/titan-lagfree-20261014`; lane status `_scratch/waveform-migration-001/runs/STATUS.md`.
Musical verdict: unassessed (agent cannot listen).

## 14 October 2026 — Captain's agreed effects restored; direct execution

**Latest verified restart — 14 October 2026:** direct Codex execution; commit09ae38717461b10c221928c0405b74e8182feadb, installed queue-repair010 build9ab90873fa37c4fadb5afca1f02699e603af082635eb4084baf5979f7c8660d7 on UID545433931bd25436593630352d068363. Lower Hybrid32/p32 + upper Tempo18/Crimson Gold39, both ON, master255/gains0.25; all63 saved controls per board retained. BOOM, weak Sonic and frozen Dopex captures completed with zero input loss/new output faults. Full-speed musical verdict unassessed. Full current boot profile compiled but UNSELECTED/UNINSTALLED; M23 Captain selection pending. Read the additive14October sections at the end before any action. Older restart points below are historical; retain their full assignments/evidence. Do not restart ports/workers or flash an unselected boot candidate.

Captain corrected the root's mistaken treatment of another team's leftover
Spectrum settings as a new Captain selection, then ordered direct execution.
All owned Grok workers were stopped; the existing broker and controller remain.
Direct Surface writes and fresh device reads restored lower Hybrid K1 mode32 /
palette32 and upper Tempo mode18 / Crimson Gold39; scene0 / active_scene0,
master255, gains0.25, both outputs ON. SQ1–SQ5 were untouched. Full before,
responses and after records are in
_scratch/waveform-migration-001/canonical-audio-v01/captain-composition-restored-20261014/.
The Spectrum recordings below cannot qualify the agreed Waveform composition.

Direct BOOM run001 passed received-audio alignment but the right plate end was
cropped; retain it as restricted evidence. The background camera UI did not open,
Center Stage was off, and the macOS capture API cannot set zoom. Captain moved
the iPhone back; opened preview002 now includes the whole plate and both ends.
BOOM run002 and Sonic6–30 run001 on the restored pairing passed AV correspondence
and complete coverage. Both had zero deltas in capture rejects, stream epochs,
discarded samples and skipped releases. Current output precision1, lower warmth
0.590551 and configured ML enabled differ from the earlier saved baseline; those
fields were preserved and are recorded. These are new current-configuration
baselines, not matched comparisons against the old recording.

Full musical perception remains UNASSESSED: the current media tool explicitly
returned “audio content omitted because you do not support audio input”. Opened
stills establish framing/emission only. No whole-movie musical judgement is claimed.
No Dopex exposure and no final M23 boot selection have occurred.

The existing queue-pressure regression still reproduces losses at805/753 queued
samples on the installed owner's early-break loop. Its proposed renderDue was
a model-only method; production owns rendering through palette_step(true).
The direct continuation is a bounded owner-loop repair using that existing path,
then relevant checks, exact-image installation, state restoration and replay.
This is an engineering queue-loss correction; a better physical musical show
remains subject to the original comparison and perceptual criteria.

## 14 October 2026 — both edges restored; Spectrum BOOM and Sonic 6–30 recorded

Captain returned Titan and E2 Lite operation to this lane. The ownership-transfer
stop in the section below is historical. S3 playback for this room was released
at 2026-10-13T21:47:02Z and was not used again. No SQ write, no flash, and no
second serial owner. Spectrum mode, palette integers, and primary intensity 1.0
were left as found. Only `secondary_output_enabled` was changed, from false to
true, after a fresh per-edge GET showed it false on both boards.

Enable evidence: `_scratch/waveform-migration-001/canonical-audio-v01/secondary-enable-20261014/`.
Both `POST /api/controls` calls returned HTTP 202 pending. Fresh GET afterwards:
board 0 Spectrum mode 1, palette 39, gain 1.0, secondary true, revision 637,
request id still 1077223628; board 1 Spectrum mode 1, palette 1, gain 1.0,
secondary true, revision 637, request 1077225767 accepted, rendered, and
emitted. Mode, palette, gain, brightness 255, and emission stayed unchanged.
The mixer PATCH log does not record this Surface control path. A following
native wire pair showed bottom channel 0 mode 1 with lane counts 80 and 80,
and top channel 1 mode 1 with lane counts 43 and 43. Official optical and
submission scores stay UNKNOWN. The wire palette word stayed 0 and is not the
control palette.

Preview `secondary-enable-20261014/preview/frame-t2.png`, SHA256
`af88cab264783abc68ff6c7b789833aef6b8f981be0147160a6b28ce8cc3c17f`: the whole
plate and both ends are in frame, and both the upper and lower edges are
emitting. Bright parts are clipped. The camera reports numeric exposure as
unavailable, so highlight colour and relative intensity are UNASSESSED. This
still is not a playback-speed review.

BOOM 0–75 on that same composition:
`_scratch/waveform-migration-001/canonical-audio-v01/boom-spectrum-twoedge-001/`.
Movie SHA256 `1fd5d53ece8d8c06dafa33a3c571be19b519d71893aa229481c4d1a70e8e6b16`.
Player return 0, gain 0.25, EiP camera `EB6E6241-CD5C-4917-91B3-7E6500000001`,
EiP mic `EB6E6241-CD5C-4917-91B3-7E6500000003`. Before and after physical state
match: both edges Spectrum, palettes 39 and 1, secondary true, emission true,
brightness 255, gain 1.0. Helper pre-roll stayed 2 seconds because a 10-second
pre-roll is admitted only with an external 100/112-second camera. The source
file itself plays from 0, so the 28-second landmark already has more than ten
seconds of musical history. `av-validation.json` passed coverage and the 28.0
and 70.5 landmarks, and failed the gate: source 49.24 correlated 0.318 against
a 0.45 minimum, peak margin 0.019 against 0.03, residual 0.707 seconds against
0.12. That movie is retained and is not an aligned baseline. 259 sample
directories, observation count 273, those counts not joined. Wire spacing
0.272–0.458 seconds, mode stayed 1/1, wire palette word stayed 0/0. Submitted
lanes were dark on the bottom in 23 samples, the top in 73, and both in 22.
That does not locate a first musical loss and does not resolve a 72-millisecond
event. Aesthetic verdict UNASSESSED. No correction is justified.

Sonic Samba 6–30 seconds, same settings, helper exit 0:
`_scratch/waveform-migration-001/canonical-audio-v01/sonic-6-30-spectrum-twoedge-001/`.
Movie SHA256 `7130c1b4e76ae33458ffb8e4b9d3b384c0dc8fa2adee5ad813f07004c99eb97d`.
`av-validation.json` passed. Landmarks at 0.5, 10.0, and 19.5 seconds
correlated 0.767, 0.709, and 0.793. Before and after physical state match the
BOOM leave-state. 108 sample directories, spacing 0.272–0.420 seconds, wire
mode 1/1, wire palette word 0/0. Submitted lanes dark: bottom 8, top 37, both
7. Same limits: not a 72-millisecond proof and not an aesthetic pass.
`physical_acceptance` is NOT_ASSESSED on both runs.

Live readback after both captures, revision 637: both boards emission true,
secondary true, brightness 255, gains 1.0, board 0 mode 1 palette 39, board 1
mode 1 palette 1. Dopex was not played. The earlier Hybrid 32 / Tempo 18
baseline remains the saved capture it already was. An incomplete directory
`boom-spectrum-m1p39-m1p1-001` is only the interrupted start from before this
secondary enable and is not a result.

## 14 October 2026 — BOOM baseline saved; Titan and E2 Lite ownership transferred

Captain's latest directive transfers Titan and the E2 Lite to another team.
That supersedes the earlier release for this lane. This session stopped all
live operations after the capture below and did not read the broker, serial,
camera, or player again. S3 remains a separate worker and is still authorised
on its own lane. This note does not speak for S3's live settings. No SQ1–SQ5
write occurred. No firmware or configuration change occurred.

The 75-second BOOM baseline was delivered before that transfer. It is an audible
recording plus traces, not a completed musical qualification and not a
correction. `after.json` is the last state saved by that capture, not a fresh
live read.

Saved tree: `_scratch/waveform-migration-001/canonical-audio-v01/boom-tempo-crimson-baseline-001/`.
`original.mov` is 126702115 bytes, SHA256
`502a96ad3ada3a31f55ca89bc9cbd07cf767245a9e07b00d59a5f6a31584c8f0`, matching
`av-validation.json`. Video duration 90.573320 s, 2717 frames. Microphone audio
4339712 decoded samples, maximum gap about 6.7e-7 s inside the movie file.
Validation passed on three acoustic landmarks (source 28.0 s, 49.24 s, 70.5 s;
correlations 0.919, 0.794, 0.854). Source-to-movie offset 2.9488038688618876 s,
scale 0.9995293379816861. Scored movie range 30.935625 s to 77.913504 s.
Timing scope recorded by the helper: received-microphone acoustic proxy, not
MCU input and not photon latency.

Source fixture, from `observation.json`: wav SHA256
`09c9a02f439cbeb66f006cf5a973ae7988b5ef2e9ecdcf470c140d6918e63652`,
MP3 SHA256 `3d60c58eccef198bb8062bef68df4d882c1b49c8f246623c4ba88d337cad8e4d`.
Recorded image identity: UID `545433931bd25436593630352d068363`, build
`a43e4842b3a7cf8b5150787f613b575551dc090d80779dae6474784de59cd606`.
The telemetry `source` field `7f86939bd7c16c4a6dd5789813e9d551246ed134` is the
Emotiscope donor id, not git HEAD. Identity `accepted:false` with reason
"no accepted checkpoint bound" is the existing broker diagnostic in these
files, while UID and build match. Playback gain 0.25, player return code 0.
`before.json` and `after.json` both record lower mode 32 / palette 32 and
upper mode 18 / palette 39, both outputs enabled, primary intensity 0.25,
hardware brightness 255. Palette 39 is Crimson Gold from that control readback.
The wire header palette word is 0 on both edges in every checked snapshot and
is not the authored palette.

Wire review used `scripts/palette_wire_snapshot.py`: 16 little-endian header
words, mode at word 8, palette at word 9, flags at word 10, then 480 RGB8
bytes and 960 GRB48 bytes. All 414 on-disk snapshot binaries matched their
saved score JSON on mode, palette word, flags, output channel, and lit-pixel
count. Bottom channel 0 mode 32; top channel 1 mode 18. Every official score
says optical UNKNOWN, submission UNKNOWN, and native-16 RGB8 preview cannot
validate the wire. Nonzero submitted pixels are not observed light and are
not a musical pass.

Interval table in `interval-score.json` is from 207 sample directories.
`capture.json` holds 222 trace records. Those two counts were not joined, so
the difference is not interpreted as dropped holes. Inside source 28–75 s the
directory spacing is 0.322–0.65 s (112 samples). `physical_timestamp_valid`
is false on every MIR sample. Stream epoch stayed 6, the audio-gap counter
stayed 5, the capture reject counter stayed 4, and generations kept advancing.
That supports continuity of this capture only. It cannot time sub-frame events
or prove the first loss of musical structure. No correction is justified.

Stills extracted with ffmpeg from the av-validation map:
`frames/src-00.png`, `src-28.png`, `src-40.png`, `src-50.png`, `src-55.png`,
`src-60.png`, `src-70.png`, `src-75.png`. An earlier sentence in this section
said the centre was washed out by the camera and that interior pixels were
unreadable. That description is withdrawn.

Parent-independent review of these same eight frames, 14 October 2026, recorded
in `/tmp/prism-titan-parent-offline-inspection-20261014.md` for source 28, 50
and 60 seconds, then the same finding on `src-00`, `src-40`, `src-55`,
`src-70` and `src-75`: the full bar and both ends are visible, and both edges
are lit. The centre is distinctly darker in most samples. Bright clipped and
bloomed areas sit nearer the upper and lower injection rails and the ends.
Precise colour and relative intensity cannot be determined. These are still
observations only. Playback-speed audiovisual perception was not available.
Aesthetic and musical verdict: UNASSESSED. Helper `physical_acceptance` is
already NOT_ASSESSED.

Still outstanding, and not done here: the rest of the canonical suite, a weak
case, a sustained case, the Dopex holdout (do not consume it until a candidate
is frozen), and M23 boot selection. The next Titan/E2 owner continues from
these files. Do not recapture this baseline to rename palette 39.

## 13 October 2026 — execution handed to a fresh Astra lane

Captain requested a comprehensive handover and no further device experiments in
the outgoing lane. [Complete execution brief](ASTRA-RESUME-20261013.md), with
[source/evidence snapshot](evidence/astra-handover-20261013/source-state.json).
Original assignments and failed evidence remain unchanged.

The already-running S3 normal commit gate completed:1757 passed,15 skipped,
93 subtests passed; required k1_hardware build passed. B489 repair commit
`be0a97e31ff51f65bdc5b6b0081585acdf88c32b` is **UNINSTALLED**. The fresh
committed-source B489-profile build, guarded deployment and actual dual-edge
capture remain next. Original installed S3 image is last observed eff9bf47.
Custom PCB3 dual-IM69D130 DATA13/CLK14 and intentional mono RIGHT programme
source are settled; two constant raw blocks do not establish a dead microphone.

No new Titan flash or control change occurred while producing the handover.
Last recorded state remains family008, lower32/palette32, upper18/palette39,
both outputs enabled, master255, gains0.25. The new39 palette selection has no
physical capture yet. No recording/player/deployment process remains from the
outgoing work; the existing autonomous Titan broker was retained. Refresh actual
identity/ownership/settings before dependent operations. Physical suite acceptance
and selected embedded boot delivery remain unfinished.

## 13 October 2026 — canonical BOOM capture and S3 reference repair in progress

Titan remains on family008 / source99c0cf44000471fd30a6b368bd2c18a4403adf46,
builda43e4842b3a7cf8b5150787f613b575551dc090d80779dae6474784de59cd606.
Latest explicit Captain composition change: lower Hybrid32/palette32, upper
Tempo18/Crimson Gold39, both enabled, master255 and gains0.25. Readback:
`_scratch/waveform-migration-001/canonical-audio-v01/top-tempo-crimson-gold-selection.json`.
SQ1–SQ5 unchanged. The earlier40 palette name was a lookup error:40 is
Ultraviolet Ascend,41 is Naberius Gold. Preserve the earlier receipt/footage as
failed-label evidence; it is not footage of39.

The camera helper now records an explicitly selected real microphone; the
existing capture helper validates source identity and passage-dependent timing.
The75-second BOOM fixture and audible baseline exist. Sonic exposed three
capture rejects; the reachable owner-loop queue-pressure regression is
reproduced on the host, with no Titan correction installed yet. Canonical suite
acceptance remains open. Follow the full original assignment and dated evidence
in [the suite](CANONICAL-AUDIO-PERFORMANCE-SUITE.md).

Captain authorised S3 reference repair on B489A500. That work is in the original
S3 repository at its installed eff9bf47 lineage; microphone DATA13/CLK14 stays.
The original S3 is currently mono RIGHT and reports constant raw samples. It
cannot yet supply music-valid parity evidence. This operation does not stop
Titan or overwrite its selected composition.

## 13 October 2026 — top optical mapping and selector repairs installed

Installed firmware source `99c0cf44000471fd30a6b368bd2c18a4403adf46`, family 008 build `a43e4842b3a7cf8b5150787f613b575551dc090d80779dae6474784de59cd606`, schema `e99c1ec147288a7beaed728fc91e42530efc91c26f223164e03d07745b68c2da`, UID `545433931bd25436593630352d068363`. Top DIN0/P603 physically serves the right half; DIN1/P714 serves the left. The source-half adapter now follows that observed map. Pins, DMA, 160 pixels per edge, brightness, gain and gamma remain unchanged. The one-sided source/payload/plate comparison demonstrates corrected side and radial positions. All 144 production runtime pair checks pass after the mapping correction; the negative test rejects the old mapping.

Two additional defects were repaired: firmware `5e11448` releases the composed override on direct effect selection; host `eb3f785` restores complete board settings without overwriting the peer scene selector. Latest operator leave-state: lower Hybrid K1 mode32/palette32, upper Octave mode2/palette41, master255, gains0.25, both outputs ON and mirrors ON. Upper scene0 is active; lower retained scene3 is inactive under Hybrid. SQ1–SQ5 are preserved; SQ5 still stores Hybrid K1/Tempo. Both the mapped Sonic passage and contrasting Carte passage add no sampled capture, microphone or missed-release faults. Pink/red dominance and camera highlight clipping remain observed limitations. Complete audiovisual aesthetic acceptance and boot selection remain open.

The existing capture helper now supports five hash-verified 24-second windows from approved Sonic/Carte PCM. Captain's newly supplied BOOM source was analysed for accelerating onsets, decay/re-entry and later pulse/subdivision behaviour. There was no BOOM-specific tuning or live BOOM quality claim. See the latest migration record, `boom-*`, `audio-window-audit.json` and `top-mapping-*` evidence. The existing browser controls tab was refreshed after host reload: all five Waveforms are visibly listed. Fresh board readback confirms mode32 below/mode2 above; no preset slot was recalled or saved by that refresh.

## 13 October 2026, finalisation — installed show preserved; boot support ready, physical acceptance blocked

Live readback: lower Hybrid K1 mode32/palette32, upper Tempo18/palette41, master255, both gains0.25, both outputs on. Installed006 build `d8e11f5ed9c775267b5ef5ff00baa2a1893c31628f1877a6d9fb3f0b07b459bf`, UID `545433931bd25436593630352d068363`. E2 signature/run recovered a missing CDC endpoint; SQ5 was then restored. Camera preview now shows the complete focused plate. No new music recording was substituted for unavailable full audiovisual review.

Source follow-up: corrected144-pair C++ matrix (old donor rows had composed override), added144 host/codec pairs, added exact consumed RMS/threshold trace, implemented complete optional v8 boot defaults. Host443tests and focused firmware checks passed; actual unselected ARM build passed with unchanged2848B DTCM scratch. It has NOT been installed or selected. Current embedded startup remains old Spectrum; SQ5 remains host persistence.

Full physical quiet/weak-input, continuous motion/music, white/blending acceptance remain BLOCKED at playback-speed audiovisual review. Then Captain selects final boot and authorised install/startup checks follow. See latest finalisation sections and receipts in `INHOUSE-WAVEFORM-MIGRATION-20261012.md`; all old assignments/failures retained below.

## 13 October 2026, 17:14 AWST — latest optical comparison blocked by camera framing

Family006 normal/weak captures finished without new sampled timing failures, but owner inspection of actual frames48–55s and78–85s shows a blurred desk area, not the K1. No physical quiet-repair acceptance claimed. Preserve `family-physical-quiet-after-001/root-visual-inspection.json` and original movie. Captain asked to restore camera framing; both edges remain running32/18 with preserved output settings.

## 13 October 2026, 17:11 AWST — five Waveforms installed; quiet correction in physical comparison

Installed source cb09979, build `d8e11f5ed9c775267b5ef5ff00baa2a1893c31628f1877a6d9fb3f0b07b459bf`, schema `e99c1ec147288a7beaed728fc91e42530efc91c26f223164e03d07745b68c2da`, UID `545433931bd25436593630352d068363`. All212 receipt inputs match source. E2 Lite target-power OFF/1MHz installation succeeded; programming17:09:39–17:10:07 interrupted output, identity verified17:10:18. Both-edge settings restored and SQ5 recalled: bottom32/palette32, top18/palette41, master255, gains0.25, emission and secondary ON. Hostf46c137 renegotiates v8/capability511 after upgrades. SMC-Mixer is deliberately absent; existing Surface works.

All five effects32/18/8/7/11 are implemented and selectable. Family005/source8f069dc fixed Tempo throughput using exact four-lag autocorrelation and only2848B temporary DTCM scratch. DualTempo observed maximum analysis1611184 cycles; six captured music passages had no new audio gaps, PDM rejects or skipped render releases. Preserve failed003/004 receipts. Successful movies remain under `_scratch/waveform-migration-001/family-physical-tempo-006/`, `family-physical-wave-fast-001/`, `family-physical-hybrid-mixed-001/`.

SQ5 complete two-board save/recall and fresh controller persistence were verified; SQ1–4 unchanged. Host persistence is not boot NVM. Root inspected actual extracted plate sequences: Waveform8 clearest darker space; Fast broader/brighter; Hybrid11 broad pink/red blending remains a weakness; mixed32/18 distinguishable material and central space. White camera cores remain unresolved optical-versus-camera clipping. Continuous playback-speed review with synchronised music remains unproved. Final boot/default selection is unfinished.

Current quiet repair excludes births when source quiet is confirmed and coherent raw RMS remains below its existing entry threshold. Production-path checks preserve .079 wake, .0447 sustain, original dwell, independent peer bytes and stale/zero drain. Sub-.04 music after confirmed quiet remains excluded; stronger acoustic transients can still create deposits. Physical normal/weak comparison is in progress at `_scratch/waveform-migration-001/family-physical-quiet-after-001/`; acceptance PENDING. Family005 remains immediate rollback. Full original M00–M24 plan and historical evidence are preserved; migration completion is not claimed.

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


### 14 October 2026 additive checkpoint — first queue repair installed; coupled admission repair under test

Captain's direct execution instruction supersedes orchestration-only work. Native workers remain stopped. Titan is running lower Hybrid K1 32/palette32 and upper Tempo18/Crimson Gold39, scene0, both outputs enabled, master255, gains0.25; SQ1–SQ5 were not written. The sole broker/controller remain in use.

Commit f79d302cf91cdb97e43c34e4bbb8269845f8e85e installed exact build609e8cf061959106968c6961fc8dadec4d0bde9ebe1a9581a8de23c81698fb14 via arm-queue-pressure-009 and programme-queue-pressure-009. Programming briefly interrupted the application; fresh UID/build/schema matched, then staged control restoration recovered all63 saved writable fields per board. The first full upper63-field patch was rejected as forbidden_or_invalid_field; selected mode/output fields followed by only the11 remaining differences restored the exact state. No cause involving output reinitialisation was established.

The first repair is INSUFFICIENT: before playback, two capture rejections discarded2098 samples. Actual capture-service witness: poll-entry filled161, rejection filled753, third pair, reject_poll_cycles4597276; poll_max_pairs3/poll_max_cycles6710912. Two296-sample pushes reached753 and the third would reach1049 in a1024 ring. BOOM on009 added no capture loss, but that does not clear the startup defect. The Sonic009 recording failed its last audio landmark (correlation0.3166/margin0.0997); retain as capture-invalid, not firmware proof. Dopex remains unplayed; the009 evaluation freeze is superseded by this defect.

The coupled correction is limited to pdm_target.c/.h and the fixture owner: before acquiring another waiting raw pair, ask the existing sole main-loop AP/render owner for at most three hops of queue room; never call from an ISR, retain no raw owner during callback, retain explicit loss fallback, and bound the producer pass to its physical slot count. No microphone acquisition, DSP, slot, gain, calibration, pin, output correction or effect implementation changes.

Sanitised host regression queue-pressure-admission-fix-003.json reproduces the161/753/third-pair loss with callback absent, and preserves110 exact serial-oracle PCM/source-index hops with callback enabled (one drain/one render, zero discard). Restoring the loss fails the required assertion with exit-6. Two prior failed test attempts are retained: an incorrect constructed ASRC prefix, then unnecessary drain after no raw pair remained. The latter was repaired by a READY look-ahead before callback, while actual ownership remains validated by acquire. Target AP/render costs and raw DMA liveness still require the010 installed run. Full-speed audiovisual perception remains UNASSESSED in this session; native audio input was omitted as unsupported. Intact microphone MOV evidence remains usable.

Captain reframed the iPhone again; preview003 is being captured for direct framing inspection before the next long recording. Existing helpers are reused unchanged.


### 14 October 2026 — coupled queue-admission correction installed (010), baseline and weak transfer captured

Commit09ae38717461b10c221928c0405b74e8182feadb owns the five-file correction and focused regression. ARM build9ab90873fa37c4fadb5afca1f02699e603af082635eb4084baf5979f7c8660d7, HEX329697392d2d00aad834f99436a57d30f0d8550162e8dc8f058f645d00983105, ELF1b10dd6ccfe4be57d2885dd591f3119aa7ef76b7a520bbaeab169c75077d7dd6; all213 receipt inputs match HEAD. Only fixture_app.cpp and pdm_target.c/.h compiled inputs changed from009. Text343660/data191128/bss499476; DTCM scratch remains2848B. The callback fits existing memory padding; no queue enlargement.

Guarded E2 Lite programming08:48:35–08:49:04AWST, probe powerOFF/1MHz. UID/build/schema matched08:49:14. Broker53500 replaces33722 through the authorised deployment procedure; controller79093 retained. No full upper63-field transaction retry: selected modes/palettes/outputs first, then actual differences; all63 writable fields per board match the before-deploy operator state. Both outputs continuously run after restoration. Legacy boot defaults remain unselected Spectrum/top-disabled; M23 fixed boot delivery remains incomplete, not silently replaced by the current live selection.

Root opened preview003/frame.png: full plate and both lit edges visible. Captain's latest reframing is retained. BOOM010 original MOV contains real iPhone microphone audio; landmarks correlate0.8527/0.4736/0.9284, map uncertainty0.020s, scored coverage complete. Zero push rejects, discarded samples, stream-epoch changes, raw pair drops, new skipped releases or emitter faults from boot/restoration through this run. Capture poll maximum4,482,612cycles at1GHz; snapshot includes actual AP/render work but does not establish physical microphone-to-photon latency. Root opened source28/60/85s plate frames: both edges illuminated, central gap, pink/red/white rail highlights during music, dim warm residuals during tail. These are stills, not a musical verdict. Separate camera sessions/framing prevent photometric superiority claims.

Sonic6–30s010 recorded actual received audio with landmarks0.7526/0.7710/0.8195; complete coverage, unchanged controls, zero input loss or new output/skipped-frame counters. This valid run is a different installed image and Captain-reframed camera; it does not overwrite the failed009 Sonic evidence. The source-bound host witness establishes why the producer admission changed, not why a prior microphone AV landmark failed.

The010 evaluation is frozen in canonical-audio-v01/admission-fix-evaluation-freeze.json before first Dopex playback. Dopex holdout is now being captured on unchanged image/settings; no holdout tuning. Before this freeze, only its retained lineage/hash/header were checked. First supported loss remains the queue-admission boundary at startup/control restoration; no first loss of musical structure or visual superiority has been established from sparse wire traces/stills. Engineering comparison: queue-repair-boom-engineering-comparison.json.

Existing complete boot consumer is reused. UNSELECTED-current-hybrid-tempo-crimson-v8-20261014.json prepares all63 fields for the current32/18,p32/39 composition; schema bool output_precision encodes integer1 readback as true without changing its meaning. It has not been selected, built as final startup, flashed or saved over SQ1–SQ5. Captain's M23 selection and full-speed physical qualification still bind.

Native workflow_context for actual Titan worktree with existing key56d249405343f8282f08d0b9 refused: Session/project mismatch: not a registered worktree of the launch project. Do not use a PRISM receipt as Titan acceptance or forge another key. Actual guarded deployment and domain tests remain recorded; no governor completion certificate is claimed.


### 14 October 2026 — frozen holdout complete; current leave state and startup decision

Dopex75s holdout010 completed once after the frozen candidate/evaluation, using the retained approved fixture. Real-microphone AV landmarks correlated0.7666/0.7703/0.5268 with0.020s mapping uncertainty and complete scored coverage. No input rejection/discard, stream epoch/skew/stale changes, raw lane overflow/error/recovery, new skipped releases or emitter faults. Root opened actual source28/60/85s frames: both edges and entire plate visible during music, dark plate at85s tail. No tuning or production change followed this holdout exposure. BOOM/weak Sonic/Dopex transfer evaluation is admission-fix-transfer-evaluation.json. Musical aesthetics remain UNASSESSED without a capable full-speed audio/video perception route, not passed by those engineering results.

The first final-scoring attempt incorrectly asserted exact symmetry for every Tempo frame and exited1. That was an added blanket check, not the frozen top-mapping criterion. Fresh comparison found asymmetric frames in both the retained baseline and009, and actual inhouse_waveform.cpp:484–487 intentionally mirrors mode18 only during injection; its drain path can retain asymmetric history. Keep the source behaviour and all observed mismatch counts. Every captured upper tap carries TOP_DIN0_RIGHT_DIN1_LEFT and nativeRGB16 mode18; lower mode32 remains symmetric. The actual compiled effect and top-packer inputs are unchanged. Physical mapping/still evidence and source preservation remain distinct from full-speed release judgement. Do not repair source Tempo merely to force a blanket mirror check green.

Prepared current full boot profile SHAa166a3fb79dd1c9a99f76dbd0520a3dcbe1f8e03046def98951668437470d27b compiled as UNSELECTED build54d9a439e7c4ca598d2bf289f9552764e5b1cc7d87c47e7a0316466d98772cd5. HEX586974852ffd85d23e0b8ff2171f7524dc591d60abe29bd9c5a95ae65682836a, ELF56b61ca7419097c6910bbc45834687ad7a737e6356f1f8ef6105cc464b91b279; text344940/data191128/bss499476.212 tracked inputs match commit09ae387; the213th explicit authored defaults input is hash-bound separately. Complete generated startup requests both outputs ON, brightness255, gains0.25, lower32/p32 and upper18/p39; actual startup/cold-start remain unexecuted. Captain's required M23 final-selection question was presented with these concrete results; elapsed silence is not a selection or permission. SQ1–SQ5 unchanged.

Fresh post-camera readback admission-fix-post-capture-readback.json retains all63 writable settings per board with zero differences; frames80900, upper completions77873, capture losses0/0 and epoch1. Both output flags ON, correct modes/palettes/master/gains after all camera sessions ended. This proves continued rendering/configuration after recording cleanup, not the unperformed full host-disconnect/cold-boot test.

WP6 index linked additively in PRISM docs/lightshow-loop/README.md; original Phase1 and all full assignments remain intact. Current charter was reread and PRISM copy matches its approved hash. No new dashboard/registry/worktree/Core repin. A read-only launch-plist check initially failed under Homebrew Python3.14's pyexpat symbol mismatch; existing supported Python3.13 is used, without changing installations. Hindsight refresh/watchdog failures were disclosed; memory remains clues only.


Continuous-output launch reconciliation: ProgramArguments names the existing titan-broker-launch.sh wrapper, so searching only that plist array for a direct --continuous-output flag returns false. The wrapper's final exec explicitly supplies --live-control --continuous-output; broker normal shutdown closes serial/control/recorders without an emit-disarm call. Existing source and successful bounded captures agree on continuous mode. No launch option, host code or service was changed by this read-only check.


### 14 October 2026 — debrief checkpoint; host-disconnect test failed before hardware action

The standalone host-disconnect attempt host-disconnect-current-010-001 exited1 at09:06:24AWST with queue.Empty: a3s wait for the next camera startup stdout event expired. Its event history contains no recording_started, audio_process_started or sole_broker_stopped. No playback, broker shutdown, disarm, reset or firmware write occurred. The camera process was terminated by this attempt's cleanup; the existing broker53500/controller79093 remained. cleanup.json broker_restored=false means it was never stopped, not that restoration failed. failure-diagnosis.json retains the actual failed scope. Host-independence remains NOT TESTED. The needed test repair is to reuse existing bounded camera-start handling rather than treating3s of quiet stdout as a failure. No retry was launched when Captain requested the full chat debrief.

At Captain's debrief request, fresh debrief-live-state-20261014.json confirms current010 build/UID, correct32/18,p32/39,master255,gains0.25,both outputs enabled, precision1, and input rejects/discards0/0 with epoch1. No camera/player remains. M23 final boot choice has not arrived; the compiled complete profile stays UNSELECTED/UNINSTALLED. Current installed images retain legacy Spectrum/top-disabled boot defaults until explicit runtime restoration: autonomous two-edge cold boot is still UNDELIVERED. Do not describe continuous current runtime as completed startup behaviour.

Timing correction for the debrief: wrong-composition BOOM002 did execute06:03–06:05AWST and passed audio validation, per its own retained events/observation. Any earlier description saying that run never executed is stale. It is immediately before the last3h window and still cannot qualify the agreed Waveform pair. Newly connected9087A500 S3's controlled audio report completed05:47AWST, before that window; no S3 implementation/flash occurred during the latest3h. Older B489 deployment/recovery and S3 incidents are historical context, not fresh completion claimed in this period.
