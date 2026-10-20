# Emotiscope.HIL complete-runtime port to Titan — 29 September 2026

<!-- BEGIN CAPTAIN PERMANENT MULTICOLOUR PALETTE MANDATE 2026-10-11 -->
## Permanent multicolour palettes — Captain's current visualiser requirement

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Captain now requires:
"visualiser shall NEVER display a single colour; PALETTE use permanent default,
single-colour visualiser modes prohibited."

This newer directive supersedes the original Emotiscope transplant instruction
to preserve native donor HSV/sourcecolour and not apply Titan palettes. Retain
that assignment and its receipts as historical evidence; it no longer authorises
palette0/native HSV defaults, presets, fallbacks or selectable single-colour
visualiser modes. Use the existing curated multicolour product palettes through
the real render/background/screensaver path. No rainbow cycling or hue-wheel
sweep is authorised. Do not add a cosmetic colour floor at silence. Enforce the
requirement in actual boot/default, preset, control and fallback consumers, not
only in host selectors or documentation. Preserve centre-origin motion, music
response, existing transport, current brightness100 and the authorised two-board
requested intensity envelope. Source/host proof and actual physical appearance
remain separate claims. No new registry or old campaign is reopened.
<!-- END CAPTAIN PERMANENT MULTICOLOUR PALETTE MANDATE 2026-10-11 -->


THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.

## Assignment and authority
The user explicitly parked the current bespoke Titan AP/VP at13:57 Perth and selected the local Emotiscope.HIL repository as the next complete firmware donor. Implement its audio-to-light pipeline as a separate candidate, then deploy the exact checked image for physical inspection. The prior Aurora appearance and continuity legs remain parked; no appearance acceptance or completion is inferred.

Destination: /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001
Branch: feat/titan-emotiscope-port-001
Donor: /Users/spectrasynq/Workspace_Management/Software/Emotiscope.HIL/v1.1_build
Donor HEAD:7f86939bd7c16c4a6dd5789813e9d551246ed134.
Frozen source identity: DONOR-SOURCE-MANIFEST.json.
Hardware/source starting point is the effects worktree at4ced4d1d plus its explicitly copied current dirtysource, recorded in evidence/parked-20260929/overlay-source-manifest.json and old-tracked.patch.
Parked current image AuroraA45c2f206…, HEX6fa13ef0…; pendingB e72e16bc…, HEXca39acd3…. Both completeHEX/ELF/buildreceipts andAprogramme receipt are copied under evidence/parked-20260929/. Target state is still to be freshly read before any programming.

## Donor choice and fidelity boundary
v1.1_build is the documented maintainedHIL target; v1.1_320 duplicates its160scene onto two160transfers and introduces a64-bin spectrum indexing risk. src is a different experimental implementation. Do not mix their algorithms and label the mixture parity.
Preserve v1.1_build64frequencybin Goertzel, original windows/interlacing/normalisation, VU, chromagram, novelty,96tempo bank, all selected visual functions and authored output ordering together. Some audio state is updated on the rendering side. Feed normalizedPCM to the donor; never feed existingTitan featurepackets or use the oldK1 gain/saliency/tempo.
Native audio:12800Hz,64samples/5ms. Native renderer:128pixels. Declare100Hz as the first port scheduling choice; it is a reference cadence, not an established historicalS3frame rate. Preserve elapsed-time phase semantics and measure scheduling on target.
Donor Spectrum mode1 is initial default. Preserve sourcecolour/defaults, avoiding independent aesthetic tuning during the transplant.
The hardware adapter takes final donorRGB8, spatially interpolates128→160 with centred geometry, expands to16bit, applies the mandated100/255 hardwaregain exactlyonce and packs the existingGRB48 DIN80+80. Do not apply Titan palettes, tone curves, gamma or its effects again.
Chip-specific ESP microphone offsets andpinconfig are not portable. Titan uses its measured-ratecapture and directASRC to12.8k. Bind the normalization and anyinputDC handling as hardware adaptation with tests.

