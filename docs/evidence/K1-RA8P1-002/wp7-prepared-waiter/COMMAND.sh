#!/bin/sh
# Recovery / return-to-live waiter. Do not start unless Captain says Rearm.
# Dry-run directory wp7-dry-live-audio-20260920-01 is consumed and must not be reused.
set -eu
cd /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware
exec python3 scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/live-audio-gpt-20260920-03 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/live-audio-gpt-prog-20260920-04 \
  --wait-seconds 180 \
  --execute
