# Live control latency — 15 Oct 2026 (atlas Experiment 3 step 2)

- **What was done:**
  - Twenty live speed changes on board 0 of the running Titan (host-007), alternating 0.50 and 0.55, through the mixer API.
  - Speed was then restored to 0.50 and confirmed.
  - Board 1 was never commanded and read 0.50 throughout.
  - No flash; the Titan kept visualising.
- **Result:**
  - All 20 confirmed by device readback.
  - Command → readback: p50 23.9 ms, p95 28.5 ms, max 29.2 ms, jitter (p95 − p50) 4.6 ms.
  - Frozen limit p95 ≤ 150 ms (CONTROL-LATENCY-FROZEN.json, 2026-10-14T22:35:13Z): PASS.
- **Planted faults:**
  - A poll starting 200 ms late fails the p95 clause.
  - A board-1 command fails the board-1 clause.
- **What "rendered" means:**
  - The device marks a request rendered on the next render frame after it accepts the patch: `liveNoteRendererConsumed`, `platform/ra8p1/fixture_app.cpp:594`, `k1_live_protocol.cpp:544`.
  - The host saw the rendered id only after about 780 ms, because the mixer refreshes that field on its roughly 1 s presence poll, not from the patch reply.
  - [INFERENCE] The light changes about one frame (10 ms) after acceptance, so a host-timed trigger lands about 25–35 ms after the command with about 5 ms jitter. A camera check with audio in the file is still needed before a trigger is trusted.
