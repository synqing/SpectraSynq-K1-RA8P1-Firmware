#!/usr/bin/env bash
set -euo pipefail
# Defaults are the captain's Mac checkout paths (docs/REFERENCE-MANIFEST.md).
# --dualmcu-root/--bsp-root (or K1_DUALMCU_ROOT/K1_TITAN_BSP_ROOT) override
# them for a container or CI checkout that does not have those paths -- this
# widens WHERE the two repos are read from, it never widens WHAT is accepted:
# the pin/ancestor checks below are unchanged, and an override that points at
# a directory failing them still exits 1.
DUAL="${K1_DUALMCU_ROOT:-/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware}"
BSP="${K1_TITAN_BSP_ROOT:-/Users/spectrasynq/Workspace_Management/Software/sdk-bsp-ra8p1-titan-mini}"
while [ $# -gt 0 ]; do
  case "$1" in
    --dualmcu-root) DUAL="$2"; shift 2 ;;
    --dualmcu-root=*) DUAL="${1#*=}"; shift ;;
    --bsp-root) BSP="$2"; shift 2 ;;
    --bsp-root=*) BSP="${1#*=}"; shift ;;
    *) echo "REFERENCE=FAIL unknown_arg=$1"; exit 1 ;;
  esac
done
DUAL_PIN=6b1e7bc5c9f9871e6ea4e900455bcb37d756304a
# TIT-2 scoped authority update (docs/reference-import-receipt-tit2.md,
# scripts/import_tit2_delta.py); ancestor-checked below like DUAL_PIN, never
# substituted for it.
DUAL_PIN_TIT2=16f70a9b2907c03c8262e79d05f5a05ed1795917
BSP_PIN=6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7
for d in "$DUAL" "$BSP"; do
  test -d "$d/.git" || { echo "REFERENCE=FAIL missing_git=$d"; exit 1; }
done
git -C "$DUAL" cat-file -e "$DUAL_PIN^{commit}"
git -C "$DUAL" cat-file -e "$DUAL_PIN_TIT2^{commit}"
git -C "$BSP" cat-file -e "$BSP_PIN^{commit}"
# Behavioural import pin must remain an ancestor of DualMCU HEAD. A parallel
# lane that rewound past the pin must go red; DualMCU being "busy" is not a
# licence to treat the pin as out of scope.
if ! git -C "$DUAL" merge-base --is-ancestor "$DUAL_PIN" HEAD; then
  echo "REFERENCE=FAIL dualmcu_pin_not_ancestor pin=$DUAL_PIN head=$(git -C "$DUAL" rev-parse HEAD)"
  exit 1
fi
if ! git -C "$DUAL" merge-base --is-ancestor "$DUAL_PIN_TIT2" HEAD; then
  echo "REFERENCE=FAIL dualmcu_pin_tit2_not_ancestor pin=$DUAL_PIN_TIT2 head=$(git -C "$DUAL" rev-parse HEAD)"
  exit 1
fi
if ! git -C "$BSP" merge-base --is-ancestor "$BSP_PIN" HEAD; then
  echo "REFERENCE=FAIL titan_bsp_pin_not_ancestor pin=$BSP_PIN head=$(git -C "$BSP" rev-parse HEAD)"
  exit 1
fi
printf 'REFERENCE_DUALMCU=PASS pin=%s current=%s ancestor=1\n' "$DUAL_PIN" "$(git -C "$DUAL" rev-parse HEAD)"
printf 'REFERENCE_DUALMCU_TIT2=PASS pin=%s current=%s ancestor=1\n' "$DUAL_PIN_TIT2" "$(git -C "$DUAL" rev-parse HEAD)"
printf 'REFERENCE_TITAN_BSP=PASS pin=%s current=%s ancestor=1\n' "$BSP_PIN" "$(git -C "$BSP" rev-parse HEAD)"
