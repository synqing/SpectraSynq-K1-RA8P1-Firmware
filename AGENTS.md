<!-- BEGIN EMOTISCOPE PORT PIVOT 2026-09-29 -->
THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.
Latest user assignment, 29 September 2026 13:57 Australia/Perth: park the current Titan AP/VP and attempt a complete known-working firmware port from /Users/spectrasynq/Workspace_Management/Software/Emotiscope.HIL.
This isolated worktree is authorised for that port: feat/titan-emotiscope-port-001. The older effects worktree and its original assignments remain parked and preserved, not completed.
The new runtime uses Emotiscope v1.1_build AP and VP together; the legacy K1 analysis/effects do not compute or drive the new show. Keep hardware capture/clock/USB identity and P601/GPT6 DMA0 plus P604/GPT8 DMA3, micDMA1/2.
Keep a single continuous160 logical output, split80+80, one brightness100/255 scale at output, emission autonomous on boot/applicationreturn. Original Emotiscope128 finalpixels map to160 in an explicit output adapter; do not set NUM_LEDS160 and index64-bin arrays outofbounds.
The donor source is read-only. Copied source, exact provenance and bounded defined-behaviour repairs live in this worktree. ESP32 deployment instructions, donor Wi-Fi/HIL overhead permissions, older24/255 and boot-off requirements do not apply to Titan.
Existing standing authority covers this in-scope repair/port deployment after actual checks and image/UID matching via E2Lite powerOFF. No blanket approval pause. Failed required checks or uncertain hardware identity still stop affected writes.
Read docs/emotiscope-port-001/EXECUTION.md. Preserve old contracts and receipts; this new experiment does not close them.
<!-- END EMOTISCOPE PORT PIVOT 2026-09-29 -->

<!-- BEGIN DEFAULT BRIGHTNESS MANDATE 2026-09-29 -->
**Latest user directive, 29 September 2026:** Normal default LED brightness must
remain **100/255 minimum**. Current boot and audition default is **100/255**.
This supersedes all earlier 24/255 brightness ceilings and default instructions
in this workspace, skills and historical handovers. Do not silently restore24
when arming emission, changing mode, reconnecting a host or rebuilding firmware.
Explicit operator dimming and emission-off controls remain available; this is a
minimum DEFAULT, not a prohibition on requested dimming or fault shutdown.
Preserve the corrected continuous160 / DIN80+80 mapping and autonomous music.
Evidence: docs/effects-language-001/evidence/captain-brightness100-20260929-01/.
<!-- END DEFAULT BRIGHTNESS MANDATE 2026-09-29 -->

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

<!-- BEGIN TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->
**Captain directive — 27 September 2026; supersedes conflicting earlier operating instructions below.**

Titan must boot with visual output ON and run continuous, autonomous, audio-reactive music visualisation whenever powered and the application is running. Normal output must not wait for a host, broker, camera, recording session, approval, or diagnostic window. Old boot-off requirements and automatic timed disarm at the end of diagnostic/camera/host sessions are superseded. A diagnostic timeout ends the diagnostic activity; it must not gate or stop normal music visualisation. Do not invent another permission gate for this already-authorised behaviour. Put this mandate first in every new Titan-facing agent instruction, plan and handover.

Preserve the current maximum brightness **24/255**, mode/control constraints, Titan UID **545433931bd25436593630352d068363**, **P601/GPT6 and P604/GPT8**, microphone **DMA1/2**, and LED **DMA0/3** unless Captain separately changes them. Report genuine hardware faults immediately and truthfully; do not hide faults or claim repaired/continuous optical output from software counters alone. This instruction does not promise physically uninterrupted light during power loss, programming or reset: disclose unavoidable interruption and restore autonomous music visualisation when the application returns. Preserve existing programming identity checks and probe target-power OFF. Historical observations, receipts and original plan assignments below remain historical evidence; conflicting boot-off, timed-disarm or normal-output permission clauses are not current authority.
<!-- END TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->

## SMC-Mixer / Titan control: load the `smc-titan-mixer` skill first

Before touching the mixer, broker, controller, dashboard, fader captures or a Titan deploy, load the `smc-titan-mixer` skill (`/Users/spectrasynq/Workspace_Management/Software/agent-skills/skills/smc-titan-mixer/SKILL.md`). First command: `python3 ~/.claude/skills/smc-titan-mixer/scripts/smc_titan.py health`. It covers launchd start/stop, symptom->fix, labelled captures, host deploy and `deploy_titan_image.sh` (flash + admission switch).


<!-- BEGIN TRANCE-EDM PLAYBACK ONLY 2026-09-29 -->
**Captain directive — 29 September 2026.** Song-0 is banned. Do not play it, mux it, serve it, or ask Captain to listen to it. Demonstration, reconstruction, comparison and review audio must be trance or EDM only. Retained local excerpts: Armin van Buuren — Sonic Samba (Rising Star Remix), Veracocha — Carte Blanche (Mixed), and held-out Gareth Emery — Dopex. Dopex stays out of estimator tuning. Synthetic clicks, silence and a named test tone may be used for an engineering check, not as a review soundtrack. Historical song-0 frame files may remain on disk; they are not the review. Ledger: `~/.claude/memory/spectrasynq/L1/CANONICAL_DECISIONS.md` §11.
<!-- END TRANCE-EDM PLAYBACK ONLY 2026-09-29 -->

