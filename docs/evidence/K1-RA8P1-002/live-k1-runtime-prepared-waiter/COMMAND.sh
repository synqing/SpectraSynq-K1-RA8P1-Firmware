#!/bin/sh
# Live-runtime candidate waiter. Do not start unless Captain says Rearm.
# Distinct from wp7-prepared-waiter (return image live-audio-gpt-20260920-03).
set -eu
cd /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware
exec python3 scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/live-k1-runtime-build-20260920-02 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/live-k1-runtime-prog-20260920-01 \
  --wait-seconds 180 \
  --execute
