#!/bin/sh
# Bound runner for live-k1-runtime-build-20260920-02. Does not flash.
# Use only after WRITE_VERIFIED of this candidate. Unused run directory required.
set -eu
cd /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware
exec python3 scripts/run_live_k1.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/live-k1-runtime-build-20260920-02 \
  --programme /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/live-k1-runtime-prog-20260920-01 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/live-k1-runtime-run-20260920-01