<!-- BEGIN SMC-TITAN STANDING DEPLOYMENT AUTHORITY -->
# SMC–Titan: repair, flash and verify — standing user instruction, 27 September 2026

The operator explicitly requires implemented fixes to reach the physical Titan immediately after the relevant software checks pass. Do not return another “built, not flashed; approve this image?” handback within the scope below.

**Required sequence:** targeted repair → relevant regression tests → ARM build → verify exact HEX/hash, board identity and build settings → programme through the existing E2 Lite/SWD workflow → read back identity/configuration → run the bounded physical test → report actual results. Complete that sequence in the executing session. A source edit or successful compilation is not a completed fix, and a flashed image is not yet proven working.

**Standing authority:** this directive supplies ongoing permission for replacement diagnostic/repair firmware, programming and bounded bench runs in SMC-TITAN-CONTROL-001. It supersedes older per-image “ask Captain”, “offline only” and separate Permission A/B handback requirements for this lane. Record the replacement image's exact path/hash and the scope in the existing admission record; do not fabricate a hardware-PASS record or bypass native tool permissions. A new file hash alone does not require another permission request.

**Scope:** only the existing SMC host worktree and Titan firmware worktree, and Titan UID `545433931bd25436593630352d068363`. Preserve the established P601/GPT6 and P604/GPT8 wiring, microphone DMA 1/2 and LED DMA 0/3 unless the user separately authorises a change. No edits or flashing in K1.Sliders, DualMCU, K1-System-R3 or the vendor BSP. Preserve mixer mappings and the prior Stage A evidence.

**Programming:** use the existing `ra8p1-e2lite-router` E2 Lite/SWD procedure, probe target power OFF, 1 MHz, `-noquery -run`, permitted range `02000000,02C9F01F`. Quiesce and hand off the existing serial owner; do not kill unrelated processes, use GDB/serial ROM, erase the chip or overwrite configuration areas. Verify the candidate before writing and full UID/build/schema/emit state afterwards.

**Bench bounds:** boot with emission OFF, retain mode 8 and the current limited control profile. Diagnostic/light runs: maximum brightness 24/255, maximum 120 seconds each, audio running, first-fault capture retained. Stop and disarm immediately on fault, audio discontinuity, identity mismatch or loss of control. If software cannot guarantee off, report it immediately and ask the operator to remove LED power. Transmission disabled is not independent proof of optical darkness. Do not re-arm the same fault-latched image or auto-restore the old emit-on archive. No cold power-cycle/reconnect reliability experiment under this scope.

**Stopping rule:** stop deployment for failed required checks, mismatched/unknown target or HEX, unavailable hardware/native permission, a genuine unsafe condition, an out-of-scope change, or a later explicit user HOLD. State the concrete obstacle and continue independent repair work. Do not stop merely to obtain another in-scope image approval; do not weaken checks or relabel failure to avoid a stop.

**Operator:** do not request slider movement during unattended driver diagnosis. Cue slider 7 only with healthy output and recording active. Preserve distinctions between accepted values, renderer use, completed transfers and observed light. Report “installed and tested” with the actual result, or the exact technical obstacle—not “ready for permission”.

**Persistence:** read this rule after handoff/context loss, before preparing a candidate or applying historical instructions. Do not reactivate superseded permission gates. A later user HOLD immediately overrides standing authority. No automatic background flashing task is created by this document.
<!-- END SMC-TITAN STANDING DEPLOYMENT AUTHORITY -->

# Anti-compression mandate

Any agent that shrinks, compresses, simplifies, or reduces an execution plan is to be treated as having destroyed the plan. The operational document is this file's task list and the parent technical plan. A shorter substitute is not an adoption of either.

This mandate is to be written, on execution, to every agent surface that binds this programme: the repository `AGENTS.md`, the E2 Lite router skill, and a user-level Cursor rule that applies in every workspace. Until that write happens, this section is the instruction.

# Agent Operating Contract