## Required safety corrections and exclusions
Retain an immutable source copy/manifest and report any modified vendorheader by before/after hash. The1024-tempo window begins atindex-1 in source and requires a precise boundedindex repair. Generic interpolation endpoint accesses require bounds-safe handling. These aredefined-behaviour corrections, not permission to redesignDSP.
Keep donor quirks that aredefined and do not prevent safe operation; record them. Do not silently replace its Gaussianwindow, binvisitation, amplitude law orBloomvariant.
Network/NVS/webdashboard/HILcontinuousCSV writes, donorESP tasks, ESPRMT andhardwaredrivers areexcluded. Implement deterministicplatformshims only where required for the actual donor engine.
No allocator in the real-time processing path; fixed storage is preferred. No fabricated readiness/timing/MIR metadata.

## Work packages
1. Freeze sources and rollback: completed checkpoint makes currentdirtysource/receipts recoverable and gives the donor exactfile hashes. Check that copying has not consumed secrets/networkcredentials. Keep source treesunchanged.
2. Portable completeengine: single translationunit with retained donorheaders/sharedstate, clock/random/DSPprimitivecompatibility,64samplePCMentry,128RGB8renderexit and truthfulsnapshot. Nativeunderstood functions getdetailed regressionproof. Defined behaviour repairs stay isolated. Defaultmode and controls documented.
3. Audio hardware adaptation: preserve PDM ownership, DMA, rawratecalibration, lossaccounting; conditional12.8k/64ASRC and source/timingintervals. Legacy24k/180 mode remains unchanged when the portflag isabsent. Distinctcontract metadata must prevent12.8k being called24k. Never concatenate samplesacrossloss orhidecounters.
4. Target runtime integration: bypass oldK1 AP/VP withportflag, retainidentity/CDC/transport; render donor atdeclaredcadence; reportrealAP/VPcost and64bins. Explicit128→160mapping andglobalgain. Maintain autonomousmusic afterhostrelease. No unrelatedmixer/wiring/BSPchanges.
5. Host verification: sanitizers atsourceboundaries, actualdonorcomparator with identicaltimestamps/config/PCM, spectrum/chroma/VU/tempo/novelty andfinalpixels comparisons; silence/signal/recovery; approvedtrance input; endpointmapping andgainonce; correct12.8k samplecount/time/conservation. A negative variant mustdemonstrate eachnewcriticalregressioncanfail. Do not confuse successfulcompilationwithparity.
6. ARM andidentity: isolatedfreshbuilddirectory, bindflaggedsources/buildidentity/HEX/ELF, linkmap/memory and no runtimeallocator. Do not weaken importcontracts; declareexplicitnewprofile. Preserve existing compiler/probe gateways.
7. Physical execution: read actualTitan UID/build/config underCDCownership, matchverifiedcandidate, E2LiteSWD1MHz powerOFF -noquery -run permittedcodeareaonly, then matchingUSBreadback. Play SonicSamba/CarteBlanche throughsameacousticpath; inspectphysicalLEDs, bufferandcapturehealth/timing. Dopexheldout. NoHTMLappearance substitute. Programminginterruptsapplication; restoreautonomousoutput onreturn.
8. Closeout: exactresidentidentity andsource, factualtestoutcomes, faults/losses andlimits, operatorappearanceverdictifprovided. Reportattemptresultwithoutclaiminghistoricaldonorappearanceorlegacycontractclosure. Ifrequiredgatefails, preservefailureandcontinuereadyrepairwork; nameexacttechnicalblocker.

## Ownership
Root: checkout/checkpoint, runtime/build/control/LEDadapter, finalintegration andalltargetaccess.
emotiscope_visual_port: portablewrapper/header/shims andretainedVPordering.
emotiscope_audio_port: isolatedtempo boundaryrepair plusdonorhostverification.
emotiscope_capture_adapter: conditionalPDM/ASRC12.8k/64 adaptation andfocusedtransporttests.
One owner forCDC/E2Lite. No workeraccess tohardware orplayback.

## Acceptance
A port must have acomplete donorAP/VPcallchain, traceablesources, meaningfulhostcomparisons andexactARMbuild. A workingtargetrequires advancingcapture/analysis/render/output, correct160/80+80mapping andbrightness, truthfulloss/faultreporting andautonomousleave-state. Appearance is acceptedonlybytheoperatorobservingtheactualLEDs. AlluntestedrequirementsremainNOTTESTED. A usefulfirstportexperimentdoesnotprovecontinuouslong-termstabilityoralloriginalVP/CTL/MOD/IVVadmissions.
