# Lever swarm brief: Titan audio-to-light pipeline (15 Oct 2026)

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This is a research swarm only: no device, serial, USB, probe, broker or launchctl access; no flashing; no commits; no pushes. You are READ-ONLY on every source repository. Write only inside this run folder:
`/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001/_scratch/waveform-migration-001/runs/LEVER-SWARM-20261015/`
Never write to ~/Desktop or ~/Downloads (reading them is fine).

## The Captain's request (verbatim core)
> "So the implemented fix was a single agent's assessment of the problem that I presented … distillation of how to 'backend' data flows from the AP to the VP (think of them as containers), and within each of these containers are further smaller containers (functions) that work in a sequential chain, translating the algorithmic design of how each snapshot of audio data is presented on each of the LED PCBs, both in motion, colour (and to a lesser degree - brightness, which has an interdependent relationship on colour). The agent was able to very astutely identify the broader systemic failure. Compression, possibly even OVER compression. So take this back to first principles. Why bother compressing? If I had to guess, it'd be because if there wasn't compression, each 'slice' of 'data to be processed' at 'full resolution' would have demanded far too much processing, so much processing that the S3 would not be capable of delivering the required output within the required time frame … A reasonable, and necessary trade off - when the system is resource/performance constrained. The agent was able to … map out and likely EXTRAPOLATE all the obvious (and to some degree, the less obvious) levers that contribute to what exactly the LEDs output (in its final stage).
> Right now, we're basically just 'copying and pasting' analysis, decomposition, CRITICAL THINKING, CREATIVE PROBLEM SOLVING, and ARTISTIC and CREATIVE VISION … a REAL HIGH VALUE, EXTREMELY LEVERAGEABLE, and a rare opportunity to tap into hugely asymmetric gains, if we piggy back off what we know, about the failures we've experienced on the Titan, the failures that were previously experienced on the ESP32-S3, PLUS the research and implemented solution … identify ALL the levers, how those levers can be manipulated, optimised use of those levers, potential value (VISUAL impact x novelty) multipliers, architectural design of the visual pipeline, and anything else I might have missed or overlooked."

AP = audio processing (microphone → features). VP = visual processing (features → LED light). Treat each as a container of smaller containers (functions) in a sequential chain.

## The worked example this swarm must generalise (colour, 14–15 Oct)
- Symptom (Captain): the palette was "not being accurately mapped to each of the 80 bins", i.e. "over-averaging". It was fixed in the K1 ESP32-S3 firmware.
- Found in K1 `/Users/spectrasynq/SpectraSynq_K1_Firmware` commit c2738b88: per-bin HD palette sampling, `palette_manual_colour(pal, freq_prog, bin)`, replacing a FastLED 16-slot averaged palette. Related commits: SensoryBridge 6109a1e (Spectrum River, hue = k/79) and d408433 (no 16-entry flattening); K1 ae5d90a (chroma position never freezes); Lightwave Feb 2026 (trails carry brightness only; colour applied per pixel at output).
- Titan fault: `src/k1/core/visual/inhouse_waveform.cpp` (around lines 209–231 at HEAD 81ac031) mixed RGB channels, used the 16-slot palette, and sampled once per frame. Every frame collapsed to one averaged colour.
- Fix (v9 "path P"): the trail carries brightness plus palette position; colour is the HD sample per pixel at output.
  - Host results: 9–16 palette positions across the strip vs 1; colour moves in almost every second vs 1–11 %.
  - A later loud-end stall was fixed by taking position from the spectrum shape before the soft knee.
  - Lesson PL14-B14: the product compile-time default did not select the fix.
- Evidence: `docs/evidence/P6/palette-forensics/FORENSICS.md` and `docs/evidence/P6/palette-forensics-2/*` (product worktree).

General principle to test everywhere: **each stage either preserves or destroys information** (resolution, rate, dynamic range, phase, spatial detail). Many destructions were budget trade-offs for the ESP32-S3 (240 MHz Xtensa, small RAM, 8-bit-per-channel LEDs). The Titan (Renesas RA8P1: Cortex-M85 at 1 GHz with Helium/MVE, Ethos-U55 NPU, Cortex-M33, large SRAM/DTCM; WS2816 LEDs with 16 bits per channel) may no longer need many of them. Equally, some "compressions" are aesthetic and deliberate (smoothing that reads as musical). Tell the two apart with evidence.

## Other known failures and facts (verify; do not trust blindly)
- **"Hits harder"** (Captain, verbatim): "During the build up phase, you can see the waveform length slow and surely contracting from its state of full extension (edges to middle), and at the peak of the tension build up, the length of the waveform is basically compressed heavily towards the left and right edges, so naturally, when the drop hits, this tension MUST be released with 'an appropriate amount' of 'explosive force'/thrust from the origin to full extension (in this case, left/right edges towards the centre/middle). 'Hits hard/harder' is when the visual impact of that release is congruent with the audio."
- **Drop detection.**
  - Three detectors failed to fire within +50 ms of annotated drops (runs/P6-DROP-DETECTOR/{,v2,v3}/DETECTOR.md).
  - The in-house analysis is blind below 110 Hz (lowest GDFT bin). Sub-bass arrives 70–120 ms after the musical drop.
  - The product tempo tracker locks to half time (88 BPM prior; docs/evidence/P7/RECEIPT.md), so predicted downbeats are 26–380 ms off.
