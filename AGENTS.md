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

When Captain says **Rearm**, the final image, hashes, fresh output directory and exact `programme_scalar.py` command must already be prepared. Start the programmer waiter before any reply, then reply exactly `WAITING`. Do not first enumerate USB, inspect source, rebuild, search for the command, or explain the plan.

While the waiter is active, the chat is the operator instrument: relay bootloader seen, identified/writing, write verified, and application identity promptly as each is observed. The latest Captain instruction supersedes the earlier silent-progress rule. Read the structured events in `events.jsonl` or the live process; never wait for the entire programmer to exit before reporting verification. Logs contain evidence events, not a second set of finger instructions. After `PROGRAMME_VERIFY_PASS`, give the release/reset instruction once only if Captain has not already completed it; if Captain reports it done or the application target is already responsive, do not repeat it and proceed directly to the bound target runner. Never reuse an existing programme/run receipt directory.

Programming success proves the exact bytes were verified on the identified target. It does not prove application startup, a peripheral transaction, register state or emitted light. Run the image-bound target scorer and retain the distinct optical-evidence boundary.

## Centre-origin mandate — user instruction, 2026-09-10

All exposed VP motion must originate at the centre and travel outward, or originate at the edges and travel inward. Native centre is pixels 79/80 of 160; the current 128-pixel bench centre is 63/64. Apply this to previews and new effects as well as musical modes. Verify mirror geometry and travel direction separately; mirror symmetry alone does not prove motion origin. Preserve the pinned reference and identify any required behavioural correction explicitly.
