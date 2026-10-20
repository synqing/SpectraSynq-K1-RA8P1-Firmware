# Q1–Q5 empty-TCM campaign pack

Authority: `docs/BRIEF-2026-09-10-uncapped-ap-timing.md`. Live Titan remains
`live-audio-gpt-20260920-03` until Captain says Rearm.

## Identity already bound (do not treat as the timing experiment)

| Item | Value |
| --- | --- |
| UID | `545433931bd25436593630352d068363` |
| Resident application | build `c7f6034a902833e3f8a17f7c5792f90990f647bccfcf7f0cd211036ebaba824c` |
| HEX | `3aa0913950c5815c28c3ae06b37ee4462c5e2537adc738517cccd8b97b60eabd` |
| Recovery artefact | `live-audio-gpt-20260920-03` — HEX and ELF hashes match the identified live image |
| Contract | 24 kHz / 180 samples / 7.5 ms |
| M33 / U55 | parked |
| D-cache | disabled |

`g4-uncapped-raw-hops-build-20260919-02` HEX `9f12869a…` / build `32dd1f5f…`
is the **named DTCM variant**. It cannot close empty-TCM Q1–Q5.

`g4-uncapped-raw-hops-prog-20260920-01` does not exist and is **not adopted**.
Fresh programme and run directories only.

## Frozen fixture (shared bytes; do not regenerate)

| Item | Path / hash |
| --- | --- |
| Profile | `docs/evidence/K1-RA8P1-002/g4-scalar-workload-profile.json` |
| Resident header | `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/resident-controls-01/resident_controls.h` |
| Header sha256 | `5004033206cb9950374ec50ca0835481a21a03ea4059103862ff7bc29f7cccba` |
| PCM sha256 | `5970e8d26787f5c9ccdda50b95077d3da503b3826495ed7232d2854fecb8d394` |
| Hops per loop | 6000 |
| Optimisation | `--optimisation o3-unroll` (`-O3 -funroll-loops -frename-registers`) |
| Placement | `--tempo-placement empty-tcm` |
| Cache / FPU / MVE | D-cache disabled; `fpv5-sp-d16`; no MVE |

## Build commands

Root: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware`

```
python3 scripts/build_scalar.py \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-raw-hops-build-20260920-01 \
  --optimisation o3-unroll --dcache disabled \
  --resident-controls /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/resident-controls-01/resident_controls.h \
  --stage-profile --tempo-placement empty-tcm
```

```
python3 scripts/build_scalar.py \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-observer-build-20260920-01 \
  --optimisation o3-unroll --dcache disabled \
  --resident-controls /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/resident-controls-01/resident_controls.h \
  --tempo-placement empty-tcm
```

Named historical DTCM variant (map proof only; not Q1–Q5):

```
python3 scripts/build_scalar.py \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-dtcm-raw-hops-build-20260920-01 \
  --optimisation o3-unroll --dcache disabled \
  --resident-controls /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/resident-controls-01/resident_controls.h \
  --stage-profile --tempo-placement dtcm
```

## Hashes (ARM builds 2026-09-20)

Frozen fixture hashes unchanged: header `50040332…`, PCM `5970e8d2…`.

| Candidate | ELF sha256 | HEX sha256 | build_id | placement evidence |
| --- | --- | --- | --- | --- |
| empty-TCM profile `g4-empty-tcm-raw-hops-build-20260920-01` | `850c307805f1954fc2795857e5fcfc7138dafa99898cd3c14bc7cbd5be644e6e` | `22d405c1d45871c45c0bfa5b88fc1868c79198de493efeef900a47f89cb81a8b` | `8a78961b5ad0e821f7f20e13cc093b37e32b3be4643995ee12e4ccbb5a6878bd` | automatic-storage; no `.dtcm` hot arrays; stack_delta 2848 B; bss 1,050,876 |
| empty-TCM observer `g4-empty-tcm-observer-build-20260920-01` | `878150270705ca5858e3901ec4482955bbd8513b42c71eb547b853f8342cdf6a` | `28eda0d11083d100f770aca67a6dad67e2699c725e3bfabc28f04cdcd2e3d29c` | `4ff1deea52d4f2e85a0630af78ac09e5243f66b69a9320d4e69b0588ad3eed43` | automatic-storage; no stage-profile; stack_delta 2848 B |
| DTCM named variant `g4-dtcm-raw-hops-build-20260920-01` | `0835cc4caef4fb0b9220b6d2e92a09cb21dcfbab271241367d1127a8a32b3f63` | `b618cb887f07736a4ee8a333df0d6a8584378dab3f674b8d5b98d6446869fbb5` | `ed3e75671158071c9317311ae669a2a89b412ab65ae70b56e5d34fef86c99107` | `acf@0x20000000` `work@0x20000320`; size matches HEX `9f12869a…` (text/data/bss) |

Receipt sha256: profile `80b69f3d…`, observer `79faceb6…`, DTCM variant `6caa70a1…`.

Expected profile identity after Rearm: UID `545433931bd25436593630352d068363`, build `8a78961b…`, HEX `22d405c1…`.

## Rearm / run (Captain Rearm only)

Programme directory is not a timing result. Fresh dirs:

```
python3 scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-raw-hops-build-20260920-01 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-raw-hops-prog-20260920-01 \
  --wait-seconds 180 --execute
```

Expected target identity: the new empty-TCM profile `build_id` and HEX, UID
`545433931bd25436593630352d068363`. Preserve any newly encountered uncaptured
LED fault before switching images.

Normal Q1–Q5:

```
python3 scripts/run_scalar_schedule.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-raw-hops-build-20260920-01 \
  --resident /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/resident-controls-01 \
  --profile /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/g4-scalar-workload-profile.json \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-raw-hops-run-20260920-01 \
  --loops 1
```

Known-delay negative (hop 136, separate directory):

```
python3 scripts/run_scalar_schedule.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-raw-hops-build-20260920-01 \
  --resident /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/resident-controls-01 \
  --profile /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/g4-scalar-workload-profile.json \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-raw-hops-mutation-20260920-01 \
  --loops 1 --timing-mutation
```

Observer control (second Rearm onto the observer image):

```
python3 scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-observer-build-20260920-01 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-observer-prog-20260920-01 \
  --wait-seconds 180 --execute
```

```
python3 scripts/run_scalar_schedule.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-observer-build-20260920-01 \
  --resident /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/resident-controls-01 \
  --profile /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/g4-scalar-workload-profile.json \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-observer-run-20260920-01 \
  --loops 1
```

A valid measurement of a failing schedule stays a failure. Summaries must
recompute from retained `raw-stage-trace.jsonl`. Release lateness remains
distinct from execution cost (`lateness_cycles` / `completion_cycles`).

Return to live audio after the campaign with a fresh programme directory of
`live-audio-gpt-20260920-03`.