- **Latency.** Device budget about 16 ms typical from sample to wire end (docs/evidence/P5/device-budget/). LED wire time is 5.1 ms per frame: 80 WS2816 pixels per data line, two lines in lockstep, 3840 bits × 1.25 µs + 300 µs reset.
- **Geometry.** Two boards; each is a 160-pixel scene split 80/80 across P601/P604; centre origin at 79/80; mirror modes; 120 Hz logical render target. Brightness cap 24/255 on this owner's unit (Kx-Titan AGENTS.md mentions 100/255 as the migration default; report the conflict, do not resolve it).
- **Captain's verdicts and rulings.**
  - v9 is "overall superior to v8".
  - Fade-to-black on silent cuts was a noticeable improvement.
  - The drop still does not hit harder than the build.
  - No rainbow defaults.
  - During testing, normal look rulings are suspended; safety, true black and always-visualising still bind.
- **Lessons.**
  - Kx-Titan `docs/migration/TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md` (PL14 A–E).
  - `docs/migration/TITAN-SESSION-LESSONS-2026-10-11.md` and `-2026-10-14.md`.
  - `docs/migration/NATIVE-FIRST-BOOT-2026-10-08.md`.

## Where things live (locate others yourself; record exact paths and commits you read)
- **Titan product source (RA8P1, lane branch lane/titan-lagfree-20261014):** `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`
  - AP: `platform/ra8p1/pdm_target.c`, `k1_poly24.c`, `inhouse_audio.{h,cpp}`, `src/k1/core/audio/*`, tempo tracker.
  - VP: `src/k1/core/visual/inhouse_waveform.{h,cpp}`, `platform/ra8p1/palette_runtime.cpp`, `k1_live_runtime.cpp`, `ws281x_*`, `fixture_app.cpp`.
  - Evidence: `docs/evidence/P2..P7`.
  - Build: `scripts/build_scalar.py`.
- **Kx-Titan (PRISM lane, docs and native port):** `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-PRISM/Kx-Titan`
  - Includes `_scratch/lightwave_native_20261012` (a Lightwave native port) and `_scratch/music_visual_value_20261008`.
- **K1 ESP32-S3 product firmware (the "S3"):** `/Users/spectrasynq/SpectraSynq_K1_Firmware` (git; c2738b88, ae5d90a).
  - Also K1 lanes under `/Users/spectrasynq/Workspace_Management/Software/`: K1.esp32s3, K1.LightwaveOS, K1.Lightwave, K1.MultiTempo, K1.tempogram, K1.reinvented, k1-ws2816-source-precision, SpectraSynq.K1_Testbed.
- **Upstream lineage:**
  - SensoryBridge: `/Users/spectrasynq/SensoryBridge-main`, `/Users/spectrasynq/Workspace_Management/Software/SensoryBridge*`.
  - Emotiscope: `/Users/spectrasynq/Workspace_Management/Software/Emotiscope`, `~/Downloads/Emotiscope.sourcecode`, `~/Downloads/Emotiscope-1`.
  - Lightwave: `/Users/spectrasynq/Workspace_Management/Software/Lightwave-Ledstrip*`, `LightwaveOS_Official`, `LightwaveOS-Production`, `/Users/spectrasynq/.claude-worktrees/Lightwave-Ledstrip`.
  - PRISM lanes: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-PRISM/{PRISM-Core,K1-S3,K1-DualMCU}`.
- **MIR references on disk:** `/Users/spectrasynq/Workspace_Management/Software/{BeatNet,madmom,hybrid-beat-tracker}`.
- **Memory:**
  - claude-mem MCP tools: use ONE search term per query, then timeline, then get_observations. Never call `/api/pending-queue`.
  - Hindsight bank `spectrasynq`. It may be degraded; say so if a call fails.
  - Memory is history, not authority: confirm in source.

## Rules for every claim
- Label every claim [FACT] (you saw the line, commit, measurement or document), [INFERENCE] or [HYPOTHESIS].
- Cite path:line, a commit hash, or a URL with an access date.
- Distinguish documented / implemented / demonstrated-on-device / product-qualified.
- Asymmetric-capability doctrine: look for secondary and embedded capabilities already present (on the chip, in the codec, in the LED protocol, in existing code) that displace work or multiply value.
- No rainbow defaults. Do not propose a brightness floor or normaliser that would break true black.
- Do not invent the Captain's taste. Where taste decides, say so and propose the smallest on-device A/B he can judge by eye.
- Every proposed experiment must name a measurable criterion that could be frozen BEFORE results (scripts/freeze_tolerance.py), and a planted fault that must turn it red.

## Lever record (use these fields)
- id;
- container (AP / map / VP / output / system);
- stage (function and file:line);
- what it controls in the final light (motion / colour / brightness / timing / space);
- current setting on Titan (v8 and v9);
- information preserved or destroyed (with numbers);
- origin of the current setting (S3 budget / aesthetic / inherited / accident / unknown, with evidence);
- still needed on the Titan? (yes / no / unknown, with cost on M85 in cycles or %, if known);
- manipulation options;
- interactions with other levers;
- visual impact 1–5;
- novelty 1–5;
- effort 1–5;
- risk;
- the smallest decisive experiment (frozen criterion + planted fault).
