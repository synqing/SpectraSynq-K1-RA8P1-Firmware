THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

Qualification override: this RC retains boot OFF, brightness <=24/255, <=120-second illuminated windows and final confirmed shutdown, as explicitly required in this lane.

# Titan real-time ownership contract

Applies to the paired WS2816 live runtime qualified as 72537888, and its diagnostic-cleaned RC1. This contract preserves the existing transmitter; it is not a new waveform design.

1. `k1_fixture_poll` owns foreground audio/DSP/palette/LED lifecycle progression. Its re-entry guard precedes time and state mutation. GPT/DMAC callbacks retain their existing generation ownership.
2. `execute`, `k1_fixture_consume`, status/metrics/catalogue/frame/wire/snapshot/events/timing/config getters and idle response service may not call `palette_step`, pair `poll`, `submit`, or lifecycle-mutating `busy`/`ready`. Observation includes clocks: `LiveAudioRuntime::observeNowUs` samples a copy. The current paired-driver `busy`/`ready` remain owner-only mutators, regardless of their names.
3. `k1_fixture_usb_service_allowed` observes both GPT start bits without advancing work. Both USB admission points must test it. A generation beginning after the first check must prevent the second service point. JSON/CRC/formatting, USB event/FIFO service and board-status MDIO run only outside active transmission.
4. USB-instance IRQs are suspended before the owner may start a generation, and restored only after both lanes stop. Preserve previously disabled IRQs and pending status. Do not globally mask PDM/GPT/DMAC interrupts.
5. Control producers validate a local candidate and publish one immutable mailbox value, never an RX-buffer or stack pointer. Only the owner consumes it. ACK construction follows application and happens in the idle control service. Backpressure and disconnect semantics are explicit. Do not synchronously reinitialise audio/effects from a getter or scalar PATCH.
6. Active DMA duty arrays belong to the driver for their generation. Pending submissions replace copied pending payloads, never active buffers. Completion, pending promotion and reset interval checks stay generation-bound.
7. Host MIDI callbacks perform bounded copying/coalescing, never serial/broker I/O. Extrema/latest retention preserves absolute soft pickup; consumers drain finite snapshots. Unknown, oversize and overflow cases stay explicit. Mapping expansion must use this same ownership boundary.
8. Telemetry counters are observation evidence, not optical proof. Missing fields, frozen audio, identity mismatch and incomplete camera evidence are invalid tests. Every changed release requires its own source/HEX/programmed-identity chain and bounded physical acceptance.

Run `python3 scripts/verify_realtime_isolation.py --output <new-directory>`. The contract is `docs/contracts/titan-realtime-regressions.json`; the runner executes real fixture, HAL, driver, protocol and host tests, retains logs and stops at the first nonzero result. It includes a compiled mutation restoring the historical command-entry poll; that mutation must fail the behavioural assertion. Frame/wire/history observers are covered as well as ordinary status/config reads. No test operates hardware.

RC1 uses `docs/contracts/titan-smc-release-rc1.json`. Its single build omits `--p601-loopback`; linked sampler symbols/GPT2/DMAC4 storage must be absent. Keep offline diagnostic source and past failures archived; do not delete safety/fault/ownership counters or read-only register witnesses. Do not treat unused legacy capability descriptions as authorisation: the live command allowlist remains controlling.

Any regression stops promotion and triggers evidence-led investigation. Do not compensate with speculative GPT/DMAC timing changes. The unchanged build sources and retained qualified implementation are the comparison reference.
