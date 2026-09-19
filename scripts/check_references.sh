#!/usr/bin/env bash
set -euo pipefail
DUAL=/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware
BSP=/Users/spectrasynq/Workspace_Management/Software/sdk-bsp-ra8p1-titan-mini
DUAL_PIN=6b1e7bc5c9f9871e6ea4e900455bcb37d756304a
BSP_PIN=6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7
for d in "$DUAL" "$BSP"; do
  test -d "$d/.git" || { echo "REFERENCE=FAIL missing_git=$d"; exit 1; }
done
git -C "$DUAL" cat-file -e "$DUAL_PIN^{commit}"
git -C "$BSP" cat-file -e "$BSP_PIN^{commit}"
# Behavioural import pin must remain an ancestor of DualMCU HEAD. A parallel
# lane that rewound past the pin must go red; DualMCU being "busy" is not a
# licence to treat the pin as out of scope.
if ! git -C "$DUAL" merge-base --is-ancestor "$DUAL_PIN" HEAD; then
  echo "REFERENCE=FAIL dualmcu_pin_not_ancestor pin=$DUAL_PIN head=$(git -C "$DUAL" rev-parse HEAD)"
  exit 1
fi
if ! git -C "$BSP" merge-base --is-ancestor "$BSP_PIN" HEAD; then
  echo "REFERENCE=FAIL titan_bsp_pin_not_ancestor pin=$BSP_PIN head=$(git -C "$BSP" rev-parse HEAD)"
  exit 1
fi
printf 'REFERENCE_DUALMCU=PASS pin=%s current=%s ancestor=1\n' "$DUAL_PIN" "$(git -C "$DUAL" rev-parse HEAD)"
printf 'REFERENCE_TITAN_BSP=PASS pin=%s current=%s ancestor=1\n' "$BSP_PIN" "$(git -C "$BSP" rev-parse HEAD)"