1. This is a greenfield RA8P1 repository. Do not import the DualMCU Git history.
2. Treat the pinned DualMCU commit in `docs/REFERENCE-MANIFEST.md` as behavioural authority for portable K1 modules.
3. Treat the pinned Titan BSP commit as vendor/platform evidence, not SpectraSynq architecture authority.
4. First-pass objective is **scalar functional parity**, not Helium/MVE optimisation, NPU use, or M33 partitioning.
5. Cortex-M85 owns AP + VP for K1-RA8P1-001. M33 and U55 remain parked until parity and timing measurements justify reopening them.
6. Preserve `MEDIA_TIME_48K`, `MUSICAL_TIME`, event-time vs availability-time separation, affine clock mapping, and target-neutral render scheduling semantics.
7. Platform code belongs under `platform/ra8p1/`. Shared K1 code must not include Renesas/FSP/RT-Thread headers.
8. Never claim physical timing, audio fidelity, LED phase, or multi-K1 synchronization from a host/compile-only result.
9. Do not modify the sibling DualMCU or Titan BSP repositories unless the user explicitly authorises it. Read them; do not develop inside them.
10. Do not silently change frameworks. Audit the live Titan bring-up/toolchain first. Any RT-Thread/FSP/bare-metal choice must be evidence-backed and documented.
11. Keep commits narrow. Do not use `git add -A` without proving every path belongs to the current slice.
12. A green build is necessary, never sufficient. Each major gate needs at least one negative/mutation proof that it can fail.
13. Before platform investigation or edits, load the retained Titan knowledge from `/Users/spectrasynq/Workspace_Management/Software/agent-skills/packages/ra8p1-titan-engineering/skills/ra8p1-titan-engineering/references/PLATFORM_MEMORY.md`. Run that skill's `scripts/platform_memory.py --check` and recall the relevant topic (`pins`, `clocks`, `dma`, `ws2816`, etc.). Use canonical source, not a stale generated plugin cache. If the package is unavailable, state that and use the linked source receipts; do not invent platform facts.
14. Reuse settled source facts while rechecking actual build, applicability, resource ownership and live identity. Reopen an architectural question for changed or contradictory evidence, not merely a fresh session. Record new reusable facts in the existing Titan skill/domain module with source identity and a regression check before handback. Keep build/campaign observations out of timeless silicon rules; a proposed GPT/DMA backend remains unimplemented until its execution evidence exists.
15. Before Titan onboard-microphone or LED2/PHY work, read `docs/evidence/K1-RA8P1-002/TITAN-MIC-LED2-SESSION-CANON.md`. It is the closeout inventory for the 2026-09-13 bring-up and supersedes the open conclusions in `LED2-MDIO-RECOVERY-HANDOVER.md` where the evidence-bound final receipt says so.

## Titan live-programming operator protocol — load-bearing

Image writes and silicon identity go through the E2 Lite over SWD with `rfp-cli`. Captain does not press buttons. Load `~/.cursor/skills/ra8p1-e2lite-router/SKILL.md` and follow that workflow. Probe power stays off. Do not attach GDB. Do not start `programme_scalar.py` for a write. The serial ROM path, including `WAITING_FOR_IDENTIFIED_ROM` and `WRITE_VERIFIED`, is retired on this bench.

Programming success proves the exact bytes were verified on the identified target. It does not prove application startup, a peripheral transaction, register state or emitted light. Run the image-bound target scorer and retain the distinct optical-evidence boundary.

## Centre-origin mandate — user instruction, 2026-09-10

All exposed VP motion must originate at the centre and travel outward, or originate at the edges and travel inward. Native centre is pixels 79/80 of 160; the current 128-pixel bench centre is 63/64. Apply this to previews and new effects as well as musical modes. Verify mirror geometry and travel direction separately; mirror symmetry alone does not prove motion origin. Preserve the pinned reference and identify any required behavioural correction explicitly.

<!-- BEGIN TITAN PHYSICAL EFFECTS DEVELOPMENT 2026-09-29 -->
**User directive29September2026:** Captain is to implement and deploy effects directly
on Titan for physical visual inspection. HTML presentations are not the appearance
review gate. Complete relevant software checks, exact-image identity/programming and
live readback in the same execution. Retain brightness24/255, UID, pins/DMA and probe
powerOFF. Preserve autonomous music on application return. Current physical audition
and compatible geometry are recorded in docs/effects-language-001/CAPTAIN-PHYSICAL-20260929.md.
<!-- END TITAN PHYSICAL EFFECTS DEVELOPMENT 2026-09-29 -->

<!-- BEGIN CURRENT WS2816 STRIP GEOMETRY 2026-09-29 -->
**User correction, 29 September 2026:** The attached WS2816 strip is one continuous
160-pixel visual surface. Physical LEDs 1–80 use DIN-A (P601/GPT6); LEDs 81–160
use DIN-B (P604/GPT8). Render the complete 160-pixel selected scene, with centre
between logical indices 79/80, then split its packed GRB48 bytes at pixel 80.
Each DIN receives 80 pixels / 480 bytes. Do not render separate scenes per DIN,
restart the spatial coordinate at the split, mirror each half independently, or
let a renderer flag change transfer length. Increasing logical order is retained
on both halves, as in the K1 Main RPL WS2816 reference. The earlier two160-pixel
DIN interpretation is rejected for this connected setup. Product dual-edge160
geometry is a different configuration and does not turn these two DINs into two
complete edges. Current repair: docs/effects-language-001/evidence/captain-continuous160-20260929-01/.
<!-- END CURRENT WS2816 STRIP GEOMETRY 2026-09-29 -->
