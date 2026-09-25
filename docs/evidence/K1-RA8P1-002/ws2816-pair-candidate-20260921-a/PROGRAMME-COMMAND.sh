#!/bin/sh
# Dry-run validated 21 September 2026. Do not add --execute until Captain says Rearm.
# Fresh output directory: must not reuse programme-ws2816-pair-dry-20260921-01.
exec python3 scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/ws2816-pair-candidate-20260921-a \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/programme-ws2816-pair-20260921-01
